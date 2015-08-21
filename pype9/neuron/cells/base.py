"""

  This package combines the common.ncml with existing pyNN classes

  Author: Thomas G. Close (tclose@oist.jp)
  Copyright: 2012-2014 Thomas G. Close.
  License: This file is part of the "NineLine" package, which is released under
           the MIT Licence, see LICENSE for details.
"""
from __future__ import absolute_import
from pype9.exceptions import Pype9RuntimeError
# MPI may not be required but NEURON sometimes needs to be initialised after
# MPI so I am doing it here just to be safe (and to save me headaches in the
# future)
try:
    from mpi4py import MPI  # @UnusedImport @IgnorePep8 This is imported before NEURON to avoid a bug in NEURON
except ImportError:
    pass
import os.path
import quantities as pq
import neo
from neuron import h, load_mechanisms
from nineml.abstraction import EventPort
from math import pi
from .code_gen import CodeGenerator
from pype9.base.cells.tree import in_units
from pype9.base.cells import base
from pype9.neuron.units import UnitHandler
from .controller import simulation_controller
from pype9.annotations import (
    PYPE9_NS, MEMBRANE_CAPACITANCE, EXTERNAL_CURRENTS,
    MEMBRANE_VOLTAGE)

basic_nineml_translations = {'Voltage': 'v', 'Diameter': 'diam', 'Length': 'L'}

NEURON_NS = 'NEURON'

import logging

logger = logging.getLogger("PyPe9")


class Cell(base.Cell):

    V_INIT_DEFAULT = -65.0

    _controller = simulation_controller
    _unit_handler = UnitHandler

    def __init__(self, *properties, **kwprops):
        """
        `propertes/kwprops` --  Can accept a single parameter, which is a
                                dictionary of parameters or kwarg parameters,
                                or a list of nineml.Property objects
        """
        self._flag_created(False)
        # Construct all the NEURON structures
        self._sec = h.Section()  # @UndefinedVariable
        # Insert dynamics mechanism (the built component class)
        HocClass = getattr(h, self.__class__.name)
        self._hoc = HocClass(0.5, sec=self._sec)
        # In order to scale the distributed current to the same units as point
        # process current, i.e. mA/cm^2 -> nA the surface area needs to be
        # 100um. mA/cm^2 = -3-(-2^2) = 10^1, 100um^2 = 2 + -6^2 = 10^(-10), nA
        # = 10^(-9). 1 - 10 = - 9. (see PyNN Izhikevich neuron implementation)
        self._sec.L = 10.0
        self._sec.diam = 10.0 / pi
        # Get the membrane capacitance property
        self._cm_prop = self.build_prototype.property(
            self.build_componentclass.annotations[
                PYPE9_NS][MEMBRANE_CAPACITANCE])
        cm = pq.Quantity(UnitHandler.to_pq_quantity(self._cm_prop), 'nF')
        # Set capacitance in mechanism
        setattr(self._hoc, self._cm_prop.name, float(cm))
        # Set capacitance in hoc
        specific_cm = pq.Quantity(cm / self.surface_area, 'uF/cm^2')
        self._sec.cm = float(specific_cm)
        # Set up members required for PyNN
        self.recordable = {'spikes': None, 'v': self.source}
        self.spike_times = h.Vector(0)
        self.traces = {}
        self.gsyn_trace = {}
        self.recording_time = 0
        self.rec = h.NetCon(self.source, None, sec=self._sec)
        self.initial_v = self.V_INIT_DEFAULT
        # Call base init (needs to be after 9ML init)
        super(Cell, self).__init__(*properties, **kwprops)
        self._flag_created(True)

    @property
    def name(self):
        return self.prototype.name

    @property
    def source_section(self):
        """
        A property used when treated as a PyNN standard model
        """
        return self._sec

    @property
    def source(self):
        """
        A property used when treated as a PyNN standard model
        """
        return self._hoc

    @property
    def surface_area(self):
        return (self._sec.L * pq.um) * (self._sec.diam * pi * pq.um)

    def get_threshold(self):
        return in_units(self._model.spike_threshold, 'mV')

    def _get(self, varname):
        try:
            return getattr(self._hoc, varname)
        except AttributeError:
            try:
                return getattr(self._sec, varname)
            except AttributeError:
                assert False

    def _set(self, varname, val):
        try:
            setattr(self._hoc, varname, val)
            # If capacitance, also set the section capacitance
            if varname == self._cm_prop.name:
                # This assumes that the value of the capacitance is in nF
                # which it should be from the super setattr method
                self._sec.cm = float(
                    pq.Quantity(val * pq.nF / self.surface_area, 'uF/cm^2'))
        except LookupError:
            setattr(self._sec, varname, val)

    def record(self, port_name):
        self._initialise_local_recording()
        port = self.component_class[port_name]
        if isinstance(port, EventPort):
            logger.warning("Assuming '{}' is voltage threshold crossing"
                           .format(port_name))
            # FIXME: This assumes that all event ports are voltage threshold
            #        crossings
            recorder = h.NetCon(
                self._sec._ref_v, None, self.get_threshold(), 0.0, 1.0,
                sec=self._sec)
        try:
            recorder = getattr(self._hoc, '_ref_' + port_name)
        except AttributeError:
            recorder = getattr(self._sec(0.5), '_ref_' + port_name)
        recording = h.Vector()
        recording.record(recorder)
        # Save the recording and recorder to ensure they don't go out of scope
        self._recorders[port_name] = (recorder, recording)

    def recording(self, port_name):
        """
        Return recorded data as a dictionary containing one numpy array for
        each neuron, ids as keys.
        """
        port = self.component_class[port_name]
        if port_name == self.componentclass.annotations[
                PYPE9_NS][MEMBRANE_VOLTAGE]:
            port_name = self.build_componentclass.annotations[
                PYPE9_NS][MEMBRANE_VOLTAGE]
        if isinstance(port, EventPort):
            recording = neo.SpikeTrain(
                self._recorders[port_name][1], t_start=0.0 * pq.ms,
                t_stop=h.t * pq.ms, units='ms')
        else:
            units_str = UnitHandler.dimension_to_unit_str(port.dimension)
            recording = neo.AnalogSignal(
                self._recorders[port_name][1], sampling_period=h.dt * pq.ms,
                t_start=0.0 * pq.ms, units=units_str, name=port_name)
        return recording

    def reset_recordings(self):
        """
        Resets the recordings for the cell and the NEURON simulator (assumes
        that only one cell is instantiated)
        """
        for _, recording in self._recorders.itervalues():
            recording.resize(0)

    def play(self, port_name, signal):
        """
        Injects current into the segment

        `current` -- a vector containing the current [neo.AnalogSignal]
        """
        try:
            port = self.componentclass.port(port_name)
        except KeyError:
            raise Pype9RuntimeError(
                "Cannot play into unrecognised port '{}'".format(port_name))
        if isinstance(port, EventPort):
            vec_stim = h.VecStim()
            vec_stim.play(pq.Quantity(signal, pq.ms))
            self._inputs[port_name] = vec_stim
        else:
            ext_is = self.build_componentclass.annotations[
                PYPE9_NS][EXTERNAL_CURRENTS]
            if port_name not in (p.name for p in ext_is):
                raise NotImplementedError(
                    "Can only play into external current ports ('{}'), not "
                    "'{}' port.".format(
                        "', '".join(p.name for p in ext_is), port_name))
            iclamp = h.IClamp(0.5, sec=self._sec)
            iclamp.delay = 0.0
            iclamp.dur = 1e12
            iclamp.amp = 0.0
            iclamp_amps = h.Vector(pq.Quantity(signal, 'nA'))
            iclamp_times = h.Vector(pq.Quantity(signal.times, 'ms'))
            iclamp_amps.play(iclamp._ref_amp, iclamp_times)
            self._inputs[port_name] = (iclamp, iclamp_amps, iclamp_times)

    def voltage_clamp(self, voltages, series_resistance=1e-3):
        """
        Clamps the voltage of a segment

        `voltage` -- a vector containing the voltages to clamp the segment
                     to [neo.AnalogSignal]
        """
        seclamp = h.SEClamp(0.5, sec=self._sec)
        seclamp.rs = series_resistance
        seclamp.dur1 = 1e12
        seclamp_amps = h.Vector(pq.Quantity(voltages, 'mV'))
        seclamp_times = h.Vector(pq.Quantity(voltages.times, 'ms'))
        seclamp_amps.play(seclamp._ref_amp, seclamp_times)
        self._inputs['_seclamp'] = (seclamp, seclamp_amps, seclamp_times)


class CellMetaClass(base.CellMetaClass):

    """
    Metaclass for building NineMLCellType subclasses Called by
    nineml_celltype_from_model
    """

    _built_types = {}
    CodeGenerator = CodeGenerator
    BaseCellClass = Cell

    @classmethod
    def load_libraries(cls, _, install_dir):
        load_mechanisms(os.path.dirname(install_dir))

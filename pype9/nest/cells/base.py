"""

  This package combines the common.ncml with existing pyNN classes

  Author: Thomas G. Close (tclose@oist.jp)
  Copyright: 2012-2014 Thomas G. Close.
  License: This file is part of the "NineLine" package, which is released under
           the MIT Licence, see LICENSE for details.
"""
from __future__ import absolute_import
import sys
import os.path
import logging
import neo
import nest
import quantities as pq
from nineml.exceptions import NineMLNameError
from .code_gen import CodeGenerator
from .controller import simulation_controller
from pype9.base.cells import base
from pype9.annotations import PYPE9_NS, MEMBRANE_VOLTAGE
from pype9.nest.units import UnitHandler
from pype9.exceptions import Pype9RuntimeError

logger = logging.getLogger('PyPe9')

basic_nineml_translations = {
    'Voltage': 'V_m', 'Diameter': 'diam', 'Length': 'L'}


class Cell(base.Cell):

    _controller = simulation_controller
    _unit_handler = UnitHandler

    def __init__(self, *properties, **kwprops):
        self._flag_created(False)
        self._cell = nest.Create(self.__class__.name)
        super(Cell, self).__init__(*properties, **kwprops)
        self._receive_ports = nest.GetDefaults(
            self.__class__.name)['receptor_types']
        self._inputs = {}
        self._flag_created(True)

    def _get(self, varname):
        return nest.GetStatus(self._cell, keys=varname)[0]

    def _set(self, varname, value):
        nest.SetStatus(self._cell, varname, value)

    def record(self, port_name, interval=None):
        # Create dictionaries for storing local recordings. These are not
        # created initially to save memory if recordings are not required or
        # handled externally
        self._initialise_local_recording()
        try:
            port = self.component_class.send_port(port_name)
        except NineMLNameError:
            # For convenient access to state variables
            port = self.component_class.state_variable(port_name)
        if port.nineml_type == 'EventSendPort':
            # FIXME: This assumes that all event send port are spikes, which
            #        I think is currently a limitation of NEST
            self._recorders[port_name] = recorder = nest.Create(
                "spike_detector", params={"precise_times": True})
            nest.Connect(self._cell, recorder)
        else:
            if interval is None:
                interval = simulation_controller.dt
            variable_name = self.build_name(port_name)
            self._recorders[port_name] = recorder = nest.Create(
                'multimeter', 1, {"interval": interval})
            nest.SetStatus(recorder, {'record_from': [variable_name]})
            nest.Connect(recorder, self._cell,
                         syn_spec={'delay': simulation_controller.min_delay})

    def recording(self, port_name):
        """
        Return recorded data as a dictionary containing one numpy array for
        each neuron, ids as keys.
        """
        # NB: Port could also be a state variable
        try:
            port = self.component_class.send_port(port_name)
        except NineMLNameError:
            # For convenient access to state variables
            port = self.component_class.state_variable(port_name)
        if port.nineml_type == 'EventSendPort':
            spikes = nest.GetStatus(
                self._recorders[port_name], 'events')[0]['times']
            data = neo.SpikeTrain(spikes, t_start=0.0 * pq.ms,
                                  t_stop=simulation_controller.t * pq.ms,
                                  name=port_name, units=pq.ms)
        else:
            port_name = self.build_name(port_name)
            events, interval = nest.GetStatus(self._recorders[port_name],
                                              ('events', 'interval'))[0]
            try:
                port = self._nineml.component_class.port(port_name)
            except NineMLNameError:
                port = self._nineml.component_class.state_variable(port_name)
            unit_str = UnitHandler.dimension_to_unit_str(port.dimension)
            variable_name = self.build_name(port_name)
            data = neo.AnalogSignal(
                events[variable_name], sampling_period=interval * pq.ms,
                t_start=0.0 * pq.ms, units=unit_str, name=port_name)
        return data

    def build_name(self, varname):
                # Get mapped port name if port corresponds to membrane voltage
        if (varname == self.component_class.annotations[
                PYPE9_NS][MEMBRANE_VOLTAGE] and
            MEMBRANE_VOLTAGE in self.build_component_class.annotations[
                PYPE9_NS]):
            varname = self.build_component_class.annotations[
                PYPE9_NS][MEMBRANE_VOLTAGE]
        return varname

    def reset_recordings(self):
        logger.warning("Haven't worked out how to implement reset recordings "
                       "for NEST yet")

    def play(self, port_name, signal, properties=[]):
        """
        Injects current into the segment

        `port_name` -- the name of the receive port to play the signal into
        `signal`    -- a neo.AnalogSignal or neo.SpikeTrain to play into the
                       port
        `weight`    -- a tuple of (port_name, value/qty) to set the weight of
                       the event port.
        """
        port = self.component_class.receive_port(port_name)
        if port.nineml_type in ('EventReceivePort',
                                'EventReceivePortExposure'):
            # Shift the signal times to account for the minimum delay and
            # match the NEURON implementation
            spike_times = (pq.Quantity(signal, 'ms') + pq.ms -
                           simulation_controller.min_delay * pq.ms)
            if any(spike_times < 0.0):
                raise Pype9RuntimeError(
                    "Some spike are less than minimum delay and so can't be "
                    "played into cell ({})".format(', '.join(
                        spike_times < 1 + simulation_controller.min_delay)))
            self._inputs[port_name] = nest.Create(
                'spike_generator', 1, {'spike_times': spike_times})
            syn_spec = {'receptor_type': self._receive_ports[port_name],
                        'delay': simulation_controller.min_delay}
            self._check_connection_properties(port_name, properties)
            if len(properties) > 1:
                raise NotImplementedError(
                    "Cannot handle more than one connection property per port")
            elif properties:
                syn_spec['weight'] = self._unit_handler.scale_value(
                    properties[0].quantity)
            nest.Connect(self._inputs[port_name], self._cell,
                         syn_spec=syn_spec)
        elif port.nineml_type in ('AnalogReceivePort', 'AnalogReducePort',
                                  'AnalogReceivePortExposure',
                                  'AnalogReducePortExposure'):
            # Signals are played into NEST cells include a delay (set to be the
            # minimum), which is is subtracted from the start of the signal so
            # that the effect of the signal aligns with other simulators
            self._inputs[port_name] = nest.Create(
                'step_current_generator', 1,
                {'amplitude_values': pq.Quantity(signal, 'pA'),
                 'amplitude_times': (pq.Quantity(signal.times, 'ms') -
                                     simulation_controller.min_delay * pq.ms),
                 'start': float(pq.Quantity(signal.t_start, 'ms')),
                 'stop': float(pq.Quantity(signal.t_stop, 'ms'))})
            nest.Connect(self._inputs[port_name], self._cell,
                         syn_spec={"receptor_type":
                                   self._receive_ports[port_name],
                                   'delay': simulation_controller.min_delay})
        else:
            raise Pype9RuntimeError(
                "Unrecognised port type '{}' to play signal into".format(port))

    def voltage_clamp(self, voltages, series_resistance=1e-3):
        raise NotImplementedError("voltage clamps are not supported for "
                                  "Pype9->NEST at this stage.")


class CellMetaClass(base.CellMetaClass):

    _built_types = {}
    CodeGenerator = CodeGenerator
    BaseCellClass = Cell

    @classmethod
    def load_libraries(cls, name, install_dir):
        lib_dir = os.path.join(install_dir, 'lib', 'nest')
        if (sys.platform.startswith('linux') or
            sys.platform in ['os2', 'os2emx', 'cygwin', 'atheos',
                             'ricos']):
            lib_path_key = 'LD_LIBRARY_PATH'
        elif sys.platform == 'darwin':
            lib_path_key = 'DYLD_LIBRARY_PATH'
        elif sys.platform == 'win32':
            lib_path_key = 'PATH'
        if lib_path_key in os.environ:
            os.environ[lib_path_key] += os.pathsep + lib_dir
        else:
            os.environ[lib_path_key] = lib_dir
        # Add module install directory to NEST path
        nest.sli_run(
            '({}) addpath'.format(os.path.join(install_dir, 'share', 'nest',
                                               'sli')))
        # Install nest module
        nest.Install(name + 'Module')

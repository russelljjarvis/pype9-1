# encoding: utf-8
"""
Implementation of the "low-level" functionality used by the common
implementation of the API.

Functions and classes useable by the common implementation:

Functions:
    create_cells()
    reset()

Classes:
    ID
    Recorder
    ConnectionManager
    Connection
    
Attributes:
    state -- a singleton instance of the _State class.
    recorder_list

All other functions and classes are private, and should not be used by other
modules.

:copyright: Copyright 2006-2011 by the PyNN team, see AUTHORS.
:license: CeCILL, see LICENSE for details.

$Id: simulator.py 957 2011-05-03 13:44:15Z apdavison $
"""

import logging
import pypcsim
import types
import numpy
from pyNN import common, errors, standardmodels, core

recorder_list = []
connection_managers = []
STATE_VARIABLE_MAP = {"v": ("Vinit", 1e-3)}

logger = logging.getLogger("PyNN")

# --- Internal PCSIM functionality -------------------------------------------- 

def is_local(id):
    """Determine whether an object exists on the local MPI node."""
    return pypcsim.SimObject.ID(id).node == net.mpi_rank()


# --- For implementation of get_time_step() and similar functions --------------

class _State(object):
    """Represent the simulator state."""

    def __init__(self):
        """Initialize the simulator."""
        self.initialized = False
        self.t = 0.0
        self.min_delay = None
        self.max_delay = None
        self.constructRNGSeed = None
        self.simulationRNGSeed = None

    @property
    def num_processes(self):
        return net.mpi_size()

    @property
    def mpi_rank(self):
        return net.mpi_rank()

    dt = property(fget=lambda self: net.get_dt().in_ms()) #, fset=lambda self,x: net.set_dt(pypcsim.Time.ms(x)))

def reset():
    """Reset the state of the current network to time t = 0."""
    net.reset()
    state.t = 0.0


# --- For implementation of access to individual neurons' parameters -----------

class ID(long, common.IDMixin):
    __doc__ = common.IDMixin.__doc__

    def __init__(self, n):
        """Create an ID object with numerical value `n`."""
        long.__init__(n)
        common.IDMixin.__init__(self)

    def _pcsim_cell(self):
        """Return the PCSIM cell with the current ID."""
        global net
        #if self.parent:
        #    pcsim_cell = self.parent.pcsim_population.object(self)
        #else:
        pcsim_cell = net.object(self)
        return pcsim_cell

    def get_native_parameters(self):
        """Return a dictionary of parameters for the PCSIM cell model."""
        pcsim_cell = self._pcsim_cell()
        pcsim_parameters = {}
        if self.is_standard_cell:
            parameter_names = [D['translated_name'] for D in self.celltype.translations.values()]
        else:
            parameter_names = [] # for native cells, is there a way to get their list of parameters?

        for translated_name in parameter_names:
            if hasattr(self.celltype, 'getterMethods') and translated_name in self.celltype.getterMethods:
                getterMethod = self.celltype.getterMethods[translated_name]
                pcsim_parameters[translated_name] = getattr(pcsim_cell, getterMethod)()
            else:
                try:
                    pcsim_parameters[translated_name] = getattr(pcsim_cell, translated_name)
                except AttributeError, e:
                    raise AttributeError("%s. Possible attributes are: %s" % (e, dir(pcsim_cell)))
        for k, v in pcsim_parameters.items():
            if isinstance(v, pypcsim.StdVectorDouble):
                pcsim_parameters[k] = list(v)
        return pcsim_parameters

    def set_native_parameters(self, parameters):
        """Set parameters of the PCSIM cell model from a dictionary."""
        simobj = self._pcsim_cell()
        for name, value in parameters.items():
            if hasattr(self.celltype, 'setterMethods') and name in self.celltype.setterMethods:
                setterMethod = self.celltype.setterMethods[name]
                getattr(simobj, setterMethod)(value)
            else:
                setattr(simobj, name, value)

    def get_initial_value(self, variable):
        pcsim_name, unit_conversion = STATE_VARIABLE_MAP[variable]
        pcsim_cell = self._pcsim_cell()
        if hasattr(self.celltype, 'getterMethods') and variable in self.celltype.getterMethods:
            getterMethod = self.celltype.getterMethods[pcsim_name]
            value = getattr(pcsim_cell, getterMethod)()
        else:
            try:
                value = getattr(pcsim_cell, pcsim_name)
            except AttributeError, e:
                raise AttributeError("%s. Possible attributes are: %s" % (e, dir(pcsim_cell)))
        return value / unit_conversion

    def set_initial_value(self, variable, value):
        pcsim_name, unit_conversion = STATE_VARIABLE_MAP[variable]
        pcsim_cell = self._pcsim_cell()
        value = unit_conversion * value
        if hasattr(self.celltype, 'setterMethods') and variable in self.celltype.setterMethods:
            setterMethod = self.celltype.setterMethods[pcsim_name]
            getattr(pcsim_cell, setterMethod)(value)
        else:
            try:
                value = setattr(pcsim_cell, pcsim_name, value)
            except AttributeError, e:
                raise AttributeError("%s. Possible attributes are: %s" % (e, dir(pcsim_cell)))
        index = self.parent.id_to_local_index(self)
        self.parent.initial_values[variable][index] = value

# --- For implementation of connect() and Connector classes --------------------

class Connection(object):
    """
    Store an individual connection and information about it. Provide an
    interface that allows access to the connection's weight, delay and other
    attributes.
    """

    def __init__(self, source, target, pcsim_connection, weight_unit_factor):
        """
        Create a new connection.
        
        `source` -- ID of pre-synaptic neuron.
        `target` -- ID of post-synaptic neuron.
        `pcsim_connection` -- a PCSIM Connection object.
        `weight_unit_factor` -- 1e9 for current-based synapses (A-->nA), 1e6 for
                                conductance-based synapses (S-->µS).
        """
        self.source = source
        self.target = target
        self.pcsim_connection = pcsim_connection
        self.weight_unit_factor = weight_unit_factor

    def _get_weight(self):
        """Synaptic weight in nA or µS."""
        return self.weight_unit_factor * self.pcsim_connection.W
    def _set_weight(self, w):
        self.pcsim_connection.W = w / self.weight_unit_factor
    weight = property(fget=_get_weight, fset=_set_weight)

    def _get_delay(self):
        """Synaptic delay in ms."""
        return 1000.0 * self.pcsim_connection.delay # s --> ms
    def _set_delay(self, d):
        self.pcsim_connection.delay = 0.001 * d
    delay = property(fget=_get_delay, fset=_set_delay)


class ConnectionManager(object):
    """
    Manage synaptic connections, providing methods for creating, listing,
    accessing individual connections.
    """

    synapse_target_ids = { 'excitatory': 1, 'inhibitory': 2 }

    def __init__(self, synapse_type, synapse_model=None, parent=None):
        """
        Create a new ConnectionManager.
        
        `synapse_type` -- the 'physiological type' of the synapse, e.g.
                          'excitatory' or 'inhibitory', or a PCSIM synapse
                          factory.
        `synapse_model` -- not used. Present for consistency with other simulators.
        `parent` -- the parent `Projection`, if any.
        """
        global connection_managers
        self.synapse_type = synapse_type
        self.connections = []
        self.parent = parent
        connection_managers.append(self)
        self.parent = parent
        #if parent is None:
        self.connections = []

    def __getitem__(self, i):
        """Return the `i`th connection on the local MPI node."""
        #if self.parent:
        #    if self.parent.is_conductance:
        #        A = 1e6 # S --> uS
        #    else:
        #        A = 1e9 # A --> nA
        #    return Connection(self.parent.pcsim_projection.object(i), A)
        #else:
        return self.connections[i]

    def __len__(self):
        """Return the number of connections on the local MPI node."""
        #if self.parent:
        #    return self.parent.pcsim_projection.size()
        #else:
        return len(self.connections)

    def __iter__(self):
        """Return an iterator over all connections on the local MPI node."""
        for i in range(len(self)):
            yield self[i]

    def connect(self, source, targets, weights, delays):
        """
        Connect a neuron to one or more other neurons with a static connection.
        
        `source`  -- the ID of the pre-synaptic cell.
        `targets` -- a list/1D array of post-synaptic cell IDs, or a single ID.
        `weight`  -- a list/1D array of connection weights, or a single weight.
                     Must have the same length as `targets`.
        `delays`  -- a list/1D array of connection delays, or a single delay.
                     Must have the same length as `targets`.
        """
        if not isinstance(source, (int, long)) or source < 0:
            errmsg = "Invalid source ID: %s" % source
            raise errors.ConnectionError(errmsg)
        if not core.is_listlike(targets):
            targets = [targets]
        if isinstance(weights, float):
            weights = [weights]
        if isinstance(delays, float):
            delays = [delays]
        assert len(targets) > 0
        for target in targets:
            if not isinstance(target, common.IDMixin):
                raise errors.ConnectionError("Invalid target ID: %s" % target)
        assert len(targets) == len(weights) == len(delays), "%s %s %s" % (len(targets), len(weights), len(delays))
        if common.is_conductance(targets[0]):
            weight_scale_factor = 1e-6 # Convert from µS to S  
        else:
            weight_scale_factor = 1e-9 # Convert from nA to A

        synapse_type = self.synapse_type or "excitatory"
        if isinstance(synapse_type, basestring):
            syn_target_id = ConnectionManager.synapse_target_ids[synapse_type]
            syn_factory = pypcsim.SimpleScalingSpikingSynapse(
                              syn_target_id, weights[0], delays[0])
        elif isinstance(synapse_type, pypcsim.SimObject):
            syn_factory = synapse_type
        else:
            raise errors.ConnectionError("synapse_type must be a string or a PCSIM synapse factory. Actual type is %s" % type(synapse_type))
        for target, weight, delay in zip(targets, weights, delays):
            syn_factory.W = weight * weight_scale_factor
            syn_factory.delay = delay * 0.001 # ms --> s
            try:
                c = net.connect(source, target, syn_factory)
            except RuntimeError, e:
                raise errors.ConnectionError(e)
            if target.local:
                self.connections.append(Connection(source, target, net.object(c), 1.0 / weight_scale_factor))

    def convergent_connect(self, sources, target, weights, delays):
        """
        Connect a neuron to one or more other neurons with a static connection.
        
        `sources`  -- a list/1D array of pre-synaptic cell IDs, or a single ID.
        `target` -- the ID of the post-synaptic cell.
        `weight`  -- a list/1D array of connection weights, or a single weight.
                     Must have the same length as `targets`.
        `delays`  -- a list/1D array of connection delays, or a single delay.
                     Must have the same length as `targets`.
        """
        if not isinstance(target, (int, long)) or target < 0:
            errmsg = "Invalid target ID: %s" % target
            raise errors.ConnectionError(errmsg)
        if not core.is_listlike(sources):
            sources = [sources]
        if isinstance(weights, float):
            weights = [weights]
        if isinstance(delays, float):
            delays = [delays]
        assert len(sources) > 0
        for source in sources:
            if not isinstance(source, common.IDMixin):
                raise errors.ConnectionError("Invalid source ID: %s" % source)
        assert len(sources) == len(weights) == len(delays), "%s %s %s" % (len(sources), len(weights), len(delays))
        if common.is_conductance(target):
            weight_scale_factor = 1e-6 # Convert from µS to S  
        else:
            weight_scale_factor = 1e-9 # Convert from nA to A

        synapse_type = self.synapse_type or "excitatory"
        if isinstance(synapse_type, basestring):
            syn_target_id = ConnectionManager.synapse_target_ids[synapse_type]
            syn_factory = pypcsim.SimpleScalingSpikingSynapse(
                              syn_target_id, weights[0], delays[0])
        elif isinstance(synapse_type, pypcsim.SimObject):
            syn_factory = synapse_type
        else:
            raise errors.ConnectionError("synapse_type must be a string or a PCSIM synapse factory. Actual type is %s" % type(synapse_type))
        for source, weight, delay in zip(sources, weights, delays):
            syn_factory.W = weight * weight_scale_factor
            syn_factory.delay = delay * 0.001 # ms --> s
            try:
                c = net.connect(source, target, syn_factory)
            except RuntimeError, e:
                raise errors.ConnectionError(e)
            if target.local:
                self.connections.append(Connection(source, target, net.object(c), 1.0 / weight_scale_factor))

    def get(self, parameter_name, format):
        """
        Get the values of a given attribute (weight, delay, etc) for all
        connections on the local MPI node.
        
        `parameter_name` -- name of the attribute whose values are wanted.
        `format` -- "list" or "array". Array format implicitly assumes that all
                    connections belong to a single Projection.
        
        Return a list or a 2D Numpy array. The array element X_ij contains the
        attribute value for the connection from the ith neuron in the pre-
        synaptic Population to the jth neuron in the post-synaptic Population,
        if such a connection exists. If there are no such connections, X_ij will
        be NaN.
        """
        if format == 'list':
            values = [getattr(c, parameter_name) for c in self]
        elif format == 'array':
            values = numpy.nan * numpy.ones((self.parent.pre.size, self.parent.post.size))
            for c in self:
                addr = (self.parent.pre.id_to_index(c.source), self.parent.post.id_to_index(c.target))
                values[addr] = getattr(c, parameter_name)
        else:
            raise Exception("format must be 'list' or 'array'")
        return values

    def set(self, name, value):
        """
        Set connection attributes for all connections on the local MPI node.
        
        `name`  -- attribute name
        `value` -- the attribute numeric value, or a list/1D array of such
                   values of the same length as the number of local connections,
                   or a 2D array with the same dimensions as the connectivity
                   matrix (as returned by `get(format='array')`)
        """
        if numpy.isscalar(value):
            for c in self:
                setattr(c, name, value)
        elif isinstance(value, numpy.ndarray) and len(value.shape) == 2:
            for c in self.connections:
                addr = (self.parent.pre.id_to_index(c.source), self.parent.post.id_to_index(c.target))
                try:
                    val = value[addr]
                except IndexError, e:
                    raise IndexError("%s. addr=%s" % (e, addr))
                if numpy.isnan(val):
                    raise Exception("Array contains no value for synapse from %d to %d" % (c.source, c.target))
                else:
                    setattr(c, name, val)
        elif core.is_listlike(value):
            for c, val in zip(self.connections, value):
                setattr(c, name, val)
        else:
            raise TypeError("Argument should be a numeric type (int, float...), a list, or a numpy array.")


# --- Initialization, and module attributes ------------------------------------

net = None
state = _State()
del _State

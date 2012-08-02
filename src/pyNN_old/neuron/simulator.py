# encoding: utf8
"""
Implementation of the "low-level" functionality used by the common
implementation of the API, for the NEURON simulator.

Functions and classes useable by the common implementation:

Functions:
    create_cells()
    reset()
    run()
    finalize()

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

from pyNN import __path__ as pyNN_path
from pyNN import common, errors, core
import platform
import logging
import numpy
import os.path
from neuron import h, load_mechanisms

# Global variables
nrn_dll_loaded = []
recorder_list = []
connection_managers = []
gid_sources = []
logger = logging.getLogger("PyNN")

# --- Internal NEURON functionality -------------------------------------------- 

def is_point_process(obj):
    """Determine whether a particular object is a NEURON point process."""
    return hasattr(obj, 'loc')

def register_gid(gid, source, section=None):
    """Register a global ID with the global `ParallelContext` instance."""
    ###print "registering gid %s to %s (section=%s)" % (gid, source, section)
    state.parallel_context.set_gid2node(gid, state.mpi_rank) # assign the gid to this node
    if is_point_process(source):
        nc = h.NetCon(source, None)                          # } associate the cell spike source
    else:
        nc = h.NetCon(source, None, sec=section)
    state.parallel_context.cell(gid, nc)              # } with the gid (using a temporary NetCon)
    gid_sources.append(source) # gid_clear (in _State.reset()) will cause a
                               # segmentation fault if any of the sources
                               # registered using pc.cell() no longer exist, so
                               # we keep a reference to all sources in the
                               # global gid_sources list. It would be nicer to
                               # be able to unregister a gid and have a __del__
                               # method in ID, but this will do for now.

def nativeRNG_pick(n, rng, distribution='uniform', parameters=[0, 1]):
    """
    Pick random numbers from a Hoc Random object.
    
    Return a Numpy array.
    """
    native_rng = h.Random(0 or rng.seed)
    rarr = [getattr(native_rng, distribution)(*parameters)]
    rarr.extend([native_rng.repick() for j in xrange(n - 1)])
    return numpy.array(rarr)

def h_property(name):
    """Return a property that accesses a global variable in Hoc."""
    def _get(self):
        return getattr(h, name)
    def _set(self, val):
        setattr(h, name, val)
    return property(fget=_get, fset=_set)

class _Initializer(object):
    """
    Manage initialization of NEURON cells. Rather than create an
    `FInializeHandler` instance for each cell that needs to initialize itself,
    we create a single instance, and use an instance of this class to maintain
    a list of cells that need to be initialized.
    
    Public methods:
        register()
    """

    def __init__(self):
        """
        Create an `FinitializeHandler` object in Hoc, which will call the
        `_initialize()` method when NEURON is initialized.
        """
        h('objref initializer')
        h.initializer = self
        self.fih = h.FInitializeHandler(1, "initializer._initialize()")
        self.clear()

    def register(self, *items):
        """
        Add items to the list of cells/populations to be initialized. Cell
        objects must have a `memb_init()` method.
        """
        for item in items:
            if isinstance(item, (common.BasePopulation, common.Assembly)):
                if "Source" not in item.celltype.__class__.__name__: # don't do memb_init() on spike sources
                    self.population_list.append(item)
            else:
                if hasattr(item._cell, "memb_init"):
                    self.cell_list.append(item)

    def _initialize(self):
        """Call `memb_init()` for all registered cell objects."""
        logger.info("Initializing membrane potential of %d cells and %d Populations." % \
                     (len(self.cell_list), len(self.population_list)))
        for cell in self.cell_list:
            cell._cell.memb_init()
        for population in self.population_list:
            for cell in population:
                cell._cell.memb_init()

    def clear(self):
        self.cell_list = []
        self.population_list = []


# --- For implementation of get_time_step() and similar functions --------------

class _State(object):
    """Represent the simulator state."""

    def __init__(self):
        """Initialize the simulator."""
        h('min_delay = 0')
        h('tstop = 0')
        h('steps_per_ms = 1/dt')
        self.parallel_context = h.ParallelContext()
        self.parallel_context.spike_compress(1, 0)
        self.num_processes = int(self.parallel_context.nhost())
        self.mpi_rank = int(self.parallel_context.id())
        self.cvode = h.CVode()
        h('objref plastic_connections')
        self.clear()
        self.default_maxstep = 10.0

    t = h_property('t')
    def __get_dt(self):
        return h.dt
    def __set_dt(self, dt):
        h.steps_per_ms = 1.0 / dt
        h.dt = dt
    dt = property(fget=__get_dt, fset=__set_dt)
    tstop = h_property('tstop')         # } these are stored in hoc so that we
    min_delay = h_property('min_delay') # } can interact with the GUI

    #Added by TGC, guessing that this is what is required
    max_delay = 1000

    def clear(self):
        global connection_managers, gid_sources
        self.parallel_context.gid_clear()
        gid_sources = []
        self.gid_counter = 0
        self.running = False
        h.plastic_connections = []
        connection_managers = []

def reset():
    """Reset the state of the current network to time t = 0."""
    state.running = False
    state.t = 0
    state.tstop = 0
    h.finitialize()

def run(simtime):
    """Advance the simulation for a certain time."""
    if not state.running:
        state.running = True
        local_minimum_delay = state.parallel_context.set_maxstep(state.default_maxstep)
        h.finitialize()
        state.tstop = 0
        logger.debug("default_maxstep on host #%d = %g" % (state.mpi_rank, state.default_maxstep))
        logger.debug("local_minimum_delay on host #%d = %g" % (state.mpi_rank, local_minimum_delay))
        if state.num_processes > 1:
            assert local_minimum_delay >= state.min_delay, \
                   "There are connections with delays (%g) shorter than the minimum delay (%g)" % (local_minimum_delay, state.min_delay)
    state.tstop += simtime
    logger.info("Running the simulation for %g ms" % simtime)
    state.parallel_context.psolve(state.tstop)

def finalize(quit=False):
    """Finish using NEURON."""
    state.parallel_context.runworker()
    state.parallel_context.done()
    if quit:
        logger.info("Finishing up with NEURON.")
        h.quit()


# --- For implementation of access to individual neurons' parameters -----------

class ID(int, common.IDMixin):
    __doc__ = common.IDMixin.__doc__

    def __init__(self, n):
        """Create an ID object with numerical value `n`."""
        int.__init__(n)
        common.IDMixin.__init__(self)

    def _build_cell(self, cell_model, cell_parameters):
        """
        Create a cell in NEURON, and register its global ID.
        
        `cell_model` -- one of the cell classes defined in the
                        `neuron.cells` module (more generally, any class that
                        implements a certain interface, but I haven't
                        explicitly described that yet).
        `cell_parameters` -- a dictionary containing the parameters used to
                             initialise the cell model.
        """
        gid = int(self)
        self._cell = cell_model(**cell_parameters)          # create the cell object
        register_gid(gid, self._cell.source, section=self._cell.source_section)
        if hasattr(self._cell, "get_threshold"):            # this is not adequate, since the threshold may be changed after cell creation
            state.parallel_context.threshold(int(self), self._cell.get_threshold()) # the problem is that self._cell does not know its own gid

    def get_native_parameters(self):
        """Return a dictionary of parameters for the NEURON cell model."""
        D = {}
        for name in self._cell.parameter_names:
            D[name] = getattr(self._cell, name)
        return D

    def set_native_parameters(self, parameters):
        """Set parameters of the NEURON cell model from a dictionary."""
        for name, val in parameters.items():
            setattr(self._cell, name, val)

    def get_initial_value(self, variable):
        """Get the initial value of a state variable of the cell."""
        return getattr(self._cell, "%s_init" % variable)

    def set_initial_value(self, variable, value):
        """Set the initial value of a state variable of the cell."""
        index = self.parent.id_to_local_index(self)
        self.parent.initial_values[variable][index] = value
        setattr(self._cell, "%s_init" % variable, value)


# --- For implementation of connect() and Connector classes --------------------

class Connection(object):
    """
    Store an individual plastic connection and information about it. Provide an
    interface that allows access to the connection's weight, delay and other
    attributes.
    """

    def __init__(self, source, target, nc):
        """
        Create a new connection.
        
        `source` -- ID of pre-synaptic neuron.
        `target` -- ID of post-synaptic neuron.
        `nc` -- a Hoc NetCon object.
        """
        self.source = source
        self.target = target
        self.nc = nc

    def useSTDP(self, mechanism, parameters, ddf):
        """
        Set this connection to use spike-timing-dependent plasticity.
        
        `mechanism`  -- the name of an NMODL mechanism that modifies synaptic
                        weights based on the times of pre- and post-synaptic spikes.
        `parameters` -- a dictionary containing the parameters of the weight-
                        adjuster mechanism.
        `ddf`        -- dendritic delay fraction. If ddf=1, the synaptic delay
                        `d` is considered to occur entirely in the post-synaptic
                        dendrite, i.e., the weight adjuster receives the pre-
                        synaptic spike at the time of emission, and the post-
                        synaptic spike a time `d` after emission. If ddf=0, the
                        synaptic delay is considered to occur entirely in the
                        pre-synaptic axon.
        """
        self.ddf = ddf
        self.weight_adjuster = getattr(h, mechanism)(0.5)
        self.pre2wa = state.parallel_context.gid_connect(int(self.source), self.weight_adjuster)
        self.pre2wa.threshold = self.nc.threshold
        self.pre2wa.delay = self.nc.delay * (1 - ddf)
        self.pre2wa.weight[0] = 1
        # directly create NetCon as wa is on the same machine as the post-synaptic cell
        self.post2wa = h.NetCon(self.target._cell.source, self.weight_adjuster,
                                sec=self.target._cell.source_section)
        self.post2wa.threshold = 1
        self.post2wa.delay = self.nc.delay * ddf
        self.post2wa.weight[0] = -1
        for name, value in parameters.items():
            setattr(self.weight_adjuster, name, value)
        # setpointer
        i = len(h.plastic_connections)
        h.plastic_connections.append(self)
        h('setpointer plastic_connections._[%d].weight_adjuster.wsyn, plastic_connections._[%d].nc.weight' % (i, i))

    def _set_weight(self, w):
        self.nc.weight[0] = w

    def _get_weight(self):
        """Synaptic weight in nA or µS."""
        return self.nc.weight[0]

    def _set_delay(self, d):
        self.nc.delay = d
        if hasattr(self, 'pre2wa'):
            self.pre2wa.delay = float(d) * (1 - self.ddf)
            self.post2wa.delay = float(d) * self.ddf

    def _get_delay(self):
        """Connection delay in ms."""
        return self.nc.delay

    weight = property(_get_weight, _set_weight)
    delay = property(_get_delay, _set_delay)

def generate_synapse_property(name):
    def _get(self):
        synapse = self.nc.syn()
        if hasattr(synapse, name):
            return getattr(synapse, name)
        else:
            raise Exception("synapse type does not have an attribute '%s'" % name)
    def _set(self, val):
        synapse = self.nc.syn()
        if hasattr(synapse, name):
            setattr(synapse, name, val)
        else:
            raise Exception("synapse type does not have an attribute '%s'" % name)
    return property(_get, _set)
setattr(Connection, 'U', generate_synapse_property('U'))
setattr(Connection, 'tau_rec', generate_synapse_property('tau_rec'))
setattr(Connection, 'tau_facil', generate_synapse_property('tau_facil'))
setattr(Connection, 'u0', generate_synapse_property('u0'))

def generate_stdp_property(name):
    def _get(self):
        return getattr(self.weight_adjuster, name)
    def _set(self, val):
        setattr(self.weight_adjuster, name, val)
    return property(_get, _set)
setattr(Connection, 'w_max', generate_stdp_property('wmax'))
setattr(Connection, 'w_min', generate_stdp_property('wmin'))
setattr(Connection, 'A_plus', generate_stdp_property('aLTP'))
setattr(Connection, 'A_minus', generate_stdp_property('aLTD'))
setattr(Connection, 'tau_plus', generate_stdp_property('tauLTP'))
setattr(Connection, 'tau_minus', generate_stdp_property('tauLTD'))



class ConnectionManager(object):
    """
    Manage synaptic connections, providing methods for creating, listing,
    accessing individual connections.
    """

    def __init__(self, synapse_type, synapse_model=None, parent=None):
        """
        Create a new ConnectionManager.
        
        `synapse_model` -- either None or 'Tsodyks-Markram'.
        `parent` -- the parent `Projection`
        """
        global connection_managers
        assert parent is not None
        self.connections = []
        self.parent = parent
        self.synapse_type = synapse_type
        self.synapse_model = synapse_model
        connection_managers.append(self)

    def __getitem__(self, i):
        """Return the `i`th connection on the local MPI node."""
        if isinstance(i, int):
            if i < len(self):
                return self.connections[i]
            else:
                raise IndexError("%d > %d" % (i, len(self) - 1))
        elif isinstance(i, slice):
            if i.stop < len(self):
                return [self.connections[j] for j in range(*i.indices(i.stop))]
            else:
                raise IndexError("%d > %d" % (i.stop, len(self) - 1))

    def __len__(self):
        """Return the number of connections on the local MPI node."""
        return len(self.connections)

    def __iter__(self):
        """Return an iterator over all connections on the local MPI node."""
        return iter(self.connections)

    def _resolve_synapse_type(self):
        if self.synapse_type is None:
            self.synapse_type = weight >= 0 and 'excitatory' or 'inhibitory'
        if self.synapse_model == 'Tsodyks-Markram' and 'TM' not in self.synapse_type:
            self.synapse_type += '_TM'

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
        if not isinstance(source, int) or source > state.gid_counter or source < 0:
            errmsg = "Invalid source ID: %s (gid_counter=%d)" % (source, state.gid_counter)
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
        self._resolve_synapse_type()
        for target, weight, delay in zip(targets, weights, delays):
            if target.local:
                if "." in self.synapse_type:  #(TGC) HERE IS WHERE THE SECTION IS SPECIFIED!!
                    section_name, synapse_type = self.synapse_type.split(".")
                    try:
                        section = getattr(target._cell, section_name)
                    except AttributeError:
                        raise Exception("%s doesn't have a section named '%s'" % (type(target._cell), section_name))
                    try:
                        synapse_object = getattr(section, synapse_type)
                    except AttributeError:
                        raise Exception("Section '%s' of cell '%s' doesn't synapse named '%s'" % (type(target._cell), section_name, synapse_type))
                else:
                    synapse_type = self.synapse_type
                    synapse_object = getattr(target._cell, self.synapse_type)
                if synapse_type != "Gap": # FIXME (TGC): Hack, should have a better way of specifying gap junction
                    nc = state.parallel_context.gid_connect(int(source), synapse_object)
                    nc.weight[0] = weight

                    # if we have a mechanism (e.g. from 9ML that includes multiple
                    # synaptic channels, need to set nc.weight[1] here
                    if nc.wcnt() > 1:
                        nc.weight[1] = target._cell.type.synapse_types.index(self.synapse_type)
                    nc.delay = delay
                    # nc.threshold is supposed to be set by ParallelContext.threshold, called in _build_cell(), above, but this hasn't been tested
                    self.connections.append(Connection(source, target, nc))
                else:
                    # (TGC) Generated a "source variable" GID (NB: that this is completely separate 
                    # from the cell gid). 
                    #
                    # Warning! This implementation assumes that there is only one projection with 
                    # gap junctions, maybe a safe offset for each could be calculated when the 
                    # projections are read from the XML and passed to the projection constructor
                    synapse_object.g = weight
                    source_var_gid = int(source) * len(self.parent.post) + int(target)
                    if source.local:
                        state.parallel_context.source_var(source._cell.source_section(0.5)._ref_v, source_var_gid)
                    state.parallel_context.target_var(synapse_object._ref_vgap, source_var_gid)


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
        if not isinstance(target, int) or target > state.gid_counter or target < 0:
            errmsg = "Invalid target ID: %s (gid_counter=%d)" % (target, state.gid_counter)
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

        if target.local:
            for source, weight, delay in zip(sources, weights, delays):
                if self.synapse_type is None:
                    self.synapse_type = weight >= 0 and 'excitatory' or 'inhibitory'
                if self.synapse_model == 'Tsodyks-Markram' and 'TM' not in self.synapse_type:
                    self.synapse_type += '_TM'
                synapse_object = getattr(target._cell, self.synapse_type)
                nc = state.parallel_context.gid_connect(int(source), synapse_object)
                nc.weight[0] = weight
                nc.delay = delay
                # nc.threshold is supposed to be set by ParallelContext.threshold, called in _build_cell(), above, but this hasn't been tested
                self.connections.append(Connection(source, target, nc))

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
        if a single such connection exists. If there are no such connections,
        X_ij will be NaN. If there are multiple such connections, the summed
        value will be given, which makes some sense for weights, but is
        pretty meaningless for delays. 
        """
        if format == 'list':
            values = [getattr(c, parameter_name) for c in self.connections]
        elif format == 'array':
            values = numpy.nan * numpy.ones((self.parent.pre.size, self.parent.post.size))
            for c in self.connections:
                value = getattr(c, parameter_name)
                addr = (self.parent.pre.id_to_index(c.source), self.parent.post.id_to_index(c.target))
                if numpy.isnan(values[addr]):
                    values[addr] = value
                else:
                    values[addr] += value
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
                   matrix (as returned by `get(format='array')`).
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


class GapConnectionManager(ConnectionManager):
    """
    Connects gap junctions instead of chemical synapses
    """
    #TODO: Implement this class

    def __init__(self, synapse_type, synapse_model=None, parent=None, var_gid_offset=0):
        """
        Create a new GapConnectionManager.
        
        `synapse_model` -- either None or 'Tsodyks-Markram'.
        `parent` -- the parent `Projection`
        `var_gid_offset` -- the offset to be used for the "source variable" gid used to register matching pc.source_var()/pc.target_var() calls. Needs to be unique for the simulation therefore this offset accounts for gid ranges previously reserved for other gap junction projections.
        """
        ConnectionManager.__init__(self, synapse_type, synapse_model, parent)
        self.var_gid_offset = var_gid_offset


    def connect(self, source, targets, weights, delays):
        raise NotImplementedError

    def convergent_connect(self, sources, target, weights, delays):
        raise NotImplementedError


# --- Initialization, and module attributes ------------------------------------

mech_path = os.path.join(pyNN_path[0], 'neuron', 'nmodl')
load_mechanisms(mech_path) # maintains a list of mechanisms that have already been imported
state = _State()  # a Singleton, so only a single instance ever exists
del _State
initializer = _Initializer()
del _Initializer

/*
 *  volume_transmitter.h
 *
 *  This file is part of NEST
 *
 *  Copyright (C) 2009-2010 by
 *  The NEST Initiative
 *
 *  See the file AUTHORS for details.
 *
 *  Permission is granted to compile and modify
 *  this file for non-commercial use.
 *  See the file LICENSE for details.
 *
 */

#ifndef VOLUME_TRANSMITTER_H
#define VOLUME_TRANSMITTER_H

#include "nest.h"
#include "event.h"
#include "archiving_node.h"
#include "ring_buffer.h"
#include "spikecounter.h"



/*BeginDocumentation

Name: volume_transmitter - Node used in combination with neuromodulated synaptic plasticity. It collects all spikes emitted by the population of neurons connected to the volume transmitter and transmits the signal to a user-specific subset of synapses.

Description:
The volume transmitter is used in combination with neuromodulated
synaptic plasticty, plasticity that depends not only on the activity
of the pre- and the postsynaptic neuron but also on a non-local
neuromodulatory third signal. It collects the spikes from all neurons
connected to the volume transmitter and delivers the spikes to a
user-specific subset of synapses.  It is assumed that the
neuromodulatory signal is a function of the spike times of all spikes
emitted by the population of neurons connected to the volume
transmitter.  The neuromodulatory dynamics is calculated in the
synapses itself. The volume transmitter interacts in a hybrid
structure with the neuromodulated synapses. In addition to the
delivery of the neuromodulatory spikes triggered by every pre-synaptic
spike, the neuromodulatory spike history is delivered in discrete time
intervals of a manifold of the minimal synaptic delay. In order to
insure the link between the neuromodulatory synapses and the volume
transmitter, the volume transmitter is passed as a parameter when a
neuromodulatory synapse is defined. The implementation is based on the
framework presented in [1].

Examples:
/volume_transmitter Create /vol Set
/iaf_neuron Create /pre_neuron Set
/iaf_neuron Create /post_neuron Set
/iaf_neuron Create /neuromod_neuron Set
/stdp_dopamine_synapse  << /vt vol >>  SetDefaults
neuromod_neuron vol Connect
pre_neuron post_neuron /stdp_dopamine_synapse Connect

Parameters:
deliver_interval - time interval given in d_min time steps, in which
                   the volume signal is delivered from the volume
                   transmitter to the assigned synapses

References:
[1] Potjans W, Morrison A and Diesmann M (2010). Enabling functional
    neural circuit simulations with distributed computing of
    neuromodulated plasticity.
    Front. Comput. Neurosci. 4:141. doi:10.3389/fncom.2010.00141

Author: Wiebke Potjans, Abigail Morrison
Receives: SpikeEvent

SeeAlso: stdp_dopamine_synapse

*/

namespace nest
{

  class Network;
  class Connector;

  /**
   * volume transmitter class.
   *
   * This class manages spike recording for normal and precise spikes. It
   * receives spikes via its handle(SpikeEvent&) method and buffers them. In the 
   * update() method it stores the newly collected buffer elements, which are delivered 
   * in time steps of (d_min*deliver_interval) to the neuromodulated synapses. 
   * In addition the synapses can ask the volume transmitter to deliver the elements stored
   * in the update() method with the method deliver_spikes().
   * 
   *
   *
   * @ingroup Devices
   */
  class volume_transmitter : public Archiving_Node 
  {
    
  public:        
    
    typedef Node base;

    volume_transmitter();
    volume_transmitter(const volume_transmitter&);
    
    bool has_proxies() const { return false; }
    bool local_receiver() const {return false;} 

    /**
     * Import sets of overloaded virtual functions.
     * We need to explicitly include sets of overloaded
     * virtual functions into the current scope.
     * According to the SUN C++ FAQ, this is the correct
     * way of doing things, although all other compilers
     * happily live without.
     */
  
    using Node::connect_sender;
    using Node::handle;

    void handle(SpikeEvent &);
    port connect_sender(SpikeEvent &, port);

    void get_status(DictionaryDatum &d) const;
    void set_status(const DictionaryDatum &d) ;

    const vector<spikecounter>& deliver_spikes();

    

  protected:

    /**
     * Update detector by recording spikes.
     *  
     * All spikes in the read_toggle() half of the spike buffer are 
     * recorded by passing them to the RecordingDevice, which then
     * stores them in memory or outputs them as desired.
     *
     * @see RecordingDevice
     */
   

    void register_connector(Connector& c);

    

  private:
    /**
     * Buffer for incoming spikes. 
     *
     * This data structure buffers all incoming spikes until they are
     * passed to the RecordingDevice for storage or output during update().
     * update() always reads from spikes_[network()->read_toggle()] and
     * deletes all events that have been read.
     *
     * Events arriving from locally sending nodes, i.e., devices without
     * proxies, are stored in spikes_[network()->write_toggle()], to ensure
     * order-independent results.
     *
     * Events arriving from globally sending nodes are delivered from the
     * global event queue by Scheduler::deliver_events() at the beginning
     * of the time slice. They are therefore written to spikes_[network()->read_toggle()]
     * so that they can be recorded by the subsequent call to update().
     * This does not violate order-independence, since all spikes are delivered
     * from the global queue before any node is updated.
     */
   

    void init_node_(Node const&);
    void init_state_(Node const&);
    void init_buffers_();
    void calibrate();
    

    void update(Time const &, const long_t, const long_t);


    // --------------------------------------------

    /**
     * Independent parameters of the model.
     */
    struct Parameters_ {
      Parameters_(); //!< Sets default parameter values
      void get(DictionaryDatum&) const; //!< Store current values in dictionary
      void set(const DictionaryDatum&);  //!< Set values from dicitonary
      long_t deliver_interval_; //!< update interval in d_min time steps
    }; 

      //-----------------------------------------------
      
    struct Buffers_{
      RingBuffer neuromodulatory_spikes_; //!< Buffer to store incoming spikes 
      vector<Connector*> targets_; //!< vector to store target synapses
      vector<spikecounter> spikecounter_; //!< vector to store and deliver spikes
    };
  
   
    struct Variables_{
      int_t counter_; //counts number of updates in d_min time steps up to deliver_interval
    };
  
    Parameters_ P_; 
    Variables_ V_;
    Buffers_ B_;

  };

 
  inline
  port volume_transmitter::connect_sender(SpikeEvent&, port receptor_type)
  {
    if (receptor_type != 0)
        throw UnknownReceptorType(receptor_type, get_name());
    return 0;
  }

  inline 
    void volume_transmitter::get_status(DictionaryDatum &d) const
  {
    P_.get(d);
    Archiving_Node::get_status(d);
  }

  inline
    void volume_transmitter::set_status(const DictionaryDatum &d)
  {
    Parameters_ ptmp = P_; // temporary copy in case of errors
    ptmp.set(d);                       // throws if BadProperty
    
    // We now know that (ptmp, stmp) are consistent. We do not 
    // write them back to (P_, S_) before we are also sure that 
    // the properties to be set in the parent class are internally 
    // consistent.
    Archiving_Node::set_status(d);

    // if we get here, temporaries contain consistent set of properties
    P_ = ptmp;
  }


  inline
  const vector<nest::spikecounter>& volume_transmitter::deliver_spikes()
  {
    return B_.spikecounter_;
  }

    
} // namespace

#endif /* #ifndef VOLUME_TRANSMITTER_H */

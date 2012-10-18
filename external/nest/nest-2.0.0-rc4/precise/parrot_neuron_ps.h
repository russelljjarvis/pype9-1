/*
 *  parrot_neuron_ps.h
 *
 *  This file is part of NEST
 *
 *  Copyright (C) 2009 by
 *  The NEST Initiative
 *
 *  See the file AUTHORS for details.
 *
 *  Permission is granted to compile and modify
 *  this file for non-commercial use.
 *  See the file LICENSE for details.
 *
 */

#ifndef PARROT_NEURON_PS_H
#define PARROT_NEURON_PS_H

#include "nest.h"
#include "event.h"
#include "node.h"
#include "slice_ring_buffer.h"
#include "connection.h"

/* BeginDocumentation
Name: parrot_neuron_ps - Neuron that repeats incoming spikes handling
precise spike times.

Description:
  The parrot neuron simply emits one spike for every incoming spike.
  One possible application for this is to create different channels
  for the output of generator devices such as the poisson_generator
  or the mip_generator.

Remarks:
  Network-wise the parrot neuron behaves like other neuron models
  regarding connections and communication. While the number of
  outgoing spikes equals that of incoming ones, the weigth of the 
  outgoing spikes solely depends on the weigth of outgoing connections.

  A Poisson generator that would send multiple spikes during a single
  time step due to a high rate will send single spikes with 
  multiple synaptic strength instead, for effiacy reasons.
  This can be realized because of the way devices are implemented
  in the threaded environment. A parrot neuron on the other 
  hand always emits single spikes. Hence, in a situation where for 
  example a poisson generator with a high firing rate is connected
  to a parrot neuron, the communication cost associated with outgoing
  spikes is much bigger for the latter.

  Please note that this node is capable of sending precise spike times
  to target nodes (on-grid spike time plus offset). If this node is
  connected to a spike_detector, the property "precise_times" of the
  spike_detector has to be set to true in order to record the offsets
  in addition to the on-grid spike times.

Parameters: 
  No parameters to be set in the status dictionary.

References:
  No references

Sends: SpikeEvent

Receives: SpikeEvent
  
Author: adapted from parrot_neuron by Kunkel
*/

namespace nest
{
  class parrot_neuron_ps :
    public Node
  {
  class Network;  
  
  public:        
    
    parrot_neuron_ps();

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

    port check_connection(Connection &, port);
    port connect_sender(SpikeEvent &, port);

    void get_status(DictionaryDatum &) const {}
    void set_status(const DictionaryDatum &) {}

    // uses off_grid events
    bool is_off_grid() const
    {
      return true;
    }

    void handle(SpikeEvent &);

  private:
      
    void init_node_(Node const &){}  // no parameters
    void init_state_(Node const &){} // no state
    void init_buffers_();
    void calibrate(){}               // no variables
    
    void update(Time const &, const long_t, const long_t);

    /** Queue for incoming events. */
    struct Buffers_
    {
      SliceRingBuffer events_;
    };
    
    Buffers_ B_;
  };

inline
port parrot_neuron_ps::check_connection(Connection &c, port receptor_type)
{
  SpikeEvent e;

  e.set_sender(*this);
  c.check_event(e);
  return c.get_target()->connect_sender(e, receptor_type);
}

inline
port parrot_neuron_ps::connect_sender(SpikeEvent &, port receptor_type)
{
  if ( receptor_type != 0 )
    throw UnknownReceptorType(receptor_type, get_name());
  return 0;
}
  
} // namespace

#endif //PARROT_NEURON_PS_H

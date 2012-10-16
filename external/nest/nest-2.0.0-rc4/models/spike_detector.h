/*
 *  spike_detector.h
 *
 *  This file is part of NEST
 *
 *  Copyright (C) 2004-2008 by
 *  The NEST Initiative
 *
 *  See the file AUTHORS for details.
 *
 *  Permission is granted to compile and modify
 *  this file for non-commercial use.
 *  See the file LICENSE for details.
 *
 */

#ifndef SPIKE_DETECTOR_H
#define SPIKE_DETECTOR_H


#include <vector>
#include "nest.h"
#include "event.h"
#include "node.h"
#include "recording_device.h"
#include "exceptions.h"

/* BeginDocumentation

Name: spike_detector - Device for detecting single spikes.

Description:
The spike_detector device is a recording device. It is used to record
spikes from a single neuron, or from multiple neurons at once. Data
is recorded in memory or to file as for all RecordingDevices. 
By default, GID and time of each spike is recorded.

The spike detector can also record spike times with full precision 
from neurons emitting precisely timed spikes. Set /precise_times to
achieve this.

Any node from which spikes are to be recorded, must be connected to 
the spike detector using a normal connect command. Any connection weight
and delay will be ignored for that connection.

Simulations progress in cycles defined by the minimum delay. During each
cycle, the spike detector records (stores in memory or writes to screen/file) 
the spikes generated during the previous cycle. As a consequence, any 
spikes generated during the cycle immediately preceding the end of the simulation
time will not be recorded. Setting the /stop parameter to at the latest one
min_delay period before the end of the simulation time ensures that all spikes
desired to be recorded, are recorded.

Spike are not necessarily written to file in chronological order.

Receives: SpikeEvent

SeeAlso: spike_detector, Device, RecordingDevice
*/


namespace nest
{

  class Network;

  /**
   * Spike detector class.
   *
   * This class manages spike recording for normal and precise spikes. It
   * receives spikes via its handle(SpikeEvent&) method, buffers them, and
   * stores them via its RecordingDevice in the update() method.
   *
   * Spikes are buffered in a two-segment buffer. We need to distinguish between
   * two types of spikes: those delivered from the global event queue (almost all 
   * spikes) and spikes delivered locally from devices that are replicated on VPs
   * (has_proxies() == false). 
   * - Spikes from the global queue are delivered by deliver_events() at the 
   *   beginning of each update cycle and are stored only until update() is called
   *   during the same update cycle. Global queue spikes are thus written to the
   *   read_toggle() segment of the buffer, from which update() reads.
   * - Spikes delivered locally may be delivered before or after 
   *   spike_detector::update() is executed. These spikes are therefore buffered in
   *   the write_toggle() segment of the buffer and output during the next cycle.
   * - After all spikes are recorded, update() clears the read_toggle() segment
   *   of the buffer.
   *
   * @ingroup Devices
   */
  class spike_detector : public Node
  {
    
  public:        
    
    spike_detector();
    spike_detector(const spike_detector&);
    
    bool has_proxies() const { return false; }
    bool local_receiver() const {return true;} 

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

    void get_status(DictionaryDatum &) const;
    void set_status(const DictionaryDatum &) ;

  private:

    void init_node_(Node const&);
    void init_state_(Node const&);
    void init_buffers_();
    void calibrate();
    void finalize();

    /**
     * Update detector by recording spikes.
     *  
     * All spikes in the read_toggle() half of the spike buffer are 
     * recorded by passing them to the RecordingDevice, which then
     * stores them in memory or outputs them as desired.
     *
     * @see RecordingDevice
     */
    void update(Time const &, const long_t, const long_t);

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
    struct Buffers_ {
      std::vector<std::vector<Event*> > spikes_; 
    };
    
    RecordingDevice device_;
    Buffers_ B_;

    bool user_set_precise_times_;
  };

  inline
  port spike_detector::connect_sender(SpikeEvent&, port receptor_type)
  {
    if (receptor_type != 0)
        throw UnknownReceptorType(receptor_type, get_name());
    return 0;
  }
  
  inline
  void spike_detector::finalize()
  {
    device_.finalize();
  }
    
} // namespace

#endif /* #ifndef SPIKE_DETECTOR_H */

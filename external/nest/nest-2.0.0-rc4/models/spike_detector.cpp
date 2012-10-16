/*
 *  spike_detector.cpp
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

#include "spike_detector.h"
#include "network.h"
#include "dict.h"
#include "integerdatum.h"
#include "doubledatum.h"
#include "dictutils.h"
#include "arraydatum.h"

#include <numeric>

nest::spike_detector::spike_detector()
        : Node(),
          device_(*this, RecordingDevice::SPIKE_DETECTOR, "gdf", true, true),  // record time and gid
          user_set_precise_times_(false)
{}

nest::spike_detector::spike_detector(const spike_detector &n)
        : Node(n),
          device_(*this, n.device_),
          user_set_precise_times_(n.user_set_precise_times_)
{}

void nest::spike_detector::init_node_(const Node& np)
{
  const spike_detector& sd = dynamic_cast<const spike_detector&>(np);
  device_.init_parameters(sd.device_);
}

void nest::spike_detector::init_state_(const Node& np)
{
  const spike_detector& sd = dynamic_cast<const spike_detector&>(np);
  device_.init_state(sd.device_);
}

void nest::spike_detector::init_buffers_()
{
  device_.init_buffers();

  std::vector<std::vector<Event*> > tmp(2, std::vector<Event*>());
  B_.spikes_.swap(tmp);
}

void nest::spike_detector::calibrate()
{
  if (! user_set_precise_times_)
  {
    DictionaryDatum d(new Dictionary);
    (*d)[names::precise_times] = network()->get_off_grid_communication();

    if (network()->get_off_grid_communication())
    {      
      (*d)[names::precision] = 15;

      network()->message(SLIInterpreter::M_INFO, "spike_detector::calibrate",
                         String::compose("Precise neuron models exist: the property precise_times "
                                         "of the %1 with gid %2 has been set to true, precision has "
                                         "been set to 15.", get_name(), get_gid()));
    }

    device_.set_status(d);
  }

  device_.calibrate();
}

void nest::spike_detector::update(Time const&, const long_t, const long_t)
{
  for(std::vector<Event*>::iterator 
      e = B_.spikes_[network()->read_toggle()].begin();
      e != B_.spikes_[network()->read_toggle()].end(); ++e)
  {
    assert(*e != 0);
    device_.record_event(**e);
    delete *e;
  }
  
  // do not use swap here to clear, since we want to keep the reserved()
  // memory for the next round
  B_.spikes_[network()->read_toggle()].clear();  
} 

void nest::spike_detector::get_status(DictionaryDatum &d) const
{
  // get the data from the device
  device_.get_status(d);

  // if we are the device on thread 0, also get the data from the
  // siblings on other threads
  if (get_thread() == 0)
  {
    const Compound* siblings = network()->get_thread_siblings(get_gid());
    std::vector<Node*>::const_iterator sibling;
    for (sibling = siblings->begin() + 1; sibling != siblings->end(); ++sibling)
      (*sibling)->get_status(d);
  }
}

void nest::spike_detector::set_status(const DictionaryDatum &d)
{
  if (d->known(names::precise_times))
    user_set_precise_times_ = true;

  device_.set_status(d);
}

void nest::spike_detector::handle(SpikeEvent & e)
{
  // accept spikes only if detector was active when spike was
  // emitted
  if ( device_.is_active(e.get_stamp()) )
  {
    assert(e.get_multiplicity() > 0);

    long_t dest_buffer;
    if ( e.get_sender().has_proxies() )
      dest_buffer = network()->read_toggle();   // events from central queue
    else
      dest_buffer = network()->write_toggle();  // locally delivered events

    for (int_t i = 0; i < e.get_multiplicity(); ++i)
    {
      // We store the complete events
      Event *event = e.clone();
      B_.spikes_[dest_buffer].push_back(event);
    }
  }
}

/*
 *  mip_generator.cpp
 *
 *  This file is part of NEST
 *
 *  Copyright (C) 2004 by
 *  The NEST Initiative
 *
 *  See the file AUTHORS for details.
 *
 *  Permission is granted to compile and modify
 *  this file for non-commercial use.
 *  See the file LICENSE for details.
 *
 */

#include "mip_generator.h"
#include "network.h"
#include "dict.h"
#include "random_datums.h"
#include "dictutils.h"
#include "exceptions.h"
#include "gslrandomgen.h"

/* ---------------------------------------------------------------- 
 * Default constructors defining default parameter
 * ---------------------------------------------------------------- */
    
nest::mip_generator::Parameters_::Parameters_()
  : rate_  (0.0),  // Hz
    p_copy_(1.0),
    mother_seed_(0),
    rng_()
{}

nest::mip_generator::Parameters_::Parameters_(const Parameters_& p)
  : rate_  (p.rate_),
    p_copy_(p.p_copy_),
    mother_seed_(p.mother_seed_),
    rng_(p.rng_)  // copies smart pointer
{}

/* ---------------------------------------------------------------- 
 * Parameter extraction and manipulation functions
 * ---------------------------------------------------------------- */

void nest::mip_generator::Parameters_::get(DictionaryDatum &d) const
{
  (*d)[names::rate]   = rate_;
  (*d)[names::p_copy] = p_copy_;
  (*d)[names::mother_seed] = mother_seed_;
}  

void nest::mip_generator::Parameters_::set(const DictionaryDatum& d)
{
  updateValue<double_t>(d, names::rate,   rate_);
  updateValue<double_t>(d, names::p_copy, p_copy_);
   
  if ( rate_ < 0 )
    throw BadProperty("Rate must be non-negative.");

  if ( p_copy_ < 0 || p_copy_ > 1 )
    throw BadProperty("Copy probability must be in [0, 1].");

  bool reset_rng = updateValue<librandom::RngPtr>(d, names::mother_rng, rng_);

  // order important to avoid short-circuitung
  reset_rng = updateValue<long_t>(d, names::mother_seed, mother_seed_) || reset_rng;
  
  if ( reset_rng )
    rng_->seed(mother_seed_);
}

/* ---------------------------------------------------------------- 
 * Default and copy constructor for node
 * ---------------------------------------------------------------- */

nest::mip_generator::mip_generator()
  : Node(),
    device_(), 
    P_()
{}

nest::mip_generator::mip_generator(const mip_generator& n)
  : Node(n), 
    device_(n.device_),
    P_(n.P_)
{
  // create new, private generator, not clean, as it ignores model status
  P_.rng_ = librandom::RandomGen::create_knuthlfg_rng(P_.mother_seed_);
}


/* ---------------------------------------------------------------- 
 * Node initialization functions
 * ---------------------------------------------------------------- */

void nest::mip_generator::init_node_(const Node& proto)
{
  const mip_generator& pr = downcast<mip_generator>(proto);

  device_.init_parameters(pr.device_);
  
  P_ = pr.P_;
}

void nest::mip_generator::init_state_(const Node& proto)
{ 
  const mip_generator& pr = downcast<mip_generator>(proto);

  device_.init_state(pr.device_);
}

void nest::mip_generator::init_buffers_()
{ 
  device_.init_buffers();
}

void nest::mip_generator::calibrate()
{
  device_.calibrate();

  // rate_ is in Hz, dt in ms, so we have to convert from s to ms
  V_.poisson_dev_.set_lambda(Time::get_resolution().get_ms() * P_.rate_ * 1e-3);
}


/* ---------------------------------------------------------------- 
 * Other functions
 * ---------------------------------------------------------------- */

void nest::mip_generator::update(Time const & T, const long_t from, const long_t to)
{
  assert(to >= 0 && (delay) from < Scheduler::get_min_delay());
  assert(from < to);

  for ( long_t lag = from ; lag < to ; ++lag )
  {
    if ( !device_.is_active(T) || P_.rate_ <= 0 )
      return; // no spikes to be generated
    
    // generate spikes of mother process for each time slice
    ulong_t n_mother_spikes = V_.poisson_dev_.uldev(P_.rng_);

    if ( n_mother_spikes )
    {
      DSSpikeEvent se;

      se.set_multiplicity(n_mother_spikes);
      network()->send(*this, se, lag);
    }
  }
}

void nest::mip_generator::event_hook(DSSpikeEvent& e)
{
  // note: event_hook() receives a reference of the spike event that
  // was originally created in the update function. there we set
  // the multiplicty to store the number of mother spikes. the *same*
  // reference will be delivered multiple times to the event hook,
  // once for every receiver. when calling handle() of the receiver
  // above, we need to change the multiplicty to the number of copied
  // child process spikes, so afterwards it needs to be reset to correctly 
  // store the number of mother spikes again during the next call of event_hook().
  // reichert

  librandom::RngPtr rng = net_->get_rng(get_thread());
  ulong_t n_mother_spikes = e.get_multiplicity();
  ulong_t n_spikes = 0;

  for (ulong_t n = 0; n < n_mother_spikes; n++)
  {
    if ( rng->drand() < P_.p_copy_ )
      n_spikes++;
  }

  if (n_spikes > 0)
  {
    e.set_multiplicity(n_spikes);
    e.get_receiver().handle(e);
  }

  e.set_multiplicity(n_mother_spikes);
}

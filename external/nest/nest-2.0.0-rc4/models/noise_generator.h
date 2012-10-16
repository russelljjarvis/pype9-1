/*
 *  noise_generator.h
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

#ifndef NOISE_GENERATOR_H
#define NOISE_GENERATOR_H


#include <vector>
#include "nest.h"
#include "event.h"
#include "node.h"
#include "stimulating_device.h"
#include "connection.h"
#include "normal_randomdev.h"

namespace nest
{

/* BeginDocumentation
Name: noise_generator - Device to generate Gaussian white noise current.
Description:
This device can be used to inject a Gaussian "white" noise current into a node.
The current is not really white, but a piecewise constant current with Gaussian 
distributed amplitude. The current changes at intervals of dt. dt must be a 
multiple of the simulation step size, the default is 1.0ms, 
corresponding to a 1kHz cut-off.

The current generated is given by

  I(t) = mean + std * N_j  for t_0 + j dt <= t < t_0 + (j-1) dt
  
where N_j are Gaussian random numbers with unit standard deviation and t_0 is
the device onset time.
  
Parameters:
The following parameters can be set in the status dictionary:

mean   double - mean value of the noise current in pA
std    double - standard deviation of noise current in pA
dt     double - interval between changes in current in ms, default 1.0ms

Remarks:
 - All targets receive different currents.
 - The currents for all targets change at the same points in time.
 - The effect of this noise current on a neuron DEPENDS ON DT. Consider
   the membrane potential fluctuations evoked when a noise current is 
   injected into a neuron. The standard deviation of these fluctuations
   across an ensemble will increase with dt for a given value of std.
   For the leaky integrate-and-fire neuron with time constant tau_m, 
   membrane potential fluctuations at times t_j+delay are given by
   
     Sigma = std * sqrt( (1-x) / (1+x) ) where x = exp(-dt/tau_m)
     
   for large t_j. In the white noise limit, dt -> 0, one has
   
     Sigma -> std * sqrt(dt / tau).
     
   To obtain comparable results for different values of dt, you must
   adapt std. 

Sends: CurrentEvent   
   
SeeAlso: Device

Author: Ported to NEST2 API 08/2007 by Jochen Eppler, updated 07/2008 by HEP
*/
  
  /**
   * Gaussian white noise generator.
   * Provide Gaussian "white" noise input current
   */
  class noise_generator: public Node
  {
    
  public:        
    
    noise_generator();
    noise_generator(const noise_generator&);

    bool has_proxies() const {return false;}

    /**
     * Import sets of overloaded virtual functions.
     * We need to explicitly include sets of overloaded
     * virtual functions into the current scope.
     * According to the SUN C++ FAQ, this is the correct
     * way of doing things, although all other compilers
     * happily live without.
     */

    using Node::event_hook;

    port check_connection(Connection&, port);

    void get_status(DictionaryDatum &) const;
    void set_status(const DictionaryDatum &) ;

  private:

    void init_node_(const Node&);
    void init_state_(const Node&);
    void init_buffers_();
    
    /**
     * Recalculates parameters and forces reinitialization
     * of amplitudes if number of targets has changed.
     */
    void calibrate();

    void update(Time const &, const long_t, const long_t);
    void event_hook(DSCurrentEvent&);

    // ------------------------------------------------------------
    
    typedef std::vector<double_t> AmpVec_;
    
    /**
     * Store independent parameters of the model.
     */
    struct Parameters_ {
      double_t mean_;   //!< mean current, in pA
      double_t std_;    //!< standard deviation of current, in pA
      Time     dt_;     //!< time interval between updates

      /**
       * Number of targets.
       * This is a hidden parameter; must be placed in parameters,
       * even though it is an implementation detail, since it 
       * concerns the connections and must not be affected by resets.
       */
      size_t num_targets_;
      
      Parameters_();  //!< Sets default parameter values
      Parameters_(const Parameters_&);

      void get(DictionaryDatum&) const;  //!< Store current values in dictionary
      void set(const DictionaryDatum&, const noise_generator&);  //!< Set values from dicitonary
    };

    // ------------------------------------------------------------
    
    struct Buffers_ {
      long_t  next_step_;  //!< time step of next change in current
      AmpVec_ amps_;       //!< amplitudes, one per target
    };
    
    // ------------------------------------------------------------

    struct Variables_ {
      long_t   dt_steps_; //!< update interval in steps
      librandom::NormalRandomDev normal_dev_;  //!< random deviate generator
    };
    
    // ------------------------------------------------------------

    StimulatingDevice<CurrentEvent> device_;
    Parameters_ P_;
    Variables_  V_;
    Buffers_    B_;
  };

  inline  
  port noise_generator::check_connection(Connection& c, port receptor_type)
  {
    DSCurrentEvent e;
    e.set_sender(*this);
    c.check_event(e);
    const port receptor = c.get_target()->connect_sender(e, receptor_type);
    ++P_.num_targets_;
    return receptor;
  }

  inline
  void noise_generator::get_status(DictionaryDatum &d) const
  {
    P_.get(d);
    device_.get_status(d);
  }

  inline
  void noise_generator::set_status(const DictionaryDatum &d)
  {
    Parameters_ ptmp = P_;  // temporary copy in case of errors
    ptmp.set(d, *this);               // throws if BadProperty

    // We now know that ptmp is consistent. We do not write it back
    // to P_ before we are also sure that the properties to be set
    // in the parent class are internally consistent.
    device_.set_status(d);

    // if we get here, temporaries contain consistent set of properties
    P_ = ptmp;
  }

}
#endif //NOISE_GENERATOR_H


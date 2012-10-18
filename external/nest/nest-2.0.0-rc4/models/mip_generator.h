/*
 *  mip_generator.h
 *
 *  This file is part of NEST
 *
 *  Copyright (C) 2006-2008 by
 *  The NEST Initiative
 *
 *  See the file AUTHORS for details.
 *
 *  Permission is granted to compile and modify
 *  this file for non-commercial use.
 *  See the file LICENSE for details.
 *
 */

#ifndef MIP_GENERATOR_H
#define MIP_GENERATOR_H
/****************************************/
/* class mip_generator                  */
/*       Vers. 1.0       moritz         */
/*                                      */
/****************************************/

#include "nest.h"
#include "event.h"
#include "node.h"
#include "stimulating_device.h"
#include "connection.h"
#include "poisson_randomdev.h"

namespace nest
{
//! class mip_generator                           
/*! Class mip_generator generates spike trains as described
    in the MIP model. 
*/


/*BeginDocumentation
Name: mip_generator - create spike trains as described by the MIP model.
Description:
  The mip_generator generates correlated spike trains using an Multiple Interaction 
  Process (MIP) as described in [1]. Underlying principle is a Poisson mother process 
  with rate r, the spikes of which are copied into the child processes with a certain 
  probability p. Every node the mip_generator is connected to receives a distinct
  child process as input, whose rate is p*r. The value of the pairwise correlation 
  coefficient of two child processes created by a MIP process equals p.

    
Parameters:
   The following parameters appear in the element's status dictionary:

   rate         double - Mean firing rate of the mother process in Hz
   p_copy       double - Copy probability
   mother_rng   rng    - Random number generator of mother process
   mother_seed  long   - Seed of RNG of mother process

Remarks:
   The MIP generator may emit more than one spike through a child process
   during a single time step, especially at high rates.  If this happens, 
   the generator does not actually send out n spikes.  Instead, it emits 
   a single spike with n-fold synaptic weight for the sake of efficiency.  
   Furthermore, note that as with the Poisson generator, different threads
   have their own copy of a MIP generator. By using the same mother_seed
   it is ensured that the mother process is identical for each of the 
   generators.
   
   IMPORTANT: mip_generator nodes will ALWAYS be created with a KNUTH_LFG
              random number generator, even if you have set a different 
              mother_rng using SetDefaults. You MUST CHANGE THE RNG OF
              THE INDIVIDUAL NODE if you want a different rng. This will
              be fixed in a future release.

Sends: SpikeEvent
              
References:
  [1] Alexandre Kuhn, Ad Aertsen, Stefan Rotter
      Higher-Order Statistics of Input Ensembles and the Response of Simple
      Model Neurons
      Neural Computation 15, 67-101 (2003)

Author: May 2006, Helias
SeeAlso: Device
*/

  /**
   * Class for MIP generator.
   * @todo Better handling of private random number generator, see #143.
   *       Most important: If RNG is changed in prototype by SetDefaults,
   *       then this is 
   */
  class mip_generator : public Node
  {

  public:
	 
    /** 
     * The generator is threaded, so the RNG to use is determined
     * at run-time, depending on thread. An additional RNG is used
     * for the mother process.
     */
    mip_generator();

    /**
     * Copy constructor. Called, when a new instance is created.
     * Needs to be overrwritten to initialize the random generator
     * for the mother process.
     */
    mip_generator(const mip_generator & rhs);

    bool has_proxies() const {return false;}


    using Node::event_hook;

    port check_connection(Connection&, port);
    
    void get_status(DictionaryDatum &) const;
    void set_status(const DictionaryDatum &) ;

  private:

    void init_node_(const Node&);
    void init_state_(const Node&);
    void init_buffers_();
    void calibrate();

    void update(Time const &, const long_t, const long_t);
    
    /**
     * @todo Should use binomial distribution
     */
    void event_hook(DSSpikeEvent&);

    // ------------------------------------------------------------
    
    /**
     * Store independent parameters of the model.
     * Mother RNG is a parameter since it can be changed. Not entirely in
     * keeping with persistence rules, since it changes state during 
     * updates. But okay in the sense that it thus is not reset on
     * ResetNetwork. Should go once we have proper global RNG scheme.
     */
    struct Parameters_ {
      double_t rate_;       //!< process rate in Hz
      double_t p_copy_;     //!< copy probability for each spike in the mother process
      ulong_t mother_seed_; //!< seed of the mother process
      librandom::RngPtr rng_; //!< random number generator for mother process
      
      Parameters_();  //!< Sets default parameter values
      Parameters_(const Parameters_&);

      void get(DictionaryDatum&) const;  //!< Store current values in dictionary
      void set(const DictionaryDatum&);  //!< Set values from dicitonary
    };
        
    // ------------------------------------------------------------

    struct Variables_ {
      librandom::PoissonRandomDev poisson_dev_;  //!< random deviate generator
    };
    
    // ------------------------------------------------------------

    StimulatingDevice<SpikeEvent> device_;
    Parameters_ P_;
    Variables_  V_;
    
  };

  inline
  port mip_generator::check_connection(Connection& c, port receptor_type)
  {
    DSSpikeEvent e;
    e.set_sender(*this);
    c.check_event(e);
    return c.get_target()->connect_sender(e, receptor_type);
  }

  inline
  void mip_generator::get_status(DictionaryDatum &d) const
  {
    P_.get(d);
    device_.get_status(d);
  }

  inline
  void mip_generator::set_status(const DictionaryDatum &d)
  {
    Parameters_ ptmp = P_;  // temporary copy in case of errors
    ptmp.set(d);                       // throws if BadProperty

    // We now know that ptmp is consistent. We do not write it back
    // to P_ before we are also sure that the properties to be set
    // in the parent class are internally consistent.
    device_.set_status(d);

    // if we get here, temporaries contain consistent set of properties
    P_ = ptmp;
  }

} // namespace nest

#endif

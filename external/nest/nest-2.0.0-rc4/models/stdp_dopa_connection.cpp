
/*
 *  stdp_connection.cpp
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

#include "network.h"
#include "dictdatum.h"
#include "connector_model.h"
#include "common_synapse_properties.h"
#include "stdp_dopa_connection.h"
#include "event.h"
#include "nestmodule.h"

namespace nest
{
  //
  // Implementation of class STDPDopaCommonProperties.
  //

  STDPDopaCommonProperties::STDPDopaCommonProperties() :
    CommonSynapseProperties(),
    vt_(0),
    tau_d_(200.0),
    tau_e_(1000.0),
    A_plus_(1.),
    tau_plus_(20.),
    A_minus_(1.5),
    tau_minus_(15.),
    dopa_base_(0.),
    Wmin_(0.),
    Wmax_(200.)
  { }

  void STDPDopaCommonProperties::get_status(DictionaryDatum & d) const
  {
    CommonSynapseProperties::get_status(d);
    if(vt_!= 0)
      def<long_t>(d, "vt", vt_->get_gid());
    else def<long_t>(d, "vt", -1);
    def<double_t>(d, "tau_d", tau_d_);
    def<double_t>(d, "tau_e", tau_e_);
    def<double_t>(d, "A_plus", A_plus_);
    def<double_t>(d, "tau_plus", tau_plus_);
    def<double_t>(d, "A_minus", A_minus_);
    def<double_t>(d, "tau_minus", tau_minus_);
    def<double_t>(d, "dopa_base", dopa_base_);
    def<double_t>(d, "Wmin", Wmin_);
    def<double_t>(d, "Wmax", Wmax_);
  }
  
  void STDPDopaCommonProperties::set_status(const DictionaryDatum & d, ConnectorModel &cm)
  {
    CommonSynapseProperties::set_status(d, cm);
 
    
    long_t vtgid;
    if(updateValue<long_t>(d, "vt", vtgid))
      {
	vt_ = dynamic_cast<volume_transmitter *>(NestModule::get_network().get_node(vtgid));

	if(vt_==0)
	  throw BadProperty("Dopamine source must be volume transmitter");
      }

    updateValue<double_t>(d, "tau_d", tau_d_);
    updateValue<double_t>(d, "tau_e", tau_e_);
    updateValue<double_t>(d, "A_plus", A_plus_);
    updateValue<double_t>(d, "tau_plus", tau_plus_);
    updateValue<double_t>(d, "A_minus", A_minus_);
    updateValue<double_t>(d, "tau_minus", tau_minus_);
    updateValue<double_t>(d, "dopa_base", dopa_base_);
    updateValue<double_t>(d, "Wmin", Wmin_);
    updateValue<double_t>(d, "Wmax", Wmax_);
  }


  Node* STDPDopaCommonProperties::get_node()
   {
     if(vt_==0)
       throw BadProperty("No volume transmitter has been assigned to the dopamine synapse.");
     else
       return vt_;
   }


  //
  // Implementation of class STDPConnectionHom.
  //

   STDPDopaConnection::STDPDopaConnection() :
     last_update_(0.),
     last_post_spike_(0.),
     last_e_update_(0.),
     eligibility_(0.0),
     last_dopa_spike_(0.),
     dopa_trace_(0.0),
     last_spike_(0.0)
       {
       }

  STDPDopaConnection::STDPDopaConnection(const STDPDopaConnection &rhs) :
    ConnectionHetWD(rhs)
  {
    last_update_ = rhs.last_update_;
    last_post_spike_ = rhs.last_post_spike_;
    last_e_update_ = rhs.last_e_update_;
    last_dopa_spike_ = rhs.last_dopa_spike_;
    last_spike_ = rhs.last_spike_;
    eligibility_ = rhs.eligibility_;
    dopa_trace_ = rhs.dopa_trace_;
  }

  void STDPDopaConnection::get_status(DictionaryDatum & d) const
   {

    // base class properties, different for individual synapse
   ConnectionHetWD::get_status(d);

   // own properties, different for individual synapse
   def<double_t>(d, "eligibility", eligibility_);
   def<double_t>(d, "dopa_trace", dopa_trace_);
   }
  
  void STDPDopaConnection::set_status(const DictionaryDatum & d, ConnectorModel &cm)
  {
    // base class properties
    ConnectionHetWD::set_status(d, cm);
     updateValue<double_t>(d, "eligibility", eligibility_);
     updateValue<double_t>(d, "dopa_trace", dopa_trace_);
  }

   /**
   * Set properties of this connection from position p in the properties
   * array given in dictionary.
   */  
  void STDPDopaConnection::set_status(const DictionaryDatum & d, index p, ConnectorModel &cm)
  {
    ConnectionHetWD::set_status(d, p, cm);

    if (d->known("tau_ds") ||
	d->known("A_pluss") ||
	d->known("tau_pluss")||
	d->known("A_minuss") ||
	d->known("tau_minuss")||
	d->known("dopa_bases")||
	d->known("Wmins")||
	d->known("Wmaxs"))
      {
	cm.network().message(SLIInterpreter::M_ERROR, "STDPDopaConnection::set_status()", "you are trying to set common properties via an individual synapse.");
      }
   
	set_property<double_t>(d, "dopa_traces", p, dopa_trace_);
	set_property<double_t>(d, "eligibility", p, eligibility_);
   
	}
     
  //}

   void STDPDopaConnection::initialize_property_arrays(DictionaryDatum & d) const
  {
    ConnectionHetWD::initialize_property_arrays(d);
    
    initialize_property_array(d, "dopa_traces");
    initialize_property_array(d, "eligibilitys");
    }
  
  /**
   * Append properties of this connection to the given dictionary. If the
   * dictionary is empty, new arrays are created first.
   */
  void STDPDopaConnection::append_properties(DictionaryDatum & d) const
  {
    ConnectionHetWD::append_properties(d);
    
    append_property<double_t>(d, "dopa_traces", dopa_trace_);
    append_property<double_t>(d, "eligibilitys", eligibility_);
  }

} // of namespace nest

/*
 *  modelsmodule.cpp
 *
 *  This file is part of NEST
 *
 *  Copyright (C) 2004-2006 by
 *  The NEST Initiative
 *
 *  See the file AUTHORS for details.
 *
 *  Permission is granted to compile and modify
 *  this file for non-commercial use.
 *  See the file LICENSE for details.
 *
 */

/* 
    This file is part of NEST

    modelsmodule.cpp -- sets up the modeldict with all models included
    with the NEST distribution. 

    Author(s): 
    Marc-Oliver Gewaltig
    R"udiger Kupper
    Hans Ekkehard Plesser

    First Version: June 2006
*/

#include "config.h"
#include "modelsmodule.h"
#include "network.h"
#include "model.h"
#include "genericmodel.h"
#include <string>

// Neuron models
#include "iaf_neuron.h"
#include "iaf_psc_alpha.h"
#include "iaf_psc_delta.h"
#include "iaf_psc_exp.h"
#include "iaf_tum_2000.h"
#include "iaf_cond_alpha.h"
#include "iaf_cond_exp.h"
#include "iaf_cond_exp_sfa_rr.h"
#include "iaf_cond_alpha_mc.h"
#include "aeif_cond_alpha.h"
#include "aeif_cond_exp.h"
#include "hh_psc_alpha.h"
#include "hh_cond_exp_traub.h"
#include "mat2_psc_exp.h"
#include "parrot_neuron.h"
#include "ht_neuron.h"
#include "sli_neuron.h"

// Stimulation devices
#include "ac_generator.h"
#include "dc_generator.h"
#include "spike_generator.h"
#include "poisson_generator.h"
#include "pulsepacket_generator.h"
#include "noise_generator.h"
#include "step_current_generator.h"
#include "mip_generator.h"
#include "smp_generator.h"

// Recording devices
#include "spike_detector.h"
#include "multimeter.h"
#include "correlation_detector.h"

//
#include "volume_transmitter.h"


// Generic (template) implementations for synapse prototypes
#include "generic_connector_model.h"
#include "generic_connector.h"

// Prototypes for synapses

#include "common_synapse_properties.h"
#include "static_connection.h"
#include "static_connection_hom_wd.h"
#include "cont_delay_connection.h"
#include "tsodyks_connection.h"
#include "stdp_connection.h"
#include "stdp_connection_hom.h"
#include "stdp_pl_connection_hom.h"
#include "stdp_dopa_connection.h"
#include "ht_connection.h"

#ifdef HAVE_MUSIC
#include "music_event_in_proxy.h"
#include "music_event_out_proxy.h"
#include "music_cont_in_proxy.h"
#include "music_message_in_proxy.h"
#endif

namespace nest
{
  // At the time when ModelsModule is constructed, the SLI Interpreter
  // must already be initialized. ModelsModule relies on the presence of
  // the following SLI datastructures: Name, Dictionary
  ModelsModule::ModelsModule(Network& net) :
          net_(net)
  {}

  ModelsModule::~ModelsModule() {}

  const std::string ModelsModule::name(void) const
  {
    return std::string("NEST Standard Models Module"); // Return name of the module
  }

  const std::string ModelsModule::commandstring(void) const
  {
    return std::string("/models-init /C++ ($Revision: 9031 $) provide-component "
                       "/models-init /SLI ($Revision: 9031 $) require-component");
  }

  //-------------------------------------------------------------------------------------

  void ModelsModule::init(SLIInterpreter *)
  {
    register_model<iaf_neuron>(net_,    "iaf_neuron");
    register_model<iaf_psc_alpha>(net_, "iaf_psc_alpha");
    register_model<iaf_psc_delta>(net_, "iaf_psc_delta");
    register_model<iaf_psc_exp>(net_,   "iaf_psc_exp");
    register_model<iaf_tum_2000>(net_,  "iaf_tum_2000");
    register_model<mat2_psc_exp>(net_,  "mat2_psc_exp");
    register_model<parrot_neuron>(net_, "parrot_neuron");

    register_model<ac_generator>(net_,           "ac_generator");
    register_model<dc_generator>(net_,           "dc_generator");
    register_model<spike_generator>(net_,        "spike_generator");
    register_model<poisson_generator>(net_,      "poisson_generator");
    register_model<pulsepacket_generator>(net_,  "pulsepacket_generator");
    register_model<noise_generator>(net_,        "noise_generator");
    register_model<step_current_generator>(net_, "step_current_generator");
    register_model<mip_generator>(net_,          "mip_generator");
    register_model<smp_generator>(net_,          "smp_generator");
    register_model<sli_neuron>(net_,             "sli_neuron");

    register_model<spike_detector>(net_,       "spike_detector");
    register_model<Multimeter>(net_,           "multimeter");
    register_model<correlation_detector>(net_, "correlation_detector");
    register_model<volume_transmitter>(net_, "volume_transmitter");

    // Create voltmeter as a multimeter pre-configured to record V_m.
    Dictionary vmdict;
    ArrayDatum ad;
    ad.push_back(LiteralDatum(names::V_m.toString())); 
    vmdict[names::record_from] = ad;
    register_preconf_model<Multimeter>(net_, "voltmeter", vmdict);

#ifdef HAVE_GSL
    register_model<iaf_cond_alpha>(net_,      "iaf_cond_alpha");
    register_model<iaf_cond_exp>(net_,        "iaf_cond_exp");
    register_model<iaf_cond_exp_sfa_rr>(net_, "iaf_cond_exp_sfa_rr");
    register_model<iaf_cond_alpha_mc>(net_,   "iaf_cond_alpha_mc");
    register_model<hh_psc_alpha>(net_,        "hh_psc_alpha");
    register_model<hh_cond_exp_traub>(net_,   "hh_cond_exp_traub");
#endif

#ifdef HAVE_GSL_1_11
    register_model<aeif_cond_alpha>(net_, "aeif_cond_alpha");
    register_model<aeif_cond_exp>(net_, "aeif_cond_exp");
    register_model<ht_neuron>(net_,       "ht_neuron");
#endif

#ifdef HAVE_MUSIC 
    //// proxies for inter-application communication using MUSIC
    register_model<music_event_in_proxy>(net_, "music_event_in_proxy");
    register_model<music_event_out_proxy>(net_, "music_event_out_proxy");
    register_model<music_cont_in_proxy>(net_, "music_cont_in_proxy");
    register_model<music_message_in_proxy>(net_, "music_message_in_proxy");
#endif

    // register synapses

    // static connection with weight, delay, rport, target
    register_prototype_connection<StaticConnection>(net_,    "static_synapse");

    // static connection with rport, target and common weight and delay
    register_prototype_connection_commonproperties_hom_d < StaticConnectionHomWD,
                                                            CommonPropertiesHomWD
                                                          > (net_, "static_synapse_hom_wd");

    register_prototype_connection<ContDelayConnection>(net_, "cont_delay_synapse");
    register_prototype_connection<TsodyksConnection>(net_,   "tsodyks_synapse");
    register_prototype_connection<STDPConnection>(net_,      "stdp_synapse");
    register_prototype_connection<HTConnection>(net_,        "ht_synapse");

    register_prototype_connection_commonproperties < STDPConnectionHom, 
                                                     STDPHomCommonProperties 
                                                   > (net_, "stdp_synapse_hom");

    register_prototype_connection_commonproperties < STDPPLConnectionHom,
                                                     STDPPLHomCommonProperties 
                                                   > (net_, "stdp_pl_synapse_hom");
    register_prototype_connection_commonproperties <STDPDopaConnection, 
                                                    STDPDopaCommonProperties
                                                   > (net_, "stdp_dopamine_synapse");

  }

} // namespace nest

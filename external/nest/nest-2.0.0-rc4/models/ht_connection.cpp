/*
 *  ht_connection.cpp
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

#include "ht_connection.h"
#include "network.h"
#include "connector_model.h"
#include "exceptions.h"

namespace nest
{

  HTConnection::HTConnection() :
    ConnectionHetWD(),
    tau_P_(50.0),
    delta_P_(0.2),
    p_(1.0)
  { }

  void HTConnection::get_status(DictionaryDatum & d) const
  {
    ConnectionHetWD::get_status(d);

    def<double_t>(d, "tau_P", tau_P_);
    def<double_t>(d, "delta_P", delta_P_);
    def<double_t>(d, "P", p_);
  }
  
  void HTConnection::set_status(const DictionaryDatum & d, ConnectorModel &cm)
  {
    ConnectionHetWD::set_status(d, cm);

    updateValue<double_t>(d, "tau_P", tau_P_);
    updateValue<double_t>(d, "delta_P", delta_P_);
    updateValue<double_t>(d, "P", p_);

    if ( tau_P_ <= 0.0 ) 
      throw BadProperty("tau_P >= 0 required.");

    if ( delta_P_ < 0.0 || delta_P_ > 1.0 )
      throw BadProperty("0 <= delta_P <= 1 required.");

    if ( p_ < 0.0 || p_ > 1.0 )
      throw BadProperty("0 <= P <= 1 required.");
  }

  /**
   * Set properties of this connection from position p in the properties
   * array given in dictionary.
   */  
  void HTConnection::set_status(const DictionaryDatum & d, 
				  index p, ConnectorModel &cm)
  {
    ConnectionHetWD::set_status(d, p, cm);

    set_property<double_t>(d, "tau_Ps", p, tau_P_);
    set_property<double_t>(d, "delta_Ps", p,  delta_P_);
    set_property<double_t>(d, "Ps", p, p_);
  }

  void HTConnection::initialize_property_arrays(DictionaryDatum & d) const
  {
    ConnectionHetWD::initialize_property_arrays(d);

    initialize_property_array(d, "tau_Ps"); 
    initialize_property_array(d, "delta_Ps"); 
    initialize_property_array(d, "Ps"); 
  }

  /**
   * Append properties of this connection to the given dictionary. If the
   * dictionary is empty, new arrays are created first.
   */
  void HTConnection::append_properties(DictionaryDatum & d) const
  {
    ConnectionHetWD::append_properties(d);

    append_property<double_t>(d, "tau_Ps", tau_P_);
    append_property<double_t>(d, "delta_Ps",  delta_P_);
    append_property<double_t>(d, "Ps", p_);
  }

} // of namespace nest

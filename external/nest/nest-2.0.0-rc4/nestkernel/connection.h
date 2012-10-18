/*
 *  connection.h
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

#ifndef CONNECTION_H
#define CONNECTION_H

#include "nest_time.h"
#include "nest_timeconverter.h"
#include "nest.h"
#include "node.h"
#include "event.h"
#include "dict.h"
#include "dictutils.h"
#include "arraydatum.h"
#include "doubledatum.h"
#include "common_synapse_properties.h"
#include "nest_names.h"
#include "generic_connector_model.h"
#include "spikecounter.h"

namespace nest
{

class ConnectorModel;

/**
 * Base class for representing connections.
 * It provides the mandatory properties receiver port and target,
 * as well as the functions get_status() and set_status()
 * to read and write them. A suitable Connector containing these
 * connections can be obtained from the template GenericConnector.
 */
class Connection
{

 public:

  /**
   * Default Constructor. Sets default values for all parameters.
   * Needed by GenericConnectorModel.
   */
  Connection();

  /**
   * Copy Constructor.
   */
  Connection(const Connection& c);

  /**
   * Default Destructor.
   */
  virtual ~Connection() {}

  /**
   * Get all properties of this connection and put them into a dictionary.
   */
  virtual
  void get_status(DictionaryDatum & d) const;

  /**
   * Create new empty arrays for the properties of this connection in the given
   * dictionary. It is assumed that they are not existing before.
   */
  void initialize_property_arrays(DictionaryDatum & d) const;

  /**
   * Append properties of this connection to the given dictionary. It is
   * assumed that the arrays were created by initialize_property_arrays()
   */
  void append_properties(DictionaryDatum & d) const;

  /**
   * Calibrate the delay of this connection to the desired resolution.
   */
  //virtual
  //void calibrate();

  /**
   * This function calls check_connection() on the sender to check if the receiver
   * accepts the event type and receptor type requested by the sender.
   * \param s The source node
   * \param r The target node
   * \param receptor The ID of the requested receptor type
   * \param the last spike produced by the presynaptic neuron (for STDP and maturing connections) 
   */
  virtual void check_connection(Node & s, Node & r, port receptor, double_t t_lastspike);

  /**
   * This function checks if the event type is supported by the concrete
   * event type. In the base class it just throws UnsupportedEvent. In
   * the derived Connection classes it can be implemented as an empty
   * function to indicate that the event type is supported.
   */
  virtual void check_event(SpikeEvent&);

  virtual void check_event(RateEvent&);
  virtual void check_event(DataLoggingRequest&);
  virtual void check_event(CurrentEvent&);
  virtual void check_event(ConductanceEvent&);
  virtual void check_event(DoubleDataEvent&);

  // We must handle DSSpikeEvent and DSCurrentEvent explicitly instead
  // of subsuming them under SpikeEvent and CurrentEvent via inheritance,
  // as they must only be transmitted via static_synapse.
  virtual void check_event(DSSpikeEvent&);
  virtual void check_event(DSCurrentEvent&);

  /**
   * Return the rport of the connection
   */
  long_t get_rport() const;

  /**
   * Return the target of the connection
   */
  Node *get_target() const;

  /**
   * triggers an update of a synaptic weight
   * this function is needed for neuromodulated synaptic plasticity 
   */
  void trigger_update_weight(const std::vector<spikecounter> &,const CommonSynapseProperties &){};
  
  

 protected:

  Node *target_;       //!< Target node
  long_t rport_;       //!< Receiver port at the target node
};

inline
void Connection::check_connection(Node & s, Node & r, port receptor_type, double_t)
{
  target_ = &r;
  rport_ = s.check_connection(*this, receptor_type);
}

inline
long_t Connection::get_rport() const
{
  return rport_;
}

inline
Node *Connection::get_target() const
{
  return target_;
}

inline
void Connection::check_event(SpikeEvent&)
{
  throw UnsupportedEvent();
}

inline
void Connection::check_event(DSSpikeEvent&)
{
  throw UnsupportedEvent();
}

inline
void Connection::check_event(RateEvent&)
{
  throw UnsupportedEvent();
}

inline
void Connection::check_event(DataLoggingRequest&)
{
  throw UnsupportedEvent();
}

inline
void Connection::check_event(CurrentEvent&)
{
  throw UnsupportedEvent();
}

inline
void Connection::check_event(DSCurrentEvent&)
{
  throw UnsupportedEvent();
}

inline
void Connection::check_event(ConductanceEvent&)
{
  throw UnsupportedEvent();
}

inline
void Connection::check_event(DoubleDataEvent&)
{
  throw UnsupportedEvent();
}

template<typename PropT>
bool set_property(const DictionaryDatum & d, Name propname, index p, PropT &prop)
{
  if (d->known(propname))
  {
    ArrayDatum* arrd = 0;
    arrd = dynamic_cast<ArrayDatum*>((*d)[propname].datum());
    if (! arrd)
    {
      ArrayDatum const arrd;
      throw TypeMismatch(arrd.gettypename().toString(), 
                         (*d)[propname].datum()->gettypename().toString());
    }
    
    prop = (*arrd)[p];
    return true;
  }

  return false;
}

} // namespace nest

#endif // CONNECTION_H

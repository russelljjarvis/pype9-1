/*
 *  node.cpp
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


#include "node.h"
#include "exceptions.h"
#include "event.h"
#include "network.h"
#include "namedatum.h"
#include "arraydatum.h"
#include "dictutils.h"

namespace nest {

  Network *Node::net_=NULL;

  Node::Node()
      :gid_(0),
       lid_(0),
       model_id_(-1),
       parent_(0),
       stat_(),
       thread_(0),
       vp_(invalid_thread_)
  {
    /**
     *
     * the scheduler starts with update_reference()==true,
     * thus we must reset the updated flag, so that
     * is_updated() will return false at the beginning of
     * each time-slice.
     */
    stat_.reset(updated); //!< Set to opposite of the scheduler's value
  }

  Node::Node(const Node &n)
      :gid_(0),
       lid_(0),
       model_id_(n.model_id_),
       parent_(n.parent_),
       stat_(n.stat_),  // copied from model prototype, frozen may be set
       thread_(n.thread_),
       vp_(n.vp_)
  {
  }

  Node::~Node()
  {}

  void Node::init_node()
  {
    Model const * const model= net_->get_model(model_id_);
    assert(model);
    init_node_(model->get_prototype());
  }

  void Node::init_state()
  {
    Model const * const model= net_->get_model(model_id_);
    assert(model);
    init_state_(model->get_prototype());
  }
  
  void Node::init_buffers()
  {
    if ( stat_.test(buffers_initialized) )
      return;
      
    init_buffers_();
    stat_.set(buffers_initialized);
  }
  
  std::string Node::get_name() const
  {
    if(net_==0 || model_id_<0)
      return std::string("UnknownNode");

    return net_->get_model(model_id_)->get_name();
  }

  Model & Node::get_model_() const
  {
    if(net_==0 || model_id_<0)
      throw UnknownModelID(model_id_);
    
    return *net_->get_model(model_id_);
  }      

  bool Node::is_updated() const
  {
    return stat_.test(updated)==net_->update_reference();
  }

  bool Node::is_local() const
  {
    return net_->is_local_vp(get_vp());
  }

  DictionaryDatum Node::get_status_dict_()
  {
    return DictionaryDatum(new Dictionary);
  }  

  DictionaryDatum Node::get_status_base()
  {
    DictionaryDatum dict = get_status_dict_();

    assert(dict.valid());
    if(parent_ != NULL)
      {
	(*dict)[names::address] = Token(net_->get_adr(this));
	(*dict)[names::global_id] = get_gid();
	(*dict)[names::local_id] = get_lid()+1;
	(*dict)[names::parent] = (parent_ != 0) ?parent_->get_gid():0;
      }
    (*dict)[names::model] = LiteralDatum(get_name());
    (*dict)[names::state] = get_status_flag();
    (*dict)[names::thread] = get_thread();
    (*dict)[names::vp] = get_vp();
    (*dict)[names::local] = is_local();
    (*dict)[names::frozen] = is_frozen();

    // now call the child class' hook
    get_status(dict);

    assert(dict.valid());
    return dict;
  }

  void Node::set_status_base(const DictionaryDatum &dict)
  {
    assert(dict.valid());

    // We call the child's set_status first, so that the Node remains
    // unchanged if the child should throw an exception.
    set_status(dict);

    if(dict->known(names::frozen))
    {
      bool frozen_val=(*dict)[names::frozen];

      if( frozen_val == true )
	set(frozen);
      else
	unset(frozen);
    }
  }

  /**
   * Default implementation of just throws UnexpectedEvent
   */
  port Node::check_connection(Connection&, port)
  {
    throw UnexpectedEvent();
    return invalid_port_;
  }

  /**
   * Default implementation of register_stdp_connection() just 
   * throws IllegalConnection
   */
  void Node::register_stdp_connection(double_t)
  {
    throw IllegalConnection();
  }

  /**
   * Default implementation of unregister_stdp_connection() just 
   * throws IllegalConnection
   */
  void Node::unregister_stdp_connection(double_t)
  {
    throw IllegalConnection();
  }

  /**
   * Default implementation of event handlers just throws
   * an UnexpectedEvent exception.
   * @see class UnexpectedEvent
   * @throws UnexpectedEvent  This is the default event to throw.
   */
  void Node::handle(SpikeEvent&)
  {
    throw UnexpectedEvent();
  }

  port Node::connect_sender(SpikeEvent&, port)
  {
    throw IllegalConnection();
    return invalid_port_;
  }

  void Node::handle(RateEvent&)
  {
    throw UnexpectedEvent();
  }

  port Node::connect_sender(RateEvent&, port)
  {
    throw IllegalConnection();
    return invalid_port_;
  }

  void Node::handle(CurrentEvent&)
  {
    throw UnexpectedEvent();
  }

  port Node::connect_sender(CurrentEvent&, port)
  {
    throw IllegalConnection();
    return invalid_port_;
  }

  void Node::handle(DataLoggingRequest&)
  {
    throw UnexpectedEvent();
  }

  port Node::connect_sender(DataLoggingRequest&, port)
  {
    throw IllegalConnection();
    return invalid_port_;
  }

  void Node::handle(DataLoggingReply&)
  {
    throw UnexpectedEvent();
  }

  void Node::handle(ConductanceEvent&)
  {
    throw UnexpectedEvent();
  }

  port Node::connect_sender(ConductanceEvent&, port)
  {
    throw IllegalConnection();
    return invalid_port_;
  }

  void Node::handle(DoubleDataEvent&)
  {
    throw UnexpectedEvent();
  }

  port Node::connect_sender(DoubleDataEvent&, port)
  {
    throw IllegalConnection();
    return invalid_port_;
  }

  double_t Node::get_K_value(double_t)
  {
    throw UnexpectedEvent();
    return 0;
  }


  void Node::get_K_values(double_t, double_t&, double_t&)
  {
    throw UnexpectedEvent();
  }

  void nest::Node::get_history(double_t, double_t,
			       std::deque<histentry>::iterator*,
			       std::deque<histentry>::iterator*)
  {
    throw UnexpectedEvent();
  }


  void Node::event_hook(DSSpikeEvent& e)
  {
    e.get_receiver().handle(e);
  }

  void Node::event_hook(DSCurrentEvent& e)
  {
    e.get_receiver().handle(e);
  }

  bool Node::allow_entry() const
  {
    return false;
  }

} // namespace

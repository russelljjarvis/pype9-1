/*
 *  model.h
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

#ifndef MODEL_H
#define MODEL_H
#include <string>

#include <new>
#include <vector>
#include "node.h"
#include "allocator.h"
#include "dictutils.h"

namespace nest
{

  /**
   * Base class for all Models.
   * Each Node class is associated with a corresponding Model
   * class. The Model class is responsible for the creation and class
   * wide parametrisation of its associated Node objects.
   *
   * class Model manages the thread-sorted memory pool of the model.
   * The default constructor uses one thread as default. Use set_threads() to
   * use more than one thread.
   * @ingroup user_interface
   * @see Node
   */
  class Model
  {
  public:

    Model(const std::string& name);

    virtual ~Model(){}

    /**
     * Create clone with new name.
     */
    virtual Model* clone(const std::string&) const =0;

    /**
     * Set number of threads based on number set in network.
     * As long as no nodes of the model have been allocated, the number
     * of threads may be changed.
     * @note Requires that network pointer in NestModule is initialized.
     */
    void set_threads();

    /**
     * Allocate new Node and return its pointer.
     * allocate() is not const, because it
     * is allowed to modify the Model object for
     * 'administrative' purposes.
     */
    Node* allocate(thread t);

    void  free(thread t, Node *);

    /**
     * Deletes all nodes which belong to this model.
     */

    void clear();

    /**
     * Reserve memory for at least n new Nodes.
     * A number of memory managers work more efficently, if they have
     * an idea about the number of Nodes to be allocated.
     * This function prepares the memory manager for the subsequent
     * allocation of n Nodes.
     * @param t Thread for which the Nodes are reserved.
     * @param n Number of Nodes to be allocated.
     * @note Due to the semantics of sli::pool::reserve(), this reserve()
     * ensures that there is space for at least @b additional @b nodes. 
     * This is different from the semantics of reserve() for STL containers.
     * @todo Adapt the semantics of reserve() to STL semantics.
     */
    void  reserve(thread t, size_t n);

    /**
     * Return name of the Model.
     * This function returns the name of the Model as C++ string. The
     * name is defined by the constructor. The result is identical to the value
     * of Node::get_name();
     * @see Model::Model()
     * @see Node::get_name()
     */
    std::string get_name() const;

    /**
     * Return the available memory. The result is given in number of elements,
     * not in bytes.
     * Note that this function reports a sum over all threads.
     */
    size_t mem_available();

    /**
     * Return the memory capacity. The result is given in number of elements,
     * not in bytes.
     * Note that this function reports a sum over all threads.
     */
    size_t mem_capacity();

    virtual bool has_proxies()=0;
    virtual bool one_node_per_process()=0;
    virtual bool is_off_grid()=0;
 
    /**
     * Change properties of the prototype node according to the
     * entries in the dictionary.
     * @param d Dictionary with named parameter settings.
     * @ingroup status_interface
     */
    void set_status(DictionaryDatum);

    /**
     * Export properties of the prototype node by setting
     * entries in the status dictionary.
     * @param d Dictionary.
     * @ingroup status_interface
     */
    DictionaryDatum get_status(void) ;

    virtual port check_connection(Connection&, port)=0;

    /**
     * Return the size of the prototype.
     */
    virtual 
    size_t get_element_size() const=0;

    /**
     * Return const reference to the prototype.
     */
    virtual 
    Node const& get_prototype(void) const =0;

    /**
     * Set the model id on the prototype.
     */
    virtual
    void set_model_id(int) =0;
    
        
  private:
  
    virtual 
    void set_status_(DictionaryDatum)=0;

    virtual 
    DictionaryDatum get_status_()=0;


    /**
     * Set the number of threads.
     * @see set_threads()
     */
    void set_threads_(thread t);
  
    /**
     * Initialize the pool allocator with the Node specific values.
     */
    virtual
    void init_memory_(sli::pool &)=0;

    /**
     * Allocate a new object at the specified memory position.
     */
    virtual
    Node * allocate_(void *)=0;

    /**
     * Name of the Model.
     * This name will be used to identify all Nodes which are
     * created by this model object.
     */
    std::string name_;

    /**
     * Memory for all nodes sorted by threads.
     */
    std::vector<sli::pool> memory_;

  };


  inline
  Node * Model::allocate(thread t)
  {
    assert((size_t)t < memory_.size());
    return allocate_(memory_[t].alloc());
  }

  inline
  void Model::free(thread t, Node *n)
  {
    assert((size_t)t < memory_.size());
    memory_[t].free(n);
  }

  inline
  std::string Model::get_name() const
  {
    return name_;
  }

}
#endif











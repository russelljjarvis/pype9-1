#ifndef RECORDABLES_MAP_H
#define RECORDABLES_MAP_H

/*
 *  recordables_map.h
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

#include <map>
#include <utility>
#include <cassert>
#include <string>
#include "name.h"
#include "nest.h"
#include "arraydatum.h"

namespace nest
{
  /**
   * Map names of recordables to data access functions.
   *
   * This map identifies the data access functions for recordable
   * state variables in model neurons. 
   * Each neuron model shall have exactly one static instance
   * of RecordablesMap.
   *
   * @note The map is initialized by the create() member function
   *       and not by the constructor because of the following 
   *       conflict: The recordablesMap_ shall be a private static
   *       member of its host class, since the same map applies
   *       to all objects. Creation by a constructor leads to static
   *       initialization conflicts with the Name class. Thus,
   *       creation is deferred to the plain constructor of the host
   *       Node class, which is called only once to create the
   *       model prototype instance.
   *
   * @note As a work-around to solve #339, this class uses std::string
   *       internally as map-key, as use of Name caused #339. The
   *       interface still requests Name, to signal to programmers 
   *       that they should use standardized names, and since internally,
   *       we should go back to Name as key once #348 is solved.
   *
   * @todo Replace std::string by Name as key when #348 is solved.
   *
   * @see multimeter, UniversalDataLogger
   * @ingroup Devices
   */
  template <typename HostNode>
  class RecordablesMap
    : public std::map<std::string, 
		      double_t (HostNode::*)() const>
  {
    typedef std::map<std::string, double_t (HostNode::*)() const> Base_;

  public:
    virtual ~RecordablesMap() {}

    //! Datatype for access functions
    typedef double_t (HostNode::*DataAccessFct)() const;
    
    /**
     * Create the map.
     * This function must be specialized for each class owning a
     * Recordables map and must fill the map. This should happen
     * as part of the original constructor for the Node.
     */
    void create() { assert(false); }

    /**
     * Obtain SLI list of all recordables, for use by get_status().
     * @todo This fct should return the recordables_ entry, but since
     *       filling recordables_ leads to seg fault on exit, we just
     *       build the list every time, even though that beats the
     *       goal of being more efficient ...
     */
    ArrayDatum get_list() const { 
      ArrayDatum recordables;
      for ( typename Base_::const_iterator it = this->begin() ;
	    it != this->end() ; ++it )
      recordables.push_back(LiteralDatum(it->first));
      return recordables;

      // the entire function should just be
      // return recordables_;
    }

  private:

    //! Insertion functions to be used in create(), adds entry to map and list
    void insert_(const Name& n, const DataAccessFct f)
    {
      Base_::insert(std::make_pair(n.toString(), f));

      // Line below leads to seg-fault if nest is quit right after start,
      // see comment on get_list()
      // recordables_.push_back(LiteralDatum(n));
    }
    
    /** 
     * SLI list of names of recordables
     * @todo Once the segfault-on-exit issue mentioned in the comment on
     * get_list() is resolved, the next code line should be activated again.
     *
     */
    // ArrayDatum recordables_; 
  };
}

#endif


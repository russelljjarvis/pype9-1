/*
 *  nodelist.h
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

#ifndef NODELIST_H
#define NODELIST_H

#include "node.h"
#include "compound.h"

namespace nest{

  /** 
   * List interface to network tree.  class NodeList is an adaptor
   * class which turns a Network object into a list.  Class NodeList
   * also provides an iterator which can be used to traverse the
   * network tree in post-order.  This iterator is not used during
   * Network update, since it is not thread safe.
   * For a list interface that only accesses the leaves of a network
   * tree, excluding the intermediate compounds, see class LeafList
   * and its iterator.
   */

  class NodeList
  {
  public:

    class iterator
    {
      friend class NodeList;
    public:
      iterator():p_(){}
    private:
      iterator(vector<Node*>::iterator const &p):p_(p){}
    public:
      iterator operator++();

      Node*    operator*();
      Node const*  operator*() const;

      bool operator==(const iterator&) const;
      bool operator!=(const iterator&) const;

    private:
      vector<Node *>::iterator p_;  //!< iterator to the current node
    };

    NodeList():root_(NULL){}
    explicit NodeList(Compound &c):root_(&c){};

    iterator begin() const;
    iterator end()   const;
    iterator lend()   const;

    bool   empty()   const;
    size_t size()    const;

    Compound& get_root() const;
    void set_root(Compound &);

  private:
    Compound *root_;  //!< root of the network

  };

  inline 
  bool NodeList::empty() const
  {
    return root_->empty();
  }

  inline
  size_t NodeList::size() const
  {
    return root_->size();
  }

  inline
  NodeList::iterator NodeList::end() const
  {
    Compound *p=root_->get_parent();
    return iterator(p == NULL ? root_->end() : p->begin()+root_->get_lid());
  }

  inline
  NodeList::iterator NodeList::lend() const
  {
    return iterator(root_->begin());
  }

  inline
  bool NodeList::iterator::operator==(const iterator&i) const
  {
    return p_==i.p_;
  }

  inline
  bool NodeList::iterator::operator!=(const iterator&i) const
  {
    return p_!=i.p_;
  }

  inline
  Node* NodeList::iterator::operator*()
  {
    return *p_;
  }

  inline
  Node const * NodeList::iterator::operator*() const
  {
    return *p_;
  }
}
#endif

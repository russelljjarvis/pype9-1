/*
 *  dictutils.h
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

#ifndef DICTUTILS_H
#define DICTUTILS_H

#include "dictdatum.h"
#include "namedatum.h"
#include "tokenutils.h"
#include "arraydatum.h"
#include <string>
#include <algorithm>
#include <functional>

/**
 * @defgroup DictUtils How to access the value contained in a Token contained in a dictionary.
 * @ingroup TokenHandling
 *
 * Class Dictionary defines the standard user interface for accessing tokens
 * from dictionaries (see there). However, this user interface returns
 * tokens, from which the actual value would still need to be
 * extracted. The utilitiy functions described in this group shortcut
 * this step and provide direct access to the underlying fundamental
 * values associated to a dictionary entry.
 */

/** Get the value of an existing dictionary entry.
 * @ingroup DictUtils
 * @throws UnknownName An entry of the given name is not known in the dictionary.
 */
template<typename FT>
FT getValue(const DictionaryDatum &d, Name const n)
{
  // We must take a reference, so that access information can be stored in the
  // token.
  const Token& t = d->lookup(n);
  
  if (t == d->getvoid())
    throw UndefinedName(n.toString());

  return getValue<FT>(t);
}  
   

/** Define a new dictionary entry from a fundamental type.
 * @ingroup DictUtils
 * @throws TypeMismatch Fundamental type and requested SLI type are incompatible.
 */
template<typename FT, class D>
void def2(DictionaryDatum &d, Name const n, FT const &value)
{
  Token t = newToken2<FT,D>(value);
  d->insert_move(n, t);
}

/** Define a new dictionary entry from a fundamental type.
 * @ingroup DictUtils
 * @throws TypeMismatch Creating a Token from the fundamental type failed, 
 *         probably due to a missing template specialization.
 */
template<typename FT>
void def(DictionaryDatum &d, Name const n, FT const &value)
{
  Token t(value); // we hope that we have a constructor for this.
  d->insert_move(n, t);
}

/** Update a variable from a dictionary entry if it exists, skip call if it doesn't.
 * @ingroup DictUtils
 * @throws see getValue(DictionaryDatum, Name)
 */
template<typename FT, typename VT>
bool updateValue(DictionaryDatum const &d, Name const n, VT &value)
{
  // We will test for the name, and do nothing if it does not exist,
  // instead of simply trying to getValue() it and catching a possible
  // exception. The latter works, however, but non-existing names are
  // the rule with updateValue(), not the exception, hence using the
  // exception mechanism would be inappropriate. (Markus pointed this
  // out, 05.02.2001, Ruediger.)

  // We must take a reference, so that access information can be stored in the
  // token.
  const Token& t = d->lookup(n);

  if (t == d->getvoid())
    return false;
  
  value = getValue<FT>(t);
  return true;
}

/** Call a member function of an object, passing the value of an dictionary entry if it exists, 
 *  skip call if it doesn't.
 * @ingroup DictUtils
 * @throws see getValue(DictionaryDatum, Name)
 */
template<typename FT, typename VT, class C>
void updateValue2(DictionaryDatum const &d, Name const n, C &obj, void (C::* setfunc)(VT))
{
  if (d->known(n)) // Does name exist in the dictionary?
  {
    // yes, call the function for update.
    (obj.*setfunc)(getValue<FT>(d, n));
  }
}


/** Create a property of type ArrayDatum in the dictionary, if it does not already exist.
 * @ingroup DictUtils
 */
void initialize_property_array(DictionaryDatum &d, Name propname);


/** Create a property of type DoubleVectorDatum in the dictionary, if it does not already exist.
 * @ingroup DictUtils
 */
void initialize_property_doublevector(DictionaryDatum &d, Name propname);


/** Create a property of type IntVectorDatum in the dictionary, if it does not already exist.
 * @ingroup DictUtils
 */
void initialize_property_intvector(DictionaryDatum &d, Name propname);


/** Append a value to a property ArrayDatum in the dictionary.
 * This is the version for scalar values
 * @ingroup DictUtils
 */
template<typename PropT>
inline
void append_property(DictionaryDatum &d, Name propname, const PropT &prop)
{
  Token t = d->lookup(propname);
  assert (t != d->getvoid());

  ArrayDatum* arrd = dynamic_cast<ArrayDatum*>(t.datum());
  assert(arrd != 0);

  Token prop_token(prop);
  arrd->push_back_dont_clone(prop_token);
}

/** Append a value to a property DoubleVectorDatum in the dictionary.
 * This is a specialization for appending vector<double>s to vector<double>s
 * @ingroup DictUtils
 */
template<>
inline
void append_property<std::vector<double> >(DictionaryDatum &d, Name propname, const std::vector<double> &prop)
{
  Token t = d->lookup(propname);
  assert (t != d->getvoid());

  DoubleVectorDatum* arrd = dynamic_cast<DoubleVectorDatum*>(t.datum());
  assert(arrd != 0);

  (*arrd)->insert((*arrd)->end(), prop.begin(), prop.end());
}


/** Append a value to a property IntVectorDatum in the dictionary.
 * This is a specialization for appending vector<long>s to vector<long>s
 * @ingroup DictUtils
 */
template<>
inline
void append_property<std::vector<long> >(DictionaryDatum &d, Name propname, const std::vector<long> &prop)
{
  Token t = d->lookup(propname);
  assert (t != d->getvoid());

  IntVectorDatum* arrd = dynamic_cast<IntVectorDatum*>(t.datum());
  assert(arrd != 0);

  (*arrd)->insert((*arrd)->end(), prop.begin(), prop.end());
}


/** Add values of a vector<double> to a property DoubleVectorDatum in the dictionary.
 * This variant of append_property is for adding vector<double>s to vector<double>s of the
 * same size. It is required for collecting data across threads when multimeter
 * is running in accumulation mode.
 * @ingroup DictUtils
 */
inline
void accumulate_property(DictionaryDatum &d, Name propname, const std::vector<double> &prop)
{
  Token t = d->lookup(propname);
  assert (t != d->getvoid());

  DoubleVectorDatum* arrd = dynamic_cast<DoubleVectorDatum*>(t.datum());
  assert(arrd != 0);

  if ( (*arrd)->empty() ) // first data, copy
    (*arrd)->insert((*arrd)->end(), prop.begin(), prop.end());
  else
  {
    assert((*arrd)->size() == prop.size());

    // add contents of prop to **arrd elementwise
    std::transform((*arrd)->begin(), (*arrd)->end(), prop.begin(), (*arrd)->begin(), std::plus<double>());
  }
}



#endif



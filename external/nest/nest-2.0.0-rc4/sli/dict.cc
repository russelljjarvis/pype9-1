/*
 *  dict.cc
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

#include "dict.h"
#include "dictdatum.h"
#include "dictutils.h"
#include "sliexceptions.h"
#include <iomanip>
#include <algorithm>
#include <vector>
#include <cstdlib>
#include <string>

Dictionary::~Dictionary()
{
}

const Token& Dictionary::operator[](const char *n) const
{
  return operator[](Name(n));
}

Token& Dictionary::operator[](const char *n)
{
  return operator[](Name(n));
}

const Token& Dictionary::operator[](const Name &n) const
{      
  if (!known(n)) 
    throw UndefinedName(n.toString());
  return lookup(n);
}

Token& Dictionary::operator[](const Name &n)
{  
  return this->TokenMap::operator[](n);
}

void Dictionary::insert_move(const Name &n, Token &t)
{
    this->TokenMap::operator[](n).move(t);
}

void Dictionary::clear()
{
// First, clear all contained dictionaries
    for(TokenMap::iterator i = TokenMap::begin(); i != end(); ++i)
    {
      DictionaryDatum *d=dynamic_cast<DictionaryDatum*>((*i).second.datum());
      if(d !=0)
      {
	if(d->get()!= this)
	  (*d)->clear();
	d->unlock();
      }
      (*i).second.clear();
    }

    // now clear dictionary itself; HEP 2004-09-08
    TokenMap::clear();
}
    
void Dictionary::info(std::ostream &out) const
{
  out.setf(std::ios::left);
  if(size()>0)
  {
    // copy to vector and sort
    typedef std::vector<std::pair<Name, Token> >DataVec;
    DataVec data;
    std::copy(begin(), end(), std::inserter(data, data.begin()));
    std::sort(data.begin(), data.end(), DictItemLexicalOrder());

    out << "--------------------------------------------------" << std::endl;
    out << std::setw(25) <<  "Name" << std::setw(20) << "Type" <<  "Value" << std::endl;
    out << "--------------------------------------------------" << std::endl;
      
    for(DataVec::const_iterator where = data.begin() ; 
	where != data.end() ; ++ where)
    {
      out  << std::setw(25) << where->first 
	   << std::setw(20) << where->second->gettypename()
	   << where->second
	   << std::endl;
    }
    out << "--------------------------------------------------" << std::endl;
  }
  out << "Total number of entries: "<< size() << std::endl;

  out.unsetf(std::ios::left);

}

void Dictionary::add_dict(const std::string& target, 
			  SLIInterpreter& i)
{
  DictionaryDatum targetdict;

    // retrieve targetdict from interpreter
    Token d = i.baselookup(Name(target));
    targetdict = getValue<DictionaryDatum>(d);

  for ( TokenMap::const_iterator it = TokenMap::begin() ; 
	it != TokenMap::end() ; ++it )
    if ( !targetdict->known(it->first) )
      targetdict->insert(it->first, it->second);
    else
      {
	throw UndefinedName((it->first).toString());
	//      throw DictError();
      }
}

void Dictionary::remove(const Name& n)
{
  TokenMap::iterator it = find(n);
  if ( it != end() )
    erase(it);
}

void Dictionary::remove_dict(const std::string& target, 
			     SLIInterpreter& i)
{
  DictionaryDatum targetdict;

  // retrieve targetdict from interpreter
  Token d = i.baselookup(Name(target));
  targetdict = getValue<DictionaryDatum>(d);

  for ( TokenMap::const_iterator it = TokenMap::begin() ; 
	it != TokenMap::end() ; ++it )
  {
    TokenMap::iterator tgt_it = targetdict->find(it->first); 
    if ( tgt_it != targetdict->end() )
      targetdict->erase(tgt_it);
  }
}

void Dictionary::clear_access_flags()
{
  for ( TokenMap::iterator it = TokenMap::begin() ; 
	it != TokenMap::end() ; ++it )
  {
    /* 
       Clear flags in nested dictionaries recursively.
       We first test whether the token is a DictionaryDatum
       and then call getValue(). This entails two dynamic casts,
       but is likely more efficient than a try-catch construction.
    */
    if ( it->second.is_a<DictionaryDatum>() )
    {
      DictionaryDatum subdict = getValue<DictionaryDatum>(it->second);
      subdict->clear_access_flags();
    }

    // in recursion, getValue sets the access flag for it->second, so
    // we must clear it after recursion is done
    it->second.clear_access_flag();
  }
}

bool Dictionary::all_accessed_(std::string& missed, std::string prefix) const
{
  missed = "";
  
  // build list of all non-accessed Token names
  for ( TokenMap::const_iterator it = TokenMap::begin() ; 
	it != TokenMap::end() ; ++it )
  {
    if ( !it->second.accessed() )	
	missed = missed + " " + prefix + it->first.toString();
    else if ( it->second.is_a<DictionaryDatum>() )
    {
      // recursively check if nested dictionary content was accessed
      // see also comments in clear_access_flags()

      // this sets access flag on it->second, but that does not matter,
      // since it is anyways set, otherwise we would not be recursing
      DictionaryDatum subdict = getValue<DictionaryDatum>(it->second);

      subdict->all_accessed_(missed, prefix + it->first.toString() + "::"); 
    }
  }

  return missed.empty();
}


std::ostream & operator<<(std::ostream &out, const Dictionary &d)
{
    out << "<<";
        
    for(TokenMap::const_iterator where = d.begin(); where != d.end(); where ++)
    {
        out << (*where).first << ' ' 
	    << (*where).second << ',';
    }
    out << ">>";
    
    return out;
    
}

bool Dictionary::DictItemLexicalOrder::nocase_compare(char c1, char c2)
{
  return std::toupper(c1) < std::toupper(c2);
}


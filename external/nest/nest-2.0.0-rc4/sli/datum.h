/*
 *  datum.h
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

#ifndef DATUM_H
#define DATUM_H

#include "slitype.h"

class DatumConverter;


/***********************************************************/
/* Datum                                                   */
/* -----                                                   */
/*  base class for all Data Objects                        */
/***********************************************************/
class Datum
{
  
  friend class Token;
  
  bool wflag; //!< writeable-flag
  virtual Datum * clone(void) const = 0;

 protected:
  // Putting the following variables here, avoids a number of virtual
  // functions.

  const SLIType     *type;   //!< Pointer to type object.
  const SLIFunction *action; //!< Shortcut to the SLIType default action.

  Datum() : wflag(true), type(NULL), action(NULL){}

  
  Datum(const SLIType *t) : wflag(true), type(t), action(t->getaction())
    { }

  Datum(const Datum &d) : wflag(d.wflag), type(d.type), action(d.action){}
//  Datum(const Token &);

 public:

  virtual ~Datum() {};
  
  virtual void  print(std::ostream & ) const =0;
  virtual void  pprint(std::ostream &) const =0;

  virtual void  list(std::ostream &o, std::string prefix, int l) const
  {
    if(l==0)
      prefix="-->"+prefix;
    else
      prefix="   "+prefix;
    o << prefix;
    print(o);
  }

  virtual void  input_form(std::ostream &o) const
  {
    pprint(o);
  }
  
  virtual bool  equals(const Datum *) const =0;
  
  virtual void  info(std::ostream &) const;
  
  bool writeable(void) const  { return wflag;  }
  void setwriteable(bool);    

  const Name & gettypename(void) const
    {
      return type->gettypename();
    }

  bool isoftype(SLIType const &t) const
    {
      return (type==&t);  // or: *type==t, there is only one t with same contents !
    }
    
  void execute(SLIInterpreter *i)
    {
      action->execute(i);
    }

  /**
   * Accept a DatumConverter as a visitor to this datum for conversion.
   * (visitor pattern).
   */
  virtual void use_converter(DatumConverter &v);
  

};

template<SLIType * slt >
class TypedDatum: public Datum
{
// This should preferably be static!
 public:
  TypedDatum(void)
    :Datum(slt)
    { }
  
  TypedDatum(const TypedDatum<slt> &d) :Datum(d){}

  inline const TypedDatum<slt>& operator=(const TypedDatum<slt>&);

};

template<SLIType * slt >
const TypedDatum<slt>& TypedDatum<slt>::operator=(const TypedDatum<slt>&)
{
  //  assert( type == d.type );
  return *this;
}



#endif


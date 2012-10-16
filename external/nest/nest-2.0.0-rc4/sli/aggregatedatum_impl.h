
/*
 *  aggregatedatum.h
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

//
// Implementation file for aggregatedatum.
// It is needed to get rid of the dependence of datumconverter.h
// in the header file. Otherwise we cannot use a forward declaration of
// DatumConverter.
//

#ifndef AGGREGATEDATUMIMPL_H
#define AGGREGATEDATUMIMPL_H


template <class C, SLIType *slt>
void AggregateDatum<C, slt>::print(std::ostream &out) const
{
  out << *dynamic_cast<C *>(const_cast<AggregateDatum<C,slt> *>(this));    
}

template <class C, SLIType *slt>
void AggregateDatum<C, slt>::pprint(std::ostream &out) const
{
   print(out);   
}

template <class C, SLIType *slt>
void AggregateDatum<C, slt>::list(std::ostream &out, std::string prefix, int l) const
{
  if(l==0)
    prefix="-->"+prefix;
  else
    prefix="   "+prefix;

  out << prefix;
  print(out);
}

template <class C, SLIType *slt>
void AggregateDatum<C, slt>::input_form(std::ostream &out) const
{
   print(out);   
}


template <class C, SLIType *slt>
void AggregateDatum<C, slt>::info(std::ostream &out) const
{
   print(out);   
}

/**
 * Accept a DatumConverter as a visitor to this datum.
 * A visitor may be used to make a conversion to a type, which is not known to NEST.
 * (visitor pattern).
 */
template<class C, SLIType *slt>
void AggregateDatum<C, slt>::use_converter(DatumConverter &converter)
{
  converter.convert_me(*this); // call visit with our own type here
                  // this will call the approproate implementation of the derived class
}


#endif

/*
 *  namedatum.h
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

#ifndef NAMEDATUM_H
#define NAMEDATUM_H
/* 
    Defines Datum classes which are derived from Names:
    class NameDatum;
    class LiteralDatum;
    class BoolDatum;
*/

// Include all headers, needed to use token and datum objects
#include <typeinfo>

// <string> MUST be included before any STL header, since there are
// some conflicts between this and the g++-2.7.2 STL version

#include <string>
#include "name.h"
#include "aggregatedatum.h"
#include "interpret.h"
#include "config.h"

/* These are declarations to specialize the static memory pool BEFORE
   we instantiate the AggregateDatum. Note, that this is only a declaration, 
   because we do not provide an initializer (see ISO14882 Sec.  14.7.3.15.)
   The definition is given in the *.CC file with the appropriate 
   initializer.

   Note that SUN's Forte 6.2 and 7 does not handle this correctly, 
   so we have to use a compiler-switch. 11/2002 Gewaltig

   The Alpha cxx V6.3-002 says that storage class extern is not allowed here,
   so I removed it. 15.2.2002 Diesmann
*/
#ifndef HAVE_STATIC_TEMPLATE_DECLARATION_FAILS
template<>
   sli::pool AggregateDatum<Name,&SLIInterpreter::Nametype>::memory;

template<>
   sli::pool AggregateDatum<Name,&SLIInterpreter::Literaltype>::memory;
#endif



class NameDatum: public AggregateDatum<Name,&SLIInterpreter::Nametype>
{
    Datum * clone(void) const
    {
        return new NameDatum(*this);
    }
public:
    NameDatum(const Name &n):
            AggregateDatum<Name,&SLIInterpreter::Nametype>(n) {}
    NameDatum(const NameDatum &n):
            AggregateDatum<Name,&SLIInterpreter::Nametype>(n) {}
    ~NameDatum()
    {}
    
};

class LiteralDatum: public AggregateDatum<Name,&SLIInterpreter::Literaltype>
{
    Datum * clone(void) const
    {
        return new LiteralDatum(*this);
    }    
public:
    LiteralDatum(const Name &n):
            AggregateDatum<Name,&SLIInterpreter::Literaltype>(n) {}
    LiteralDatum(const LiteralDatum &n):
            AggregateDatum<Name,&SLIInterpreter::Literaltype>(n) {}
    ~LiteralDatum()
    {}
  void pprint(std::ostream &) const;

  /**
   * Accept a DatmConverter as a visitor to the datum (visitor pattern).
   * This member has to be overridden in the derived classes
   * to call visit and passing themselves as an argument.
   */
  void use_converter(DatumConverter &);

};

#endif

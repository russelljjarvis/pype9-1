/* 
 * Copyright (C) 2004-2008 The NEST Initiative.
 * This file is part of NEST.
 */

#include "gamma_randomdev.h"
#include "dictutils.h"

#include <cmath>

// by default, init as exponential density with mean 1
librandom::GammaRandomDev::GammaRandomDev(RngPtr r_source, double a_in) 
: RandomDev(r_source), a(a_in) 
{ 
  set_order(a);
}

librandom::GammaRandomDev::GammaRandomDev(double a_in) 
: RandomDev(), a(a_in) 
{ 
  set_order(a);
}

double librandom::GammaRandomDev::operator()(RngPtr r)
{
  assert(r.valid());  // make sure we have RNG
  
  // algorithm depends on order a
  if ( a == 1 )
    return -std::log(r->drandpos());
  else if ( a < 1 )
    { 
      // Johnk's rejection algorithm, see [1], p. 418
      double X;
      double Y;
      double S;
      do {
        X = std::pow(r->drand(), ju);
        Y = std::pow(r->drand(), jv);
        S = X + Y;
      } while ( S > 1 );
      if ( X > 0 )
        return -std::log(r->drandpos()) * X / S;
      else
        return 0;
    }
  else
    { 
      // Best's rejection algorithm, see [1], p. 410
      bool accept = false;
      double X = 0.0;
      do {
        const double U = r->drand();
        if ( U == 0 || U == 1 )
          continue;  // accept guaranteed false
        const double V = r->drand();
        const double W = U * (1-U); // != 0
        const double Y = std::sqrt(bc/W) * (U-0.5);
        X = bb + Y;
        if ( X > 0 )
        {
          const double Z = 64 * W * W * W * V * V;
          accept = Z <= 1 - 2*Y*Y/X;
          if ( !accept )
            accept = std::log(Z) <= 2 * (bb * std::log(X/bb) - Y);
        } 
      } while ( !accept );
      
      return X;
    }

}

void librandom::GammaRandomDev::set_status(const DictionaryDatum& d)
{
  double a_tmp;

  if ( updateValue<double>(d, "order", a_tmp) )
    set_order(a_tmp);
} 

void librandom::GammaRandomDev::get_status(DictionaryDatum &d) const 
{
  def<double>(d, "order", a);
}

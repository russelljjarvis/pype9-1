/*
 *  slibuiltins.cc
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

/* 
    Interpreter builtins
*/
#include "slibuiltins.h"
#include "interpret.h"
#include "callbackdatum.h"
#include "arraydatum.h"
#include "integerdatum.h"
#include "stringdatum.h"

void IlookupFunction::execute(SLIInterpreter *i) const
{
    i->EStack.pop(2);
}

void IsetcallbackFunction::execute(SLIInterpreter *i) const
{
       // move the hopefully present callback action
        // into the interpreters callback token.
    i->EStack.pop();
    assert(dynamic_cast<CallbackDatum *>(i->EStack.top().datum()) !=NULL);
    i->EStack.pop_move(i->ct);
}

void IiterateFunction::backtrace(SLIInterpreter *i, int p) const
{
  ProcedureDatum const *pd= static_cast<ProcedureDatum *>(i->EStack.pick(p+2).datum());   
  assert(pd != NULL);

  IntegerDatum   *id= static_cast<IntegerDatum *>(i->EStack.pick(p+1).datum());
  assert(id !=NULL);

  std::cerr << "In procedure:" << std::endl;
  
  pd->list(std::cerr,"   ",id->get()-1);
  std::cerr << std::endl;
}

void IiterateFunction::execute(SLIInterpreter *i) const
{
/* 
   This function is responsible for executing a procedure
   object. Iiterate expects the procedure to execute as first
   and the iteration counter as second argument.

   Like in all internal function, no error checking is done.
   
   */

/* Stack Layout:
      3       2       1
   <proc>  <pos>   %iterate
*/

    ProcedureDatum const *pd= static_cast<ProcedureDatum *>(i->EStack.pick(2).datum());   

    IntegerDatum   *id= static_cast<IntegerDatum *>(i->EStack.pick(1).datum());

    size_t pos = static_cast<size_t>(id->get());
    
    if( pos < pd->size())
    {
      Token t(pd->get(pos));
      if(i->step_mode())
      {
	//std::cerr << std::endl;
	do{
	  char cmd=i->debug_commandline(t);
	  if(cmd=='l') // List the procedure
	  {
	    if(pd !=NULL)
	    {
	      pd->list(std::cerr,"   ",pos);
	      std::cerr <<std::endl;
	    }
	  }
	  else 
	    break;
	} while (true);
      }

      if((pos+1 == pd->size()) 
	 && i->optimize_tailrecursion() ) // This handles tailing recursion
      {
	i->EStack.pop(3);
	i->dec_call_depth();
      }
      else
        id->incr();

      i->EStack.push_move(t);

    }
    else
    {
      i->EStack.pop(3);
      i->dec_call_depth();
    }
    
}

void IloopFunction::execute(SLIInterpreter *i) const
{
        // stack: mark procedure n   %loop
        // level:  4      3      2     1
    
    IntegerDatum
        *proccount= static_cast<IntegerDatum *>(i->EStack.pick(1).datum());
    
    ProcedureDatum
        const *proc= static_cast<ProcedureDatum *>(i->EStack.pick(2).datum());
    
    size_t pos = static_cast<size_t>(proccount->get());
    
    if(  pos < proc->size())
    {
        proccount->incr();
        i->EStack.push(proc->get(pos));
	if(i->step_mode())
	{
	  do{
	    char cmd=i->debug_commandline(i->EStack.top());
	    if(cmd=='l') // List the procedure
	    {
	      proc->list(std::cerr,"   ",pos);
	      std::cerr <<std::endl;
	    }
	    else 
	      break;
	  } while (true);
	}
    }
    else
    {
      (*proccount)=0;
      if(i->step_mode())
      {
	std::cerr << "Loop: " << std::endl
		  << " starting new iteration." << std::endl;
      }	  
    }
      
}

void IloopFunction::backtrace(SLIInterpreter *i, int p) const
{
  ProcedureDatum const *pd= static_cast<ProcedureDatum *>(i->EStack.pick(p+2).datum());   
  assert(pd !=NULL);

  IntegerDatum   *id= static_cast<IntegerDatum *>(i->EStack.pick(p+1).datum());
  assert(id != NULL);

  std::cerr << "During loop:" << std::endl;
  
  pd->list(std::cerr,"   ",id->get()-1);
  std::cerr <<std::endl;

}


/**********************************************/
/* %repeat                                    */
/*  call: mark  count proc  n %repeat         */
/*  pick   5      4     3   2    1            */        
/**********************************************/
void IrepeatFunction::execute(SLIInterpreter *i) const
{

    IntegerDatum
        *proccount= static_cast<IntegerDatum *>(i->EStack.pick(1).datum());
    
    ProcedureDatum
        const *proc= static_cast<ProcedureDatum *>(i->EStack.pick(2).datum());
    
    size_t pos = static_cast<size_t>(proccount->get());
    
    if(  pos < proc->size())
    {
        proccount->incr();
        i->EStack.push(proc->get(pos));
	if(i->step_mode())
	{
	  do{
	    char cmd=i->debug_commandline(i->EStack.top());
	    if(cmd=='l') // List the procedure
	    {
	      proc->list(std::cerr,"   ",pos);
	      std::cerr <<std::endl;
	    }
	    else 
	      break;
	  } while (true);
	}
    }
    else
    {
        IntegerDatum
            *loopcount= static_cast<IntegerDatum *>(i->EStack.pick(3).datum());
        
        if( loopcount->get() > 0 )
        {
            (*proccount)=0;     // reset procedure iterator
            loopcount->decr();
	    if(i->step_mode())
	    {
	      std::cerr << "repeat: " << loopcount->get()
			<< " iterations left." << std::endl;
	    }
        }
        else
	{
            i->EStack.pop(5);
	    i->dec_call_depth();
	}
    }
    
}

void IrepeatFunction::backtrace(SLIInterpreter *i, int p) const
{
  IntegerDatum   *count= static_cast<IntegerDatum *>(i->EStack.pick(p+3).datum());
  assert(count != NULL);

  ProcedureDatum const *pd= static_cast<ProcedureDatum *>(i->EStack.pick(p+2).datum());   
  assert(pd!= NULL);

  IntegerDatum   *id= static_cast<IntegerDatum *>(i->EStack.pick(p+1).datum());
  assert(id != NULL);

  std::cerr << "During repeat with " << count->get() << " iterations remaining." << std::endl;
  
  pd->list(std::cerr,"   ",id->get()-1);
  std::cerr <<std::endl;

}

/*****************************************************/
/* %for                                              */
/*  call: mark incr limit count proc  n  %for        */
/*  pick   6     5    4     3    2    1    0         */        
/*****************************************************/
void IforFunction::execute(SLIInterpreter *i) const
{
    
    IntegerDatum *proccount=
        static_cast<IntegerDatum *>(i->EStack.pick(1).datum());
    
    ProcedureDatum
        const *proc= static_cast<ProcedureDatum *>(i->EStack.pick(2).datum());
    
    size_t pos = static_cast<size_t>(proccount->get());
    
    if(  pos < proc->size())
    {
        i->EStack.push(proc->get(pos));
        proccount->incr();
	if(i->step_mode())
	{
	  do{
	    char cmd=i->debug_commandline(i->EStack.top());
	    if(cmd=='l') // List the procedure
	    {
	      proc->list(std::cerr,"   ",pos);
	      std::cerr <<std::endl;
	    }
	    else   
	      break;
	  } while (true);
	}
    }
    else
    {
        
        IntegerDatum *count=
            static_cast<IntegerDatum *>(i->EStack.pick(3).datum());
    
        IntegerDatum *lim  =
            static_cast<IntegerDatum *>(i->EStack.pick(4).datum());

        IntegerDatum *inc  =
            static_cast<IntegerDatum *>(i->EStack.pick(5).datum());

        if(( (inc->get()> 0) && (count->get() <= lim->get())) ||
           ( (inc->get()< 0) && (count->get() >= lim->get())))
        {
            (*proccount)=0;
        
            i->OStack.push(i->EStack.pick(3)); // push counter to user
            count->add(inc->get());            // increment count
	    if(i->step_mode())
	    {
	      std::cerr << "for:" 
			<< " Limit : " << lim->get()
			<< " Step : " << inc->get()
			<< " Iterator: " << count->get() 
			<< std::endl;
	    }
        }
        else
	{
            i->EStack.pop(7);
	    i->dec_call_depth();
	}
    }
    
}

void IforFunction::backtrace(SLIInterpreter *i, int p) const
{
  IntegerDatum   *count= static_cast<IntegerDatum *>(i->EStack.pick(p+3).datum());
  assert(count!=NULL);
  ProcedureDatum const *pd= static_cast<ProcedureDatum *>(i->EStack.pick(p+2).datum());   
  assert(pd != NULL);
  IntegerDatum   *id= static_cast<IntegerDatum *>(i->EStack.pick(p+1).datum());
  assert(id != NULL);

  std::cerr << "During for at iterator value " << count->get() << "." << std::endl;
  
  pd->list(std::cerr,"   ",id->get()-1);
  std::cerr <<std::endl;

}

/*********************************************************/
/* %forallarray                                          */
/*  call: mark object limit count proc %forallarray  */
/*  pick   5      4    3     2    1      0         */        
/*********************************************************/
void IforallarrayFunction::execute(SLIInterpreter *i) const
{
    IntegerDatum *count=
        static_cast<IntegerDatum *>(i->EStack.pick(2).datum());

    IntegerDatum *limit=
        static_cast<IntegerDatum *>(i->EStack.pick(3).datum());
    
    if(count->get() < limit->get())
    {
      ArrayDatum *obj=
	static_cast<ArrayDatum *>(i->EStack.pick(4).datum());

      i->OStack.push(obj->get(count->get()));  // push element to user
      count->incr();
      i->EStack.push(i->EStack.pick(1));
      if(i->step_mode())
      {
	std::cerr << "forall:"
		  << " Limit: " << limit->get()
		  << " Pos: " << count->get()
		  << " Iterator: ";
	i->OStack.pick(0).pprint(std::cerr);
	std::cerr << std::endl;
      }
    }
    else
    {
      i->EStack.pop(6);
      i->dec_call_depth();
    }
}

void IforallarrayFunction::backtrace(SLIInterpreter *i, int p) const
{
  IntegerDatum   *count= static_cast<IntegerDatum *>(i->EStack.pick(p+2).datum());
  assert(count!=NULL);

  std::cerr << "During forall (array) at iteration " << count->get() << "." << std::endl;

}

/*********************************************************/
/* %forallindexedarray                                   */
/*  call: mark object limit count proc forallindexedarray  */
/*  pick   5      4    3     2    1      0         */        
/*********************************************************/
void IforallindexedarrayFunction::execute(SLIInterpreter *i) const
{
    IntegerDatum *count=
        static_cast<IntegerDatum *>(i->EStack.pick(2).datum());
    IntegerDatum *limit=
        static_cast<IntegerDatum *>(i->EStack.pick(3).datum());
     
    
    if(count->get() < limit->get())
    {
      ArrayDatum *obj=
	static_cast<ArrayDatum *>(i->EStack.pick(4).datum());

      i->OStack.push(obj->get(count->get()));  // push element to user
      i->OStack.push(count->get());            // push index to user
      count->incr();
      i->EStack.push(i->EStack.pick(1));
      if(i->step_mode())
      {
	std::cerr << "forallindexed:"
		  << " Limit: " << limit->get()
		  << " Pos: " << count->get() -1
		  << " Iterator: "; 
	i->OStack.pick(1).pprint(std::cerr);
	std::cerr << std::endl;
      }
    }
    else
    {
      i->EStack.pop(6);
      i->dec_call_depth();
    }

}

void IforallindexedarrayFunction::backtrace(SLIInterpreter *i, int p) const
{
  IntegerDatum   *count= static_cast<IntegerDatum *>(i->EStack.pick(p+2).datum());
  assert(count!=NULL);

  std::cerr << "During forallindexed (array) at iteration " << count->get()-1 << "." << std::endl;

}

void IforallindexedstringFunction::backtrace(SLIInterpreter *i, int p) const
{
  IntegerDatum   *count= static_cast<IntegerDatum *>(i->EStack.pick(p+2).datum());
  assert(count!=NULL);

  std::cerr << "During forallindexed (string) at iteration " << count->get()-1 << "." << std::endl;

}

/*********************************************************/
/* %forallindexedarray                                   */
/*  call: mark object limit count proc forallindexedarray  */
/*  pick   5      4    3     2    1      0         */        
/*********************************************************/
void IforallindexedstringFunction::execute(SLIInterpreter *i) const
{
    IntegerDatum *count=
        static_cast<IntegerDatum *>(i->EStack.pick(2).datum());
    IntegerDatum *limit=
        static_cast<IntegerDatum *>(i->EStack.pick(3).datum());
    
    if(count->get() < limit->get())
    {
      StringDatum const *obj=
	static_cast<StringDatum *>(i->EStack.pick(4).datum());

      i->OStack.push((*obj)[count->get()]);  // push element to user
      i->OStack.push(count->get());          // push index to user
      count->incr();
      i->EStack.push(i->EStack.pick(1));
      if(i->step_mode())
      {
	std::cerr << "forallindexed:"
		  << " Limit: " << limit->get()
		  << " Pos: " << count->get()
		  << " Iterator: "; 
	i->OStack.pick(1).pprint(std::cerr);
	std::cerr << std::endl;
      }
    }
    else
    {
      i->EStack.pop(6);	    
      i->dec_call_depth();
    }

}

void IforallstringFunction::backtrace(SLIInterpreter *i, int p) const
{
  IntegerDatum   *count= static_cast<IntegerDatum *>(i->EStack.pick(p+2).datum());
  assert(count!=NULL);

  std::cerr << "During forall (string) at iteration " << count->get()-1 << "." << std::endl;

}
/*********************************************************/
/* %forallstring                                         */
/*  call: mark object limit count proc %forallarray  */
/*  pick   5      4    3     2    1      0         */        
/*********************************************************/
void IforallstringFunction::execute(SLIInterpreter *i) const
{
    IntegerDatum *count=
        static_cast<IntegerDatum *>(i->EStack.pick(2).datum());
    IntegerDatum *limit=
        static_cast<IntegerDatum *>(i->EStack.pick(3).datum());
    
    if(count->get() < limit->get())
    {
      StringDatum const *obj=
	static_cast<StringDatum *>(i->EStack.pick(4).datum());
      i->OStack.push(new IntegerDatum((*obj)[count->get()]));  // push element to user
      count->incr();
      i->EStack.push(i->EStack.pick(1));
      if(i->step_mode())
      {
	std::cerr << "forall:"
		  << " Limit: " << limit->get()
		  << " Pos: " << count->get()
		  << " Iterator: "; 
	i->OStack.top().pprint(std::cerr);
	std::cerr << std::endl;
      }
    }
    else
    {
      i->EStack.pop(6);
      i->dec_call_depth();
    }

}


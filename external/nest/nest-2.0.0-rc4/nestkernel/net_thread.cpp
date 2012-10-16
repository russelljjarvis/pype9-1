/*
 *  net_thread.cpp
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

#include <iostream>
#include <cstring>
#include "net_thread.h"
#include "scheduler.h"

nest::Thread::Thread()
  :id_(-1),
   scheduler_(NULL)
{}

nest::Thread::Thread(const Thread&)
  :id_(-1),
   scheduler_(NULL)
{}


nest::Thread nest::Thread::operator=(const Thread &t)
{  
  assert(t.id_==-1);
  id_=-1;
  scheduler_=NULL;
  return *this;
}

void nest::Thread::init(int i, Scheduler* s)
{

  assert(s != NULL);
  assert(i>=0);
  assert(id_==-1);

  scheduler_=s;
  id_=i;

#ifdef HAVE_PTHREADS
  // We have only a small number of threads, so it is better
  // to assign them individually to LWPs and have them scheduled
  // by the OS kernel
  pthread_attr_t thread_attribute;
  pthread_attr_init(&thread_attribute);

  int status=pthread_attr_setscope(&thread_attribute, PTHREAD_SCOPE_SYSTEM);
  if(status != 0)
  {
    std::cerr << "Error while setting the scheduling scope." << std::endl;
    throw PthreadException(status);
  }

  //std::cerr << "Starting Thread no. " << i << std::endl;
  
  status= pthread_create(&p_, &thread_attribute,
			 nest_thread_handler,
			 static_cast<void *>(this)
			 );
  if(status!=0)
  {
    std::cerr << "Error creating thread. Error code " 
	      << status << std::endl
	      << "which is: " << std::strerror(status) << std::endl;
    throw PthreadException(status);
  }
#else
  if (i > 0)
    {
      std::cerr << "Multithreading not available" << std::endl;
      throw KernelException();
    }
#endif
}

#ifdef HAVE_PTHREADS
void nest::Thread::run(void)
{
  assert(id_ >= 0);
  scheduler_->threaded_update(id_);
}

int nest::Thread::join()
{
  return pthread_join(p_,(void **)NULL);
}

// global thread handler function.
extern "C"
void* nest_thread_handler(void *t)
{
  nest::Thread *my_thread=static_cast<nest::Thread *>(t);
  assert(my_thread != NULL);
  
  my_thread->run();
  pthread_exit(0);
  return NULL;
}

#else
 void nest::Thread::run(void)
{
}

int nest::Thread::join()
{
  return 0;
}

// global thread handler function.
extern "C"
void* nest_thread_handler(void *t)
{
  return NULL;
}

#endif //HAVE_PTHREADS

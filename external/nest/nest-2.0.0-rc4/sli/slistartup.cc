/*
 *  slistartup.cc
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

#include <cstdlib>
#include <cstring>
#include <fstream>
#include "slistartup.h"
#include "interpret.h"
#include "namedatum.h"
#include "iostreamdatum.h"
#include "arraydatum.h"
#include "stringdatum.h"
#include "integerdatum.h"
#include "booldatum.h"
#include "dictdatum.h"
#include "config.h"

extern int SLIsignalflag;
/*
1.  Propagate commandline to the sli level.
    Commandline options will be handled by the startup file.

2.  Locate startup file and prepare the start symbol
    to run the startup script.
3.  If startup-script cannot be located, issue meaningful diagnostic
    messages and exit gracefuly.
*/

/**
 * Returns true, if the interpreter startup file sli-init.sli 
 * is in the supplied path and false otherwise.
 */
bool SLIStartup::checkpath(std::string const &path, std::string &result) const
{
  const std::string fullpath=path+slilibpath;
  const std::string fullname=fullpath+"/"+startupfilename;
  
  std::ifstream in(fullname.c_str());
  if(in)
  {
    result=fullname;
  }
  else
    result.erase();

  return(in);
}

  
/*BeginDocumentation
Name: getenv - evaluates if a string is an evironment variable

Synopsis: string getenv -> path true
string getenv -> false

Description: getenv checks if the string is an environment variable. If
this is the case the path of the variable and true is pushed on the stack,
otherwise a false is pushed on the stack and the string is lost.

Examples:

SLI ] (HOME) getenv
SLI [2] pstack
true
(/home/gewaltig)

SLI ] (NONEXISTING) getenv =
false

SLI ] (SLIDATADIR) getenv
SLI [2] { (Using root path: )  =only = }
SLI [3] { (Warning: $SLIDATADIR undefined) =}
SLI [4] ifelse
Using root path: /home/gewaltig/nest/release/release

SLI ] (/home) getenv
false

Remarks: if getenv is used with the wrong argument (e.g. integer), 
the SLI Session is terminated

Author: docu by Marc Oliver Gewaltig and Sirko Straube

SeeAlso: environment, setenvironment
 */

std::string SLIStartup::getenv(const std::string &v) const
{
  char *s = ::getenv(v.c_str());
  if(!s)
    return std::string();
  else
    return std::string(s);
}

void SLIStartup::GetenvFunction::execute(SLIInterpreter *i) const
{
  // string getenv -> string true
  //               -> false

  i->assert_stack_load(1);

  StringDatum  *sd= dynamic_cast<StringDatum *>(i->OStack.top().datum());
  assert(sd !=NULL);
  const char *s = ::getenv(sd->c_str());
  i->OStack.pop();
  if(s != NULL)
  {
    Token t(new StringDatum(s));
    i->OStack.push_move(t);
    i->OStack.push(i->baselookup(i->true_name));
  }
  else
    i->OStack.push(i->baselookup(i->false_name));

  i->EStack.pop();
}

/**
 * Checks if the environment variable envvar contains a directory. If yes, the
 * path is returned, else an empty string is returned.
 */
std::string SLIStartup::checkenvpath(std::string const &envvar, SLIInterpreter *i, std::string defaultval = "") const
{
  const std::string envpath = getenv(envvar);

  if (envpath != "")
  {
    if ( opendir(envpath.c_str()) != NULL )
      return envpath;
    else
    {
      std::string msg;
      switch(errno)
      {
        case ENOTDIR:
  	  msg = String::compose("'%1' is not a directory.", envpath);
  	  break;
  	case ENOENT:
  	  msg = String::compose("Directory '%1' does not exist.", envpath);
  	  break;
  	default:
  	  msg = String::compose("Errno %1 received when trying to open '%2'", errno, envpath);
  	  break;
      }

      i->message(SLIInterpreter::M_ERROR, "SLIStartup", String::compose("%1 is not usable:", envvar).c_str());
      i->message(SLIInterpreter::M_ERROR, "SLIStartup", msg.c_str());
      if (defaultval != "")
        i->message(SLIInterpreter::M_ERROR, "SLIStartup", String::compose("I'm using the default: %1", defaultval).c_str());
    }
  }
  return std::string();
}
  

SLIStartup::SLIStartup(int argc, char** argv)
: startupfilename("sli-init.sli"),
  slilibpath("/sli"),
  slihomepath(PKGDATADIR),
  slidocdir(PKGDOCDIR),
  verbosity_(SLIInterpreter::M_INFO), // default verbosity level
  debug_(false),
  argv_name("argv"),
  prgname_name("prgname"),
  exitcode_name("exitcode"),
  prgmajor_name("prgmajor"),
  prgminor_name("prgminor"),
  prgpatch_name("prgpatch"),
  prgbuilt_name("built"),
  prefix_name("prefix"),
  prgsourcedir_name("prgsourcedir"),
  prgbuilddir_name("prgbuilddir"),
  prgdatadir_name("prgdatadir"),
  prgdocdir_name("prgdocdir"),
  host_name("host"),
  hostos_name("hostos"),
  hostvendor_name("hostvendor"),
  hostcpu_name("hostcpu"),
  getenv_name("getenv"),
  statusdict_name("statusdict"),
  start_name("start"),
  intsize_name("int"),
  longsize_name("long"),
  havelonglong_name("have long long"),
  longlongsize_name("long long"),
  doublesize_name("double"),
  pointersize_name("void *"),
  architecturedict_name("architecture"),
  have_mpi_name("have_mpi"),
  ismpi_name("is_mpi"),
  have_gsl_name("have_gsl"),
  have_pthreads_name("have_pthreads"),
  havemusic_name("have_music"),
  ndebug_name("ndebug"),
  exitcodes_name("exitcodes"),
  exitcode_success_name("success"),
  exitcode_scripterror_name("scripterror"),
  exitcode_abort_name("abort"),
  exitcode_segfault_name("segfault"),
  exitcode_exception_name("exception"),
  exitcode_fatal_name("fatal"),
  exitcode_unknownerror_name("unknownerror")

{
  ArrayDatum ad;

  // argv[0] is the name of the program that was given to the shell.
  // This name must be given to SLI, otherwise initialization fails.
  // To catch accidental removal, e.g. in pyNEST, we explicitly assert
  // that argv[0] is a non-empty string.
  assert(std::strlen(argv[0]) >0);
  for(int i=0; i<argc; ++i)
  {
    StringDatum *sd= new StringDatum(argv[i]);
    if(*sd == "-d" || *sd =="--debug")
    {
      debug_=true;
      verbosity_=SLIInterpreter::M_ALL; // make the interpreter verbose.
      continue;
    }

    if(*sd =="--verbosity=ALL")
    {
      verbosity_=SLIInterpreter::M_ALL;
      continue;
    }
    if(*sd =="--verbosity=DEBUG")
    {
      verbosity_=SLIInterpreter::M_DEBUG;
      continue;
    }
    if(*sd =="--verbosity=STATUS")
    {
      verbosity_=SLIInterpreter::M_STATUS;
      continue;
    }
    if(*sd =="--verbosity=INFO")
    {
      verbosity_=SLIInterpreter::M_INFO;
      continue;
    }
    if(*sd =="--verbosity=WARNING")
    {
      verbosity_=SLIInterpreter::M_WARNING;
      continue;
    }
    if(*sd =="--verbosity=ERROR")
    {
      verbosity_=SLIInterpreter::M_ERROR;
      continue;
    }
    if(*sd =="--verbosity=FATAL")
    {
      verbosity_=SLIInterpreter::M_FATAL;
      continue;
    }
    if(*sd =="--verbosity=QUIET")
    {
      verbosity_=SLIInterpreter::M_QUIET;
      continue;
    }

    ad.push_back(Token(sd));
  }
  targs=ad;
}

void SLIStartup::init(SLIInterpreter *i)
{
  i->verbosity(verbosity_);

  i->createcommand(getenv_name,&getenvfunction);
  std::string fname;

  // Check for supplied SLIDATADIR
  std::string slihomepath_env = checkenvpath("SLIDATADIR", i, slihomepath);
  if (slihomepath_env != "")
  {
    slihomepath = slihomepath_env;  // absolute path & directory exists
    i->message(SLIInterpreter::M_INFO, "SLIStartup", String::compose("Using SLIDATADIR=%1", slihomepath).c_str());
  }

  // check for supplied SLIDOCDIR
  std::string slidocdir_env = checkenvpath("SLIDOCDIR", i, slidocdir);
  if (slidocdir_env != "")
  {
    slidocdir = slidocdir_env;  // absolute path & directory exists
    i->message(SLIInterpreter::M_INFO, "SLIStartup", String::compose("Using SLIDOCDIR=%1", slidocdir).c_str());
  }

  if(!checkpath(slihomepath, fname))
    {
      i->message(SLIInterpreter::M_FATAL, "SLIStartup","Your NEST installation seems broken. \n"); 
      i->message(SLIInterpreter::M_FATAL, "SLIStartup","I could not find the startup file that"); 
      i->message(SLIInterpreter::M_FATAL, "SLIStartup", std::string(std::string("should have been in ") + 
					slihomepath).c_str());
      i->message(SLIInterpreter::M_FATAL, "SLIStartup","Please re-build NEST and try again.");
      i->message(SLIInterpreter::M_FATAL, "SLIStartup","The file install.html in NEST's doc directory tells you how.");

      i->message(SLIInterpreter::M_FATAL, "SLIStartup","Bye.");

      SLIsignalflag=255; // this exits the interpreter.
      debug_=false;    // switches off the -d/--debug switch!
      i->verbosity(SLIInterpreter::M_QUIET);// suppress all further output.
    }
  else
    {
      std::string fname_msg=std::string("Initialising from file: ")+fname;
      i->message(SLIInterpreter::M_DEBUG, "SLIStartup",fname_msg.c_str());
    }

  Token cin_token(new XIstreamDatum(std::cin));
  Token start_token(new NameDatum(start_name));

  DictionaryDatum statusdict(new Dictionary());
  i->statusdict=&(*statusdict);
  assert(statusdict.valid());

  statusdict->insert_move(argv_name,targs);
  statusdict->insert(prgname_name,Token(new StringDatum(SLI_PRGNAME)));
  statusdict->insert(exitcode_name,Token(new IntegerDatum(EXIT_SUCCESS)));
  statusdict->insert(prgmajor_name,Token(new IntegerDatum(SLI_MAJOR_REVISION)));
  statusdict->insert(prgminor_name,Token(new IntegerDatum(SLI_MINOR_REVISION)));
  statusdict->insert(prgpatch_name,Token(new StringDatum(SLI_PATCHLEVEL)));
  statusdict->insert(prgbuilt_name,Token(new StringDatum(String::compose("%1 %2", __DATE__, __TIME__))));
  statusdict->insert(prefix_name,Token(new StringDatum(SLI_PREFIX)));
  statusdict->insert(prgsourcedir_name,Token(new StringDatum(PKGSOURCEDIR)));
  statusdict->insert(prgbuilddir_name, Token(new StringDatum(SLI_BUILDDIR)));
  statusdict->insert(prgdatadir_name,Token(new StringDatum(slihomepath)));
  statusdict->insert(prgdocdir_name,Token(new StringDatum(slidocdir)));
  statusdict->insert(host_name,Token(new StringDatum(SLI_HOST)));
  statusdict->insert(hostos_name,Token(new StringDatum(SLI_HOSTOS)));
  statusdict->insert(hostvendor_name,Token(new StringDatum(SLI_HOSTVENDOR)));
  statusdict->insert(hostcpu_name,Token(new StringDatum(SLI_HOSTCPU)));

#ifdef HAVE_MPI
  statusdict->insert(have_mpi_name, Token(new BoolDatum(true)));
#else
  statusdict->insert(have_mpi_name, Token(new BoolDatum(false)));
#endif

#ifdef HAVE_GSL
  statusdict->insert(have_gsl_name, Token(new BoolDatum(true)));
#else
  statusdict->insert(have_gsl_name, Token(new BoolDatum(false)));
#endif

#ifdef HAVE_PTHREADS
  statusdict->insert(have_pthreads_name, Token(new BoolDatum(true)));
#else
  statusdict->insert(have_pthreads_name, Token(new BoolDatum(false)));
#endif

#ifdef HAVE_MUSIC
  statusdict->insert(havemusic_name, Token(new BoolDatum(true)));
#else
  statusdict->insert(havemusic_name, Token(new BoolDatum(false)));
#endif

#ifdef NDEBUG
  statusdict->insert(ndebug_name, Token(new BoolDatum(true)));
#else
  statusdict->insert(ndebug_name, Token(new BoolDatum(false)));
#endif

  DictionaryDatum architecturedict(new Dictionary());
  assert(architecturedict.valid());

  architecturedict->insert(doublesize_name, Token(new IntegerDatum( sizeof(double) )));
  architecturedict->insert(pointersize_name,Token(new IntegerDatum( sizeof(void *) )));
  architecturedict->insert(intsize_name,    Token(new IntegerDatum( sizeof(int)    )));
  architecturedict->insert(longsize_name,   Token(new IntegerDatum( sizeof(long)   )));

  statusdict->insert(architecturedict_name, architecturedict); 

  DictionaryDatum exitcodes(new Dictionary());
  assert(exitcodes.valid());

  exitcodes->insert(exitcode_success_name,Token(new IntegerDatum(EXIT_SUCCESS)));
  exitcodes->insert(exitcode_scripterror_name,Token(new IntegerDatum(126)));
  exitcodes->insert(exitcode_abort_name,Token(new IntegerDatum(SLI_EXITCODE_ABORT)));
  exitcodes->insert(exitcode_segfault_name,Token(new IntegerDatum(SLI_EXITCODE_SEGFAULT)));
  exitcodes->insert(exitcode_exception_name,Token(new IntegerDatum(125)));
  exitcodes->insert(exitcode_fatal_name,Token(new IntegerDatum(127)));
  exitcodes->insert(exitcode_unknownerror_name,Token(new IntegerDatum(10)));

  statusdict->insert(exitcodes_name, exitcodes); 

#ifdef HAVE_LONG_LONG
  typedef long long longlong_t;
  architecturedict->insert(havelonglong_name,   Token(new BoolDatum(true)));
#else
  typedef long longlong_t;
  architecturedict->insert(havelonglong_name,   Token(new BoolDatum(false)));
#endif

  architecturedict->insert(longlongsize_name,   Token(new IntegerDatum( sizeof(longlong_t) )));
  
  i->def(statusdict_name, statusdict);

  if(!fname.empty())
  {
    std::ifstream *input = new std::ifstream(fname.c_str());
    Token input_token(new XIstreamDatum(input));
            
    i->EStack.push_move(input_token);
    i->EStack.push(i->baselookup(i->iparse_name));
  }

  // If we start with debug option, we set the debugging mode, but disable stepmode.
  // This way, the debugger is entered only on error.
  if(debug_)
  {
    i->debug_mode_on();
    i->backtrace_on();
  }
}

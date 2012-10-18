/*
 *  RecordingDevice.cpp
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

#include "recording_device.h"
#include "network.h"
#include "dictutils.h"
#include "iostreamdatum.h"
#include "arraydatum.h"
#include "config.h"
#include "exceptions.h"
#include "sliexceptions.h"
#include <iostream> // using cerr for error message.
#include <iomanip>
#include "fdstream.h"

// nestmodule provides global access to the network, so we can
// issue warning messages. This is messy and needs cleaning up.
#include "nestmodule.h"


/* ---------------------------------------------------------------- 
 * Default constructors defining default parameters and state
 * ---------------------------------------------------------------- */

nest::RecordingDevice::Parameters_::Parameters_(const std::string& file_ext,
						bool withtime, bool withgid)
  : to_file_(false),
    to_screen_(false),
    to_memory_(true),
    to_accumulator_(false),
    time_in_steps_(false),
    precise_times_(false),
    withgid_(withgid),
    withpath_(false),
    withtime_(withtime),
    withweight_(false),
    precision_(3),
    scientific_(false),
    label_(),
    file_ext_(file_ext),
    filename_(),
    close_after_simulate_(false),
    flush_after_simulate_(true),
    flush_records_(false),
    close_on_reset_(true)
{}

nest::RecordingDevice::State_::State_()
  : events_(0),
    event_senders_(),
    event_times_ms_(),
    event_times_steps_(),
    event_times_offsets_()
{}  

/* ---------------------------------------------------------------- 
 * Parameter extraction and manipulation functions
 * ---------------------------------------------------------------- */

void nest::RecordingDevice::Parameters_::get(const RecordingDevice& rd,
                                             DictionaryDatum &d) const
{
  (*d)[names::label] = label_;

  (*d)[names::withtime] = withtime_;
  (*d)[names::withgid]  = withgid_;
  (*d)[names::withpath] = withpath_;
  (*d)[names::withweight] = withweight_;

  (*d)[names::time_in_steps]  = time_in_steps_;
  if ( rd.mode_ == RecordingDevice::SPIKE_DETECTOR )
    (*d)[names::precise_times]  = precise_times_;

  // We must maintain /to_file, /to_screen, and /to_memory, because
  // the new /record_to feature is not working in Pynest.
  (*d)[names::to_screen]= to_screen_;
  (*d)[names::to_memory]= to_memory_;
  (*d)[names::to_file]= to_file_;
  if ( rd.mode_ == RecordingDevice::MULTIMETER )
    (*d)[names::to_accumulator]= to_accumulator_;

  ArrayDatum ad;
  if ( to_file_   ) ad.push_back(LiteralDatum(names::file));
  if ( to_memory_ ) ad.push_back(LiteralDatum(names::memory));
  if ( to_screen_ ) ad.push_back(LiteralDatum(names::screen));
  if ( rd.mode_ == RecordingDevice::MULTIMETER )
    if ( to_accumulator_ ) ad.push_back(LiteralDatum(names::accumulator));
  (*d)[names::record_to] = ad;
    
  (*d)[names::file_extension] = file_ext_;
  (*d)[names::precision] = precision_;
  (*d)[names::scientific] = scientific_;
  
  (*d)[names::close_after_simulate] = close_after_simulate_;
  (*d)[names::flush_after_simulate] = flush_after_simulate_;
  (*d)[names::flush_records]  = flush_records_;
  (*d)[names::close_on_reset] = close_on_reset_;
   
  if ( to_file_ && !filename_.empty() )
  {
    initialize_property_array(d, names::filenames);
    append_property(d, names::filenames, filename_);
  }
}  

void nest::RecordingDevice::Parameters_::set(const RecordingDevice& rd,
                                             const DictionaryDatum& d)
{
  updateValue<std::string>(d, names::label, label_);
  updateValue<bool>(d, names::withgid, withgid_);
  updateValue<bool>(d, names::withtime, withtime_);
  updateValue<bool>(d, names::withpath, withpath_);
  updateValue<bool>(d, names::withweight, withweight_);
  updateValue<bool>(d, names::time_in_steps, time_in_steps_);
  if ( rd.mode_ == RecordingDevice::SPIKE_DETECTOR )
    updateValue<bool>(d, names::precise_times, precise_times_);
  updateValue<std::string>(d, names::file_extension, file_ext_);
  updateValue<long>(d, names::precision, precision_);
  updateValue<bool>(d, names::scientific, scientific_);
  updateValue<bool>(d, names::close_after_simulate, close_after_simulate_);
  updateValue<bool>(d, names::flush_after_simulate, flush_after_simulate_);
  updateValue<bool>(d, names::flush_records, flush_records_);
  updateValue<bool>(d, names::close_on_reset, close_on_reset_);

  // In Pynest we cannot use /record_to, because we have no way to pass 
  // values as LiteralDatum. Thus, we must keep the boolean flags.
  // We must have || rec_change at the end, otherwise short-circuiting may
  // mean that some flags are not read.
  bool rec_change = false;
  rec_change = updateValue<bool>(d, names::to_screen, to_screen_) || rec_change;
  rec_change = updateValue<bool>(d, names::to_memory, to_memory_) || rec_change;
  rec_change = updateValue<bool>(d, names::to_file, to_file_)     || rec_change;
  if ( rd.mode_ == RecordingDevice::MULTIMETER )
    rec_change = updateValue<bool>(d, names::to_accumulator, to_accumulator_) || rec_change;

  const bool have_record_to = d->known(names::record_to);
  if ( have_record_to ) 
  {
    // clear all flags
    to_file_ = to_screen_ = to_memory_ = to_accumulator_ = false;

    // check for flags present in array, could be far more elegant ...
    ArrayDatum ad = getValue<ArrayDatum>(d, names::record_to);
    for ( Token* t = ad.begin() ; t != ad.end() ; ++t )
      if ( *t == LiteralDatum(names::file) || *t == Token(names::file.toString()) )
        to_file_ = true;
      else if ( *t == LiteralDatum(names::memory) || *t == Token(names::memory.toString()) )
        to_memory_ = true;
      else if ( *t == LiteralDatum(names::screen) || *t == Token(names::screen.toString()) )
        to_screen_ = true;
      else if ( rd.mode_ == RecordingDevice::MULTIMETER  &&
                ( *t == LiteralDatum(names::accumulator) || *t == Token(names::accumulator.toString()) ) )
        to_accumulator_ = true;
      else
      {
        if ( rd.mode_ == RecordingDevice::MULTIMETER )
          throw BadProperty("/to_record must be array, allowed entries: /file, /memory, /screen, /accumulator.");
        else
          throw BadProperty("/to_record must be array, allowed entries: /file, /memory, /screen.");
      }
}
  
  if ( ( rec_change || have_record_to ) && to_file_ && to_memory_ )
    NestModule::get_network().message(SLIInterpreter::M_INFO, "RecordingDevice::set_status",
                                   "Data will be recorded to file and to memory.");

  if ( to_accumulator_ && ( to_file_ || to_screen_ || to_memory_ || withgid_ || withpath_ || withweight_ ) )
  {
      to_file_ = to_screen_ = to_memory_ = withgid_ = withpath_ = withweight_ = false;
      Node::network()->message(SLIInterpreter::M_WARNING, "RecordingDevice::set_status()",
                               "Accumulator mode selected. All incompatible properties "
                               "(to_file, to_screen, to_memory, withgid, withpath, withweight) "
                               "have been set to false.");
  }
}

void nest::RecordingDevice::State_::get(DictionaryDatum& d, const Parameters_& p) const
{
  // if we already have the n_events entry, we add to it, otherwise we create it
  if (d->lookup(names::n_events) != d->getvoid())
    (*d)[names::n_events] = getValue<long>(d, names::n_events) + events_;
  else 
    (*d)[names::n_events] = events_;

  DictionaryDatum dict;

  // if we already have the events dict, we use it, otherwise we create it
  if (d->lookup(names::events) == d->getvoid())
    dict = DictionaryDatum(new Dictionary);
  else
    dict = getValue<DictionaryDatum>(d, names::events);

  if ( p.withgid_ || p.withpath_ )
  {
    initialize_property_intvector(dict, names::senders);
    append_property(dict, names::senders, std::vector<long>(event_senders_));
  }  

  if ( p.withweight_ )
  {
    initialize_property_doublevector(dict, names::weights);
    append_property(dict, names::weights, std::vector<double_t>(event_weights_));
  }  

  if ( p.withtime_ )
  {
    if ( p.time_in_steps_ )
    {  
      {
        initialize_property_intvector(dict, names::times);
        append_property(dict, names::times, std::vector<long>(event_times_steps_));
      }  
      if ( p.precise_times_ )
      {
        initialize_property_doublevector(dict, names::offsets);
        append_property(dict, names::offsets, std::vector<double_t>(event_times_offsets_));
      }  
    }
    else
    {
      initialize_property_doublevector(dict, names::times);
      append_property(dict, names::times, std::vector<double_t>(event_times_ms_));
    }  
  }

  (*d)[names::events] = dict;
}

void nest::RecordingDevice::State_::set(const DictionaryDatum& d)
{
  long_t ne = 0;
  if ( updateValue<long_t>(d, names::n_events, ne) )
  {
    if ( ne == 0 )
      events_ = 0;
    else
      throw BadProperty("n_events can only be set to 0.");
  }
}

 /* ---------------------------------------------------------------- 
  * Default and copy constructor for device
  * ---------------------------------------------------------------- */

nest::RecordingDevice::RecordingDevice(const Node& n, Mode mode, const std::string& file_ext,
				       bool withtime, bool withgid)
   : Device(),
     node_(n),
     mode_(mode),
     P_(file_ext, withtime, withgid),
     S_()
 {}

 nest::RecordingDevice::RecordingDevice(const Node& n, const RecordingDevice& d)
   : Device(d),
     node_(n),
     mode_(d.mode_),
     P_(d.P_),
     S_(d.S_)
 {}


 /* ---------------------------------------------------------------- 
  * Device initialization functions
  * ---------------------------------------------------------------- */

 void nest::RecordingDevice::init_parameters(const RecordingDevice& pr)
 {
   Device::init_parameters(pr);

   P_ = pr.P_;
   S_ = pr.S_;
 }

 void nest::RecordingDevice::init_state(const RecordingDevice& pr)
 { 
   Device::init_state(pr);
   S_ = pr.S_;
 }

 void nest::RecordingDevice::init_buffers()
 {
   Device::init_buffers();

   // we only close files here, opening is left to calibrate()
   if ( P_.close_on_reset_ && B_.fs_.is_open() )
   {
     B_.fs_.close();
     P_.filename_.clear();  // filename_ only visible while file open
   }
   
 }

 void nest::RecordingDevice::calibrate()
 {
   Device::calibrate();

   if ( P_.to_file_ )
   {
     // do we need to (re-)open the file
     bool newfile = false;
     
     if ( !B_.fs_.is_open() )
     {
       newfile = true;   // no file from before
       P_.filename_ = build_filename_();
     }
     else
     {
       std::string newname = build_filename_();
       if ( newname != P_.filename_ )
       {
         Node::network()->message(SLIInterpreter::M_INFO, 
				  "RecordingDevice::calibrate()",
				  "Closing file " + P_.filename_ +
				  ", opening file " + newname);
                               
         B_.fs_.close(); // close old file
         P_.filename_ = newname;
         newfile = true;
       }
     }
     
     if ( newfile )
     {
       assert(!B_.fs_.is_open());
       
       if ( Node::network()->overwrite_files() )
         B_.fs_.open(P_.filename_.c_str());
       else
       {
         // try opening for reading
         std::ifstream test(P_.filename_.c_str());
         if ( test.good() )
         {
           Node::network()->message(SLIInterpreter::M_ERROR, "RecordingDevice::calibrate()",
             "The device file " + P_.filename_ + " exists already and will not be overwritten.\n"
             "Please change data_path, data_prefix or label, or set /overwrite_files to true in the root node." );
           throw IOError();
         }
         else 
           test.close();
           
         // file does not exist, so we can open
         B_.fs_.open(P_.filename_.c_str());
       }
     }
       
     if ( !B_.fs_.good() )
     {
       Node::network()->message(SLIInterpreter::M_ERROR, "RecordingDevice::calibrate()", 
                              "I/O error while opening file " + P_.filename_);
       if ( B_.fs_.is_open() )
         B_.fs_.close();                        
       P_.filename_.clear();
       throw IOError();
     }
     
     /* Set formatting
        Formatting is not applied to std::cout for screen output,
        since different devices may have different settings and
        this would lead to a mess.
      */
     if ( P_.scientific_ )
       B_.fs_ << std::scientific;
     else
       B_.fs_ << std::fixed;

     B_.fs_ << std::setprecision(P_.precision_); 
   }

 }

 void nest::RecordingDevice::finalize()
 {
   if ( B_.fs_.is_open() )
   {
     if ( P_.close_after_simulate_ ) 
     {
       B_.fs_.close();
       return;
     }
     
     if ( P_.flush_after_simulate_ )
       B_.fs_.flush();
       
     if ( !B_.fs_.good() )
     {
       Node::network()->message(SLIInterpreter::M_ERROR, "RecordingDevice::finalize()", 
                              "I/O error while writing to file " + P_.filename_);
       throw IOError();
     }
   }
 }

 /* ---------------------------------------------------------------- 
  * Other functions
  * ---------------------------------------------------------------- */
  
void nest::RecordingDevice::set_status(const DictionaryDatum &d)
{
  Parameters_ ptmp = P_;  // temporary copy in case of errors
  ptmp.set(*this, d);                       // throws if BadProperty
  State_      stmp = S_;
  stmp.set(d);

  // We now know that (ptmp, stmp) are consistent. We do not 
  // write them back to (P_, S_) before we are also sure that 
  // the properties to be set in the parent class are internally 
  // consistent.
  Device::set_status(d);

  // if we get here, temporaries contain consistent set of properties
  P_ = ptmp;
  S_ = stmp;
  
  if ( !P_.to_file_ && B_.fs_.is_open() )
  {
    B_.fs_.close();
    P_.filename_.clear();
  }

  if ( S_.events_ == 0 )
    S_.clear_events();
}
    

void nest::RecordingDevice::record_event(const Event& event, bool endrecord)
{
  ++S_.events_;
  const index  sender = event.get_sender().get_gid();
  const Time   stamp  = event.get_stamp();
  const double offset = event.get_offset();
  const double weight = event.get_weight();

  if ( P_.to_screen_ )
  {
    print_id_(std::cout, sender);
    print_time_(std::cout, stamp, offset);
    print_weight_(std::cout, weight);
    if ( endrecord )
      std::cout << '\n';
  }
  
  if ( P_.to_file_ )
  {
    print_id_(B_.fs_, sender);
    print_time_(B_.fs_, stamp, offset);
    print_weight_(std::cout, weight);
    if ( endrecord )
    {
      B_.fs_ << '\n';
      if ( P_.flush_records_ )
        B_.fs_.flush();
    }
  }

  // storing data when recording to accumulator relies on the fact
  // that multimeter will call us only once per accumulation step
  if ( P_.to_memory_ || P_.to_accumulator_ )
      store_data_(sender, stamp, offset, weight);
}

void nest::RecordingDevice::print_id_(std::ostream& os, index gid)
{
  if ( P_.withgid_ )
    os << gid << '\t';

  if ( P_.withpath_ )
  {
    const std::vector<size_t> ta = Node::network()->get_adr(gid);
    for ( size_t j = 0 ; j < ta.size() ; ++j )
      os << ta[j] << ' ';
    os << '\t';
  }
}

void nest::RecordingDevice::print_time_(std::ostream& os, const Time& t, double offs)
{
  if ( !P_.withtime_ )
    return;

  if ( P_.time_in_steps_ )
  {
    os << t.get_steps() << '\t';
    if ( P_.precise_times_ )
      os << offs << '\t';
  }
  else if ( P_.precise_times_ )
    os << t.get_ms() - offs << '\t';
  else
    os << t.get_ms() << '\t';
}

void nest::RecordingDevice::print_weight_(std::ostream& os, double weight)
{
  if ( P_.withweight_ )
    os << weight << '\t';
}


void nest::RecordingDevice::store_data_(index sender, const Time& t, double offs, double weight)
{
  if ( P_.withgid_ || P_.withpath_ )
    S_.event_senders_.push_back(sender);
  
  if ( P_.withtime_ )
  {
    if ( P_.time_in_steps_ )
    {
      S_.event_times_steps_.push_back(t.get_steps());
      if ( P_.precise_times_ )
        S_.event_times_offsets_.push_back(offs);
    }
    else if ( P_.precise_times_ )
      S_.event_times_ms_.push_back(t.get_ms()-offs);
    else   
      S_.event_times_ms_.push_back(t.get_ms());
  }

  if ( P_.withweight_ )
    S_.event_weights_.push_back(weight);
}

const std::string nest::RecordingDevice::build_filename_() const
{
  // number of digits in number of virtual processes
  const int vpdigits = static_cast<int>(std::floor(std::log10(static_cast<float>(Communicator::get_num_virtual_processes()))) + 1);
  const int gidigits = static_cast<int>(std::floor(std::log10(static_cast<float>(Node::network()->size()))) + 1);
    
  std::ostringstream basename;
  const std::string& path = Node::network()->get_data_path();
  if ( !path.empty() )
    basename << path << '/';
  basename << Node::network()->get_data_prefix();
  

  if ( !P_.label_.empty() )        
    basename << P_.label_;
  else
    basename << node_.get_name();

  basename << "-" << std::setfill('0') << std::setw(gidigits) << node_.get_gid()
           << "-" << std::setfill('0') << std::setw(vpdigits) << node_.get_vp();
  return basename.str() + '.' + P_.file_ext_;
}

void nest::RecordingDevice::State_::clear_events()
{
  events_ = 0;
  event_senders_.clear();
  event_times_ms_.clear();
  event_times_steps_.clear();
  event_times_offsets_.clear();
  event_weights_.clear();
}

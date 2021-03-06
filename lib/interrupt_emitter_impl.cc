/* -*- c++ -*- */
/*
 * Copyright 2018 National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains certain rights in this software.
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "interrupt_emitter_impl.h"
#include <cmath>

namespace gr {
  namespace timing_utils {

    interrupt_emitter::sptr
    interrupt_emitter::make(double rate, bool drop_late)
    {
      return gnuradio::get_initial_sptr
        (new interrupt_emitter_impl(rate, drop_late));
    }    
    
    /*
     * The private constructor
     */
    interrupt_emitter_impl::interrupt_emitter_impl(double rate, bool drop_late)
      : gr::sync_block("interrupt_emitter",
              gr::io_signature::make(1, 1, sizeof(gr_complex)),
              gr::io_signature::make(0, 0, 0)), 
          ReferenceTimer(),
          d_rate(rate),          
          d_drop_late(drop_late)          
    {
      message_port_register_out(pmt::mp("trig"));
      message_port_register_in(pmt::mp("set"));

      set_msg_handler(pmt::mp("set"),
          boost::bind(&interrupt_emitter_impl::handle_set_time, this, _1));
          
      boost::posix_time::ptime epoch(boost::gregorian::date(1970,1,1));
      d_epoch = epoch;
      timer_thread = new boost::thread(boost::bind(&boost::asio::io_service::run, &io));
      last_error = 0;
      debug = false;
    }

    bool interrupt_emitter_impl::start() {
      time_offset = (boost::get_system_time() - epoch).total_microseconds()*1e-6;
      last_time = boost::get_system_time() - boost::posix_time::microseconds(1e6*time_offset);
    }

    /*
     * Our virtual destructor.
     */
    interrupt_emitter_impl::~interrupt_emitter_impl()
    {
        stop();        
        delete timer_thread;       
    }
    
    pmt::pmt_t interrupt_emitter_impl::samples_to_tpmt(uint64_t trigger_sample) {
        // reference this sample to the end of the last buffer processed.
        double time = (trigger_sample - d_start_sample) / d_rate + d_start_time;
        uint64_t t_int = uint64_t(time);
        double t_frac = time - t_int;
        if (t_frac > 1) {
            t_int += 1;
            t_frac -= 1.0;
        }
        
        return pmt::cons(pmt::from_uint64(t_int), pmt::from_double(t_frac));
    }
    
    uint64_t interrupt_emitter_impl::time_to_samples(double time) {
        uint64_t sample = (time - d_start_time) * d_rate + d_start_sample;
        return sample; 
    }
    
    void interrupt_emitter_impl::handle_set_time(pmt::pmt_t time_pmt) {
        if (debug) std::cout << "Received msg: " << time_pmt << std::endl;

        uint64_t trigger_sample;
        pmt::pmt_t trigger_time = pmt::PMT_NIL;
        uint64_t t_int;
        double t_frac;
        if (pmt::is_uint64(time_pmt)) {            
            // if the tuple is a single uint64_t it is the sample to trigger
            trigger_sample = pmt::to_uint64(time_pmt);            
            trigger_time = samples_to_tpmt(trigger_sample);            
            t_int = pmt::to_uint64(pmt::car(trigger_time));            
            t_frac = pmt::to_double(pmt::cdr(trigger_time));            
        } else if (pmt::is_pair(time_pmt)) {
            // if it is a pair, check the car and cdr for correct types
            if (pmt::is_uint64(pmt::car(time_pmt)) & pmt::is_real(pmt::cdr(time_pmt))) {
                trigger_time = time_pmt;
                t_int = pmt::to_uint64(pmt::car(time_pmt));
                t_frac = pmt::to_double(pmt::cdr(time_pmt));
                trigger_sample = time_to_samples(t_int + t_frac);
            } else {
                // for anything else, ignore
                return;
            }
        } else if (pmt::is_tuple(time_pmt)) {
            if (pmt::is_uint64(pmt::tuple_ref(time_pmt,0)) & pmt::is_real(pmt::tuple_ref(time_pmt, 1))) {
                t_int = pmt::to_uint64(pmt::tuple_ref(time_pmt, 0));
                t_frac = pmt::to_double(pmt::tuple_ref(time_pmt, 1));
                trigger_time = pmt::cons(pmt::tuple_ref(time_pmt, 0), pmt::tuple_ref(time_pmt, 1));
                trigger_sample = time_to_samples(t_int + t_frac);
            } else {
                return;
            }
        } else return;
        
        pmt::pmt_t pmt_out = pmt::make_dict();
        pmt_out = pmt::dict_add(pmt_out, pmt::mp("trigger_time"), trigger_time);
        pmt_out = pmt::dict_add(pmt_out, pmt::mp("trigger_sample"), pmt::from_uint64(trigger_sample));
        
        double current_time((boost::get_system_time() - epoch).total_microseconds()/1000000.0 - time_offset);
        double wait_time = t_int + t_frac - current_time;
        
        if (wait_time < 0) {
            if (d_drop_late) {
                std::cout << "Dropping late interrupt request: " << wait_time << std::endl;
            } else {
                // Interrupt right now
                if (debug) std::cout << "Sending late message\n";
                // Modify the time that goes out to be right now.  Since this is a real time interrupt system
                // bad things can happen if we go off of the sample time rather than clock time.
                pmt::pmt_t p_int = pmt::from_uint64(int(current_time));
                pmt::pmt_t p_frac = pmt::from_double(current_time - int(current_time));
                pmt_out = pmt::dict_add(pmt_out, pmt::mp("trigger_time"), pmt::cons(p_int, p_frac));
                io.dispatch(boost::bind(&interrupt_emitter_impl::StartTimer, this, 0, pmt_out));
            }
        } else {
            if (debug) std::cout << "arming interrupt\n";
            io.dispatch(boost::bind(&interrupt_emitter_impl::StartTimer, this, wait_time, pmt_out));
        }
    }
    
    bool interrupt_emitter_impl::stop() {
        io.dispatch(boost::bind(&interrupt_emitter_impl::StopTimer, this));
        timer_thread->join();
        return true;
    }
    
    void interrupt_emitter_impl::process_interrupt() {
        pmt::pmt_t time_pmt = pmt::dict_ref(d_out_pmt, pmt::mp("trigger_time"), pmt::PMT_NIL);
        double int_time = pmt::to_uint64(pmt::car(time_pmt)) + pmt::to_double(pmt::cdr(time_pmt));
        d_out_pmt = pmt::dict_add(d_out_pmt, pmt::mp("trigger_sample"), pmt::from_uint64(time_to_samples(int_time)));
        message_port_pub(pmt::mp("trig"), d_out_pmt); 
    }
    
    int interrupt_emitter_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
        {
          // Assume that the current time corresponds with 1 sample after the end of the buffer
          boost::posix_time::ptime current_ptime = boost::get_system_time() - boost::posix_time::microseconds(1e6*time_offset);
          double current_time((current_ptime - epoch).total_microseconds()/1e6);
          d_start_sample = nitems_read(0) + noutput_items;
          d_start_time = current_time;
          
          const gr_complex *in = (const gr_complex *) input_items[0];

          std::vector<tag_t> tags;          

          // check for uhd rx_time tags to set baseline time and correct for overflows
          get_tags_in_range(tags, 0, nitems_read(0), (nitems_read(0) + noutput_items), pmt::mp("rx_time"));
          if (tags.size()) {
            // Only need to look at the last one
            tag_t last_tag = tags[tags.size()-1];
            uint64_t t_int = pmt::to_uint64(pmt::tuple_ref(last_tag.value, 0));
            double t_frac = pmt::to_double(pmt::tuple_ref(last_tag.value, 1));
            uint64_t samp = last_tag.offset;
            //printf("Tag at time: %f, sample: %ld, offset: %f, time:%f\n", t_int+t_frac, samp, time_offset, current_time);
            // What time did we expect to see samp at??
            // Note that we expect to receive the whole buffer at the same time, so we need to reref to the last sample
            double expect_samp = current_time - (nitems_read(0) + noutput_items - samp)/d_rate;
            double error = expect_samp - t_int - t_frac;            
            time_offset += error;
            d_start_time -= error;
            current_ptime -= boost::posix_time::microseconds(1e6*error);            
            UpdateTimer(error);
            if (debug) printf("tag_error = %f\n", error);
          } else if (time_offset > 0) {
            // See if it took us as long as we think to receive this buffer
            double buffer_time = noutput_items / d_rate;
            double time_diff = (current_ptime - last_time).total_microseconds()*1e-6;
            double error = time_diff - buffer_time + last_error;
            if (std::abs(error) > 200e-6) {
                // Allow for some sample error, because the system clock isn't perfect
                time_offset += error;
                d_start_time -= error;                
                current_ptime -= boost::posix_time::microseconds(1e6*error);
                if (debug) printf("sample_error (ms) = %f, items = %d, last_error = %f\n", 1e3*error, noutput_items, last_error);
                last_error = 0;
            } else last_error = error;
          }   
          last_time = current_ptime;       

          // Tell runtime system how many output items we produced.
          return noutput_items;
        }


  } /* namespace timing_utils */
} /* namespace gr */

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

/* WARNING: this file is machine generated. Edits will be overwritten */

#ifndef INCLUDED_TIMING_UTILS_SYSTEM_TIME_DIFF_B_H
#define INCLUDED_TIMING_UTILS_SYSTEM_TIME_DIFF_B_H

#include <timing_utils/api.h>
#include <gnuradio/sync_block.h>

namespace gr {
  namespace timing_utils {

    /*!
     * \brief <+description of block+>
     * \ingroup timing_utils
     *
     */
    class TIMING_UTILS_API system_time_diff_b : virtual public gr::block
    {
     public:
       typedef boost::shared_ptr<system_time_diff_b> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of timing_utils::system_time_diff_b.
       *
       * To avoid accidental use of raw pointers, timing_utils::system_time_diff_b's
       * constructor is in a private implementation
       * class. timing_utils::system_time_diff_b::make is the public interface for
       * creating new instances.
       */
      static sptr make(bool output_data, bool update_time, bool output_diff);
    };

  } // namespace timing_utils
} // namespace gr

#endif /* INCLUDED_TIMING_UTILS_SYSTEM_TIME_DIFF_B_H */


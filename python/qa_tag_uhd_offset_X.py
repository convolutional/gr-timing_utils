#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Copyright 2018 National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains certain rights in this software.
#
# This is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
#
# This software is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this software; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.
#

from builtins import range
from gnuradio import gr, gr_unittest
from gnuradio import blocks
import timing_utils_swig as timing_utils
import pmt
import time
from math import ceil

class qa_tag_uhd_offset_X (gr_unittest.TestCase):

    def setUp (self):
        self.tb2 = gr.top_block ()

    def tearDown (self):
        self.tb2 = None

    def test_001_basic (self):
        self.tb = gr.top_block ()
        self.rate = 250000
        self.interval = 173
        self.duration = 9000

        self.src = blocks.vector_source_c(list(range(self.duration)), False, 1, [])
        self.utag = timing_utils.add_usrp_tags_c(1090e6, self.rate, 0, .98765)
        self.tags = timing_utils.tag_uhd_offset_c(self.rate, self.interval)
        self.tag_dbg = blocks.tag_debug(gr.sizeof_gr_complex*1, '', "");
        self.tag_dbg.set_display(False)
        self.tb.connect((self.src, 0), (self.utag, 0))
        self.tb.connect((self.utag, 0), (self.tags, 0))
        self.tb.connect((self.tags, 0), (self.tag_dbg, 0))
        e_n_tags = int(ceil(1.0*self.duration / self.interval)) + 3

        self.tb.run ()
        time.sleep(0.05)

        tags = self.tag_dbg.current_tags()
        tprev = None
        for t in tags:
            if pmt.eq(t.key, pmt.intern("rx_time_offset")):
                self.assertAlmostEqual(self.rate, pmt.to_double(pmt.tuple_ref(t.value, 3)))
                self.assertEqual(t.offset, pmt.to_uint64(pmt.tuple_ref(t.value, 2)))
                self.assertTrue((pmt.to_uint64(pmt.tuple_ref(t.value, 2)) / (1.0*self.interval)).is_integer())
                if tprev is not None:
                    tcur = pmt.to_uint64(pmt.tuple_ref(t.value, 0)) + pmt.to_double(pmt.tuple_ref(t.value, 1))
                    self.assertAlmostEqual(tcur-tprev, 1.0*self.interval / self.rate)
                    tprev = tcur
                else:
                    tprev = pmt.to_uint64(pmt.tuple_ref(t.value, 0)) + pmt.to_double(pmt.tuple_ref(t.value, 1))
        self.assertEqual(self.tag_dbg.num_tags(), e_n_tags)
        self.tb = None

    def test_002_alt (self):
        self.tb = gr.top_block ()
        self.rate = 1
        self.interval = 133
        self.duration = 4532

        self.src = blocks.vector_source_c(list(range(self.duration)), False, 1, [])
        self.utag = timing_utils.add_usrp_tags_c(1090e6, self.rate, 0, .98765)
        self.tags = timing_utils.tag_uhd_offset_c(self.rate, self.interval)
        self.tag_dbg = blocks.tag_debug(gr.sizeof_gr_complex*1, "", "");
        self.tag_dbg.set_display(False)
        self.tb.connect((self.src, 0), (self.utag, 0))
        self.tb.connect((self.utag, 0), (self.tags, 0))
        self.tb.connect((self.tags, 0), (self.tag_dbg, 0))
        e_n_tags = int(ceil(1.0*self.duration / self.interval)) + 3

        self.tb.run ()

        tags = self.tag_dbg.current_tags()
        tprev = None
        for t in tags:
            if pmt.eq(t.key, pmt.intern("rx_time_offset")):
                self.assertAlmostEqual(self.rate, pmt.to_double(pmt.tuple_ref(t.value, 3)))
                self.assertEqual(t.offset, pmt.to_uint64(pmt.tuple_ref(t.value, 2)))
                self.assertTrue((pmt.to_uint64(pmt.tuple_ref(t.value, 2)) / (1.0*self.interval)).is_integer())
                if tprev is not None:
                    tcur = pmt.to_uint64(pmt.tuple_ref(t.value, 0)) + pmt.to_double(pmt.tuple_ref(t.value, 1))
                    self.assertAlmostEqual(tcur-tprev, 1.0*self.interval / self.rate)
                    tprev = tcur
                else:
                    tprev = pmt.to_uint64(pmt.tuple_ref(t.value, 0)) + pmt.to_double(pmt.tuple_ref(t.value, 1))
        self.assertEqual(self.tag_dbg.num_tags(), e_n_tags)
        self.tb = None

    def test_003_every (self):
        self.tb = gr.top_block ()
        self.rate = 99999999999
        self.interval = 1
        self.duration = 4321

        self.src = blocks.vector_source_c(list(range(self.duration)), False, 1, [])
        self.utag = timing_utils.add_usrp_tags_c(1090e6, self.rate, 0, .98765)
        self.tags = timing_utils.tag_uhd_offset_c(self.rate, self.interval)
        self.tag_dbg = blocks.tag_debug(gr.sizeof_gr_complex*1, "", "");
        self.tag_dbg.set_display(False)
        self.tb.connect((self.src, 0), (self.utag, 0))
        self.tb.connect((self.utag, 0), (self.tags, 0))
        self.tb.connect((self.tags, 0), (self.tag_dbg, 0))
        e_n_tags = int(ceil(1.0*self.duration / self.interval)) + 3

        self.tb.run ()

        tags = self.tag_dbg.current_tags()
        tprev = None
        for t in tags:
            if pmt.eq(t.key, pmt.intern("rx_time_offset")):
                self.assertAlmostEqual(self.rate, pmt.to_double(pmt.tuple_ref(t.value, 3)),-4)
                self.assertEqual(t.offset, pmt.to_uint64(pmt.tuple_ref(t.value, 2)))
                self.assertTrue((pmt.to_uint64(pmt.tuple_ref(t.value, 2)) / (1.0*self.interval)).is_integer())
                if tprev is not None:
                    tcur = pmt.to_uint64(pmt.tuple_ref(t.value, 0)) + pmt.to_double(pmt.tuple_ref(t.value, 1))
                    self.assertAlmostEqual(tcur-tprev, 1.0*self.interval / self.rate)
                    tprev = tcur
                else:
                    tprev = pmt.to_uint64(pmt.tuple_ref(t.value, 0)) + pmt.to_double(pmt.tuple_ref(t.value, 1))
        self.assertEqual(self.tag_dbg.num_tags(), e_n_tags)
        self.tb = None

    def test_004_onlyusrp (self):
        self.tb = gr.top_block ()
        self.rate = 250000
        self.interval = 0
        self.duration = 4532

        self.src = blocks.vector_source_c(list(range(self.duration)), False, 1, [])
        self.utag = timing_utils.add_usrp_tags_c(1090e6, self.rate, 0, .98765)
        self.tags = timing_utils.tag_uhd_offset_c(self.rate, self.interval)
        self.tag_dbg = blocks.tag_debug(gr.sizeof_gr_complex*1, "", "");
        self.tag_dbg.set_display(False)
        self.tb.connect((self.src, 0), (self.utag, 0))
        self.tb.connect((self.utag, 0), (self.tags, 0))
        self.tb.connect((self.tags, 0), (self.tag_dbg, 0))
        e_n_tags = 4

        self.tb.run ()
        self.assertEqual(self.tag_dbg.num_tags(), e_n_tags)
        self.tb = None



if __name__ == '__main__':
    gr_unittest.run(qa_tag_uhd_offset_X, "qa_tag_uhd_offset_X.xml")

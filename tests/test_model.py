#!/usr/bin/env python

# run tests with: python -m unittest test_model
#             or  python -m unittest discover

import os
import sys
import unittest
import fityk


class TestCopy(unittest.TestCase):
    def setUp(self):
        self.ftk = fityk.Fityk()
        self.ftk.set_option_as_number("verbosity", -1)

    def test_copy_func(self):
        self.ftk.execute("%f = Gaussian(~10, ~0, 0.5)")
        self.ftk.execute("%g = copy(%f)")
        self.assertEqual(self.ftk.get_parameter_count(), 4)
        self.ftk.execute("%g.center = ~2.3")
        self.assertEqual(self.ftk.get_parameter_count(), 4)
        self.ftk.execute("F = %f + %g")
        self.ftk.execute("@+ = 0")
        self.ftk.execute("@1.F = copy(@0.F)")
        self.assertEqual(self.ftk.get_parameter_count(), 8)
        self.ftk.execute("@+ = 0")
        self.ftk.execute("@2.F = @0.F")
        self.assertEqual(self.ftk.get_parameter_count(), 8)
        self.ftk.execute("%g.center = ~2.4")
        d0_funcs = self.ftk.get_components(0)
        d1_funcs = self.ftk.get_components(1)
        d2_funcs = self.ftk.get_components(2)
        self.assertEqual(d0_funcs[0].get_param_value('center'), 0)
        self.assertEqual(d0_funcs[1].get_param_value('center'), 2.4)
        self.assertEqual(d1_funcs[0].get_param_value('center'), 0)
        self.assertEqual(d1_funcs[1].get_param_value('center'), 2.3)
        self.assertEqual(d2_funcs[0].get_param_value('center'), 0)
        self.assertEqual(d2_funcs[1].get_param_value('center'), 2.4)

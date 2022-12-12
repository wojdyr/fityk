#!/usr/bin/env python3

import os
import sys
import math
import unittest
import fityk

class TestGuessability(unittest.TestCase):
    def setUp(self):
        xx = [(n-5)/2. for n in range(25)]
        yy = [11.2/(1+(x-3.3)**2) + math.sin(40*x)/40 - x/10. for x in xx]
        self.ftk = fityk.Fityk()
        self.ftk.set_option_as_number("verbosity", -1)
        self.ftk.load_data(0, xx, yy, [1]*len(xx), "loren")
        #self.ftk.execute("info state > debug.fit")

def create_guessability_test(func_type):
    def do_test(self):
        self.ftk.execute("guess %f = " + func_type)
        f = self.ftk.get_function('f')
        self.assertIsInstance(f, fityk.Func)
        self.assertEqual(f.get_template_name(), func_type)
        try:
            ctr = f.get_param_value("Center")
            if func_type not in ['Sigmoid']:
                self.assertAlmostEqual(ctr, 3.3, delta=0.3)
        except fityk.ExecuteError:
            pass
        self.ftk.execute("delete %f")
    return do_test

def add_guessability_tests():
    all_types = fityk.Fityk().get_info('types')
    for func_type in all_types.split():
        if func_type not in ['FCJAsymm']:
            test = create_guessability_test(func_type)
            test.__name__ = 'test_' + func_type
            setattr(TestGuessability, test.__name__, test)

add_guessability_tests()

if __name__ == '__main__':
    unittest.main()

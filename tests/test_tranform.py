#!/usr/bin/env python

# run tests with: python -m unittest test_tranform
#                 python -m unittest discover

import os
import sys
import unittest
import fityk
try:
    import numpy
except ImportError:
    numpy = None

def get_data_as_lists(ftk):
    data = ftk.get_data()
    return [p.x for p in data], [p.y for p in data], [p.sigma for p in data]

class TestTransform(unittest.TestCase):
    def setUp(self):
        self.x = [(n-5)/2. for n in range(20)]
        self.y = [x*(x-3.1) for x in self.x]
        self.sigma = [1 for n in self.y]
        self.ftk = fityk.Fityk()
        self.ftk.set_option_as_number("verbosity", -1)
        self.ftk.load_data(0, self.x, self.y, self.sigma, "test-data")

    def test_load(self):
        xx, yy, ss = get_data_as_lists(self.ftk)
        self.assertEqual(xx, self.x)
        self.assertEqual(yy, self.y)
        self.assertEqual(ss, self.sigma)
        #print self.x
        #print self.y

    def assert_expr(self, expr, val, places=None):
        expr_val = self.ftk.calculate_expr(expr)
        if places is None:
            self.assertEqual(expr_val, val)
        else:
            self.assertAlmostEqual(expr_val, val, places=places)

    def test_expr(self):
        xx, yy, ss = get_data_as_lists(self.ftk)
        M = len(yy)
        self.assert_expr("M", M)
        self.assert_expr("count(y>0)", len([y for y in yy if y > 0]))
        self.assert_expr("sum(x)", sum(xx))
        self.assert_expr("sum(y)", sum(yy))
        self.assert_expr("darea(y)",
                         sum(yy[n]*(xx[min(n+1, M-1)]-xx[max(n-1, 0)])/2
                             for n in range(M)))
        if not numpy:
            return
        self.assert_expr("avg(x)",    numpy.mean(xx))
        self.assert_expr("min(x)",    numpy.min(xx))
        self.assert_expr("max(x)",    numpy.max(xx))
        self.assert_expr("stddev(x)", numpy.std(xx, ddof=1))
        self.assert_expr("avg(y)",    numpy.mean(yy), places=12)
        self.assert_expr("min(y)",    numpy.min(yy))
        self.assert_expr("max(y)",    numpy.max(yy))
        self.assert_expr("stddev(y)", numpy.std(yy, ddof=1), places=12)
        self.assert_expr("argmin(y)", xx[numpy.argmin(yy)])
        self.assert_expr("argmax(y)", xx[numpy.argmax(yy)])

    def test_delete(self):
        self.ftk.execute("delete(x < 0)")
        xx, yy, ss = get_data_as_lists(self.ftk)
        self.assertEqual(xx, [x for x in self.x if x >= 0])

    def test_delete2(self):
        self.ftk.execute("A = x >= 0; delete(not a)")
        xx, yy, ss = get_data_as_lists(self.ftk)
        self.assertEqual(xx, [x for x in self.x if x >= 0])

    def test_normalize(self):
        self.ftk.execute("Y = y / darea(y)")
        self.assertEqual(self.ftk.calculate_expr("darea(y)"), 1)

    def test_set_y(self):
        self.ftk.execute("Y[3] = 1.2; Y[-2] = 12.34")
        xx, yy, ss = get_data_as_lists(self.ftk)
        self.assertEqual(yy[3], 1.2)
        self.assertEqual(yy[-2], 12.34)

    def test_xy_swap(self):
        self.ftk.execute("X=y, Y=x") # swap & sort!
        xx, yy, ss = get_data_as_lists(self.ftk)
        self.assertEqual(xx, sorted(self.y))
        xymap = { x: y for x, y in zip(self.y, self.x) }
        self.assertEqual(yy, [xymap[x] for x in xx])
        self.assertEqual(ss, self.sigma)


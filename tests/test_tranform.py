#!/usr/bin/env python3

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
        #print(self.x)
        #print(self.y)

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


class TestFuncProperties(unittest.TestCase):
    def setUp(self):
        self.ftk = fityk.Fityk()
        self.ftk.set_option_as_number("verbosity", -1)
        self.ftk.execute("%f = Quadratic(~1, 2, ~3)")
        self.ftk.execute("%g = Constant(7.5)")
        self.ftk.execute("F = %f + %g")

    def _check(self, expr, val, places=12):
        expr_val = self.ftk.calculate_expr(expr)
        self.assertAlmostEqual(expr_val, val, places=places)

    def test_numarea(self):
        # Integral of 1+2*x+3*x^2 = x+x^2+x^3| from 2 to 4 = 84-14 = 70
        self._check("%f.numarea(2, 4, 1000)", 70, places=5)
        self._check("%g.numarea(2, 4, 3)", 2*7.5)
        self._check("F.numarea(2, 4, 300)", 70 + 2*7.5, places=4)

    def test_findx(self):
        self._check("%f.findx(0, 5, 6)", 1.0, places=12)
        self._check("%f.findx(-1.5, 1.2, 6)", 1.0, places=12)
        self.ftk.calculate_expr("%g.findx(-10, 10, 7.5)") # anything
        self._check("F.findx(-1.5, 1.2, 13.5)", 1.0, places=12)
        self.ftk.execute("Z = Constant(0.2)")
        self._check("F.findx(-1.5, 1.2, 13.5)", 1.0-0.2, places=12)

    def test_extremum(self):
        self._check("%f.extremum(-10, 10)", -1.0/3, places=13)
        self.ftk.calculate_expr("%g.extremum(-10, 10)") # anything
        self._check("F.extremum(-10, 10)", -1.0/3, places=13)
        self.ftk.execute("Z = Constant(0.2)")
        self._check("F.extremum(-10, 10)", -1.0/3-0.2, places=12)

if __name__ == '__main__':
    unittest.main()


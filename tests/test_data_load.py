#!/usr/bin/env python
# -*- coding: utf-8 -*-

# run tests with: python -m unittest test_data_load
#             or  python -m unittest discover

import os
import sys
import random
import tempfile
import gzip
import unittest
import fityk

#locale.setlocale(locale.LC_NUMERIC, 'C')

class FileLoadBase(unittest.TestCase):
    def setUp(self):
        self.ftk = fityk.Fityk()
        self.ftk.set_option_as_number("verbosity", -1)
        prefix = getattr(self, 'file_prefix', 'tmp')
        f = tempfile.NamedTemporaryFile(prefix=prefix, delete=False)
        self.data = self.generate_data()
        for d in self.data:
            f.write(self.line_format(d))
        f.close()
        self.data.sort()
        self.filename = f.name

    def generate_data(self):
        return [(random.uniform(-100, 100), random.gauss(10, 20))
                for _ in range(30)]

    def tearDown(self):
        #print "remove " + self.filename
        os.unlink(self.filename)

    def compare(self, out, ndigits):
        self.assertEqual([i.x for i in out],
                         [round(i[0], ndigits) for i in self.data])
        self.assertEqual([i.y for i in out],
                         [round(i[1], ndigits) for i in self.data])

class TextFileLoadBase(FileLoadBase):
    def line_format(self, t):
        return " %.7f %.7f\n" % t
    def test_load(self):
        self.ftk.execute("@0 < '%s'" % self.filename)
        self.compare(self.ftk.get_data(), 7)
    def test_load_with_lua(self):
        self.ftk.execute("lua F:load([[%s]])" % self.filename)
        self.compare(self.ftk.get_data(), 7)
    def test_load_with_py(self):
        self.ftk.load(self.filename, 0)
        self.compare(self.ftk.get_data(), 7)

class TestText(TextFileLoadBase):
    def test_load_strict(self):
        self.ftk.execute("@0 < '%s' text strict" % self.filename)
        self.compare(self.ftk.get_data(), 7)
    def test_load_decimal_comma(self):
        # this option only replaces commas with dots, so it's still possible
        # to read file with decimal point '.'
        self.ftk.execute("@0 < '%s' text decimal_comma" % self.filename)
        self.compare(self.ftk.get_data(), 7)

class TestUtf8Filename(TextFileLoadBase):
    def setUp(self):
        self.file_prefix = 'tmp_Zażółć_gęślą_jaźń' # string, not u''
        FileLoadBase.setUp(self)

class TestFilenameWithQuotes(TextFileLoadBase):
    def setUp(self):
        self.file_prefix = """tmp'"!@#"""
        FileLoadBase.setUp(self)
    def test_load(self): pass # would fail

class TestTextComma(FileLoadBase):
    def line_format(self, t):
        return (" %.7f %.7f\n" % t).replace('.', ',')
    def test_load(self):
        self.ftk.execute("@0 < '%s' text decimal_comma" % self.filename)
        self.compare(self.ftk.get_data(), 7)
    def test_load_strict(self):
        self.ftk.execute("@0 < '%s' text decimal_comma strict" % self.filename)
        self.compare(self.ftk.get_data(), 7)
    def test_load_decimal_point(self): # should fail
        self.ftk.execute("@0 < '%s'" % self.filename)
        self.assertNotEqual([i.x for i in self.ftk.get_data()],
                            [round(i[0], 7) for i in self.data])


class Test5ColsFile(FileLoadBase):
    def generate_data(self):
        return [(random.uniform(-100, 100),
                 random.gauss(0, 20),
                 random.gauss(10, 1),
                 random.gauss(20, 1),
                 random.gauss(30, 1))
                for _ in range(30)]
    def line_format(self, t):
        return "%.7f %.7f %.7f %.7f %.7f\n" % t
    def test_load_default(self):
        self.ftk.execute("@0 < '%s'" % self.filename)
        self.compare(self.ftk.get_data(), ndigits=7)
        self.ftk.execute("@0 < '%s::::'" % self.filename)
        self.compare(self.ftk.get_data(), ndigits=7)
        self.ftk.execute("@0 < '%s:1:2::0'" % self.filename)
        self.compare(self.ftk.get_data(), ndigits=7)
    def test_load_with_index(self):
        self.ftk.execute("@0 < '%s:0:2::'" % self.filename)
        out = self.ftk.get_data()
        self.assertEqual([i.x for i in out], range(len(out)))
    def test_load_with_sigma(self):
        self.ftk.execute("@0 < '%s:1:2:3:'" % self.filename)
        out = self.ftk.get_data()
        self.assertEqual([i.sigma for i in out],
                         [round(i[2], 7) for i in self.data])
    def test_load_multi_y(self):
        self.ftk.execute("@+ < '%s:1:2..3,5:4:'" % self.filename)
        for n, y_col in enumerate([2,3,5]):
            out = self.ftk.get_data(n)
            self.assertEqual([i.x for i in out],
                             [round(i[0], 7) for i in self.data])
            self.assertEqual([i.y for i in out],
                             [round(i[y_col-1], 7) for i in self.data])
            self.assertEqual([i.sigma for i in out],
                             [round(i[3], 7) for i in self.data])


class TestSimpleScript(unittest.TestCase):
    def setUp(self):
        self.ftk = fityk.Fityk()
        self.ftk.set_option_as_number("verbosity", -1)
        f = tempfile.NamedTemporaryFile(delete=False)
        f.write("M=1\n X[0]=1.1, Y[0]=-5, S[0]=0.8")
        f.close()
        self.filename = f.name

    def tearDown(self):
        os.unlink(self.filename)

    def test_simple_script(self):
        self.ftk.get_ui_api().exec_fityk_script(self.filename)
        data = self.ftk.get_data()
        self.assertEqual(len(data), 1)
        self.assertEqual(data[0].x, 1.1)
        self.assertEqual(data[0].y, -5)
        self.assertEqual(data[0].sigma, 0.8)

class TestGzippedScript(unittest.TestCase):
    def setUp(self):
        self.ftk = fityk.Fityk()
        self.ftk.set_option_as_number("verbosity", -1)
        f = tempfile.NamedTemporaryFile(suffix='.gz', delete=False)
        self.filename = f.name
        gf = gzip.GzipFile(fileobj=f)
        gf.write("M=1\n X[0]=1.1, Y[0]=-5, S[0]=0.8")
        gf.close()

    def tearDown(self):
        os.unlink(self.filename)

    def test_gzipped_script(self):
        self.ftk.get_ui_api().exec_fityk_script(self.filename)
        data = self.ftk.get_data()
        self.assertEqual(len(data), 1)
        self.assertEqual(data[0].x, 1.1)
        self.assertEqual(data[0].y, -5)
        self.assertEqual(data[0].sigma, 0.8)


if __name__ == '__main__':
    unittest.main()


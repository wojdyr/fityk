#!/usr/bin/env python

import os.path, sys
from fityk import Fityk

class GaussianFitter(Fityk):
    def __init__(self, filename):
        Fityk.__init__(self)
        if not os.path.isfile(filename):
            raise ValueError("File `%s' not found." % filename)
        self.filename = filename
        self.execute("@0 < '%s'" % filename)
        print "Data info:", self.get_info("data", 0)

    def run(self):
        self.execute("guess %g = Gaussian")
        print "Fitting %s ..." % self.filename
        self.execute("fit")
        print "WSSR=", self.get_wssr()
        print "Gaussian center: %.5g" % self.calculate_expr("%g.center")

    def save_session(self, filename):
        self.execute("info state >'%s'" % filename)

f = Fityk()
print f.get_info("version")
print "ln(2) =", f.calculate_expr("ln(2)")
del f

g = GaussianFitter("nacl01.dat")
g.run()
g.save_session("tmp_save.fit")


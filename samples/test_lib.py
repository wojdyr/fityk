#!/usr/bin/env python

import os.path
from fityk import Fityk

class GaussianFitter(Fityk):
    def __init__(self, filename):
        Fityk.__init__(self)
        if not os.path.isfile(filename):
            raise ValueError("File `%s' not found." % filename)
        self.filename = filename
        self.execute("@0 < '%s'" % filename)
        print "Data info:", self.get_info("@0")

    def run(self):
        self.execute("%g = guess Gaussian")
        print "Fitting %s ..." % self.filename
        self.execute("fit")
        print "WSSR=", self.get_wssr()
        print "Gaussian center: %.5g" % self.get_variable_value("%g.center")

f = Fityk()
print f.get_info("version", True)
print "ln(2) =", f.get_info("ln(2)")
del f

g = GaussianFitter("nacl01.dat")
g.run()
g.execute("dump >'%s'" % "tmp_dump.fit")




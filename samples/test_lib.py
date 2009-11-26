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
        print "Data info:", self.get_info("data in @0")

    def run(self):
        self.execute("%g = guess Gaussian")
        print "Fitting %s ..." % self.filename
        self.execute("fit")
        print "WSSR=", self.get_wssr()
        print "Gaussian center: %.5g" % self.get_variable_value("%g.center")

    def save_session(self, filename):
        self.execute("dump >'%s'" % filename)

f = Fityk()
print f.get_info("version", True)
print "ln(2) =", f.get_info("ln(2)")
del f

g = GaussianFitter("nacl01.dat")
g.run()
g.save_session("tmp_dump.fit")

# output from commands can be handled by callback function in Python
def show_msg(s):
    print "output:", s
g.py_set_show_message(show_msg)

# or it can be redirected to file
g.redir_messages(sys.stderr)


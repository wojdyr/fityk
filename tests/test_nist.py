#!/usr/bin/env python

import os
import sys
import re
import urllib2
import unittest
import fityk

DATA_URL_BASE = "http://www.itl.nist.gov/div898/strd/nls/data/LINKS/DATA/"
CACHE_DIR = os.path.join(os.path.dirname(__file__), "strd_data")
VERBOSE = 1 # 0, 1, 2 or 3

class NistParameter:
    def __init__(self, name, start1, start2, param_str, stddev_str):
        self.name = name
        self.start1 = float(start1)
        self.start2 = float(start2)
        self.value_str = param_str
        self.value = float(param_str)
        self.stddev_str = stddev_str
        self.stddev = float(stddev_str)

class NistReferenceData:
    def __init__(self, name):
        self.name = name
        self.data = []
        self.model = None
        self.parameters = []
        self.dof = None
        self.ssr = None
    def __str__(self):
        return "<NistReferenceData: %d points, %d param.>" % (
                len(self.data), len(self.parameters))


def open_nist_data(name):
    name_ext = name + ".dat"
    local_file = os.path.join(CACHE_DIR, name_ext)
    if os.path.exists(local_file):
        text = open(local_file).read()
    else:
        sys.stderr.write("Local data copy not found. Trying itl.nist.gov...\n")
        text = urllib2.urlopen(DATA_URL_BASE + name_ext).read()
        if not os.path.isdir(CACHE_DIR):
            os.mkdir(CACHE_DIR)
        open(local_file, "wb").write(text)
    return text


def read_reference_data(name):
    refdat = NistReferenceData(name)
    text = open_nist_data(name)
    n_data = int(re.search("\nNumber of Observations: +(\d+)", text).group(1))

    refdat.dof = int(re.search("\nDegrees of Freedom: +(\d+)", text).group(1))
    # correct wrong number in one of the files:
    if name == "Rat43":
        refdat.dof = 11

    refdat.ssr = float(re.search("\nResidual Sum of Squares:(.+)\n", text)
                     .group(1))
    data_start = re.search("Data: +y +x\s+", text).end()
    for line in text[data_start:].splitlines():
        x, y = line.split()
        refdat.data.append((float(x), float(y)))
    param_block = re.search(
          #"Start 1 +Start 2 +Parameter +Standard Deviation\s+(.+?\n)\r?\n",
          "Start 1 +Start 2 +Parameter +Standard Deviation\s+((.+?=.+\n)+)",
          text).group(1)
    for line in param_block.splitlines():
        tokens = line.split()
        tokens.remove('=')
        refdat.parameters.append(NistParameter(*tokens))
    assert len(refdat.data) == n_data
    assert n_data - len(refdat.parameters) == refdat.dof, \
           "%d - %d != %d" % (n_data, len(refdat.parameters), refdat.dof)
    model = re.search("\n *y += *(.*?) +\+ +e", text, flags=re.DOTALL).group(1)
    repl = { "[": "(", "]": ")", "**": "^", "arctan": "atan" }
    for key in repl:
        model = model.replace(key, repl[key])
    refdat.model = re.sub("\s{2,}", " ", model)
    return refdat


def has_nlopt():
    ftk = fityk.Fityk()
    return "NLopt" in ftk.get_info("compiler")


def run(data_name, fit_method, easy=True):
    uses_gradient = (fit_method in ("mpfit", "levenberg_marquardt"))
    if uses_gradient:
        tolerance = { "wssr": 1e-10, "param": 4e-7, "err": 5e-3 }
    else:
        tolerance = { "wssr": 1e-7, "param": 1e-4 }
    #if fit_method in ("mpfit", "levenberg_marquardt"):
    if VERBOSE > 0:
        print "Testing %s (start%s) on %-10s" % (fit_method, easy+1, data_name),
    if VERBOSE > 1:
        print
    ref = read_reference_data(data_name)
    if VERBOSE > 2:
        print ref.model
    ftk = fityk.Fityk()
    if VERBOSE < 3:
        ftk.execute("set verbosity=-1")
    y, x = zip(*ref.data)
    ftk.load_data(0, x, y, [1]*len(x), data_name)
    par_names = [p.name for p in ref.parameters]
    par_inits = ["~%g" % (p.start2 if easy else p.start1)
                 for p in ref.parameters]
    ftk.execute("define OurFunc(%s) = %s" % (", ".join(par_names), ref.model))
    ftk.execute("F = OurFunc(%s)" % ", ".join(par_inits))
    ftk.execute("set fitting_method=" + fit_method)
    ftk.execute("set pseudo_random_seed=1234567")
    #ftk.execute("set numeric_format='%.10E'")
    ftk.execute("set lm_stop_rel_change=1e-16")
    #ftk.execute("set lm_max_lambda=1e+50")
    ftk.execute("set nm_convergence=1e-10")
    if fit_method == "mpfit":
        ftk.execute("set ftol_rel=1e-18")
        ftk.execute("set xtol_rel=1e-18")
    if fit_method == "genetic_algorithms":
        ftk.execute("set max_wssr_evaluations=5e5")
    elif not uses_gradient:
        ftk.execute("set max_wssr_evaluations=2e4")
    try:
        ftk.execute("fit")
        #ftk.execute("set fitting_method=levenberg_marquardt")
        #ftk.execute("fit")
    except fityk.ExecuteError as e:
        print "fityk.ExecuteError: %s" % e
        return False
    ssr = ftk.get_ssr()
    ssr_diff = (ssr - ref.ssr) / ref.ssr

    # Lanczos1 and Lanczos2 have near-zero SSR, we need to be more tolerant
    if ssr < 1e-20:
        tolerance["wssr"] *= 1e8
    elif ssr < 1e-10:
        tolerance["wssr"] *= 1e2

    ok = (abs(ssr_diff) < tolerance["wssr"])
    if ref.ssr > 1e-10 and ssr_diff < -1e-10:
        print "Eureka! %.10E < %.10E" % (ssr, ref.ssr)
    fmt =  " %8s  %13E %13E  %+.1E"
    if VERBOSE > 2 or (VERBOSE == 2 and not ok):
        print fmt % ("SSR", ssr, ref.ssr, ssr_diff)
    our_func = ftk.all_functions()[0]
    for par in ref.parameters:
        v = ftk.get_var(our_func, par.name)
        calc_value = v.get_value()
        val_diff = (calc_value - par.value) / par.value
        param_ok = (abs(val_diff) < tolerance["param"])
        err_ok = True
        if uses_gradient:
            calc_err = ftk.calculate_expr("$" + v.name + ".error")
            err_diff = (calc_err - par.stddev) / par.stddev
            err_ok = (abs(err_diff) < tolerance["err"])
        if VERBOSE > 2 or (VERBOSE == 2 and (not param_ok or not err_ok)):
            print fmt % (par.name, calc_value, par.value, val_diff)
            if uses_gradient:
                print fmt % ("+/-", calc_err, par.stddev, err_diff)
        ok = (ok and param_ok and err_ok)
    if VERBOSE == 1:
        print("OK" if ok else "FAILED")
    return ok

datasets = [
  # lower difficulty
  "Misra1a", "Chwirut2", "Chwirut1", "Lanczos3",
  "Gauss1", "Gauss2", "DanWood", "Misra1b",
  # average difficulty (skipping Nelson which is y(x1,x2))
  "Kirby2", "Hahn1", "MGH17",
  "Lanczos1", "Lanczos2", "Gauss3",
  "Misra1c", "Misra1d", "Roszman1", "ENSO",
  # higher difficulty
  "MGH09", "Thurber", "BoxBOD",
  "Rat42", "MGH10", "Eckerle4",
  "Rat43", "Bennett5"
]

# L-M finds local minimum when starting from start1 for:
lm_fails    = ["MGH17", "BoxBOD", "MGH10", "Eckerle4"]
mpfit_fails = ["MGH17", "BoxBOD", "MGH10", "MGH09", "Bennett5"]  #, "ENSO"
nm_fails =    ["MGH17", "BoxBOD", "ENSO", "Eckerle4", "MGH09", "Bennett5"]
nl_nm_fails = ["MGH17", "BoxBOD", "MGH10", "ENSO"]


def try_method(method):
    all_count = 0
    ok_count = 0
    for data_name in datasets:
        for is_easy in (True, False):
            ok = run(data_name, method, is_easy)
            all_count += 1
            if ok:
                ok_count += 1
            sys.stdout.flush()
    print "OK: %2d / %2d" % (ok_count, all_count)


if __name__ == '__main__':
    try_method("mpfit")
    #try_method("nlopt_bobyqa")
# "nlopt_bobyqa", "nlopt_nm", "nlopt_sbplx",
# "nlopt_cobyla", # doesn't work?
# "nlopt_lbfgs", "nlopt_mma", "nlopt_slsqp", "nlopt_var2",
# "nlopt_crs2", "nlopt_praxis"


class TestSequenceFunctions(unittest.TestCase):
    def setUp(self):
        global VERBOSE
        VERBOSE = 0

    def test_levmar(self):
        for data_name in datasets:
            self.assertTrue(run(data_name, "mpfit", easy=True))
            self.assertTrue(run(data_name, "levenberg_marquardt", True))
            self.assertIs(run(data_name, "mpfit", easy=False),
                          data_name not in mpfit_fails)
            self.assertIs(run(data_name, "levenberg_marquardt", False),
                          data_name not in lm_fails)

    def test_nelder_mead(self):
        for data_name in datasets:
            ## Lanczos* converge slowly with gradient-less methods, avoid
            if "Lanczos" in data_name:
                continue
            self.assertTrue(run(data_name, "nelder_mead_simplex", easy=True))
            self.assertTrue(run(data_name, "nlopt_nm", easy=True))
            self.assertIs(run(data_name, "nelder_mead_simplex", easy=False),
                          data_name not in nm_fails)
            self.assertIs(run(data_name, "nlopt_nm", easy=False),
                          data_name not in nl_nm_fails)

    def test_ga(self):
        # Since in these problems parameter domains are not defined,
        # we only have starting point, methods that have random-search element
        # don't work well.
        # Like Genetic Algorithms. Additionally, GA are not very practical for
        # most of tasks -- slow and require adjusting plenty of parameters.
        # But still it may solve some cases where other methods fail:
        self.assertTrue(run("BoxBOD", "genetic_algorithms", easy=False))

    @unittest.skipIf(not has_nlopt(), "Fityk compiled w/o NLopt support.")
    def test_nlopt(self):
        # Selected methods from NLopt library.
        # For this test suite (which may not be representative for real
        # problems) the best NLopt method is Nelder-Mead (tested above).
        # The methods below can be useful as well.
        # (I haven't tried all algorithms, but almost all).
        self.assertTrue(run("MGH17", "nlopt_lbfgs", easy=False))
        for data_name in ["BoxBOD", "Eckerle4", "ENSO"]:
            self.assertTrue(run(data_name, "nlopt_var2", easy=False))
        for data_name in ["BoxBOD", "Eckerle4", "ENSO"]:
            self.assertTrue(run(data_name, "nlopt_praxis", easy=False))
        for data_name in ["Thurber", "BoxBOD", "Rat43"]:
            self.assertTrue(run(data_name, "nlopt_bobyqa", easy=False))
        for data_name in ["Rat42", "Eckerle4"]:
            self.assertTrue(run(data_name, "nlopt_sbplx", easy=False))


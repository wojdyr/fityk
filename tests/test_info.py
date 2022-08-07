#!/usr/bin/env python3

# run tests with: python -m unittest test_info
#             or  python -m unittest discover

import unittest
import fityk

class TestFormula(unittest.TestCase):
    def setUp(self):
        self.ftk = fityk.Fityk()
        self.ftk.set_option_as_number("verbosity", -1)
        self.voigt = "Voigt(926, 43.2, 0.144, 0.1)"
        self.splitvoigt = "SplitVoigt(926, 43.2, 0.144, 0.143, 0.1, 0.13)"
        self.splitvoigt_formula = ('x < 43.2 ? Voigt(926, 43.2, 0.144, 0.1) '
                                            ': Voigt(926, 43.2, 0.143, 0.13)')
    def test_voigt(self):
        self.ftk.execute("F = " + self.voigt)
        formula = self.ftk.get_info("formula")
        self.assertEqual(formula, self.voigt)
        gaussian_fwhm=self.ftk.calculate_expr
        f_voigt = self.ftk.get_components(0)[0]
        gauss_fwhm = f_voigt.get_param_value('GaussianFWHM')
        self.assertEqual(round(gauss_fwhm, 13), 0.2397757280134) # not verified
    def test_voigt_s(self):
        self.ftk.execute("F = " + self.voigt)
        formula = self.ftk.get_info("simplified_formula")
        self.assertEqual(formula, self.voigt)
    def test_splitvoigt(self):
        self.ftk.execute("F = " + self.splitvoigt)
        formula = self.ftk.get_info("formula")
        self.assertEqual(formula, self.splitvoigt_formula)
    def test_splitvoigt_s(self):
        self.ftk.execute("F = " + self.splitvoigt)
        formula = self.ftk.get_info("simplified_formula")
        self.assertEqual(formula, self.splitvoigt_formula)

if __name__ == '__main__':
    unittest.main()


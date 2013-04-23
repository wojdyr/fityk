
#include "fityk/fityk.h"

// When the deriv_debug option is used in MPFIT
// both user-calculated and MPFIT-calculated numerical derivatives
// are printed to stdout. This test is half-manual.

namespace fityk { extern bool debug_deriv_in_mpfit; }

int main()
{
    fityk::debug_deriv_in_mpfit=true;
    fityk::Fityk* ftk = new fityk::Fityk;
    ftk->execute("set verbosity=-1");
    ftk->execute("set pseudo_random_seed=1234567");
    ftk->execute("M=40; x=n/5");
    ftk->execute("Y = 4.5 + 0.5*x + 37*exp(-ln(2)*((x-3.1)/0.7)^2)");
    ftk->execute("guess Linear; guess Gaussian");
    ftk->execute("set fitting_method=mpfit");
    ftk->execute("fit");
    delete ftk;
    return 0;
}

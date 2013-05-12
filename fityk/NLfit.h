// This file is part of fityk program. Copyright Marcin Wojdyr.
// Licence: GNU General Public License ver. 2+

/// wrapper around NLopt library, http://ab-initio.mit.edu/nlopt/
/// which is a library for nonlinear optimization with a common interface
/// for a number of different algorithms.

#ifndef FITYK_NLFIT_H_
#define FITYK_NLFIT_H_

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#if HAVE_LIBNLOPT

#include <nlopt.h>
#include "fit.h"

namespace fityk {

class NLfit : public Fit
{
public:
    NLfit(Full* F, const char* name, nlopt_algorithm algorithm);
    ~NLfit();
    virtual double run_method(std::vector<realt>* best_a);

    // implementation (must be public to be called inside callback function)
    double calculate(int n, const double* par, double* grad);
private:
    nlopt_algorithm algorithm_;
    nlopt_opt opt_;
};

} // namespace fityk
#endif //HAVE_LIBNLOPT

#endif // FITYK_NLFIT_H_


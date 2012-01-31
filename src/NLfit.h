// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

/// wrapper around NLopt library, http://ab-initio.mit.edu/nlopt/
/// which is a library for nonlinear optimization with a common interface
/// for a number of different algorithms.

#ifndef FITYK_NLFIT_H_
#define FITYK_NLFIT_H_

#include <nlopt.h>
#include "fit.h"

class NLfit : public Fit
{
public:
    NLfit(Ftk* F);
    ~NLfit();
    virtual void init(); // called before do_iteration()/autoiter()
    void autoiter();

    // implementation (must be public to be called inside callback function)
    double calculate(int n, const double* par, double* grad);
private:
    nlopt_opt opt_;
    int start_iter_;
};

#endif


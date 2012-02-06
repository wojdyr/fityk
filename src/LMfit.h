// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

/// Simple implementation of the Levenberg-Marquardt method,
/// uses Jordan elimination with partial pivoting.

#ifndef FITYK_LMFIT_H_
#define FITYK_LMFIT_H_
#include <vector>
#include "common.h"
#include "fit.h"

class LMfit : public Fit
{
public:
    LMfit(Ftk* F, const char* name) : Fit(F, name) {}
    virtual void init(); // called before do_iteration()/autoiter()
    void autoiter();
private:
    std::vector<realt> alpha, alpha_;            // matrices
    std::vector<realt> beta, beta_;   // and vectors
    std::vector<realt> a;    // parameters table
    realt chi2, chi2_;
    double lambda;

    bool do_iteration();
};

#endif

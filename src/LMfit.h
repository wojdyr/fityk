// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License version 2
// $Id$

#ifndef FITYK__LMFIT__H__
#define FITYK__LMFIT__H__
#include "common.h"
#include <vector>
#include <map>
#include <string>
#include "fit.h"

///           Levenberg-Marquardt method
class LMfit : public Fit
{
public:
    LMfit(Fityk* F);
    ~LMfit();
    fp init(); // called before do_iteration()/autoiter()
    void autoiter();
private:
    fp shake_before;
    char shake_type;
    std::vector<fp> alpha, alpha_;            // matrices
    std::vector<fp> beta, beta_;   // and vectors
    std::vector<fp> a;    // parameters table
    fp chi2 , chi2_;
    fp lambda;

    bool do_iteration();
};

#endif


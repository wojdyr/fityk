// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#ifndef FITYK__LMFIT__H__
#define FITYK__LMFIT__H__
#include "common.h"
#include <vector>
#include <map>
#include <string>
#include "fit.h"

/*     this class contains Levenberg-Marquardt method
 */


class LMfit : public Fit
{
public:
    LMfit ();
    ~LMfit ();
    fp init(); // called before do_iteration()/autoiter()
    int autoiter ();
private:
    fp lambda_starting_value;
    fp lambda_up_factor;
    fp lambda_down_factor;
    fp stop_rel;
    fp shake_before;
    char shake_type;
    std::vector<fp> alpha, alpha_;            // matrices
    std::vector<fp> beta, beta_;   // and vectors
    std::vector<fp> a;    // parameters table
    fp chi2 , chi2_;
    fp lambda;

    int do_iteration();
};

#endif


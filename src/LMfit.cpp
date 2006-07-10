// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#include "common.h"
#include "LMfit.h"
#include "ui.h"
#include "settings.h"
#include <math.h>
#include <vector>
#include <algorithm>

using namespace std;

LMfit::LMfit() 
    : Fit("Levenberg-Marquardt"),
      shake_before (0), shake_type ('u'),
      alpha(0), alpha_(0), beta(0), beta_(0)
{
    /*
    fpar ["shake-before"] = &shake_before;
    epar.insert(pair<string, Enum_string>("shake-type", 
                               Enum_string (Distrib_enum, &shake_type)));
    */
}    
    
LMfit::~LMfit() {}

// WSSR is also called chi2
fp LMfit::init()   
{
    alpha.resize (na*na);
    alpha_.resize (na*na);
    beta.resize (na);
    beta_.resize (na);
    if (na < 1 ) {
        warn ("No data points. What should I fit ?");
        return -1;
    }
    lambda = getSettings()->get_f("lm-lambda-start");
    //TODO what to do with this shake?
    if (shake_before > 0.) {
        for (int i = 0; i < na; i++) 
            a[i] = draw_a_from_distribution (i, shake_type, shake_before);
    }
    else
        a = a_orig; 

    info (print_matrix (a, 1, na, "Initial A"));
    //no need to optimise it (and compute chi2 and derivatives together)
    chi2 = compute_wssr(a, datsums);
    compute_derivatives(a, datsums, alpha, beta);
    return chi2;
}

void LMfit::autoiter() 
{
    wssr_before = (shake_before > 0. ? compute_wssr(a_orig, datsums) : chi2);
    fp prev_chi2 = chi2;
    verbose("\t === Levenberg-Marquardt method ===");
    info ("Initial values:  lambda=" + S(lambda) + "  WSSR=" + S(chi2));
    verbose ("Max. number of iterations: " + max_iterations);
    fp stop_rel = getSettings()->get_f("lm-stop-rel-change");
    fp max_lambda = getSettings()->get_f("lm-max-lambda");
    if (stop_rel > 0) {
        verbose ("Stopping when relative change of WSSR is "
                  "twice in row below " + S (stop_rel * 100.) + "%");
    }
    int small_change_counter = 0;
    for (int iter = 0; !common_termination_criteria(iter); iter++) {
        bool better_fit = do_iteration();
        if (better_fit) { 
            fp d = prev_chi2 - chi2;
            info ("#" + S(iter_nr) + ":  WSSR=" + S(chi2) 
                        + "  lambda=" + S(lambda) + "  d(WSSR)=" +  S(-d) 
                        + "  (" + S (d / prev_chi2 * 100) + "%)");  
            // another termination criterium: neglectable change of chi2
            if (d / prev_chi2 < stop_rel || chi2 == 0) { 
                small_change_counter++;
                if (small_change_counter >= 2 || chi2 == 0) {
                    info("Fit converged.");
                    break;
                }
            }
            else
                small_change_counter = 0;
            prev_chi2 = chi2;
            iteration_plot(a);
        }
        else { // no better fit
            info("#"+S(iter_nr)+": (WSSR="+S(chi2_)+")  lambda="+S(lambda));
            if (lambda > max_lambda) { // another termination criterium
                info("In L-M method: lambda=" + S(lambda) + " > " 
                        + S(max_lambda) + ", stopped.");
                break;
            }
        }
    }
    post_fit (a, chi2);
}

bool LMfit::do_iteration()
    //pre: init() callled
{
    if (na < 1) 
        throw ExecuteError("No parameters to fit.");
    iter_nr++;
    alpha_ = alpha;
    for (int j = 0; j < na; j++) 
        alpha_[na * j + j] *= (1.0 + lambda);
    beta_ = beta;
#ifdef debug
    info (print_matrix (beta_, 1, na, "beta"));
    info (print_matrix (alpha_, na, na, "alpha'"));
#endif /*debug*/

    // Matrix solution (Ax=b)  alpha_ * da == beta_
    if (!Jordan (alpha_, beta_, na))
        throw ExecuteError("Error when processing iteration " + S(iter_nr));

    // da is in beta_  
    if (getUI()->getVerbosity() >= 4) {
        vector<fp> rel (na);
        for (int q = 0; q < na; q++)
            rel[q] = beta_[q] / a[q] * 100;
        verbose (print_matrix (rel, 1, na, "delta(A)/A[%]"));
    }
    for (int i = 0; i < na; i++) 
        beta_[i] = a[i] + beta_[i];   // and now there is new a[] in beta_[] 
    verbose_lazy (print_matrix (beta_, 1, na, "Trying A"));
    //  compute chi2_
    chi2_ = compute_wssr(beta_, datsums);

    if (chi2_ < chi2) { // better fitting
        chi2 = chi2_; 
        a = beta_;
        compute_derivatives(a, datsums, alpha, beta);
        lambda /= getSettings()->get_f("lm-lambda-down-factor");
        return true;
    }
    else {// worse fitting
        lambda *= getSettings()->get_f("lm-lambda-up-factor");
        return false;
    }
}    


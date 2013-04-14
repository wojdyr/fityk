// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#define BUILDING_LIBFITYK
#include "LMfit.h"

#include <math.h>
#include <vector>
#include <algorithm>

#include "common.h"
#include "ui.h"
#include "settings.h"
#include "logic.h"
#include "numfuncs.h"

using namespace std;

namespace fityk {

// note: WSSR is also called chi2

double LMfit::run_method(std::vector<realt>* best_a)
{
    const realt stop_rel = F_->get_settings()->lm_stop_rel_change;
    const realt max_lambda = F_->get_settings()->lm_max_lambda;

    double lambda = F_->get_settings()->lm_lambda_start;
    alpha_.resize(na_*na_);
    beta_.resize(na_);
    *best_a = a_orig_;

    if (F_->get_verbosity() >= 2) {
        F_->ui()->mesg(format_matrix(a_orig_, 1, na_, "Initial A"));
        F_->ui()->mesg("Starting with lambda=" + S(lambda));
        if (stop_rel > 0)
            F_->ui()->mesg("Will stop when relative change of WSSR is "
                           "twice in row below " + S(stop_rel * 100.) + "%");
    }

    realt chi2 = initial_wssr_;
    compute_derivatives(a_orig_, dmdm_, alpha_, beta_);

    int small_change_counter = 0;
    for (int iter = 0; !common_termination_criteria(); iter++) {
        prepare_next_parameters(lambda, *best_a); // -> temp_beta_
        double new_chi2 = compute_wssr(temp_beta_, dmdm_);
        if (F_->get_verbosity() >= 1)
            F_->ui()->mesg(iteration_info(new_chi2) +
                           format1<double,32>("  lambda=%.5g", lambda) +
                           format1<double,32>("  iter #%d", iter));
        if (new_chi2 < chi2) {
            realt rel_change = (chi2 - new_chi2) / chi2;
            chi2 = new_chi2;
            *best_a = temp_beta_;

            // termination criterium: negligible change of chi2
            if (rel_change < stop_rel || chi2 == 0) {
                small_change_counter++;
                if (small_change_counter >= 2 || chi2 == 0) {
                    F_->msg("... converged.");
                    break;
                }
            }
            else
                small_change_counter = 0;

            compute_derivatives(*best_a, dmdm_, alpha_, beta_);
            lambda /= F_->get_settings()->lm_lambda_down_factor;
        }

        else { // worse fitting
            // termination criterium: large lambda
            if (lambda > max_lambda) {
                F_->msg("In L-M method: lambda=" + S(lambda) + " > "
                        + S(max_lambda) + ", stopped.");
                break;
            }
            lambda *= F_->get_settings()->lm_lambda_up_factor;
        }

        iteration_plot(*best_a, chi2);
    }
    return chi2;
}


// puts result into temp_beta_
void LMfit::prepare_next_parameters(double lambda, const vector<realt> &a)
{
    temp_alpha_ = alpha_;
    for (int j = 0; j < na_; j++)
        temp_alpha_[na_ * j + j] *= (1.0 + lambda);
    temp_beta_ = beta_;

    if (F_->get_verbosity() > 2) { // level: debug
        F_->ui()->mesg(format_matrix(temp_beta_, 1, na_, "beta"));
        F_->ui()->mesg(format_matrix(temp_alpha_, na_, na_, "alpha'"));
    }

    // Matrix solution (Ax=b)  temp_alpha_ * da == temp_beta_
    jordan_solve(temp_alpha_, temp_beta_, na_);

    for (int i = 0; i < na_; i++)
        // put new a[] into temp_beta_[]
        temp_beta_[i] = a[i] + temp_beta_[i];

    if (F_->get_verbosity() >= 2)
        output_tried_parameters(temp_beta_);
}

} // namespace fityk

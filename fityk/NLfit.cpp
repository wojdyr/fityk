// This file is part of fityk program. Copyright Marcin Wojdyr.
// Licence: GNU General Public License ver. 2+

#include "NLfit.h"
#include "logic.h"
#include "data.h"
#include "settings.h"

using namespace std;

namespace fityk {

// int major, minor, bugfix;
// nlopt_version(&major, &minor, &bugfix);

NLfit::NLfit(Ftk* F, const char* name, nlopt_algorithm algorithm)
    : Fit(F, name), algorithm_(algorithm), opt_(NULL)
{
}

NLfit::~NLfit()
{
    if (opt_ != NULL)
        nlopt_destroy(opt_);
}

double calculate_for_nlopt(unsigned n, const double* x,
                           double* grad, void* f_data)
{
    return static_cast<NLfit*>(f_data)->calculate(n, x, grad);
}


void NLfit::init()
{
}


double NLfit::calculate(int n, const double* par, double* grad)
{
    assert(n == na_);
    vector<realt> A(par, par+na_);
    if (F_->get_verbosity() >= 1)
        output_tried_parameters(A);
    bool stop = common_termination_criteria(iter_nr_-start_iter_);
    if (stop)
        nlopt_force_stop(opt_);

    double wssr;
    if (!grad || stop)
        wssr = compute_wssr(A, dmdm_);
    else
        wssr = compute_derivatives_nl(A, dmdm_, grad);
    ++iter_nr_;
    if (F_->get_verbosity() >= 1) {
        realt rel_diff = (wssr - wssr_before_) / wssr_before_;
        F_->ui()->mesg("... #" + S(iter_nr_) + ":  WSSR=" + S(wssr) +
                       format1<double, 32>("  (%+g%%)", rel_diff * 100));
    }
    return wssr;
}

static
const char* nlresult_to_string(nlopt_result r)
{
    switch (r) {
        case NLOPT_FAILURE: return "failure";
        case NLOPT_INVALID_ARGS: return "invalid arguments";
        case NLOPT_OUT_OF_MEMORY: return "out of memory";
        case NLOPT_ROUNDOFF_LIMITED: return "roundoff errors limit progress";
        case NLOPT_FORCED_STOP: return "interrupted";
        case NLOPT_SUCCESS: return "success";
        case NLOPT_STOPVAL_REACHED: return "stop-value reached";
        case NLOPT_FTOL_REACHED: return "ftol-value reached";
        case NLOPT_XTOL_REACHED: return "xtol-value reached";
        case NLOPT_MAXEVAL_REACHED: return "max. evaluation number reached";
        case NLOPT_MAXTIME_REACHED: return "max. time reached";
    }
    return NULL;
}

void NLfit::autoiter()
{
    if (opt_ != NULL && na_ != (int) nlopt_get_dimension(opt_)) {
        nlopt_destroy(opt_);
        opt_ = NULL;
    }

    if (opt_ == NULL) {
        opt_ = nlopt_create(algorithm_, na_);
        nlopt_set_min_objective(opt_, calculate_for_nlopt, this);
    }

    start_iter_ = iter_nr_;
    wssr_before_ = compute_wssr(a_orig_, dmdm_);

    // this is also handled in Fit::common_termination_criteria()
    nlopt_set_maxtime(opt_, F_->get_settings()->max_fitting_time);
    nlopt_set_maxeval(opt_, F_->get_settings()->max_wssr_evaluations);

    double opt_f;
    double *a = new double[na_];
    for (int i = 0; i < na_; ++i)
        a[i] = a_orig_[i];
    nlopt_result r = nlopt_optimize(opt_, a, &opt_f);
    F_->msg("NLopt says: " + S(nlresult_to_string(r)));
    post_fit(vector<realt>(a, a+na_), opt_f);
    delete [] a;
}

} // namespace fityk

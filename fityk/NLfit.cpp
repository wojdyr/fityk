// This file is part of fityk program. Copyright Marcin Wojdyr.
// Licence: GNU General Public License ver. 2+

#define BUILDING_LIBFITYK
#include "NLfit.h"
#include "logic.h"
#include "data.h"
#include "settings.h"
#include "var.h"

#if HAVE_LIBNLOPT

using namespace std;

namespace fityk {

NLfit::NLfit(Full* F, const char* name, nlopt_algorithm algorithm)
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


double NLfit::calculate(int n, const double* par, double* grad)
{
    assert(n == na_);
    vector<realt> A(par, par+n);
    if (F_->get_verbosity() >= 1)
        output_tried_parameters(A);
    bool stop = common_termination_criteria();
    if (stop)
        nlopt_force_stop(opt_);

    double wssr;
    if (!grad || stop)
        wssr = compute_wssr(A, fitted_datas_);
    else
        wssr = compute_wssr_gradient(A, fitted_datas_, grad);
    if (F_->get_verbosity() >= 1)
        F_->ui()->mesg(iteration_info(wssr));
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

double NLfit::run_method(vector<realt>* best_a)
{
    if (opt_ != NULL && na_ != (int) nlopt_get_dimension(opt_)) {
        nlopt_destroy(opt_);
        opt_ = NULL;
    }

    if (opt_ == NULL) {
        opt_ = nlopt_create(algorithm_, na_);
        nlopt_set_min_objective(opt_, calculate_for_nlopt, this);
    }

    // this is also handled in Fit::common_termination_criteria()
    nlopt_set_maxtime(opt_, F_->get_settings()->max_fitting_time);
    nlopt_set_maxeval(opt_, max_eval() - 1); // save 1 eval for final calc.
    nlopt_set_ftol_rel(opt_, F_->get_settings()->ftol_rel);
    nlopt_set_xtol_rel(opt_, F_->get_settings()->xtol_rel);

    double *lb = new double[na_];
    double *ub = new double[na_];
    for (int i = 0; i < na_; ++i) {
        const RealRange& d = F_->mgr.get_variable(i)->domain;
        lb[i] = d.lo;
        ub[i] = d.hi;
    }
    nlopt_set_lower_bounds(opt_, lb);
    nlopt_set_upper_bounds(opt_, ub);
    delete [] lb;
    delete [] ub;

    double opt_f;
    double *a = new double[na_];
    for (int i = 0; i < na_; ++i)
        a[i] = a_orig_[i];
    nlopt_result r = nlopt_optimize(opt_, a, &opt_f);
    F_->msg("NLopt says: " + S(nlresult_to_string(r)));
    best_a->assign(a, a+na_);
    delete [] a;
    return opt_f;
}

} // namespace fityk

#endif //HAVE_LIBNLOPT

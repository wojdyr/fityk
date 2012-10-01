// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#include "MPfit.h"
#include "logic.h"
#include "data.h"

using namespace std;

namespace fityk {

void MPfit::init()
{
    // 0 value means default
    mp_conf_.ftol = 0.;
    mp_conf_.xtol = 0.;
    mp_conf_.gtol = 0.;
    mp_conf_.epsfcn = 0.;
    mp_conf_.stepfactor = 0.;
    mp_conf_.covtol = 0.;
    mp_conf_.maxiter = 0;
    mp_conf_.maxfev = 0;
    mp_conf_.nprint = 0;
    mp_conf_.douserscale = 0;
    mp_conf_.nofinitecheck = 0;
    mp_conf_.iterproc = NULL;
}

int calculate_for_mpfit(int m, int npar, double *par, double *deviates,
                        double **derivs, void *mpfit)
{
    return static_cast<MPfit*>(mpfit)->
        calculate(m, npar, par, deviates, derivs);
}

/*
void on_mpfit_iteration(void *mpfit)
{
    static_cast<MPfit*>(mpfit)->on_iteration();
}
*/

int MPfit::on_iteration()
{
    // max. iterations/evaluations number is handled in proper place by mpfit
    return (int) common_termination_criteria(iter_nr_-start_iter_, false);
}

int MPfit::calculate(int /*m*/, int npar, double *par, double *deviates,
                     double **derivs)
{
    int stop = on_iteration();
    if (stop)
        return -1; // error code reserved for user function

    vector<realt> A(par, par+npar);
    if (F_->get_verbosity() >= 1)
        output_tried_parameters(A);
    //printf("wssr=%g, p0=%g, p1=%g\n", compute_wssr(A, dmdm_), par[0], par[1]);
    if (!derivs)
        compute_deviates(A, deviates);
    else {
        ++iter_nr_;
        compute_derivatives_mp(A, dmdm_, derivs, deviates);
    }
    return 0;
}

static
const char* mpstatus_to_string(int n)
{
    switch (n) {
        case MP_ERR_INPUT: return "General input parameter error";
        case MP_ERR_NAN: return "User function produced non-finite values";
        case MP_ERR_FUNC: return "No user function was supplied";
        case MP_ERR_NPOINTS: return "No user data points were supplied";
        case MP_ERR_NFREE: return "No free parameters";
        case MP_ERR_MEMORY: return "Memory allocation error";
        case MP_ERR_INITBOUNDS:
                            return "Initial values inconsistent w constraints";
        case MP_ERR_BOUNDS: return "Initial constraints inconsistent";
        case MP_ERR_PARAM: return "General input parameter error";
        case MP_ERR_DOF: return "Not enough degrees of freedom";

        // Potential success status codes
        case MP_OK_CHI: return "Convergence in chi-square value";
        case MP_OK_PAR: return "Convergence in parameter value";
        case MP_OK_BOTH: return "Convergence in chi2 and parameter value";
        case MP_OK_DIR: return "Convergence in orthogonality";
        case MP_MAXITER: return "Maximum number of iterations reached";
        case MP_FTOL: return "ftol is too small; no further improvement";
        case MP_XTOL: return "xtol is too small; no further improvement";
        case MP_GTOL: return "gtol is too small; no further improvement";

        // user-defined codes
        case -1: return "One of user-defined criteria stopped fitting.";
        default: return "unexpected status code";
    }
}

void MPfit::autoiter()
{
    start_iter_ = iter_nr_;
    wssr_before_ = compute_wssr(a_orig_, dmdm_);

    int m = 0;
    v_foreach (DataAndModel*, i, dmdm_)
        m += (*i)->data()->get_n();

    mp_conf_.maxiter = max_iterations_;
    mp_conf_.maxfev = F_->get_settings()->max_wssr_evaluations;

    //mp_conf_.ftol = F_->get_settings()->lm_stop_rel_change;

    double *perror = new double[na_];
    double *a = new double[na_];
    mp_par *pars = new mp_par[na_];
    for (int i = 0; i < na_; ++i) {
        a[i] = a_orig_[i];
        mp_par& p = pars[i];
        p.fixed = 0;
        p.limited[0] = 0; // no lower limit
        p.limited[1] = 0; // no upper limit
        p.limits[0] = 0.;
        p.limits[1] = 0.;
        p.parname = NULL;
        p.step = 0.;      // step size for finite difference
        p.relstep = 0.;   // relative step size for finite difference
        p.side = 3;       // Sidedness of finite difference derivative 
                          //   0 - one-sided derivative computed automatically
                          //   1 - one-sided derivative (f(x+h) - f(x)  )/h
                          //   -1 - one-sided derivative (f(x)   - f(x-h))/h
                          //   2 - two-sided derivative (f(x+h) - f(x-h))/(2*h) 
                          //   3 - user-computed analytical derivatives
        p.deriv_debug = 0;
        p.deriv_reltol = 0.;
        p.deriv_abstol = 0.;
    }

    // zero result_
    result_.bestnorm = result_.orignorm = 0.;
    result_.niter = result_.nfev = result_.status = 0;
    result_.npar = result_.nfree = result_.npegged = result_.nfunc = 0;
    result_.resid = result_.covar = NULL;
    result_.xerror = perror;

    int status = mpfit(calculate_for_mpfit, m, na_, a, pars, &mp_conf_, this,
                       &result_);
    //printf("%d :: %d\n", iter_nr_, result_.niter);
    //soft_assert(iter_nr_ + 1 == result_.niter);
    //soft_assert(result_.nfev + 1 == evaluations_);
    soft_assert(status == result_.status);
    F_->msg("mpfit status: " + S(mpstatus_to_string(status)));
    post_fit(vector<realt>(a, a+na_), result_.bestnorm);
    delete [] pars;
    delete [] a;
    delete [] perror;
}

} // namespace fityk

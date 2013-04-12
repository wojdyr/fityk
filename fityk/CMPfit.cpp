// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#define BUILDING_LIBFITYK
#include "CMPfit.h"
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

static
mp_par* allocate_and_init_mp_par(const std::vector<bool>& par_usage)
{
    mp_par *pars = new mp_par[par_usage.size()];
    for (size_t i = 0; i < par_usage.size(); ++i) {
        mp_par& p = pars[i];
        p.fixed = !par_usage[i];
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
    return pars;
}

static
void zero_init_result(mp_result *result)
{
    result->bestnorm = result->orignorm = 0.;
    result->niter = result->nfev = result->status = 0;
    result->npar = result->nfree = result->npegged = result->nfunc = 0;
    result->resid = result->covar = NULL;
    result->xerror = NULL;
}

// final_a either has the same size as parameters or is NULL
int MPfit::run_mpfit(const vector<DataAndModel*>& dms,
                     const vector<realt>& parameters,
                     const vector<bool>& par_usage,
                     double *final_a)
{
    assert(par_usage.size() == parameters.size());

    double *a = final_a ? final_a : new double[parameters.size()];
    for (size_t i = 0; i != parameters.size(); ++i)
        a[i] = parameters[i];

    mp_par *pars = allocate_and_init_mp_par(par_usage_);

    // dms cannot be easily passed to the calculate_for_mpfit() callback
    // in a different way than through member variable (dmdm_).
    int status;
    if (&dms != &dmdm_) {
        vector<DataAndModel*> saved = dms;
        dmdm_.swap(saved);
        status = mpfit(calculate_for_mpfit, count_points(dms),
                       parameters.size(), a, pars, &mp_conf_, this, &result_);
        dmdm_.swap(saved);
    }
    else
        status = mpfit(calculate_for_mpfit, count_points(dms),
                       parameters.size(), a, pars, &mp_conf_, this, &result_);
    soft_assert(status == result_.status);
    delete [] pars;
    if (final_a == NULL)
        delete [] a;
    return status;
}

void MPfit::autoiter()
{
    start_iter_ = iter_nr_;
    wssr_before_ = compute_wssr(a_orig_, dmdm_);

    mp_conf_.maxiter = max_iterations_;
    mp_conf_.maxfev = F_->get_settings()->max_wssr_evaluations;
    //mp_conf_.ftol = F_->get_settings()->lm_stop_rel_change;

    zero_init_result(&result_);

    double *a = new double[na_];
    int status = run_mpfit(dmdm_, a_orig_, par_usage_, a);
    //printf("%d :: %d\n", iter_nr_, result_.niter);
    //soft_assert(iter_nr_ + 1 == result_.niter);
    //soft_assert(result_.nfev + 1 == evaluations_);
    F_->msg("mpfit status: " + S(mpstatus_to_string(status)));
    post_fit(vector<realt>(a, a+na_), result_.bestnorm);
    delete [] a;
}

// pre: update_parameters()
// returns array of size na_ that needs to be delete[]'d
double* MPfit::get_errors(const vector<DataAndModel*>& dms)
{
    double *perror = new double[na_];
    for (int i = 0; i < na_; ++i)
        perror[i] = 0.;

    mp_conf_.maxiter = 0;

    zero_init_result(&result_);
    result_.xerror = perror;

    int status = run_mpfit(dms, F_->mgr.parameters(), par_usage_);
    soft_assert(status == MP_MAXITER);

    return perror;
}

// pre: update_parameters()
// returns array of size na_*na_ that needs to be delete[]'d
double* MPfit::get_covar(const vector<DataAndModel*>& dms)
{
    double *covar = new double[na_*na_];

    mp_conf_.maxiter = 0;

    zero_init_result(&result_);
    result_.covar = covar;

    int status = run_mpfit(dms, F_->mgr.parameters(), par_usage_);
    soft_assert(status == MP_MAXITER);

    return covar;
}


} // namespace fityk

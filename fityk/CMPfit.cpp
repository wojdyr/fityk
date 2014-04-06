// This file is part of fityk program. Copyright 2001-2013 Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#define BUILDING_LIBFITYK
#include "CMPfit.h"
#include "logic.h"
#include "var.h"

using namespace std;

namespace fityk {

#ifndef NDEBUG
bool debug_deriv_in_mpfit=false; // changed only for tests (must be non-static)
#endif

static
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
    return (int) common_termination_criteria();
}

int MPfit::calculate(int /*m*/, int npar, double *par, double *deviates,
                     double **derivs)
{
    // MP_NO_ITER is used only to calculate covariance matrix
    if (mp_conf_.maxiter != MP_NO_ITER) {
        int stop = on_iteration();
        if (stop)
            return -1; // error code reserved for user function
    }

    vector<realt> A(par, par+npar);
    if (F_->get_verbosity() >= 1)
        output_tried_parameters(A);
    if (!derivs)
        compute_deviates(A, deviates);
    else
        compute_derivatives_mp(A, fitted_datas_, derivs, deviates);
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
        case MP_MAXITER: return "Maximum number of evaluations reached";
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

static
void init_config(mp_config_struct* mp_conf)
{
    // 0 value means default
    mp_conf->ftol = 0.;
    mp_conf->xtol = 0.;
    mp_conf->gtol = 1e-100;
    mp_conf->epsfcn = 0.;
    mp_conf->stepfactor = 0.;
    mp_conf->covtol = 0.;
    mp_conf->maxiter = 0;
    mp_conf->maxfev = 0;
    mp_conf->nprint = 0;
    mp_conf->douserscale = 0;
    mp_conf->nofinitecheck = 0;
    mp_conf->iterproc = NULL;
}


// final_a either has the same size as parameters or is NULL
int MPfit::run_mpfit(const vector<Data*>& datas,
                     const vector<realt>& parameters,
                     const vector<bool>& param_usage,
                     double *final_a)
{
    assert(param_usage.size() == parameters.size());

    double *a = final_a ? final_a : new double[parameters.size()];
    for (size_t i = 0; i != parameters.size(); ++i)
        a[i] = parameters[i];

    mp_par *pars = allocate_and_init_mp_par(param_usage);

    for (size_t i = 0; i < parameters.size(); ++i) {
        const Var* var = F_->mgr.gpos_to_var(i);
        if (!var->domain.lo_inf()) {
            pars[i].limited[0] = 1;
            pars[i].limits[0] = var->domain.lo;
        }
        if (!var->domain.hi_inf()) {
            pars[i].limited[1] = 1;
            pars[i].limits[1] = var->domain.hi;
        }
    }

#ifndef NDEBUG
    if (debug_deriv_in_mpfit)
        for (size_t i = 0; i < parameters.size(); ++i) {
            // don't use side=2 (two-side) because of bug in CMPFIT
            pars[i].side = 1;
            pars[i].deriv_debug = 1;
        }
#endif

    // datas cannot be easily passed to the calculate_for_mpfit() callback
    // in a different way than through member variable (fitted_datas_).
    int status;
    if (&datas != &fitted_datas_) {
        vector<Data*> saved = datas;
        fitted_datas_.swap(saved);
        status = mpfit(calculate_for_mpfit, count_points(datas),
                       parameters.size(), a, pars, &mp_conf_, this, &result_);
        fitted_datas_.swap(saved);
    } else
        status = mpfit(calculate_for_mpfit, count_points(datas),
                       parameters.size(), a, pars, &mp_conf_, this, &result_);
    soft_assert(status == result_.status);
    delete [] pars;
    if (final_a == NULL)
        delete [] a;
    return status;
}

double MPfit::run_method(vector<realt>* best_a)
{
    init_config(&mp_conf_);
    mp_conf_.maxiter = -2; // can't use 0 or 1 here (0=default, -1=MP_NO_ITER)
    mp_conf_.maxfev = max_eval() - 1; // MPFIT has 1 evaluation extra
    mp_conf_.ftol = F_->get_settings()->ftol_rel;
    mp_conf_.xtol = F_->get_settings()->xtol_rel;
    //mp_conf_.gtol = F_->get_settings()->mpfit_gtol;

    zero_init_result(&result_);

    double *a = new double[na_];
    int status = run_mpfit(fitted_datas_, a_orig_, par_usage(), a);
    //soft_assert(result_.nfev + 1 == evaluations_);
    F_->msg("mpfit status: " + S(mpstatus_to_string(status)));
    best_a->assign(a, a+na_);
    delete [] a;
    return result_.bestnorm;
}

vector<double> MPfit::get_covariance_matrix(const vector<Data*>& datas)
{
    update_par_usage(datas);
    vector<double> alpha(na_*na_, 0.);
    init_config(&mp_conf_);
    mp_conf_.maxiter = MP_NO_ITER;
    zero_init_result(&result_);
    result_.covar = &alpha[0]; // that's legal, vectors use contiguous storage
    int status = run_mpfit(datas, F_->mgr.parameters(), par_usage());
    soft_assert(status == MP_MAXITER);
    return alpha;
}

vector<double> MPfit::get_standard_errors(const vector<Data*>& datas)
{
    double wssr = compute_wssr(F_->mgr.parameters(), datas, true);
    double err_factor = sqrt(wssr / get_dof(datas));
    // `na_' was set by get_dof() above, from update_par_usage()
    vector<double> errors(na_, 0.);

    init_config(&mp_conf_);
    mp_conf_.maxiter = MP_NO_ITER;
    zero_init_result(&result_);
    result_.xerror = &errors[0]; // that's legal

    int status = run_mpfit(datas, F_->mgr.parameters(), par_usage());
    soft_assert(status == MP_MAXITER || status == MP_OK_DIR);

    for (int i = 0; i < na_; ++i)
        errors[i] *= err_factor;
    return errors;
}

} // namespace fityk

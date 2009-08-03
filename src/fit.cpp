// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
// $Id$

#include <algorithm>
#include <sstream>
#include <math.h>
#include <string.h>

#include "fit.h"
#include "logic.h"
#include "model.h"
#include "data.h"
#include "ui.h"
#include "numfuncs.h"
#include "settings.h"
#include "var.h"
#include "LMfit.h"
#include "GAfit.h"
#include "NMfit.h"

using namespace std;

Fit::Fit(Ftk *F_, string const& m)
    : name(m), F(F_), evaluations(0), iter_nr (0), na(0), last_refresh_time_(0)
{
}

/// dof = degrees of freedom = (number of points - number of parameters)
int Fit::get_dof(vector<DataAndModel*> const& dms)
{
    update_parameters(dms);
    int dof = 0;
    for (vector<DataAndModel*>::const_iterator i = dms.begin();
                                                    i != dms.end(); ++i)
        dof += (*i)->data()->get_n();
    dof -= count(par_usage.begin(), par_usage.end(), true);
    return dof;
}

string Fit::get_info(vector<DataAndModel*> const& dms)
{
    vector<fp> const &pp = F->get_parameters();
    int dof = get_dof(dms);
    //update_parameters(dms);
    fp wssr = do_compute_wssr(pp, dms, true);
    return "WSSR = " + S(wssr)
           + ";  DoF = " + S(dof)
           + ";  WSSR/DoF = " + S(wssr/dof)
           + ";  SSR = " + S(do_compute_wssr(pp, dms, false))
           + ";  R-squared = " + S(compute_r_squared(pp, dms)) ;
}

vector<fp> Fit::get_covariance_matrix(vector<DataAndModel*> const& dms)
{
    vector<fp> const &pp = F->get_parameters();
    update_parameters(dms);

    vector<fp> alpha(na*na), beta(na);
    compute_derivatives(pp, dms, alpha, beta);
    for (int i = 0; i < na; ++i) //to avoid singular matrix, we put fake values
        if (!par_usage[i]) {     // corresponding unused parameters
            alpha[i*na + i] = 1.;
        }
    //sometimes some parameters are unused, although formally are "used".
    //E.g. SplitGaussian with center < min(active x) will have hwhm1 unused
    //Anyway, if i'th column/row in alpha are only zeros, we must
    //do something about it -- symmetric error is undefined
    vector<int> undef;
    for (int i = 0; i < na; ++i) {
        bool has_nonzero = false;
        for (int j = 0; j < na; j++)
            if (alpha[na*i+j] != 0.) {
                has_nonzero = true;
                break;
            }
        if (!has_nonzero) {
            undef.push_back(i);
            alpha[i*na + i] = 1.;
        }
    }

    reverse_matrix(alpha, na);

    for (vector<int>::const_iterator i = undef.begin(); i != undef.end(); ++i)
        alpha[(*i)*na + (*i)] = 0.;

    return alpha;
}

vector<fp> Fit::get_symmetric_errors(vector<DataAndModel*> const& dms)
{
    vector<fp> alpha = get_covariance_matrix(dms);
    vector<fp> errors(na);
    for (int i = 0; i < na; ++i)
        errors[i] = sqrt(alpha[i*na + i]);
    return errors;
}

string Fit::get_error_info(vector<DataAndModel*> const& dms, bool matrix)
{
    vector<fp> alpha = get_covariance_matrix(dms);
    vector<fp> const &pp = F->get_parameters();
    string s;
    s = "Symmetric errors: ";
    for (int i = 0; i < na; i++) {
        if (par_usage[i]) {
            fp err = sqrt(alpha[i*na + i]);
            s += "\n" + F->find_variable_handling_param(i)->xname
                + " = " + S(pp[i])
                + " +- " + (err == 0. ? string("??") : S(err));
        }
    }
    if (matrix) {
        s += "\nCovariance matrix\n    ";
        for (int i = 0; i < na; ++i)
            if (par_usage[i])
                s += "\t" + F->find_variable_handling_param(i)->xname;
        for (int i = 0; i < na; ++i) {
            if (par_usage[i]) {
                s += "\n" + F->find_variable_handling_param(i)->xname;
                for (int j = 0; j < na; ++j) {
                    if (par_usage[j])
                        s += "\t" + S(alpha[na*i + j]);
                }
            }
        }
    }
    return s;
}

fp Fit::do_compute_wssr(vector<fp> const &A, vector<DataAndModel*> const& dms,
                        bool weigthed)
{
    fp wssr = 0;
    F->use_external_parameters(A); //that's the only side-effect
    for (vector<DataAndModel*>::const_iterator i = dms.begin();
                                                    i != dms.end(); ++i) {
        wssr += compute_wssr_for_data(*i, weigthed);
    }
    return wssr;
}

//static
fp Fit::compute_wssr_for_data(DataAndModel const* dm, bool weigthed)
{
    Data const* data = dm->data();
    int n = data->get_n();
    vector<fp> xx(n);
    for (int j = 0; j < n; j++)
        xx[j] = data->get_x(j);
    vector<fp> yy(n, 0.);
    dm->model()->compute_model(xx, yy);
    fp wssr = 0;
    for (int j = 0; j < n; j++) {
        fp dy = data->get_y(j) - yy[j];
        if (weigthed)
            dy /= data->get_sigma(j);
        wssr += dy * dy;
    }
    return wssr;
}

fp Fit::compute_r_squared(vector<fp> const &A, vector<DataAndModel*> const& dms)
{
    fp r_squared = 0;
    F->use_external_parameters(A);
    for (vector<DataAndModel*>::const_iterator i = dms.begin();
                                                    i != dms.end(); ++i) {
        r_squared += compute_r_squared_for_data(*i);
    }
    return r_squared ;
}

//static
fp Fit::compute_r_squared_for_data(DataAndModel const* dm)
{
    Data const* data = dm->data();
    int n = data->get_n();
    vector<fp> xx(n);
    for (int j = 0; j < n; j++)
        xx[j] = data->get_x(j);
    vector<fp> yy(n, 0.);
    dm->model()->compute_model(xx, yy);
    fp mean = 0;
    fp ssr_curve = 0; // Sum of squares of dist. between fitted curve and data
    fp ssr_mean = 0 ;  // Sum of squares of distances between mean and data
    for (int j = 0; j < n; j++) {
        mean += data->get_y(j) ;
        fp dy = data->get_y(j) - yy[j];
        ssr_curve += dy * dy ;
    }
    mean = mean / n;    // Mean computed here.

    for (int j = 0 ; j < n ; j++) {
        fp dy = data->get_y(j) - mean;
        ssr_mean += dy * dy;
    }

    return 1 - (ssr_curve/ssr_mean); // R^2 as defined.
}

//results in alpha and beta
void Fit::compute_derivatives(vector<fp> const &A,
                              vector<DataAndModel*> const& dms,
                              vector<fp>& alpha, vector<fp>& beta)
{
    assert (size(A) == na && size(alpha) == na * na && size(beta) == na);
    fill(alpha.begin(), alpha.end(), 0.0);
    fill(beta.begin(), beta.end(), 0.0);

    F->use_external_parameters(A);
    for (vector<DataAndModel*>::const_iterator i = dms.begin();
                                                    i != dms.end(); ++i) {
        compute_derivatives_for(*i, alpha, beta);
    }
    // filling second half of alpha[]
    for (int j = 1; j < na; j++)
        for (int k = 0; k < j; k++)
            alpha[na * k + j] = alpha[na * j + k];
}

//results in alpha and beta
//it computes only half of alpha matrix
void Fit::compute_derivatives_for(DataAndModel const* dm,
                                  vector<fp>& alpha, vector<fp>& beta)
{
    Data const* data = dm->data();
    int n = data->get_n();
    vector<fp> xx(n);
    for (int j = 0; j < n; ++j)
        xx[j] = data->get_x(j);
    vector<fp> yy(n, 0.);
    const int dyn = na+1;
    vector<fp> dy_da(n*dyn, 0.);
    dm->model()->compute_model_with_derivs(xx, yy, dy_da);
    for (int i = 0; i != n; i++) {
        fp inv_sig = 1.0 / data->get_sigma(i);
        fp dy_sig = (data->get_y(i) - yy[i]) * inv_sig;
        vector<fp>::iterator const t = dy_da.begin() + i*dyn;
        for (int j = 0; j != na; ++j) {
            if (par_usage[j]) {
                *(t+j) *= inv_sig;
                for (int k = 0; k <= j; ++k)    //half of alpha[]
                    alpha[na * j + k] += *(t+j) * *(t+k);
                beta[j] += dy_sig * *(t+j);
            }
        }
    }
}

string Fit::print_matrix (const vector<fp>& vec, int m, int n,
                          const char *mname)
    //m rows, n columns
{
    if (F->get_verbosity() <= 0)  //optimization (?)
        return "";
    assert (size(vec) == m * n);
    if (m < 1 || n < 1)
        throw ExecuteError("In `print_matrix': It is not a matrix.");
    ostringstream h;
    h << mname << "={ ";
    if (m == 1) { // vector
        for (int i = 0; i < n; i++)
            h << vec[i] << (i < n - 1 ? ", " : " }") ;
    }
    else { //matrix
        std::string blanks (strlen(mname) + 1, ' ');
        for (int j = 0; j < m; j++){
            if (j > 0)
                h << blanks << "  ";
            for (int i = 0; i < n; i++)
                h << vec[j * n + i] << ", ";
            h << endl;
        }
        h << blanks << "}";
    }
    return h.str();
}

bool Fit::post_fit (const std::vector<fp>& aa, fp chi2)
{
    F->msg(name + " method. " + S(iter_nr) + " iterations, "
            + S(evaluations) + " func. evaluations in "
            + S(time(0)-start_time_) + "s.");
    bool better = (chi2 < wssr_before);
    if (better) {
        F->get_fit_container()->push_param_history(aa);
        F->put_new_parameters(aa);
        F->msg ("Better fit found (WSSR = " + S(chi2)
                 + ", was " + S(wssr_before)
                 + ", " + S((chi2 - wssr_before) / wssr_before * 100) + "%).");
    }
    else {
        F->msg ("Better fit NOT found (WSSR = " + S(chi2)
                    + ", was " + S(wssr_before) + ").\nParameters NOT changed");
        F->use_external_parameters(a_orig);
        F->get_ui()->draw_plot(3, true);
    }
    F->get_ui()->enable_compute_ui(false);
    return better;
}

fp Fit::draw_a_from_distribution (int nr, char distribution, fp mult)
{
    assert (nr >= 0 && nr < na);
    fp dv = 0;
    switch (distribution) {
        case 'g':
            dv = rand_gauss();
            break;
        case 'l':
            dv = rand_cauchy();
            break;
        case 'b':
            dv = rand_bool() ? -1 : 1;
            break;
        default: // 'u' - uniform
            dv = rand_1_1();
            break;
    }
    return F->variation_of_a(nr, dv * mult);
}

/// initialize and run fitting procedure for not more than max_iter iterations
void Fit::fit(int max_iter, vector<DataAndModel*> const& dms)
{
    start_time_ = last_refresh_time_ = time(0);
    // update_parameters() can throw exceptions, so don't freeze UI before
    // calling it.
    update_parameters(dms);
    F->get_ui()->enable_compute_ui(true);
    dmdm_ = dms;
    a_orig = F->get_parameters();
    F->get_fit_container()->push_param_history(a_orig);
    iter_nr = 0;
    evaluations = 0;
    max_evaluations_ = F->get_settings()->get_i("max-wssr-evaluations");
    user_interrupt = false;
    init(); //method specific init
    max_iterations = max_iter;

    // print stats
    int nu = count(par_usage.begin(), par_usage.end(), true);
    int np = 0;
    for (vector<DataAndModel*>::const_iterator i = dms.begin();
                                                    i != dms.end(); ++i)
        np += (*i)->data()->get_n();
    F->msg ("Fit " + S(nu) + " (of " + S(na) + ") parameters to " + S(np)
            + " points ...");

    autoiter();
}

/// run fitting procedure (without initialization)
void Fit::continue_fit(int max_iter)
{
    start_time_ = last_refresh_time_ = time(0);
    for (vector<DataAndModel*>::const_iterator i = dmdm_.begin();
                                                      i != dmdm_.end(); ++i)
        if (!F->contains_dm(*i) || na != size(F->get_parameters()))
            throw ExecuteError(name + " method should be initialized first.");
    update_parameters(dmdm_);
    a_orig = F->get_parameters();  //should it be also updated?
    user_interrupt = false;
    evaluations = 0;
    max_iterations = max_iter;
    autoiter();
}

void Fit::update_parameters(vector<DataAndModel*> const& dms)
{
    if (F->get_parameters().empty())
        throw ExecuteError("there are no fittable parameters.");
    if (dms.empty())
        throw ExecuteError("No datasets to fit.");

    na = F->get_parameters().size();

    par_usage = vector<bool>(na, false);
    for (int idx = 0; idx < na; ++idx) {
        int var_idx = F->find_nr_var_handling_param(idx);
        for (vector<DataAndModel*>::const_iterator i = dms.begin();
                                                        i != dms.end(); ++i) {
            if ((*i)->model()->is_dependent_on_var(var_idx)) {
                par_usage[idx] = true;
                break; //go to next idx
            }
            //vmsg(F->find_variable_handling_param(idx)->xname
            //        + " is not in chi2.");
        }
    }
    if (count(par_usage.begin(), par_usage.end(), true) == 0)
        throw ExecuteError("No parametrized functions are used in the model.");
}

/// checks termination criteria common for all fitting methods
bool Fit::common_termination_criteria(int iter)
{
    bool stop = false;
    if (user_interrupt) {
        user_interrupt = false;
        F->msg ("Fitting stopped manually.");
        stop = true;
    }
    if (max_iterations >= 0 && iter >= max_iterations) {
        F->msg("Maximum iteration number reached.");
        stop = true;
    }
    if (max_evaluations_ > 0 && evaluations >= max_evaluations_) {
        F->msg("Maximum evaluations number reached.");
        stop = true;
    }
    return stop;
}

void Fit::iteration_plot(vector<fp> const &A, bool changed, fp wssr)
{
    int refresh_period = F->get_settings()->get_i("refresh-period");
    if (refresh_period < 0)
        return;
    time_t now = time(0);
    if (now - last_refresh_time_ < refresh_period)
        return;
    if (changed) {
        F->use_external_parameters(A);
        F->get_ui()->draw_plot(3, true);
    }
    if (refresh_period > 0)
        F->msg("Iter: " + S(iter_nr) + "/"
                + (max_iterations > 0 ? S(max_iterations) : string("oo"))
                + "  Eval: " + S(evaluations) + "/"
                + (max_evaluations_ > 0 ? S(max_evaluations_) : string("oo"))
                + "  WSSR=" + S(wssr)
                + " (" + S(wssr * 100. / wssr_before)+ "%)"
                + "  Elapsed " + S(now - start_time_) + "s.");
    F->get_ui()->refresh();
    last_refresh_time_ = time(0);
}


/// This function solves a set of linear algebraic equations using
/// Jordan elimination with partial pivoting.
///
/// A * x = b
///
/// A is n x n matrix, fp A[n*n]
/// b is vector b[n],
/// Function returns vector x[] in b[], and 1-matrix in A[].
/// return value: true=OK, false=singular matrix
///   with special exception:
///     if i'th row, i'th column and i'th element in b all contains zeros,
///     it's just ignored,
void Fit::Jordan(vector<fp>& A, vector<fp>& b, int n)
{
    assert (size(A) == n*n && size(b) == n);
    for (int i = 0; i < n; i++) {
        fp amax = 0;                    // looking for a pivot element
        int maxnr = -1;
        for (int j = i; j < n; j++)
            if (fabs (A[n*j+i]) > amax) {
                maxnr = j;
                amax = fabs (A[n * j + i]);
            }
        if (maxnr == -1) {    // singular matrix
            // i-th column has only zeros.
            // If it's the same about i-th row, and b[i]==0, let x[i]==0.
            for (int j = i; j < n; j++)
                if (A[n * i + j] || b[i]) {
                    F->vmsg (print_matrix(A, n, n, "A"));
                    F->msg (print_matrix(b, 1, n, "b"));
                    throw ExecuteError("In iteration " + S(iter_nr)
                                       + ": trying to reverse singular matrix."
                                        " Column " + S(i) + " is zeroed.");
                }
            continue; // x[i]=b[i], b[i]==0
        }
        if (maxnr != i) {                            // interchanging rows
            for (int j = i; j < n; j++)
                swap (A[n*maxnr+j], A[n*i+j]);
            swap (b[i], b[maxnr]);
        }
        fp c = 1.0 / A[i*n+i];
        for (int j = i; j < n; j++)
            A[i*n+j] *= c;
        b[i] *= c;
        for (int k = 0; k < n; k++)
            if (k != i) {
                fp d = A[k * n + i];
                for (int j = i; j < n; j++)
                    A[k * n + j] -= A[i * n + j] * d;
                b[k] -= b[i] * d;
            }
    }
}

/// A - matrix n x n; returns A^(-1) in A
void Fit::reverse_matrix (vector<fp>&A, int n)
{
    //no need to optimize it
    assert (size(A) == n*n);
    vector<fp> A_result(n*n);
    for (int i = 0; i < n; i++) {
        vector<fp> A_copy = A;
        vector<fp> v(n, 0);
        v[i] = 1;
        Jordan(A_copy, v, n);
        for (int j = 0; j < n; j++)
            A_result[j * n + i] = v[j];
    }
    A = A_result;
}

//-------------------------------------------------------------------

FitMethodsContainer::FitMethodsContainer(Ftk *F_)
    : ParameterHistoryMgr(F_), dirty_error_cache_(true)

{
    methods_.push_back(new LMfit(F));
    methods_.push_back(new NMfit(F));
    methods_.push_back(new GAfit(F));
}

FitMethodsContainer::~FitMethodsContainer()
{
    purge_all_elements(methods_);
}

fp FitMethodsContainer::get_symmetric_error(Variable const* var)
{
    if (!var->is_simple())
        return -1.; // value signaling unknown symmetric error
    if (dirty_error_cache_
            || errors_cache_.size() != F->get_parameters().size()) {
        errors_cache_ = F->get_fit()->get_symmetric_errors(F->get_dms());
    }
    return errors_cache_[var->get_nr()];
}

/// loads vector of parameters from the history
/// "relative" is used for undo/redo commands
/// if history is not empty and current parameters are different from
///     the ones pointed by param_hist_ptr (but have the same size),
///     load_param_history(-1, true), i.e undo, will load the parameters
///     pointed by param_hist_ptr
void ParameterHistoryMgr::load_param_history(int item_nr, bool relative)
{
    if (item_nr == -1 && relative && !param_history.empty() //undo
         && param_history[param_hist_ptr].size() == F->get_parameters().size()
         && param_history[param_hist_ptr] != F->get_parameters())
        item_nr = 0; // load parameters from param_hist_ptr
    if (relative)
        item_nr += param_hist_ptr;
    else if (item_nr < 0)
        item_nr += param_history.size();
    if (item_nr < 0 || item_nr >= size(param_history))
        throw ExecuteError("There is no parameter history item #"
                            + S(item_nr) + ".");
    F->put_new_parameters(param_history[item_nr]);
    param_hist_ptr = item_nr;
}

bool ParameterHistoryMgr::can_undo() const
{
    return !param_history.empty()
        && (param_hist_ptr > 0 || param_history[0] != F->get_parameters());
}

bool ParameterHistoryMgr::push_param_history(vector<fp> const& aa)
{
    param_hist_ptr = param_history.size() - 1;
    if (param_history.empty() || param_history.back() != aa) {
        param_history.push_back(aa);
        ++param_hist_ptr;
        return true;
    }
    else
        return false;
}


std::string ParameterHistoryMgr::param_history_info() const
{
    string s = "Parameter history contains " + S(param_history.size())
        + " items.";
    if (!param_history.empty())
        s += " Now at #" + S(param_hist_ptr);
    return s;
}



// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#include <algorithm>
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

Fit::Fit(Ftk *F, const string& m)
    : name(m), F_(F), evaluations(0), iter_nr (0), na(0), last_refresh_time_(0)
{
}

/// dof = degrees of freedom = (number of points - number of parameters)
int Fit::get_dof(const vector<DataAndModel*>& dms)
{
    update_parameters(dms);
    int dof = 0;
    v_foreach (DataAndModel*, i, dms)
        dof += (*i)->data()->get_n();
    dof -= count(par_usage.begin(), par_usage.end(), true);
    return dof;
}

string Fit::get_goodness_info(const vector<DataAndModel*>& dms)
{
    const SettingsMgr *sm = F_->settings_mgr();
    const vector<realt>& pp = F_->parameters();
    int dof = get_dof(dms);
    //update_parameters(dms);
    realt wssr = do_compute_wssr(pp, dms, true);
    return "WSSR = " + sm->format_double(wssr)
           + ";  DoF = " + S(dof)
           + ";  WSSR/DoF = " + sm->format_double(wssr/dof)
           + ";  SSR = " + sm->format_double(do_compute_wssr(pp, dms, false))
           + ";  R-squared = " + sm->format_double(compute_r_squared(pp, dms));
}

vector<realt> Fit::get_covariance_matrix(const vector<DataAndModel*>& dms)
{
    const vector<realt> &pp = F_->parameters();
    update_parameters(dms);

    vector<realt> alpha(na*na), beta(na);
    compute_derivatives(pp, dms, alpha, beta);

    // To avoid singular matrix, put fake values corresponding to unused
    // parameters.
    for (int i = 0; i < na; ++i)
        if (!par_usage[i]) {
            alpha[i*na + i] = 1.;
        }
    // We may have unused parameters with par_usage[] set true,
    // e.g. SplitGaussian with center < min(active x) will have hwhm1 unused.
    // If i'th column/row in alpha are only zeros, we must
    // do something about it -- standard error is undefined
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

    v_foreach (int, i, undef)
        alpha[(*i)*na + (*i)] = 0.;

    return alpha;
}

vector<realt> Fit::get_standard_errors(const vector<DataAndModel*>& dms)
{
    const vector<realt> &pp = F_->parameters();
    realt wssr = do_compute_wssr(pp, dms, true);
    int dof = get_dof(dms);
    vector<realt> alpha = get_covariance_matrix(dms);
    // `na' was set by functions above
    vector<realt> errors(na);
    for (int i = 0; i < na; ++i)
        errors[i] = sqrt(wssr / dof * alpha[i*na + i]);
    return errors;
}

string Fit::get_error_info(const vector<DataAndModel*>& dms)
{
    const SettingsMgr *sm = F_->settings_mgr();
    vector<realt> errors = get_standard_errors(dms);
    const vector<realt>& pp = F_->parameters();
    string s = "Standard errors:";
    for (int i = 0; i < na; i++) {
        if (par_usage[i]) {
            realt err = errors[i];
            s += "\n$" + F_->find_variable_handling_param(i)->name
                + " = " + sm->format_double(pp[i])
                + " +- " + (err == 0. ? string("??") : sm->format_double(err));
        }
    }
    return s;
}

string Fit::get_cov_info(const vector<DataAndModel*>& dms)
{
    string s;
    const SettingsMgr *sm = F_->settings_mgr();
    vector<realt> alpha = get_covariance_matrix(dms);
    s += "\nCovariance matrix\n    ";
    for (int i = 0; i < na; ++i)
        if (par_usage[i])
            s += "\t$" + F_->find_variable_handling_param(i)->name;
    for (int i = 0; i < na; ++i) {
        if (par_usage[i]) {
            s += "\n$" + F_->find_variable_handling_param(i)->name;
            for (int j = 0; j < na; ++j) {
                if (par_usage[j])
                    s += "\t" + sm->format_double(alpha[na*i + j]);
            }
        }
    }
    return s;
}

realt Fit::do_compute_wssr(const vector<realt> &A,
                           const vector<DataAndModel*>& dms,
                           bool weigthed)
{
    realt wssr = 0;
    F_->use_external_parameters(A); //that's the only side-effect
    v_foreach (DataAndModel*, i, dms) {
        wssr += compute_wssr_for_data(*i, weigthed);
    }
    return wssr;
}

//static
realt Fit::compute_wssr_for_data(const DataAndModel* dm, bool weigthed)
{
    const Data* data = dm->data();
    int n = data->get_n();
    vector<realt> xx(n);
    for (int j = 0; j < n; j++)
        xx[j] = data->get_x(j);
    vector<realt> yy(n, 0.);
    dm->model()->compute_model(xx, yy);
    realt wssr = 0;
    for (int j = 0; j < n; j++) {
        realt dy = data->get_y(j) - yy[j];
        if (weigthed)
            dy /= data->get_sigma(j);
        wssr += dy * dy;
    }
    return wssr;
}

// R^2 for multiple datasets is calculated with separate mean y for each dataset
realt Fit::compute_r_squared(const vector<realt> &A,
                             const vector<DataAndModel*>& dms)
{
    realt sum_err = 0, sum_tot = 0, se = 0, st = 0;
    F_->use_external_parameters(A);
    v_foreach (DataAndModel*, i, dms) {
        compute_r_squared_for_data(*i, &se, &st);
        sum_err += se;
        sum_tot += st;
    }
    return 1 - (sum_err / sum_tot);
}

//static
realt Fit::compute_r_squared_for_data(const DataAndModel* dm,
                                      realt* sum_err, realt* sum_tot)
{
    const Data* data = dm->data();
    int n = data->get_n();
    vector<realt> xx(n);
    for (int j = 0; j < n; j++)
        xx[j] = data->get_x(j);
    vector<realt> yy(n, 0.);
    dm->model()->compute_model(xx, yy);
    realt ysum = 0;
    realt ss_err = 0; // Sum of squares of dist. between fitted curve and data
    for (int j = 0; j < n; j++) {
        ysum += data->get_y(j) ;
        realt dy = data->get_y(j) - yy[j];
        ss_err += dy * dy ;
    }
    realt mean = ysum / n;

    realt ss_tot = 0;  // Sum of squares of distances between mean and data
    for (int j = 0; j < n; j++) {
        realt dy = data->get_y(j) - mean;
        ss_tot += dy * dy;
    }

    if (sum_err != NULL)
        *sum_err = ss_err;
    if (sum_tot != NULL)
        *sum_tot = ss_tot;

    // R^2, formula from
    // http://en.wikipedia.org/wiki/Coefficient_of_determination
    return 1 - (ss_err / ss_tot);
}

//results in alpha and beta
void Fit::compute_derivatives(const vector<realt> &A,
                              const vector<DataAndModel*>& dms,
                              vector<realt>& alpha, vector<realt>& beta)
{
    assert (size(A) == na && size(alpha) == na * na && size(beta) == na);
    fill(alpha.begin(), alpha.end(), 0.0);
    fill(beta.begin(), beta.end(), 0.0);

    F_->use_external_parameters(A);
    v_foreach (DataAndModel*, i, dms) {
        compute_derivatives_for(*i, alpha, beta);
    }
    // filling second half of alpha[]
    for (int j = 1; j < na; j++)
        for (int k = 0; k < j; k++)
            alpha[na * k + j] = alpha[na * j + k];
}

//results in alpha and beta
//it computes only half of alpha matrix
void Fit::compute_derivatives_for(const DataAndModel* dm,
                                  vector<realt>& alpha, vector<realt>& beta)
{
    const Data* data = dm->data();
    int n = data->get_n();
    vector<realt> xx(n);
    for (int j = 0; j < n; ++j)
        xx[j] = data->get_x(j);
    vector<realt> yy(n, 0.);
    const int dyn = na+1;
    vector<realt> dy_da(n*dyn, 0.);
    dm->model()->compute_model_with_derivs(xx, yy, dy_da);
    for (int i = 0; i != n; i++) {
        realt inv_sig = 1.0 / data->get_sigma(i);
        realt dy_sig = (data->get_y(i) - yy[i]) * inv_sig;
        vector<realt>::iterator t = dy_da.begin() + i*dyn;
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

string Fit::print_matrix(const vector<realt>& vec, int m, int n,
                         const char *mname)
    //m rows, n columns
{
    if (F_->get_verbosity() <= 0)  //optimization (?)
        return "";
    assert (size(vec) == m * n);
    if (m < 1 || n < 1)
        throw ExecuteError("In `print_matrix': It is not a matrix.");
    string h = S(mname) + "={ ";
    if (m == 1) { // vector
        for (int i = 0; i < n; i++)
            h += S(vec[i]) + (i < n - 1 ? ", " : " }") ;
    }
    else { //matrix
        string blanks (strlen(mname) + 1, ' ');
        for (int j = 0; j < m; j++){
            if (j > 0)
                h += blanks + "  ";
            for (int i = 0; i < n; i++)
                h += S(vec[j * n + i]) + ", ";
            h += "\n";
        }
        h += blanks + "}";
    }
    return h;
}

bool Fit::post_fit(const vector<realt>& aa, realt chi2)
{
    double elapsed = (clock() - start_time_) / (double) CLOCKS_PER_SEC;
    F_->msg(name + ": " + S(iter_nr) + " iterations, "
           + S(evaluations) + " evaluations, "
           + format1<double,16>("%.2f", elapsed) + " s. of CPU time.");
    bool better = (chi2 < wssr_before);
    const SettingsMgr *sm = F_->settings_mgr();
    if (better) {
        F_->get_fit_container()->push_param_history(aa);
        F_->put_new_parameters(aa);
        double percent_change = (chi2 - wssr_before) / wssr_before * 100.;
        F_->msg("WSSR: " + sm->format_double(chi2) +
                " (" + S(percent_change) + "%)");
    }
    else {
        F_->msg("Better fit NOT found (WSSR = " + sm->format_double(chi2)
                + ", was " + sm->format_double(wssr_before) + ")."
                "\nParameters NOT changed");
        F_->use_external_parameters(a_orig);
        if (F_->get_settings()->fit_replot)
            F_->get_ui()->draw_plot(UserInterface::kRepaintImmediately);
    }
    return better;
}

realt Fit::draw_a_from_distribution (int nr, char distribution, realt mult)
{
    assert (nr >= 0 && nr < na);
    if (!par_usage[nr])
        return a_orig[nr];
    realt dv = 0;
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
    return F_->variation_of_a(nr, dv * mult);
}

class ComputeUI
{
public:
    ComputeUI(UserInterface *ui) : ui_(ui) { ui->enable_compute_ui(true); }
    ~ComputeUI() { ui_->enable_compute_ui(false); }
private:
    UserInterface *ui_;
};

/// initialize and run fitting procedure for not more than max_iter iterations
void Fit::fit(int max_iter, const vector<DataAndModel*>& dms)
{
    start_time_ = clock();
    last_refresh_time_ = time(0);
    ComputeUI compute_ui(F_->get_ui());
    update_parameters(dms);
    dmdm_ = dms;
    a_orig = F_->parameters();
    F_->get_fit_container()->push_param_history(a_orig);
    iter_nr = 0;
    evaluations = 0;
    max_evaluations_ = F_->get_settings()->max_wssr_evaluations;
    user_interrupt = false;
    init(); //method specific init
    max_iterations = max_iter;

    // print stats
    int nu = count(par_usage.begin(), par_usage.end(), true);
    int np = 0;
    v_foreach (DataAndModel*, i, dms)
        np += (*i)->data()->get_n();
    F_->msg ("Fitting " + S(nu) + " (of " + S(na) + ") parameters to "
            + S(np) + " points ...");

    autoiter();
}

/// run fitting procedure (without initialization)
void Fit::continue_fit(int max_iter)
{
    start_time_ = clock();
    last_refresh_time_ = time(0);
    v_foreach (DataAndModel*, i, dmdm_)
        if (!F_->contains_dm(*i) || na != size(F_->parameters()))
            throw ExecuteError(name + " method should be initialized first.");
    update_parameters(dmdm_);
    a_orig = F_->parameters();  //should it be also updated?
    user_interrupt = false;
    evaluations = 0;
    max_iterations = max_iter;
    autoiter();
}

void Fit::update_parameters(const vector<DataAndModel*>& dms)
{
    if (F_->parameters().empty())
        throw ExecuteError("there are no fittable parameters.");
    if (dms.empty())
        throw ExecuteError("No datasets to fit.");

    na = F_->parameters().size();

    par_usage = vector<bool>(na, false);
    for (int idx = 0; idx < na; ++idx) {
        int var_idx = F_->find_nr_var_handling_param(idx);
        v_foreach (DataAndModel*, i, dms) {
            if ((*i)->model()->is_dependent_on_var(var_idx)) {
                par_usage[idx] = true;
                break; //go to next idx
            }
            //vmsg(F_->find_variable_handling_param(idx)->xname
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
        F_->msg ("Fitting stopped manually.");
        stop = true;
    }
    if (max_iterations >= 0 && iter >= max_iterations) {
        F_->msg("Maximum iteration number reached.");
        stop = true;
    }
    if (max_evaluations_ > 0 && evaluations >= max_evaluations_) {
        F_->msg("Maximum evaluations number reached.");
        stop = true;
    }
    return stop;
}

void Fit::iteration_plot(const vector<realt> &A, realt wssr)
{
    int p = F_->get_settings()->refresh_period;
    if (p < 0 || (p > 0 && time(0) - last_refresh_time_ < p))
        return;
    if (F_->get_settings()->fit_replot) {
        F_->use_external_parameters(A);
        F_->get_ui()->draw_plot(UserInterface::kRepaintImmediately);
    }
    double elapsed = (clock() - start_time_) / (double) CLOCKS_PER_SEC;
    double percent_change = (wssr - wssr_before) / wssr_before * 100.;
    F_->msg("Iter: " + S(iter_nr) + "/"
            + (max_iterations > 0 ? S(max_iterations) : string("oo"))
            + "  Eval: " + S(evaluations) + "/"
            + (max_evaluations_ > 0 ? S(max_evaluations_) : string("oo"))
            + "  WSSR=" + F_->settings_mgr()->format_double(wssr)
            + " (" + S(percent_change)+ "%)"
            + "  CPU time: " + format1<double,16>("%.2f", elapsed) + "s.");
    F_->get_ui()->refresh();
    last_refresh_time_ = time(0);
}


/// This function solves a set of linear algebraic equations using
/// Jordan elimination with partial pivoting.
///
/// A * x = b
///
/// A is n x n matrix, realt A[n*n]
/// b is vector b[n],
/// Function returns vector x[] in b[], and 1-matrix in A[].
/// return value: true=OK, false=singular matrix
///   with special exception:
///     if i'th row, i'th column and i'th element in b all contains zeros,
///     it's just ignored,
void Fit::Jordan(vector<realt>& A, vector<realt>& b, int n)
{
    assert (size(A) == n*n && size(b) == n);
    for (int i = 0; i < n; i++) {
        realt amax = 0;                    // looking for a pivot element
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
                    F_->vmsg (print_matrix(A, n, n, "A"));
                    F_->msg (print_matrix(b, 1, n, "b"));
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
        realt c = 1.0 / A[i*n+i];
        for (int j = i; j < n; j++)
            A[i*n+j] *= c;
        b[i] *= c;
        for (int k = 0; k < n; k++)
            if (k != i) {
                realt d = A[k * n + i];
                for (int j = i; j < n; j++)
                    A[k * n + j] -= A[i * n + j] * d;
                b[k] -= b[i] * d;
            }
    }
}

/// A - matrix n x n; returns A^(-1) in A
void Fit::reverse_matrix (vector<realt>&A, int n)
{
    //no need to optimize it
    assert (size(A) == n*n);
    vector<realt> A_result(n*n);
    for (int i = 0; i < n; i++) {
        vector<realt> A_copy = A;
        vector<realt> v(n, 0);
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
    methods_.push_back(new LMfit(F_));
    methods_.push_back(new NMfit(F_));
    methods_.push_back(new GAfit(F_));
}

FitMethodsContainer::~FitMethodsContainer()
{
    purge_all_elements(methods_);
}

realt FitMethodsContainer::get_standard_error(const Variable* var) const
{
    if (!var->is_simple())
        return -1.; // value signaling unknown standard error
    if (dirty_error_cache_
            || errors_cache_.size() != F_->parameters().size()) {
        errors_cache_ = F_->get_fit()->get_standard_errors(F_->get_dms());
    }
    return errors_cache_[var->get_nr()];
}

/// loads vector of parameters from the history
/// "relative" is used for undo/redo commands
/// if history is not empty and current parameters are different from
///     the ones pointed by param_hist_ptr_ (but have the same size),
///     load_param_history(-1, true), i.e undo, will load the parameters
///     pointed by param_hist_ptr_
void ParameterHistoryMgr::load_param_history(int item_nr, bool relative)
{
    if (item_nr == -1 && relative && !param_history_.empty() //undo
         && param_history_[param_hist_ptr_].size() == F_->parameters().size()
         && param_history_[param_hist_ptr_] != F_->parameters())
        item_nr = 0; // load parameters from param_hist_ptr_
    if (relative)
        item_nr += param_hist_ptr_;
    else if (item_nr < 0)
        item_nr += param_history_.size();
    if (item_nr < 0 || item_nr >= size(param_history_))
        throw ExecuteError("There is no parameter history item #"
                            + S(item_nr) + ".");
    F_->put_new_parameters(param_history_[item_nr]);
    param_hist_ptr_ = item_nr;
}

bool ParameterHistoryMgr::can_undo() const
{
    return !param_history_.empty()
        && (param_hist_ptr_ > 0 || param_history_[0] != F_->parameters());
}

bool ParameterHistoryMgr::push_param_history(const vector<realt>& aa)
{
    param_hist_ptr_ = param_history_.size() - 1;
    if (param_history_.empty() || param_history_.back() != aa) {
        param_history_.push_back(aa);
        ++param_hist_ptr_;
        return true;
    }
    else
        return false;
}


string ParameterHistoryMgr::param_history_info() const
{
    string s = "Parameter history contains " + S(param_history_.size())
        + " items.";
    if (!param_history_.empty())
        s += " Now at #" + S(param_hist_ptr_);
    return s;
}



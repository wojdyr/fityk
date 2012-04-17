// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#include <algorithm>
#include <math.h>
#include <string.h>
#include <boost/math/distributions/students_t.hpp>

#include "fit.h"
#include "logic.h"
#include "model.h"
#include "data.h"
#include "ui.h"
#include "numfuncs.h"
#include "settings.h"
#include "var.h"
#include "LMfit.h"
#include "MPfit.h"
#include "GAfit.h"
#include "NMfit.h"
#include "NLfit.h"

using namespace std;

Fit::Fit(Ftk *F, const string& m)
    : name(m), F_(F),
      evaluations_(0), iter_nr_(0), na_(0), last_refresh_time_(0)
{
}

/// dof = degrees of freedom = (number of points - number of parameters)
int Fit::get_dof(const vector<DataAndModel*>& dms)
{
    update_parameters(dms);
    int dof = 0;
    v_foreach (DataAndModel*, i, dms)
        dof += (*i)->data()->get_n();
    dof -= count(par_usage_.begin(), par_usage_.end(), true);
    return dof;
}

string Fit::get_goodness_info(const vector<DataAndModel*>& dms)
{
    const SettingsMgr *sm = F_->settings_mgr();
    const vector<realt>& pp = F_->parameters();
    int dof = get_dof(dms);
    //update_parameters(dms);
    realt wssr = do_compute_wssr(pp, dms, true);
    return "WSSR=" + sm->format_double(wssr)
           + "  DoF=" + S(dof)
           + "  WSSR/DoF=" + sm->format_double(wssr/dof)
           + "  SSR=" + sm->format_double(do_compute_wssr(pp, dms, false))
           + "  R2=" + sm->format_double(compute_r_squared(pp, dms));
}

vector<realt> Fit::get_covariance_matrix(const vector<DataAndModel*>& dms)
{
    const vector<realt> &pp = F_->parameters();
    update_parameters(dms);

    vector<realt> alpha(na_*na_), beta(na_);
    compute_derivatives(pp, dms, alpha, beta);

    // To avoid singular matrix, put fake values corresponding to unused
    // parameters.
    for (int i = 0; i < na_; ++i)
        if (!par_usage_[i]) {
            alpha[i*na_ + i] = 1.;
        }
    // We may have unused parameters with par_usage_[] set true,
    // e.g. SplitGaussian with center < min(active x) will have hwhm1 unused.
    // If i'th column/row in alpha are only zeros, we must
    // do something about it -- standard error is undefined
    vector<int> undef;
    for (int i = 0; i < na_; ++i) {
        bool has_nonzero = false;
        for (int j = 0; j < na_; j++)
            if (alpha[na_*i+j] != 0.) {
                has_nonzero = true;
                break;
            }
        if (!has_nonzero) {
            undef.push_back(i);
            alpha[i*na_ + i] = 1.;
        }
    }

    reverse_matrix(alpha, na_);

    v_foreach (int, i, undef)
        alpha[(*i)*na_ + (*i)] = 0.;

    return alpha;
}

vector<realt> Fit::get_standard_errors(const vector<DataAndModel*>& dms)
{
    const vector<realt> &pp = F_->parameters();
    realt wssr = do_compute_wssr(pp, dms, true);
    int dof = get_dof(dms);
    vector<realt> alpha = get_covariance_matrix(dms);
    // `na_' was set by functions above
    vector<realt> errors(na_);
    for (int i = 0; i < na_; ++i)
        errors[i] = sqrt(wssr / dof * alpha[i*na_ + i]);
    return errors;
}

vector<realt> Fit::get_confidence_limits(const vector<DataAndModel*>& dms,
                                         double level_percent)
{
    vector<realt> v = get_standard_errors(dms);
    int dof = get_dof(dms);
    double level = 1. - level_percent / 100.;
    boost::math::students_t dist(dof);
    double t = boost::math::quantile(boost::math::complement(dist, level/2));
    vm_foreach (realt, i, v)
        *i *= t;
    return v;
}

string Fit::get_cov_info(const vector<DataAndModel*>& dms)
{
    string s;
    const SettingsMgr *sm = F_->settings_mgr();
    vector<realt> alpha = get_covariance_matrix(dms);
    s += "\nCovariance matrix\n    ";
    for (int i = 0; i < na_; ++i)
        if (par_usage_[i])
            s += "\t$" + F_->find_variable_handling_param(i)->name;
    for (int i = 0; i < na_; ++i) {
        if (par_usage_[i]) {
            s += "\n$" + F_->find_variable_handling_param(i)->name;
            for (int j = 0; j < na_; ++j) {
                if (par_usage_[j])
                    s += "\t" + sm->format_double(alpha[na_*i + j]);
            }
        }
    }
    return s;
}

int Fit::compute_deviates(const vector<realt> &A, double *deviates)
{
    ++evaluations_;
    F_->use_external_parameters(A); //that's the only side-effect
    int ntot = 0;
    v_foreach (DataAndModel*, i, dmdm_)
        ntot += compute_deviates_for_data(*i, deviates + ntot);
    return ntot;
}

//static
int Fit::compute_deviates_for_data(const DataAndModel* dm, double *deviates)
{
    const Data* data = dm->data();
    int n = data->get_n();
    vector<realt> xx = data->get_xx();
    vector<realt> yy(n, 0.);
    dm->model()->compute_model(xx, yy);
    for (int j = 0; j < n; ++j)
        deviates[j] = (data->get_y(j) - yy[j]) / data->get_sigma(j);
    return n;
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
    vector<realt> xx = data->get_xx();
    vector<realt> yy(n, 0.);
    dm->model()->compute_model(xx, yy);
    // use here always long double, because it does not effect (much)
    // the efficiency and it notably increases accuracy of results
    // If better accuracy is needed, Kahan summation algorithm could be used.
    long double wssr = 0;
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
    vector<realt> xx = data->get_xx();
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
    assert (size(A) == na_ && size(alpha) == na_ * na_ && size(beta) == na_);
    fill(alpha.begin(), alpha.end(), 0.0);
    fill(beta.begin(), beta.end(), 0.0);

    F_->use_external_parameters(A);
    v_foreach (DataAndModel*, i, dms) {
        compute_derivatives_for(*i, alpha, beta);
    }
    // filling second half of alpha[]
    for (int j = 1; j < na_; j++)
        for (int k = 0; k < j; k++)
            alpha[na_ * k + j] = alpha[na_ * j + k];
}

//results in alpha and beta
//it computes only half of alpha matrix
void Fit::compute_derivatives_for(const DataAndModel* dm,
                                  vector<realt>& alpha, vector<realt>& beta)
{
    const Data* data = dm->data();
    int n = data->get_n();
    vector<realt> xx = data->get_xx();
    vector<realt> yy(n, 0.);
    const int dyn = na_+1;
    vector<realt> dy_da(n*dyn, 0.);
    dm->model()->compute_model_with_derivs(xx, yy, dy_da);
    for (int i = 0; i != n; i++) {
        realt inv_sig = 1.0 / data->get_sigma(i);
        realt dy_sig = (data->get_y(i) - yy[i]) * inv_sig;
        vector<realt>::iterator t = dy_da.begin() + i*dyn;
        for (int j = 0; j != na_; ++j) {
            if (par_usage_[j]) {
                *(t+j) *= inv_sig;
                for (int k = 0; k <= j; ++k)    //half of alpha[]
                    alpha[na_ * j + k] += *(t+j) * *(t+k);
                beta[j] += dy_sig * *(t+j);
            }
        }
    }
}

// similar to compute_derivatives(), but adjusted for MPFIT interface
void Fit::compute_derivatives_mp(const vector<realt> &A,
                                 const vector<DataAndModel*>& dms,
                                 double **derivs, double *deviates)
{
    ++evaluations_;
    F_->use_external_parameters(A);
    int ntot = 0;
    v_foreach (DataAndModel*, i, dms) {
        ntot += compute_derivatives_mp_for(*i, ntot, derivs, deviates);
    }
}

int Fit::compute_derivatives_mp_for(const DataAndModel* dm, int offset,
                                    double **derivs, double *deviates)
{
    const Data* data = dm->data();
    int n = data->get_n();
    vector<realt> xx = data->get_xx();
    vector<realt> yy(n, 0.);
    const int dyn = na_+1;
    vector<realt> dy_da(n*dyn, 0.);
    dm->model()->compute_model_with_derivs(xx, yy, dy_da);
    for (int i = 0; i != n; ++i)
        deviates[offset+i] = (data->get_y(i) - yy[i]) / data->get_sigma(i);
    for (int j = 0; j != na_; ++j)
        if (derivs[j] != NULL)
            for (int i = 0; i != n; ++i)
                derivs[j][offset+i] = -dy_da[i*dyn+j] / data->get_sigma(i);
    return n;
}

// similar to compute_derivatives(), but adjusted for NLopt interface
realt Fit::compute_derivatives_nl(const vector<realt> &A,
                                  const vector<DataAndModel*>& dms,
                                  double *grad)
{
    ++evaluations_;
    F_->use_external_parameters(A);
    realt wssr = 0.;
    fill(grad, grad+na_, 0.0);
    v_foreach (DataAndModel*, i, dms)
        wssr += compute_derivatives_nl_for(*i, grad);
    return wssr;
}

realt Fit::compute_derivatives_nl_for(const DataAndModel* dm, double *grad)
{
    realt wssr = 0;
    const Data* data = dm->data();
    int n = data->get_n();
    vector<realt> xx = data->get_xx();
    vector<realt> yy(n, 0.);
    const int dyn = na_+1;
    vector<realt> dy_da(n*dyn, 0.);
    dm->model()->compute_model_with_derivs(xx, yy, dy_da);
    for (int i = 0; i != n; i++) {
        realt sig = data->get_sigma(i);
        realt dy_sig = (data->get_y(i) - yy[i]) / sig;
        wssr += dy_sig * dy_sig;
        for (int j = 0; j != na_; ++j)
            //if (par_usage_[j])
                grad[j] += -2 * dy_sig * dy_da[i*dyn+j] / sig;
    }
    return wssr;
}

string Fit::print_matrix(const vector<realt>& vec, int m, int n,
                         const char *mname)
    //m rows, n columns
{
    if (F_->get_verbosity() <= 0)  //optimization (?)
        return "";
    assert (size(vec) == m * n);
    soft_assert(!vec.empty());
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
    F_->msg(name + ": " + S(iter_nr_) + " iterations, "
           + S(evaluations_) + " evaluations, "
           + format1<double,16>("%.2f", elapsed) + " s. of CPU time.");
    bool better = (chi2 < wssr_before_);
    const SettingsMgr *sm = F_->settings_mgr();
    if (better) {
        F_->get_fit_container()->push_param_history(aa);
        F_->put_new_parameters(aa);
        double percent_change = (chi2 - wssr_before_) / wssr_before_ * 100.;
        F_->msg("WSSR: " + sm->format_double(chi2) +
                " (" + S(percent_change) + "%)");
    }
    else {
        F_->msg("Better fit NOT found (WSSR = " + sm->format_double(chi2)
                + ", was " + sm->format_double(wssr_before_) + ")."
                "\nParameters NOT changed");
        F_->use_external_parameters(a_orig_);
        if (F_->get_settings()->fit_replot)
            F_->ui()->draw_plot(UserInterface::kRepaintImmediately);
    }
    return better;
}

realt Fit::draw_a_from_distribution (int nr, char distribution, realt mult)
{
    assert (nr >= 0 && nr < na_);
    if (!par_usage_[nr])
        return a_orig_[nr];
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
    ComputeUI(UserInterface *ui) : ui_(ui) { ui->hint_ui(0); }
    ~ComputeUI() { ui_->hint_ui(1); }
private:
    UserInterface *ui_;
};

/// initialize and run fitting procedure for not more than max_iter iterations
void Fit::fit(int max_iter, const vector<DataAndModel*>& dms)
{
    start_time_ = clock();
    last_refresh_time_ = time(0);
    ComputeUI compute_ui(F_->ui());
    update_parameters(dms);
    dmdm_ = dms;
    a_orig_ = F_->parameters();
    F_->get_fit_container()->push_param_history(a_orig_);
    iter_nr_ = 0;
    evaluations_ = 0;
    fityk::user_interrupt = false;
    init(); //method specific init
    max_iterations_ = max_iter;

    // print stats
    int nu = count(par_usage_.begin(), par_usage_.end(), true);
    int np = 0;
    v_foreach (DataAndModel*, i, dms)
        np += (*i)->data()->get_n();
    F_->msg ("Fitting " + S(nu) + " (of " + S(na_) + ") parameters to "
            + S(np) + " points ...");

    autoiter();
}

/// run fitting procedure (without initialization)
void Fit::continue_fit(int max_iter)
{
    start_time_ = clock();
    last_refresh_time_ = time(0);
    v_foreach (DataAndModel*, i, dmdm_)
        if (!F_->contains_dm(*i) || na_ != size(F_->parameters()))
            throw ExecuteError(name + " method should be initialized first.");
    update_parameters(dmdm_);
    a_orig_ = F_->parameters();  //should it be also updated?
    fityk::user_interrupt = false;
    evaluations_ = 0;
    max_iterations_ = max_iter;
    autoiter();
}

void Fit::update_parameters(const vector<DataAndModel*>& dms)
{
    if (F_->parameters().empty())
        throw ExecuteError("there are no fittable parameters.");
    if (dms.empty())
        throw ExecuteError("No datasets to fit.");

    na_ = F_->parameters().size();

    par_usage_ = vector<bool>(na_, false);
    for (int idx = 0; idx < na_; ++idx) {
        int var_idx = F_->find_nr_var_handling_param(idx);
        v_foreach (DataAndModel*, i, dms) {
            if ((*i)->model()->is_dependent_on_var(var_idx)) {
                par_usage_[idx] = true;
                break; //go to next idx
            }
            //vmsg(F_->find_variable_handling_param(idx)->xname
            //        + " is not in chi2.");
        }
    }
    if (count(par_usage_.begin(), par_usage_.end(), true) == 0)
        throw ExecuteError("No parametrized functions are used in the model.");
}

/// checks termination criteria common for all fitting methods
bool Fit::common_termination_criteria(int iter)
{
    bool stop = false;
    if (fityk::user_interrupt) {
        F_->msg ("Fitting stopped manually.");
        stop = true;
    }
    if (max_iterations_ >= 0 && iter >= max_iterations_) {
        F_->msg("Maximum iteration number reached.");
        stop = true;
    }
    int max_eval = F_->get_settings()->max_wssr_evaluations;
    if (max_eval > 0 && evaluations_ >= max_eval) {
        F_->msg("Maximum evaluations number reached.");
        stop = true;
    }
    double max_time = F_->get_settings()->max_fitting_time;
    if (max_time > 0 && clock() >= start_time_ + max_time * CLOCKS_PER_SEC) {
        F_->msg("Maximum processor time exceeded.");
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
        F_->ui()->draw_plot(UserInterface::kRepaintImmediately);
    }
    double elapsed = (clock() - start_time_) / (double) CLOCKS_PER_SEC;
    double percent_change = (wssr - wssr_before_) / wssr_before_ * 100.;
    int max_eval = F_->get_settings()->max_wssr_evaluations;
    F_->msg("Iter: " + S(iter_nr_) + "/"
            + (max_iterations_ > 0 ? S(max_iterations_) : string("oo"))
            + "  Eval: " + S(evaluations_) + "/"
            + (max_eval > 0 ? S(max_eval) : string("oo"))
            + "  WSSR=" + F_->settings_mgr()->format_double(wssr)
            + " (" + S(percent_change)+ "%)"
            + "  CPU time: " + format1<double,16>("%.2f", elapsed) + "s.");
    F_->ui()->hint_ui(-1);
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
                    throw ExecuteError("In iteration " + S(iter_nr_)
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


void Fit::output_tried_parameters(const vector<realt>& a)
{
    const SettingsMgr *sm = F_->settings_mgr();
    string s = "Trying ( ";
    s.reserve(s.size() + a.size() * 12); // rough guess
    v_foreach (realt, j, a)
        s += sm->format_double(*j) + (j+1 == a.end() ? " )" : ", ");
    F_->vmsg(s);
}

//-------------------------------------------------------------------

// this list should be kept sync with to FitMethodsContainer ctor.
const char* fit_method_enum[] = {
    "levenberg_marquardt", // LMfit.cpp
    "mpfit",               // MPfit.cpp
    "nelder_mead_simplex", // NMfit.cpp
    "genetic_algorithms",  // GAfit.cpp
#if HAVE_LIBNLOPT
    "nlopt_mma",           // NLfit.cpp
    "nlopt_slsqp",         // NLfit.cpp
    "nlopt_lbfgs",         // NLfit.cpp
    "nlopt_var2",          // NLfit.cpp
    "nlopt_cobyla",        // NLfit.cpp
    "nlopt_bobyqa",        // NLfit.cpp
    "nlopt_nm",            // NLfit.cpp
    "nlopt_sbplx",         // NLfit.cpp
#endif
    NULL
};

FitMethodsContainer::FitMethodsContainer(Ftk *F_)
    : ParameterHistoryMgr(F_), dirty_error_cache_(true)

{
    // these methods correspond to fit_method_enum[]
    methods_.push_back(new LMfit(F_, fit_method_enum[0]));
    methods_.push_back(new MPfit(F_, fit_method_enum[1]));
    methods_.push_back(new NMfit(F_, fit_method_enum[2]));
    methods_.push_back(new GAfit(F_, fit_method_enum[3]));
#if HAVE_LIBNLOPT
    methods_.push_back(new NLfit(F_, fit_method_enum[4], NLOPT_LD_MMA));
    methods_.push_back(new NLfit(F_, fit_method_enum[5], NLOPT_LD_SLSQP));
    methods_.push_back(new NLfit(F_, fit_method_enum[6], NLOPT_LD_LBFGS));
    methods_.push_back(new NLfit(F_, fit_method_enum[7], NLOPT_LD_VAR2));
    methods_.push_back(new NLfit(F_, fit_method_enum[8], NLOPT_LN_COBYLA));
    methods_.push_back(new NLfit(F_, fit_method_enum[9], NLOPT_LN_BOBYQA));
    methods_.push_back(new NLfit(F_, fit_method_enum[10], NLOPT_LN_NELDERMEAD));
    methods_.push_back(new NLfit(F_, fit_method_enum[11], NLOPT_LN_SBPLX));
#endif
}


const char* FitMethodsContainer::full_method_names[][2] =
{
    { "Levenberg-Marquardt",     "gradient based method" },
    { "MPFIT (another Lev-Mar)", "alternative Lev-Mar implementation" },
    { "Nelder-Mead Simplex",     "slow but simple and reliable method" },
    { "Genetic Algorithm",       "not really maintained" },
#if HAVE_LIBNLOPT
    { "MMA (from NLopt)",        "Method of Moving Asymptotes" },
    { "SLSQP (from NLopt)",      "sequential quadratic programming" },
    { "BFGS (from NLopt)",       "L-BFGS" },
    { "VAR2 (from NLopt)",       "shifted limited-memory variable-metric" },
    { "COBYLA (from NLopt)",     "Constrained Optimization BY Linear Approx." },
    { "BOBYQA (from NLopt)",     "Bound Optimization BY Quadratic Approx." },
    { "Nelder-Mead (from NLopt)","Nelder-Mead Simplex" },
    { "Sbplx (from NLopt)",      "(based on Subplex)" },
#endif
    { NULL, NULL }
};

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



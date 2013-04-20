// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#define BUILDING_LIBFITYK
#include "fit.h"

#include <algorithm>
#include <math.h>
#include <string.h>
#include <boost/math/distributions/students_t.hpp>

#include "logic.h"
#include "model.h"
#include "data.h"
#include "ui.h"
#include "numfuncs.h"
#include "settings.h"
#include "var.h"
#include "LMfit.h"
#include "CMPfit.h"
#include "GAfit.h"
#include "NMfit.h"
#include "NLfit.h"

using namespace std;

namespace fityk {

int count_points(const vector<DataAndModel*>& dms)
{
    int n = 0;
    v_foreach (DataAndModel*, i, dms)
        n += (*i)->data()->get_n();
    return n;
}

Fit::Fit(Ftk *F, const string& m)
    : name(m), F_(F),
      evaluations_(0), na_(0), last_refresh_time_(0)
{
}

/// dof = degrees of freedom = (number of points - number of parameters)
int Fit::get_dof(const vector<DataAndModel*>& dms)
{
    update_par_usage(dms);
    int used_parameters = count(par_usage_.begin(), par_usage_.end(), true);
    return count_points(dms) - used_parameters;
}

string Fit::get_goodness_info(const vector<DataAndModel*>& dms)
{
    const SettingsMgr *sm = F_->settings_mgr();
    const vector<realt>& pp = F_->mgr.parameters();
    int dof = get_dof(dms);
    //update_par_usage(dms);
    realt wssr = compute_wssr(pp, dms, true);
    return "WSSR=" + sm->format_double(wssr)
           + "  DoF=" + S(dof)
           + "  WSSR/DoF=" + sm->format_double(wssr/dof)
           + "  SSR=" + sm->format_double(compute_wssr(pp, dms, false))
           + "  R2=" + sm->format_double(compute_r_squared(pp, dms));
}

vector<realt> Fit::get_covariance_matrix(const vector<DataAndModel*>& dms)
{
    update_par_usage(dms);
    vector<realt> alpha(na_*na_, 0.);

    // temporarily there are two ways of calculating errors, just for testing
    MPfit* mpfit = dynamic_cast<MPfit*>(this);
    if (mpfit) {
        double *covar = mpfit->get_covar(dms);
        alpha.assign(covar, covar + alpha.size());
        delete [] covar;
    }
    else {
        vector<realt> beta(na_);
        compute_derivatives(F_->mgr.parameters(), dms, alpha, beta);

        // To avoid singular matrix, put fake values corresponding to unused
        // parameters.
        for (int i = 0; i < na_; ++i)
            if (!par_usage_[i]) {
                alpha[i*na_ + i] = 1.;
            }
        // We may have unused parameters with par_usage_[] set true,
        // e.g. SplitGaussian with center < min(active x)  has hwhm1 unused.
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
    }

    return alpha;
}

vector<realt> Fit::get_standard_errors(const vector<DataAndModel*>& dms)
{
    const vector<realt> &pp = F_->mgr.parameters();
    realt wssr = compute_wssr(pp, dms, true);
    int dof = get_dof(dms);
    // `na_' was set by get_dof() above, from update_par_usage()
    vector<realt> errors(na_);

    // temporarily there are two ways of calculating errors, just for testing
    MPfit* mpfit = dynamic_cast<MPfit*>(this);
    if (mpfit) {
        double *perror = mpfit->get_errors(dms);
        for (int i = 0; i < na_; ++i)
            errors[i] = sqrt(wssr / dof) * perror[i];
        delete [] perror;
    }
    else {
        vector<realt> alpha = get_covariance_matrix(dms);
        // `na_' was set by functions above
        for (int i = 0; i < na_; ++i)
            errors[i] = sqrt(wssr / dof * alpha[i*na_ + i]);
    }
    return errors;
}

vector<realt> Fit::get_confidence_limits(const vector<DataAndModel*>& dms,
                                         double level_percent)
{
    vector<realt> v = get_standard_errors(dms);
    int dof = get_dof(dms);
    double level = 1. - level_percent / 100.;
#if 1
    // for unknown reasons this crashes when the program is run under valgrind
    // (Boost 1.50, Valgrind 3.8.1, Fedora 18) with message:
    // terminate called after throwing an instance of 'boost::exception_detail::clone_impl<boost::exception_detail::error_info_injector<std::overflow_error> >'
    //   what():  Error in function boost::math::erfc_inv<e>(e, e): Overflow Error
    boost::math::students_t dist(dof);
    double t = boost::math::quantile(boost::math::complement(dist, level/2));
#else
    double t = 0;
#endif
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
            s += "\t$" + F_->mgr.find_variable_handling_param(i)->name;
    for (int i = 0; i < na_; ++i) {
        if (par_usage_[i]) {
            s += "\n$" + F_->mgr.find_variable_handling_param(i)->name;
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
    F_->mgr.use_external_parameters(A); //that's the only side-effect
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


realt Fit::compute_wssr(const vector<realt> &A,
                        const vector<DataAndModel*>& dms,
                        bool weigthed)
{
    realt wssr = 0;
    F_->mgr.use_external_parameters(A); //that's the only side-effect
    v_foreach (DataAndModel*, i, dms) {
        wssr += compute_wssr_for_data(*i, weigthed);
    }
    ++evaluations_;
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
    // using long double, because it does not effect (much) the efficiency
    // and notably increases the accuracy of WSSR.
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
    F_->mgr.use_external_parameters(A);
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

    F_->mgr.use_external_parameters(A);
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
    F_->mgr.use_external_parameters(A);
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
    F_->mgr.use_external_parameters(A);
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
    return F_->mgr.variation_of_a(nr, dv * mult);
}

class ComputeUI
{
public:
    ComputeUI(UserInterface *ui) : ui_(ui) { ui->hint_ui(0); }
    ~ComputeUI() { ui_->hint_ui(1); }
private:
    UserInterface *ui_;
};

/// initialize and run fitting procedure for not more than max_eval evaluations
void Fit::fit(int max_eval, const vector<DataAndModel*>& dms)
{
    // initialization
    start_time_ = clock();
    last_refresh_time_ = time(0);
    ComputeUI compute_ui(F_->ui());
    update_par_usage(dms);
    dmdm_ = dms;
    a_orig_ = F_->mgr.parameters();
    F_->get_fit_container()->push_param_history(a_orig_);
    evaluations_ = 0;
    fityk::user_interrupt = false;
    max_eval_ = (max_eval > 0 ? max_eval
                              : F_->get_settings()->max_wssr_evaluations);
    int nu = count(par_usage_.begin(), par_usage_.end(), true);
    F_->msg("Fitting " + S(nu) + " (of " + S(na_) + ") parameters to "
            + S(count_points(dms)) + " points ...");
    initial_wssr_ = compute_wssr(a_orig_, dmdm_);
    best_shown_wssr_ = initial_wssr_;
    const SettingsMgr *sm = F_->settings_mgr();
    if (F_->get_verbosity() >= 1)
        F_->ui()->mesg("Method: " + name + ". Initial WSSR="
                       + sm->format_double(initial_wssr_));

    // here the work is done
    vector<realt> best_a;
    realt wssr = run_method(&best_a);

    // finalization
    F_->msg(name + ": " + S(evaluations_) + " evaluations, "
            + format1<double,16>("%.2f", elapsed()) + " s. of CPU time.");
    if (wssr < initial_wssr_) {
        F_->get_fit_container()->push_param_history(best_a);
        F_->mgr.put_new_parameters(best_a);
        double percent_change = (wssr - initial_wssr_) / initial_wssr_ * 100.;
        F_->msg("WSSR: " + sm->format_double(wssr) +
                " (" + S(percent_change) + "%)");
    }
    else {
        F_->msg("Better fit NOT found (WSSR = " + sm->format_double(wssr)
                + ", was " + sm->format_double(initial_wssr_) + ")."
                "\nParameters NOT changed");
        F_->mgr.use_external_parameters(a_orig_);
        if (F_->get_settings()->fit_replot)
            F_->ui()->draw_plot(UserInterface::kRepaintImmediately);
    }
}

// sets na_ and par_usage_ based on F_->mgr and dms
void Fit::update_par_usage(const vector<DataAndModel*>& dms)
{
    if (F_->mgr.parameters().empty())
        throw ExecuteError("there are no fittable parameters.");
    if (dms.empty())
        throw ExecuteError("No datasets to fit.");

    na_ = F_->mgr.parameters().size();

    par_usage_ = vector<bool>(na_, false);
    for (int idx = 0; idx < na_; ++idx) {
        int var_idx = F_->mgr.find_nr_var_handling_param(idx);
        v_foreach (DataAndModel*, i, dms) {
            if ((*i)->model()->is_dependent_on_var(var_idx)) {
                par_usage_[idx] = true;
                break; //go to next idx
            }
        }
    }
    if (count(par_usage_.begin(), par_usage_.end(), true) == 0)
        throw ExecuteError("No parametrized functions are used in the model.");
}

/// checks termination criteria common for all fitting methods
bool Fit::common_termination_criteria() const
{
    bool stop = false;
    if (fityk::user_interrupt) {
        F_->msg ("Fitting stopped manually.");
        stop = true;
    }
    double max_time = F_->get_settings()->max_fitting_time;
    if (max_time > 0 && elapsed() >= max_time) {
        F_->msg("Maximum processor time exceeded.");
        stop = true;
    }
    if (max_eval_ > 0 && evaluations_ >= max_eval_) {
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
        F_->mgr.use_external_parameters(A);
        F_->ui()->draw_plot(UserInterface::kRepaintImmediately);
    }
    F_->msg(iteration_info(wssr) +
            "  CPU time: " + format1<double,16>("%.2f", elapsed()) + "s.");
    F_->ui()->hint_ui(-1);
    last_refresh_time_ = time(0);
}


void Fit::output_tried_parameters(const vector<realt>& a)
{
    const SettingsMgr *sm = F_->settings_mgr();
    string s = "Trying ( ";
    s.reserve(s.size() + a.size() * 12); // rough guess
    v_foreach (realt, j, a)
        s += sm->format_double(*j) + (j+1 == a.end() ? " )" : ", ");
    F_->ui()->mesg(s);
}

double Fit::elapsed() const
{
    return (clock() - start_time_) / (double) CLOCKS_PER_SEC;
}

string Fit::iteration_info(realt wssr)
{
    const SettingsMgr *sm = F_->settings_mgr();
    double last_change = (best_shown_wssr_ - wssr) / best_shown_wssr_ * 100;
    double total_change = (initial_wssr_ - wssr) / initial_wssr_ * 100;
    string first_char = " ";
    if (wssr < best_shown_wssr_) {
        best_shown_wssr_ = wssr;
        first_char = "*";
    }
    return first_char + " eval: " + S(evaluations_) +
           "/" + (max_eval_ > 0 ? S(max_eval_) : string("oo")) +
           "  WSSR=" + sm->format_double(wssr) +
           format1<double,32>("  (%+.3g%%,", last_change) +
           format1<double,32>(" total %+.3g%%)", total_change) +
           format1<double,16>("  CPU: %.2fs.", elapsed());
}

//-------------------------------------------------------------------

// keep sync with to FitMethodsContainer ctor and full_method_names
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
    "nlopt_crs2",          // NLfit.cpp
    "nlopt_praxis",        // NLfit.cpp
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
    methods_.push_back(new NLfit(F_, fit_method_enum[12], NLOPT_GN_CRS2_LM));
    methods_.push_back(new NLfit(F_, fit_method_enum[13], NLOPT_LN_PRAXIS));
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
    { "CRS2 (from NLopt)",       "Controlled Random Search" },
    { "PRAXIS (from NLopt)",     "principal-axis method" },
#endif
    { NULL, NULL }
};

FitMethodsContainer::~FitMethodsContainer()
{
    purge_all_elements(methods_);
}

Fit* FitMethodsContainer::get_method(const string& name) const
{
    v_foreach(Fit*, i, methods_)
        if ((*i)->name == name)
            return *i;
    throw ExecuteError("fitting method `" + name + "' not available.");
    return NULL; // avoid compiler warning
}

realt FitMethodsContainer::get_standard_error(const Variable* var) const
{
    if (!var->is_simple())
        return -1.; // value signaling unknown standard error
    if (dirty_error_cache_
            || errors_cache_.size() != F_->mgr.parameters().size()) {
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
    if (item_nr == -1 && relative && !param_history_.empty() && //undo
            param_history_[param_hist_ptr_] != F_->mgr.parameters())
        item_nr = 0; // load parameters from param_hist_ptr_
    if (relative)
        item_nr += param_hist_ptr_;
    else if (item_nr < 0)
        item_nr += param_history_.size();
    if (item_nr < 0 || item_nr >= size(param_history_))
        throw ExecuteError("There is no parameter history item #"
                            + S(item_nr) + ".");
    F_->mgr.put_new_parameters(param_history_[item_nr]);
    param_hist_ptr_ = item_nr;
}

bool ParameterHistoryMgr::can_undo() const
{
    return !param_history_.empty()
        && (param_hist_ptr_ > 0 || param_history_[0] != F_->mgr.parameters());
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

} // namespace fityk

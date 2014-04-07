// This file is part of fityk program. Copyright 2001-2013 Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#define BUILDING_LIBFITYK
#include "fit.h"

#include <algorithm>
#include <cmath>
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

int count_points(const vector<Data*>& datas)
{
    int n = 0;
    v_foreach (Data*, i, datas)
        n += (*i)->get_n();
    return n;
}

Fit::Fit(Full *F, const string& m)
    : name(m), F_(F),
      evaluations_(0), na_(0), last_refresh_time_(0)
{
}

/// dof = degrees of freedom = (number of points - number of parameters)
int Fit::get_dof(const vector<Data*>& datas)
{
    update_par_usage(datas);
    int used_parameters = count(par_usage_.begin(), par_usage_.end(), true);
    return count_points(datas) - used_parameters;
}

string Fit::get_goodness_info(const vector<Data*>& datas)
{
    const SettingsMgr *sm = F_->settings_mgr();
    const vector<realt>& pp = F_->mgr.parameters();
    int dof = get_dof(datas);
    //update_par_usage(datas);
    realt wssr = compute_wssr(pp, datas, true);
    return "WSSR=" + sm->format_double(wssr)
           + "  DoF=" + S(dof)
           + "  WSSR/DoF=" + sm->format_double(wssr/dof)
           + "  SSR=" + sm->format_double(compute_wssr(pp, datas, false))
           + "  R2=" + sm->format_double(compute_r_squared(pp, datas));
}

vector<double> Fit::get_covariance_matrix(const vector<Data*>& datas)
{
    return MPfit(F_, "").get_covariance_matrix(datas);
}

vector<double> Fit::get_standard_errors(const vector<Data*>& datas)
{
    return MPfit(F_, "").get_standard_errors(datas);
}

vector<double> Fit::get_confidence_limits(const vector<Data*>& datas,
                                          double level_percent)
{
    vector<double> v = get_standard_errors(datas);
    int dof = get_dof(datas);
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
    vm_foreach (double, i, v)
        *i *= t;
    return v;
}

string Fit::get_cov_info(const vector<Data*>& datas)
{
    string s;
    const SettingsMgr *sm = F_->settings_mgr();
    vector<double> alpha = get_covariance_matrix(datas);
    s += "\nCovariance matrix\n    ";
    for (int i = 0; i < na_; ++i)
        if (par_usage_[i])
            s += "\t$" + F_->mgr.gpos_to_var(i)->name;
    for (int i = 0; i < na_; ++i) {
        if (par_usage_[i]) {
            s += "\n$" + F_->mgr.gpos_to_var(i)->name;
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
    v_foreach (Data*, i, fitted_datas_)
        ntot += compute_deviates_for_data(*i, deviates + ntot);
    return ntot;
}

//static
int Fit::compute_deviates_for_data(const Data* data, double *deviates)
{
    int n = data->get_n();
    vector<realt> xx = data->get_xx();
    vector<realt> yy(n, 0.);
    data->model()->compute_model(xx, yy);
    for (int j = 0; j < n; ++j)
        deviates[j] = (data->get_y(j) - yy[j]) / data->get_sigma(j);
    return n;
}


realt Fit::compute_wssr(const vector<realt> &A,
                        const vector<Data*>& datas,
                        bool weigthed)
{
    realt wssr = 0;
    F_->mgr.use_external_parameters(A); //that's the only side-effect
    v_foreach (Data*, i, datas) {
        wssr += compute_wssr_for_data(*i, weigthed);
    }
    ++evaluations_;
    return wssr;
}

//static
realt Fit::compute_wssr_for_data(const Data* data, bool weigthed)
{
    int n = data->get_n();
    vector<realt> xx = data->get_xx();
    vector<realt> yy(n, 0.);
    data->model()->compute_model(xx, yy);
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
                             const vector<Data*>& datas)
{
    realt sum_err = 0, sum_tot = 0, se = 0, st = 0;
    F_->mgr.use_external_parameters(A);
    v_foreach (Data*, i, datas) {
        compute_r_squared_for_data(*i, &se, &st);
        sum_err += se;
        sum_tot += st;
    }
    return 1 - (sum_err / sum_tot);
}

//static
realt Fit::compute_r_squared_for_data(const Data* data,
                                      realt* sum_err, realt* sum_tot)
{
    int n = data->get_n();
    vector<realt> xx = data->get_xx();
    vector<realt> yy(n, 0.);
    data->model()->compute_model(xx, yy);
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
                              const vector<Data*>& datas,
                              vector<realt>& alpha, vector<realt>& beta)
{
    assert (size(A) == na_ && size(alpha) == na_ * na_ && size(beta) == na_);
    fill(alpha.begin(), alpha.end(), 0.0);
    fill(beta.begin(), beta.end(), 0.0);

    F_->mgr.use_external_parameters(A);
    v_foreach (Data*, i, datas) {
        compute_derivatives_for(*i, alpha, beta);
    }
    // filling second half of alpha[]
    for (int j = 1; j < na_; j++)
        for (int k = 0; k < j; k++)
            alpha[na_ * k + j] = alpha[na_ * j + k];
}

//results in alpha and beta
//it computes only half of alpha matrix
void Fit::compute_derivatives_for(const Data* data,
                                  vector<realt>& alpha, vector<realt>& beta)
{
    int n = data->get_n();
    vector<realt> xx = data->get_xx();
    vector<realt> yy(n, 0.);
    const int dyn = na_+1;
    vector<realt> dy_da(n*dyn, 0.);
    data->model()->compute_model_with_derivs(xx, yy, dy_da);
    for (int i = 0; i != n; ++i) {
        realt inv_sig = 1.0 / data->get_sigma(i);
        realt dy_sig = (data->get_y(i) - yy[i]) * inv_sig;
        vector<realt>::iterator t = dy_da.begin() + i*dyn;
        // The program spends here a lot of time.
        // Testing on GCC 4.8 with -O3 on x64 i7 processor:
        //  the first loop (j) is faster when iterating upward,
        //  and the other one (k) is faster downward.
        // Removing par_usage_ only slows down this loop (!?).
        for (int j = 0; j != na_; ++j) {
            if (par_usage_[j] && *(t+j) != 0) {
                *(t+j) *= inv_sig;
                for (int k = j; k != -1; --k)    //half of alpha[]
                    alpha[na_ * j + k] += *(t+j) * *(t+k);
                beta[j] += dy_sig * *(t+j);
            }
        }
    }
}

// similar to compute_derivatives(), but adjusted for MPFIT interface
void Fit::compute_derivatives_mp(const vector<realt> &A,
                                 const vector<Data*>& datas,
                                 double **derivs, double *deviates)
{
    ++evaluations_;
    F_->mgr.use_external_parameters(A);
    int ntot = 0;
    v_foreach (Data*, i, datas) {
        ntot += compute_derivatives_mp_for(*i, ntot, derivs, deviates);
    }
}

int Fit::compute_derivatives_mp_for(const Data* data, int offset,
                                    double **derivs, double *deviates)
{
    int n = data->get_n();
    vector<realt> xx = data->get_xx();
    vector<realt> yy(n, 0.);
    const int dyn = na_+1;
    vector<realt> dy_da(n*dyn, 0.);
    data->model()->compute_model_with_derivs(xx, yy, dy_da);
    for (int i = 0; i != n; ++i)
        deviates[offset+i] = (data->get_y(i) - yy[i]) / data->get_sigma(i);
    for (int j = 0; j != na_; ++j)
        if (derivs[j] != NULL)
            for (int i = 0; i != n; ++i)
                derivs[j][offset+i] = -dy_da[i*dyn+j] / data->get_sigma(i);
    return n;
}

// similar to compute_derivatives(), but adjusted for NLopt interface
realt Fit::compute_wssr_gradient(const vector<realt> &A,
                                 const vector<Data*>& datas,
                                 double *grad)
{
    assert(size(A) == na_);
    ++evaluations_;
    F_->mgr.use_external_parameters(A);
    realt wssr = 0.;
    fill(grad, grad+na_, 0.0);
    v_foreach (Data*, i, datas)
        wssr += compute_wssr_gradient_for(*i, grad);
    return wssr;
}

realt Fit::compute_wssr_gradient_for(const Data* data, double *grad)
{
    realt wssr = 0;
    int n = data->get_n();
    vector<realt> xx = data->get_xx();
    vector<realt> yy(n, 0.);
    const int dyn = na_+1;
    vector<realt> dy_da(n*dyn, 0.);
    data->model()->compute_model_with_derivs(xx, yy, dy_da);
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

realt Fit::draw_a_from_distribution(int gpos, char distribution, realt mult)
{
    assert (gpos >= 0 && gpos < na_);
    if (!par_usage_[gpos])
        return a_orig_[gpos];
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
    return F_->mgr.variation_of_a(gpos, dv * mult);
}

class ComputeUI
{
public:
    ComputeUI(UserInterface *ui) : ui_(ui) { ui->hint_ui("busy", "1"); }
    ~ComputeUI() { ui_->hint_ui("busy", ""); }
private:
    UserInterface *ui_;
};

/// initialize and run fitting procedure for not more than max_eval evaluations
void Fit::fit(int max_eval, const vector<Data*>& datas)
{
    // initialization
    start_time_ = clock();
    last_refresh_time_ = time(0);
    ComputeUI compute_ui(F_->ui());
    update_par_usage(datas);
    fitted_datas_ = datas;
    a_orig_ = F_->mgr.parameters();
    F_->fit_manager()->push_param_history(a_orig_);
    evaluations_ = 0;
    fityk::user_interrupt = false;
    max_eval_ = (max_eval > 0 ? max_eval
                              : F_->get_settings()->max_wssr_evaluations);
    int nu = count(par_usage_.begin(), par_usage_.end(), true);
    F_->msg("Fitting " + S(nu) + " (of " + S(na_) + ") parameters to "
            + S(count_points(datas)) + " points ...");
    initial_wssr_ = compute_wssr(a_orig_, fitted_datas_);
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
        F_->fit_manager()->push_param_history(best_a);
        F_->mgr.put_new_parameters(best_a);
        double percent_change = (wssr - initial_wssr_) / initial_wssr_ * 100.;
        F_->msg("WSSR: " + sm->format_double(wssr) +
                " (" + S(percent_change) + "%)");
    } else {
        F_->msg("Better fit NOT found (WSSR = " + sm->format_double(wssr)
                + ", was " + sm->format_double(initial_wssr_) + ")."
                "\nParameters NOT changed");
        F_->mgr.use_external_parameters(a_orig_);
        if (F_->get_settings()->fit_replot)
            F_->ui()->draw_plot(UserInterface::kRepaintImmediately);
    }
}

// sets na_ and par_usage_ based on F_->mgr and datas
void Fit::update_par_usage(const vector<Data*>& datas)
{
    if (F_->mgr.parameters().empty())
        throw ExecuteError("there are no fittable parameters.");
    if (datas.empty())
        throw ExecuteError("No datasets to fit.");

    na_ = F_->mgr.parameters().size();

    par_usage_ = vector<bool>(na_, false);
    for (int idx = 0; idx < na_; ++idx) {
        int var_idx = F_->mgr.gpos_to_vpos(idx);
        v_foreach (Data*, i, datas) {
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
    F_->ui()->hint_ui("yield", "");
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

// keep sync with to FitManager ctor
const char* FitManager::method_list[][3] =
{
 { "mpfit", "Lev-Mar (from MPFIT)", "Levenberg-Marquardt" },
 { "levenberg_marquardt", "Lev-Mar (own)", "Levenberg-Marquardt" },
#if HAVE_LIBNLOPT
 { "nlopt_nm", "Nelder-Mead (from NLopt)","Nelder-Mead Simplex" },
 { "nlopt_lbfgs", "BFGS (from NLopt)", "L-BFGS" },
 { "nlopt_var2", "VAR2 (from NLopt)",
                               "shifted limited-memory variable-metric" },
 { "nlopt_praxis", "PRAXIS (from NLopt)", "principal-axis method" },
 { "nlopt_bobyqa", "BOBYQA (from NLopt)",
                               "Bound Optimization BY Quadratic Approx." },
 { "nlopt_sbplx", "Sbplx (from NLopt)", "(based on Subplex)" },
 //{ "nlopt_crs2", "CRS2 (from NLopt)", "Controlled Random Search" },
 //{ "nlopt_slsqp", "SLSQP (from NLopt)", "sequential quadratic programming" },
 //{ "nlopt_mma", "MMA (from NLopt)", "Method of Moving Asymptotes" },
 //{ "nlopt_cobyla", "COBYLA (from NLopt)",
 //                            "Constrained Optimization BY Linear Approx." },
#endif
 { "nelder_mead_simplex", "Nelder-Mead Simplex", "(own implementation)" },
 { "genetic_algorithms", "Genetic Algorithm", "(not really maintained)" },
 { NULL, NULL }
};


FitManager::FitManager(Full *F_)
    : ParameterHistoryMgr(F_), dirty_error_cache_(true)

{
    // these methods correspond to method_list[]
    methods_.push_back(new MPfit(F_, method_list[0][0]));
    methods_.push_back(new LMfit(F_, method_list[1][0]));
#if HAVE_LIBNLOPT
    methods_.push_back(new NLfit(F_, method_list[2][0], NLOPT_LN_NELDERMEAD));
    methods_.push_back(new NLfit(F_, method_list[3][0], NLOPT_LD_LBFGS));
    methods_.push_back(new NLfit(F_, method_list[4][0], NLOPT_LD_VAR2));
    methods_.push_back(new NLfit(F_, method_list[5][0], NLOPT_LN_PRAXIS));
    methods_.push_back(new NLfit(F_, method_list[6][0], NLOPT_LN_BOBYQA));
    methods_.push_back(new NLfit(F_, method_list[7][0], NLOPT_LN_SBPLX));
    //methods_.push_back(new NLfit(F_, method_list[8][0], NLOPT_LD_MMA));
    //methods_.push_back(new NLfit(F_, method_list[9][0], NLOPT_LD_SLSQP));
    //methods_.push_back(new NLfit(F_, method_list[10][0], NLOPT_LN_COBYLA));
    //methods_.push_back(new NLfit(F_, method_list[11][0], NLOPT_GN_CRS2_LM));
#endif
    methods_.push_back(new NMfit(F_, method_list[methods_.size()][0]));
    methods_.push_back(new GAfit(F_, method_list[methods_.size()][0]));
}


FitManager::~FitManager()
{
    purge_all_elements(methods_);
}

Fit* FitManager::get_method(const string& name) const
{
    v_foreach(Fit*, i, methods_)
        if ((*i)->name == name)
            return *i;
    throw ExecuteError("fitting method `" + name + "' not available.");
    return NULL; // avoid compiler warning
}

double FitManager::get_standard_error(const Variable* var) const
{
    if (!var->is_simple())
        return -1.; // value signaling unknown standard error
    if (dirty_error_cache_
            || errors_cache_.size() != F_->mgr.parameters().size()) {
        errors_cache_ = F_->get_fit()->get_standard_errors(F_->dk.datas());
    }
    return errors_cache_[var->gpos()];
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
    } else
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

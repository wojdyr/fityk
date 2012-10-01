// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#ifndef FITYK__FIT__H__
#define FITYK__FIT__H__
#include <vector>
#include <map>
#include <string>
#include <time.h>
#include "common.h"

namespace fityk {

class DataAndModel;
class Ftk;
class Variable;

///   interface of fitting method and implementation of common functions
class Fit
{
public:
    const std::string name;

    Fit(Ftk *F, const std::string& m);
    virtual ~Fit() {};
    void fit(int max_iter, const std::vector<DataAndModel*>& dms);
    void continue_fit(int max_iter);
    bool can_continue() const;
    std::string get_goodness_info(const std::vector<DataAndModel*>& dms);
    int get_dof(const std::vector<DataAndModel*>& dms);
    std::string get_cov_info(const std::vector<DataAndModel*>& dms);
    std::vector<realt>
        get_covariance_matrix(const std::vector<DataAndModel*>& dms);
    std::vector<realt>
        get_standard_errors(const std::vector<DataAndModel*>& dms);
    std::vector<realt>
        get_confidence_limits(const std::vector<DataAndModel*>& dms,
                              double level_percent);
    const std::vector<DataAndModel*>& get_last_dm() const { return dmdm_; }
    static realt compute_wssr_for_data (const DataAndModel* dm, bool weigthed);
    static int compute_deviates_for_data(const DataAndModel* dm,
                                         double *deviates);
    // called from GUI
    realt do_compute_wssr(const std::vector<realt> &A,
                         const std::vector<DataAndModel*>& dms,
                         bool weigthed);
    static realt compute_r_squared_for_data(const DataAndModel* dm,
                                           realt* sum_err, realt* sum_tot);
    void Jordan(std::vector<realt>& A, std::vector<realt>& b, int n);
    void reverse_matrix (std::vector<realt>&A, int n);
    // pretty-print matrix m x n stored in vec. `mname' is name/comment.
    std::string print_matrix(const std::vector<realt>& vec,
                             int m, int n, const char *mname);
    realt compute_r_squared(const std::vector<realt> &A,
                           const std::vector<DataAndModel*>& dms);
    bool is_param_used(int n) const { return par_usage_[n]; }
protected:
    Ftk *F_;
    std::vector<DataAndModel*> dmdm_;
    int evaluations_;
    int max_iterations_; //it is set before calling autoiter()
    int iter_nr_;
    realt wssr_before_;
    std::vector<realt> a_orig_;
    std::vector<bool> par_usage_;
    int na_; ///number of fitted parameters

    virtual void init() = 0; // called before autoiter()
    virtual void autoiter() = 0;
    bool common_termination_criteria(int iter, bool all=true);
    void compute_derivatives(const std::vector<realt> &A,
                          const std::vector<DataAndModel*>& dms,
                          std::vector<realt>& alpha, std::vector<realt>& beta);
    void compute_derivatives_mp(const std::vector<realt> &A,
                                const std::vector<DataAndModel*>& dms,
                                double **derivs, double *deviates);
    realt compute_derivatives_nl(const std::vector<realt> &A,
                                 const std::vector<DataAndModel*>& dms,
                                 double *grad);
    realt compute_wssr(const std::vector<realt> &A,
                    const std::vector<DataAndModel*>& dms, bool weigthed=true)
        { ++evaluations_; return do_compute_wssr(A, dms, weigthed); }
    int compute_deviates(const std::vector<realt> &A, double *deviates);
    bool post_fit(const std::vector<realt>& aa, realt chi2);
    realt draw_a_from_distribution(int nr, char distribution = 'u',
                                   realt mult = 1.);
    void iteration_plot(const std::vector<realt> &A, realt wssr);
    void output_tried_parameters(const std::vector<realt>& a);
private:
    time_t last_refresh_time_;
    clock_t start_time_;

    void compute_derivatives_for(const DataAndModel *dm,
                                 std::vector<realt>& alpha,
                                 std::vector<realt>& beta);
    int compute_derivatives_mp_for(const DataAndModel* dm, int offset,
                                   double **derivs, double *deviates);
    realt compute_derivatives_nl_for(const DataAndModel* dm, double *grad);
    void update_parameters(const std::vector<DataAndModel*>& dms);
};

/// handles parameter history
class ParameterHistoryMgr
{
public:
    ParameterHistoryMgr(Ftk *F) : F_(F), param_hist_ptr_(0) {}
    bool push_param_history(const std::vector<realt>& aa);
    void clear_param_history() { param_history_.clear(); param_hist_ptr_ = 0; }
    int get_param_history_size() const { return param_history_.size(); }
    void load_param_history(int item_nr, bool relative);
    bool has_param_history_rel_item(int rel_nr) const
        { return is_index(param_hist_ptr_ + rel_nr, param_history_); }
    bool can_undo() const;
    std::string param_history_info() const;
    const std::vector<realt>& get_item(int n) const {return param_history_[n];}
    int get_active_nr() const { return param_hist_ptr_; }
protected:
    Ftk *F_;
private:
    std::vector<std::vector<realt> > param_history_; /// old parameter vectors
    int param_hist_ptr_; /// points to the current/last parameter vector
};

/// gives access to fitting methods, enables swithing between them
/// also stores parameter history
class FitMethodsContainer : public ParameterHistoryMgr
{
public:
    static const char* full_method_names[][2];
    FitMethodsContainer(Ftk *F_);
    ~FitMethodsContainer();
    Fit* get_method(int n) const
                    { assert(n >= 0 && n<size(methods_)); return methods_[n]; }
    const std::vector<Fit*>& methods() const { return methods_; }
    realt get_standard_error(const Variable* var) const;
    void outdated_error_cache() { dirty_error_cache_ = true; }

private:
    std::vector<Fit*> methods_;
    mutable std::vector<realt> errors_cache_;
    bool dirty_error_cache_;

    DISALLOW_COPY_AND_ASSIGN(FitMethodsContainer);
};

extern const char* fit_method_enum[]; // used in settings.cpp

} // namespace fityk
#endif


// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#ifndef FITYK__FIT__H__
#define FITYK__FIT__H__
#include <vector>
#include <map>
#include <string>
#include <time.h>
#include "common.h"


class DataAndModel;
class Ftk;
class Variable;

///   interface of fitting method and implementation of common functions
class Fit
{
public:
    std::string const name;

    Fit(Ftk *F_, std::string const& m);
    virtual ~Fit() {};
    void fit(int max_iter, std::vector<DataAndModel*> const& dms);
    void continue_fit(int max_iter);
    bool is_initialized() const { return !dmdm_.empty(); }
    bool is_initialized(std::vector<DataAndModel*> const& dms) const
                                                    { return dms == dmdm_; }
    std::string get_goodness_info(std::vector<DataAndModel*> const& dms);
    int get_dof(std::vector<DataAndModel*> const& dms);
    std::string get_error_info(std::vector<DataAndModel*> const& dms);
    std::string get_cov_info(std::vector<DataAndModel*> const& dms);
    std::vector<fp>
        get_covariance_matrix(std::vector<DataAndModel*> const& dms);
    std::vector<fp>
        get_standard_errors(std::vector<DataAndModel*> const& dms);
    //std::vector<DataAndModel*> const& get_datsums() const { return dmdm_; }
    static fp compute_wssr_for_data (DataAndModel const* dm, bool weigthed);
    fp do_compute_wssr(std::vector<fp> const &A,
                       std::vector<DataAndModel*> const& dms,
                       bool weigthed);
    static fp compute_r_squared_for_data(DataAndModel const* dm,
                                         fp* sum_err, fp* sum_tot);
    void Jordan (std::vector<fp>& A, std::vector<fp>& b, int n);
    void reverse_matrix (std::vector<fp>&A, int n);
    // pretty-print matrix m x n stored in vec. `mname' is name/comment.
    std::string print_matrix (const std::vector<fp>& vec,
                                     int m, int n, const char *mname);
    fp compute_r_squared(std::vector<fp> const &A,
                         std::vector<DataAndModel*> const& dms);
    bool is_param_used(int n) const { return par_usage[n]; }
protected:
    Ftk *F;
    std::vector<DataAndModel*> dmdm_;
    int evaluations;
    int max_evaluations_;
    int max_iterations; //it is set before calling autoiter()
    int iter_nr;
    fp wssr_before;
    std::vector<fp> a_orig;
    std::vector<bool> par_usage;
    int na; ///number of fitted parameters

    virtual fp init() = 0; // called before autoiter()
    virtual void autoiter() = 0;
    bool common_termination_criteria(int iter);
    void compute_derivatives(std::vector<fp> const &A,
                             std::vector<DataAndModel*> const& dms,
                             std::vector<fp>& alpha, std::vector<fp>& beta);
    fp compute_wssr(std::vector<fp> const &A,
                    std::vector<DataAndModel*> const& dms, bool weigthed=true)
        { ++evaluations; return do_compute_wssr(A, dms, weigthed); }
    bool post_fit (const std::vector<fp>& aa, fp chi2);
    fp draw_a_from_distribution (int nr, char distribution = 'u', fp mult = 1.);
    void iteration_plot(std::vector<fp> const &A, fp wssr);
private:
    time_t last_refresh_time_;
    clock_t start_time_;

    void compute_derivatives_for(DataAndModel const *dm,
                                 std::vector<fp>& alpha, std::vector<fp>& beta);
    void update_parameters(std::vector<DataAndModel*> const& dms);
};

/// handles parameter history
class ParameterHistoryMgr
{
public:
    ParameterHistoryMgr(Ftk *F_) : F(F_), param_hist_ptr(0) {}
    bool push_param_history(std::vector<fp> const& aa);
    void clear_param_history() { param_history.clear(); param_hist_ptr = 0; }
    int get_param_history_size() const { return param_history.size(); }
    void load_param_history(int item_nr, bool relative);
    bool has_param_history_rel_item(int rel_nr) const
        { return is_index(param_hist_ptr + rel_nr, param_history); }
    bool can_undo() const;
    std::string param_history_info() const;
    std::vector<fp> const& get_item(int n) const { return param_history[n]; }
    int get_active_nr() const { return param_hist_ptr; }
protected:
    Ftk *F;
private:
    std::vector<std::vector<fp> > param_history; /// old parameter vectors
    int param_hist_ptr; /// points to the current/last parameter vector
};

/// gives access to fitting methods, enables swithing between them
/// also stores parameter history
class FitMethodsContainer : public ParameterHistoryMgr
{
public:
    FitMethodsContainer(Ftk *F_);
    ~FitMethodsContainer();
    Fit* get_method(int n) const
                    { assert(n >= 0 && n<size(methods_)); return methods_[n]; }
    const std::vector<Fit*>& methods() const { return methods_; }
    fp get_standard_error(const Variable* var) const;
    void outdated_error_cache() { dirty_error_cache_ = true; }

private:
    std::vector<Fit*> methods_;
    mutable std::vector<fp> errors_cache_;
    bool dirty_error_cache_;

    DISALLOW_COPY_AND_ASSIGN(FitMethodsContainer);
};

#endif


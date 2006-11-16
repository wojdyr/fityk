// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#ifndef FITYK__FIT__H__
#define FITYK__FIT__H__
#include <vector>
#include <map>
#include <string>
#include "common.h"


class DataWithSum;

///   interface of fitting method and implementation of common functions
class Fit 
{               
public:
    std::string const name;

    Fit(std::string m);
    virtual ~Fit() {};
    void fit(int max_iter, std::vector<DataWithSum*> const& dsds);
    void continue_fit(int max_iter);
    bool is_initialized(DataWithSum const* ds=0) const
     { return !ds && !datsums.empty() || datsums.size()==1 && ds==datsums[0]; }
    bool is_initialized(std::vector<DataWithSum*> const& dsds) const
                                                    { return dsds == datsums; }
    std::string getInfo(std::vector<DataWithSum*> const& dsds);
    std::string getErrorInfo(std::vector<DataWithSum*> const& dsds, 
                             bool matrix=false);
    std::vector<fp> get_covariance_matrix(std::vector<DataWithSum*> const&dsds);
    std::vector<fp> get_symmetric_errors(std::vector<DataWithSum*> const& dsds);
    std::vector<DataWithSum*> const& get_datsums() const { return datsums; }
    static fp compute_wssr_for_data (DataWithSum const* ds, bool weigthed);
    static fp do_compute_wssr(std::vector<fp> const &A, 
                   std::vector<DataWithSum*> const& dsds, bool weigthed);
    static fp compute_r_squared_for_data(DataWithSum const* ds) ;
    static bool Jordan (std::vector<fp>& A, std::vector<fp>& b, int n); 
    static void reverse_matrix (std::vector<fp>&A, int n);
    static std::string print_matrix (const std::vector<fp>& vec, 
                                     int m, int n, char *mname);//m x n
protected:
    std::vector<DataWithSum*> datsums;
    int evaluations;
    int max_iterations; //it is set before calling autoiter()
    int iter_nr;
    fp wssr_before;
    std::vector<fp> a_orig;
    std::vector<bool> par_usage;
    int na; //number of fitted parameters
    std::map<char, std::string> Distrib_enum;

    virtual fp init() = 0; // called before autoiter()
    virtual void autoiter() = 0;
    bool common_termination_criteria(int iter);
    fp compute_wssr(std::vector<fp> const &A, 
                    std::vector<DataWithSum*> const& dsds, bool weigthed=true)
        { ++evaluations; return do_compute_wssr(A, dsds, weigthed); }
    fp compute_r_squared(std::vector<fp> const &A, std::vector<DataWithSum*> const& dsds) ;
    void compute_derivatives(std::vector<fp> const &A, 
                             std::vector<DataWithSum*> const& dsds, 
                             std::vector<fp>& alpha, std::vector<fp>& beta);
    bool post_fit (const std::vector<fp>& aa, fp chi2);
    fp draw_a_from_distribution (int nr, char distribution = 'u', fp mult = 1.);
    void iteration_plot(std::vector<fp> const &A);
private:
    void compute_derivatives_for(DataWithSum const *ds, 
                                 std::vector<fp>& alpha, std::vector<fp>& beta);
    void update_parameters(std::vector<DataWithSum*> const& dss);
};

/// handles parameter history
class ParameterHistoryMgr
{
public:
    ParameterHistoryMgr() : param_hist_ptr(0) {}
    bool push_param_history(std::vector<fp> const& aa);
    void clear_param_history() { param_history.clear(); param_hist_ptr = 0; }
    int get_param_history_size() const { return param_history.size(); }
    void load_param_history(int item_nr, bool relative=false);
    bool has_param_history_rel_item(int rel_nr) const 
        { return is_index(param_hist_ptr + rel_nr, param_history); }
    bool can_undo() const; 
    std::string param_history_info() const;
    std::vector<fp> const& get_item(int n) const { return param_history[n]; }
    int get_active_nr() const { return param_hist_ptr; }
private:
    std::vector<std::vector<fp> > param_history; /// old parameter vectors
    int param_hist_ptr; /// points to the current/last parameter vector
};

/// singleton, gives access to fitting methods, enables swithing between them
/// also stores parameter history
class FitMethodsContainer : public ParameterHistoryMgr
{
public:
    static FitMethodsContainer* getInstance();
    int current_method_number() const;
    Fit const* get_method(int n) const
                    { assert(n >= 0 && n<size(methods)); return methods[n]; }
    std::vector<Fit*> const& get_methods() const { return methods; }
    Fit* getFit() { return methods[current_method_number()]; }

private:
    static FitMethodsContainer* instance;
    std::vector<Fit*> methods;

    FitMethodsContainer();
    FitMethodsContainer (FitMethodsContainer const&); //disable
    ~FitMethodsContainer();
};

inline Fit* getFit() { return FitMethodsContainer::getInstance()->getFit(); }

#endif


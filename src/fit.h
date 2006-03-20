// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#ifndef FITYK__FIT__H__
#define FITYK__FIT__H__
#include <vector>
#include <map>
#include <string>
#include "common.h"


class DataWithSum;

//     generic fit class interface
class Fit //: public DotSet
{               
public:
    std::string const name;
    int default_max_iterations;

    Fit(std::string m);
    virtual ~Fit() {};
    void fit(int max_iter, std::vector<DataWithSum*> const& dsds_);
    void continue_fit(int max_iter);
    std::string getInfo();
    std::string getErrorInfo(bool matrix=false);
    int get_default_max_iter() { return default_max_iterations; }
    static fp compute_wssr_for_data (DataWithSum const* ds, bool weigthed);
    static int Jordan (std::vector<fp>& A, std::vector<fp>& b, int n); 
    static int reverse_matrix (std::vector<fp>&A, int n);
    static std::string print_matrix (const std::vector<fp>& vec, 
                                     int m, int n, char *mname);//m x n
protected:
    std::vector<DataWithSum*> dsds;
    int output_one_of;
    int max_evaluations;
    int evaluations;
    int max_iterations; //it is set before calling autoiter()
    int iter_nr;
    fp wssr_before;
    std::vector<fp> a_orig;
    int na; //number of fitted parameters
    std::map<char, std::string> Distrib_enum;

    virtual fp init() = 0; // called before autoiter()
    virtual int autoiter() = 0;
    bool common_termination_criteria(int iter);
    fp compute_wssr (std::vector<fp> const &A, bool weigthed=true);
    void compute_derivatives(std::vector<fp> const &A, 
                             std::vector<fp>& alpha, std::vector<fp>& beta);
    bool post_fit (const std::vector<fp>& aa, fp chi2);
    fp draw_a_from_distribution (int nr, char distribution = 'u', fp mult = 1.);
    void iteration_plot(std::vector<fp> const &A);
private:
    void compute_derivatives_for(DataWithSum const *ds, 
                                 std::vector<fp>& alpha, std::vector<fp>& beta);
};

///singleton
class FitMethodsContainer
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


// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#ifndef v_fit__h__
#define v_fit__h__
#include <vector>
#include <map>
#include <string>
#include "common.h"
#include "dotset.h"

//     generic fit class interface
class v_fit : public DotSet
{               
public:
    const char symbol;
    const std::string method;
    int default_max_iterations;

    v_fit (char symb, std::string m);
    virtual ~v_fit () {};
    void fit (bool ini, int max_iter);
    std::string getInfo (int mode);
    int get_default_max_iter() { return default_max_iterations; }
    static fp compute_wssr_for_data (const Data* data, const Sum* sum, 
                                     bool weigthed);
    static int Jordan (std::vector<fp>& A, std::vector<fp>& b, int n); 
    static int reverse_matrix (std::vector<fp>&A, int n);
    static std::string print_matrix (const std::vector<fp>& vec, 
                                     int m, int n, char *name);//m x n
protected:
    int output_one_of;
    int random_seed;
    int max_evaluations;
    int evaluations;
    int max_iterations; //it is set before calling autoiter()
    int iter_nr;
    fp wssr_before;
    std::vector<fp> a_orig;
    int na; //number of fitted parameters
    int nf; //number of frozen parameters
    std::map<char, std::string> Distrib_enum;

    virtual fp init() = 0; // called before autoiter()
    virtual int autoiter () = 0;
    bool common_termination_criteria(int iter);
    fp compute_wssr (const std::vector<fp>& A = fp_v0, bool weigthed = true);
    void compute_derivatives(const std::vector<fp>& A, 
                             std::vector<fp>& alpha, std::vector<fp>& beta);
    bool post_fit (const std::vector<fp>& aa, fp chi2);
    fp draw_a_from_distribution (int nr, char distribution = 'u', fp mult = 1.);
    void iteration_plot (const std::vector<fp>& a);
private:
    std::vector<fp> fitted2all (const std::vector<fp>& A);
    int fitted2all (int nr);
    std::vector<fp> all2fitted (const std::vector<fp>& A);
    void compute_derivatives_for(const Data* data, const Sum *sum,
                                 const std::vector<fp>& A, 
                                 std::vector<fp>& alpha, std::vector<fp>& beta);
};

class FitMethodsContainer
{
public:
    FitMethodsContainer();
    ~FitMethodsContainer();
    std::string list_available_methods();
    void export_methods_settings_as_script (std::ostream& os);
    void change_method (char c);
    std::string print_current_method();
    int current_method_number();
    char symbol(int n) { assert(n < size(methods)); return methods[n]->symbol; }
private:
    std::vector<v_fit*> methods;
};

inline fp rand_1_1 () { return 2.0 * rand() / RAND_MAX - 1.; }
inline fp rand_0_1 () { return static_cast<fp>(rand()) / RAND_MAX; }
inline bool rand_bool () { return rand() < RAND_MAX / 2; }
fp rand_gauss();
fp rand_cauchy();

extern v_fit *my_fit;
extern FitMethodsContainer *fitMethodsContainer;

#endif


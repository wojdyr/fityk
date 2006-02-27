// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$
#ifndef FITYK__FUNC__H__
#define FITYK__FUNC__H__

#include <map>
#include "var.h"


class Function : public VariableUser
{
    static int unnamed_counter;
public:
    struct Multi { 
        int p; int n; fp mult; 
        Multi(int n_, Variable::ParMult const&pm): p(pm.p),n(n_),mult(pm.mult){}
    };
    std::string const type_formula; //eg. Gaussian(a,b,c) = a*(...)
    std::string const type_name;
    std::vector<std::string> const type_var_names;
    std::vector<std::string> const type_var_eq;
    std::string const type_rhs;
    int const nv;
    fp cutoff_level;

    Function(std::string const &name_, std::vector<std::string> const &vars,
             std::string const &formula_);
    virtual ~Function() {}
    static Function* factory(std::string const &name_, 
                             std::string const &type_name,
                             std::vector<std::string> const &vars);
    static std::vector<std::string> get_all_types();
    static std::string get_formula(std::string const& type);
    static std::string next_auto_name() { return "_" + S(++unnamed_counter); }

    static std::map<std::string, std::string> default_variables;
    static std::string get_typename_from_formula(std::string const &formula)
     {return strip_string(std::string(formula, 0, formula.find_first_of("(")));}
    static std::string get_rhs_from_formula(std::string const &formula)
     { return strip_string(std::string(formula, formula.rfind('=')+1)); }
    static std::vector<std::string> 
      get_varnames_from_formula(std::string const &formula, bool get_eq=false);

    virtual void calculate_value(std::vector<fp> const &x, 
                                 std::vector<fp> &y) const = 0; 
    virtual void calculate_value_deriv(std::vector<fp> const &x, 
                                       std::vector<fp> &y, 
                                       std::vector<fp> &dy_da,
                                       bool in_dx=false) const  = 0; 
    virtual void do_precomputations(std::vector<Variable*> const &variables); 
    void erased_parameter(int k);
    fp calculate_value(fp x) const; ///wrapper around array version
    /// calculate function value assuming function parameters has given values
    void calculate_values_with_params(std::vector<fp> const& x, 
                                      std::vector<fp>& y,
                                      std::vector<fp> const& alt_vv) const; 
    virtual bool get_nonzero_range(fp/*level*/, fp&/*left*/, fp&/*right*/) const
                                                              { return false; }
    void get_nonzero_idx_range(std::vector<fp> const &x, 
                               int &first, int &last) const;
                           
    //true if FWHM, height, center and area of the function can be calculated
    virtual bool is_peak() const { return false; } 
    virtual fp center() const { return center_idx==-1 ? 0. : vv[center_idx]; }
    virtual bool has_center() const {return this->is_peak() || center_idx!=-1;}
    virtual fp height() const { return 0; }
    virtual fp fwhm() const { return 0; }
    virtual fp area() const { return 0; }
    fp get_var_value(int n) const 
             { assert(n>=0 && n<size(vv)); return vv[n]; }
    std::string get_info(std::vector<Variable*> const &variables, 
                    std::vector<fp> const &parameters, 
                    bool extended=false) const;
    std::string get_current_definition(std::vector<Variable*> const &variables, 
                                       std::vector<fp> const &parameters) const;
    std::string get_current_formula(std::string const& x = "x") const;
    int find_param_nr(std::string const& param) const;
protected:
    int const center_idx;
    std::vector<fp> vv; /// current variable values
    std::vector<Multi> multi;
private:
    static std::vector<fp> calc_val_xx, calc_val_yy;
};

//////////////////////////////////////////////////////////////////////////

// a new class can be derived from class-derived-from-Function,
// but it should use the first constructor (with formula)
#define DECLARE_FUNC_OBLIGATORY_METHODS(NAME) \
    friend class Function;\
protected:\
    Func##NAME (std::string const &name, std::vector<std::string> const &vars,\
                std::string const &formula_) \
        : Function(name, vars, formula_) {}\
private:\
    Func##NAME (std::string const &name, std::vector<std::string> const &vars)\
        : Function(name, vars, formula) {}\
    Func##NAME (const Func##NAME&); \
public:\
    static const char *formula; \
    void calculate_value(std::vector<fp> const &xx, std::vector<fp> &yy) const;\
    void calculate_value_deriv(std::vector<fp> const &xx, \
                               std::vector<fp> &yy, std::vector<fp> &dy_da,\
                               bool in_dx=false) const; 

class FuncConstant : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(Constant)
};
    
class FuncLinear : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(Linear)
};

class FuncQuadratic : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(Quadratic)
};

class FuncCubic : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(Cubic)
};

class FuncPolynomial4 : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(Polynomial4)
};

class FuncPolynomial5 : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(Polynomial5)
};


class FuncPolynomial6 : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(Polynomial6)
};

class FuncGaussian : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(Gaussian)
    void do_precomputations(std::vector<Variable*> const &variables);
    bool get_nonzero_range (fp level, fp &left, fp &right) const;  
    bool is_peak() const { return true; } 
    fp center() const { return vv[1]; }
    fp height() const { return vv[0]; }
    fp fwhm() const   { return 2 * fabs(vv[2]); }
    fp area() const   { return vv[0] * fabs(vv[2]) * sqrt(M_PI / M_LN2); }
};

class FuncSplitGaussian : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(SplitGaussian)
    void do_precomputations(std::vector<Variable*> const &variables);
    bool get_nonzero_range (fp level, fp &left, fp &right) const;  
    bool is_peak() const { return true; } 
    fp center() const { return vv[1]; }
    fp height() const { return vv[0]; }
    fp fwhm() const   { return fabs(vv[2]) + fabs(vv[3]); }
    fp area() const   { return vv[0] * (fabs(vv[2]) + fabs(vv[3])) 
                                     * sqrt(M_PI/M_LN2); }
};

class FuncLorentzian : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(Lorentzian)
    void do_precomputations(std::vector<Variable*> const &variables);
    bool get_nonzero_range (fp level, fp &left, fp &right) const;  
    bool is_peak() const { return true; } 
    fp center() const { return vv[1]; }
    fp height() const { return vv[0]; }
    fp fwhm() const   { return 2 * fabs(vv[2]); }
    fp area() const   { return vv[0] * fabs(vv[2]) * M_PI; }
};

class FuncPearson7 : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(Pearson7)
    void do_precomputations(std::vector<Variable*> const &variables); 
    bool get_nonzero_range (fp level, fp &left, fp &right) const;  
    bool is_peak() const { return true; } 
    fp center() const { return vv[1]; }
    fp height() const { return vv[0]; }
    fp fwhm() const   { return 2 * fabs(vv[2]); }
    fp area() const;
};

class FuncSplitPearson7 : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(SplitPearson7)
    void do_precomputations(std::vector<Variable*> const &variables); 
    bool get_nonzero_range (fp level, fp &left, fp &right) const;  
    bool is_peak() const { return true; } 
    fp center() const { return vv[1]; }
    fp height() const { return vv[0]; }
    fp fwhm() const   { return fabs(vv[2]) + fabs(vv[3]); }
    fp area() const;
};

class FuncPseudoVoigt : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(PseudoVoigt)
    void do_precomputations(std::vector<Variable*> const &variables); 
    bool get_nonzero_range (fp level, fp &left, fp &right) const;  
    bool is_peak() const { return true; } 
    fp center() const { return vv[1]; }
    fp height() const { return vv[0]; }
    fp fwhm() const   { return 2 * fabs(vv[2]); }
    fp area() const { return vv[0] * fabs(vv[2]) 
                      * ((vv[3] * M_PI) + (1 - vv[3]) * sqrt (M_PI / M_LN2)); }
};

class FuncVoigt : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(Voigt)
    void do_precomputations(std::vector<Variable*> const &variables); 
    bool get_nonzero_range (fp level, fp &left, fp &right) const;  
    bool is_peak() const { return true; } 
    fp center() const { return vv[1]; }
    fp height() const { return vv[0]; }
    fp fwhm() const;
    fp area() const;
};

class FuncEMG : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(EMG)
    void do_precomputations(std::vector<Variable*> const &variables); 
    bool get_nonzero_range (fp level, fp &left, fp &right) const;  
    bool is_peak() const { return false; } 
    bool has_center() const { return true; }
    fp center() const { return vv[1]; }
};

class FuncDoniachSunjic : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(DoniachSunjic)
    bool get_nonzero_range (fp level, fp &left, fp &right) const;  
    bool is_peak() const { return false; } 
    bool has_center() const { return true; }
    fp center() const { return vv[3]; }
};

class FuncPielaszekCube : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(PielaszekCube)
    bool is_peak() const { return false; } 
    fp center() const { return vv[1]; }
};

class FuncValente : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(Valente)
    bool is_peak() const { return false; } 
    fp center() const { return vv[3]; }
};



#endif 


// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$
#ifndef FITYK__FUNC__H__
#define FITYK__FUNC__H__

#include <map>
#include "var.h"

class Settings;

class Function : public VariableUser
{
public:
    struct Multi { 
        int p; int n; fp mult; 
        Multi(int n_, Variable::ParMult const&pm): p(pm.p),n(n_),mult(pm.mult){}
    };
    std::string const type_formula; //eg. Gaussian(a,b,c) = a*(...)
    std::string const type_name;
    std::vector<std::string> const type_var_names;
    std::string const type_rhs;
    int const nv;

    Function(std::string const &name_, std::vector<std::string> const &vars,
             std::string const &formula_);
    static Function* factory(std::string const &name_, 
                             std::string const &type_name,
                             std::vector<std::string> const &vars);
    static std::vector<std::string> get_all_types();
    static std::string get_formula(std::string const& type);

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
                           
    virtual bool has_center() const { return center_idx != -1; }
    virtual fp center() const { return center_idx==-1 ? 0. : vv[center_idx]; }
    virtual bool has_height() const { return false; } 
    virtual fp height() const { return 0; }
    virtual bool has_fwhm() const { return false; } 
    virtual fp fwhm() const { return 0; }
    virtual bool has_area() const { return false; } 
    virtual fp area() const { return 0; }
    bool has_iwidth() const { return this->has_area() && this->has_height(); }
    fp iwidth() const { fp h=this->height(); return h ? this->area()/h : 0.; }
    fp get_var_value(int n) const 
             { assert(n>=0 && n<size(vv)); return vv[n]; }
    std::string get_info(std::vector<Variable*> const &variables, 
                    std::vector<fp> const &parameters, 
                    bool extended=false) const;
    std::string get_current_definition(std::vector<Variable*> const &variables, 
                                       std::vector<fp> const &parameters) const;
    virtual std::string get_current_formula(std::string const& x = "x") const;
    int get_param_nr(std::string const& param) const;
    fp get_param_value(std::string const& param) const;
    fp numarea(fp x1, fp x2, int nsteps) const;
    fp find_x_with_value(fp x1, fp x2, fp val, 
                         fp xacc=1e-9, int max_iter=1000) const;
    fp find_extremum(fp x1, fp x2, fp xacc=1e-9, int max_iter=1000) const;
protected:
    Settings *settings;
    int const center_idx;
    std::vector<fp> vv; /// current variable values
    std::vector<Multi> multi;
private:
    static std::vector<fp> calc_val_xx, calc_val_yy;
};

//////////////////////////////////////////////////////////////////////////

/// Function which definition is based on other function(s)
class CompoundFunction: public Function
{
    friend class Function;
public:
    /// checks partially the definition and puts formula into formulae
    static void define(std::string const &formula);
    /// removes the definition from formulae
    static void undefine(std::string const &type);
    static bool is_defined(std::string const &type, bool only_udf=false);
    static std::string const& get_formula(std::string const& type);
    static std::vector<std::string> const& get_formulae() { return formulae; }

    void do_precomputations(std::vector<Variable*> const &variables_);
    void calculate_value(std::vector<fp> const &xx, std::vector<fp> &yy) const;
    void calculate_value_deriv(std::vector<fp> const &xx, 
                               std::vector<fp> &yy, std::vector<fp> &dy_da,
                               bool in_dx=false) const;
    std::string get_current_formula(std::string const& x = "x") const;
    bool has_center() const;
    fp center() const { return vmgr.get_function(0)->center(); }
    bool has_height() const;
    fp height() const;
    bool has_fwhm() const;
    fp fwhm() const;
    bool has_area() const;
    fp area() const;
    bool get_nonzero_range(fp level, fp& left, fp& right) const;
private:
    static std::vector<std::string> formulae; 
    static const int harddef_count = 3;
    
    VariableManager vmgr;

    CompoundFunction(std::string const &name, std::string const &type,
                     std::vector<std::string> const &vars);
    CompoundFunction (const CompoundFunction&); //disable
    static void check_rhs_function(std::string const &fun,
                                   std::vector<std::string> const& lhs_vars);
    static 
    std::vector<std::string> get_rhs_components(std::string const &formula);
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

//////////////////////////////////////////////////////////////////////////

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
    fp center() const { return vv[1]; }
    bool has_height() const { return true; } 
    fp height() const { return vv[0]; }
    bool has_fwhm() const { return true; } 
    fp fwhm() const   { return 2 * fabs(vv[2]); }
    bool has_area() const { return true; } 
    fp area() const   { return vv[0] * fabs(vv[2]) * sqrt(M_PI / M_LN2); }
};

class FuncSplitGaussian : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(SplitGaussian)
    void do_precomputations(std::vector<Variable*> const &variables);
    bool get_nonzero_range (fp level, fp &left, fp &right) const;  
    fp center() const { return vv[1]; }
    bool has_height() const { return true; } 
    fp height() const { return vv[0]; }
    bool has_fwhm() const { return true; } 
    fp fwhm() const   { return fabs(vv[2]) + fabs(vv[3]); }
    bool has_area() const { return true; } 
    fp area() const   { return vv[0] * (fabs(vv[2]) + fabs(vv[3])) 
                                     * sqrt(M_PI/M_LN2); }
};

class FuncLorentzian : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(Lorentzian)
    void do_precomputations(std::vector<Variable*> const &variables);
    bool get_nonzero_range (fp level, fp &left, fp &right) const;  
    fp center() const { return vv[1]; }
    bool has_height() const { return true; } 
    fp height() const { return vv[0]; }
    bool has_fwhm() const { return true; } 
    fp fwhm() const   { return 2 * fabs(vv[2]); }
    bool has_area() const { return true; } 
    fp area() const   { return vv[0] * fabs(vv[2]) * M_PI; }
};

class FuncPearson7 : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(Pearson7)
    void do_precomputations(std::vector<Variable*> const &variables); 
    bool get_nonzero_range (fp level, fp &left, fp &right) const;  
    fp center() const { return vv[1]; }
    bool has_height() const { return true; } 
    fp height() const { return vv[0]; }
    bool has_fwhm() const { return true; } 
    fp fwhm() const   { return 2 * fabs(vv[2]); }
    bool has_area() const { return vv[3] > 0.5; } 
    fp area() const;
};

class FuncSplitPearson7 : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(SplitPearson7)
    void do_precomputations(std::vector<Variable*> const &variables); 
    bool get_nonzero_range (fp level, fp &left, fp &right) const;  
    fp center() const { return vv[1]; }
    bool has_height() const { return true; } 
    fp height() const { return vv[0]; }
    bool has_fwhm() const { return true; } 
    fp fwhm() const   { return fabs(vv[2]) + fabs(vv[3]); }
    bool has_area() const { return vv[4] > 0.5 && vv[5] > 0.5; } 
    fp area() const;
};

class FuncPseudoVoigt : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(PseudoVoigt)
    void do_precomputations(std::vector<Variable*> const &variables); 
    bool get_nonzero_range (fp level, fp &left, fp &right) const;  
    fp center() const { return vv[1]; }
    bool has_height() const { return true; } 
    fp height() const { return vv[0]; }
    bool has_fwhm() const { return true; } 
    fp fwhm() const   { return 2 * fabs(vv[2]); }
    bool has_area() const { return true; } 
    fp area() const { return vv[0] * fabs(vv[2]) 
                      * ((vv[3] * M_PI) + (1 - vv[3]) * sqrt (M_PI / M_LN2)); }
};

class FuncVoigt : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(Voigt)
    void do_precomputations(std::vector<Variable*> const &variables); 
    bool get_nonzero_range (fp level, fp &left, fp &right) const;  
    fp center() const { return vv[1]; }
    bool has_height() const { return true; } 
    fp height() const { return vv[0]; }
    bool has_fwhm() const { return true; } 
    fp fwhm() const;
    bool has_area() const { return true; } 
    fp area() const;
};

class FuncVoigtA : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(VoigtA)
    void do_precomputations(std::vector<Variable*> const &variables); 
    bool get_nonzero_range (fp level, fp &left, fp &right) const;  
    fp center() const { return vv[1]; }
    bool has_height() const { return true; } 
    fp height() const; 
    bool has_fwhm() const { return true; } 
    fp fwhm() const;
    bool has_area() const { return true; } 
    fp area() const { return vv[0]; }
};

class FuncEMG : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(EMG)
    void do_precomputations(std::vector<Variable*> const &variables); 
    bool get_nonzero_range (fp level, fp &left, fp &right) const;  
    bool has_center() const { return true; }
    fp center() const { return vv[1]; }
};

class FuncDoniachSunjic : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(DoniachSunjic)
    bool get_nonzero_range (fp level, fp &left, fp &right) const;  
    bool has_center() const { return true; }
    fp center() const { return vv[3]; }
};

class FuncPielaszekCube : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(PielaszekCube)
    fp center() const { return vv[1]; }
};

class FuncValente : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(Valente)
    fp center() const { return vv[3]; }
};



#endif 


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
    const std::string type_formula; //eg. Gaussian(a,b,c) = a*(...)
    const std::string type_name;
    const std::vector<std::string> type_var_names;
    const std::vector<std::string> type_var_eq;
    const std::string type_rhs;
    const int nv;
    fp cutoff_level;

    Function(std::string const &name_, std::vector<std::string> const &vars,
             std::string const &formula_);
    virtual ~Function() {}
    static Function* factory(std::string const &name_, 
                             std::string const &type_name,
                             std::vector<std::string> const &vars);
    static std::vector<std::string> get_all_types();
    static std::string get_formula(std::string const& type);
    static std::string next_auto_name() { return "fun" + S(++unnamed_counter); }

    static bool statics_initialized;
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
    fp calculate_value(fp x) const; ///wrapper around array version
    virtual bool get_nonzero_range(fp/*level*/, fp&/*left*/, fp&/*right*/) const
                                                              { return false; }
    void get_nonzero_idx_range(std::vector<fp> const &x, 
                               int &first, int &last) const;
                           
    virtual bool is_peak() const { return false; } 
    virtual bool knows_peak_properties() const { return false; }
    virtual fp center() const { return 0; }
    virtual fp height() const { return 0; }
    virtual fp fwhm() const { return 0; }
    virtual fp area() const { return 0; }
    std::vector<fp> const& get_var_values() const { return vv; }
    std::string get_info(std::vector<Variable*> const &variables, 
                    std::vector<fp> const &parameters, 
                    bool extended=false) const;
    std::string get_current_formula(std::string const& x = "x") const;
protected:
    std::vector<fp> vv; /// current variable values
    std::vector<Multi> multi;
private:
    static std::vector<fp> calc_val_xx, calc_val_yy;
};

//////////////////////////////////////////////////////////////////////////

#define DECLARE_FUNC_OBLIGATORY_METHODS(NAME) \
    Func##NAME (std::string const &name, std::vector<std::string> const &vars)\
        : Function(name, vars, formula) {}\
    Func##NAME (const Func##NAME&); \
    friend class Function;\
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

class FuncGaussian : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(Gaussian)
    void do_precomputations(std::vector<Variable*> const &variables);
    bool get_nonzero_range (fp level, fp &left, fp &right) const;  
    bool is_peak() const { return true; } 
    bool knows_peak_properties() const { return true; }
    fp center() const { return vv[0]; }
    fp height() const { return vv[1]; }
    fp fwhm() const   { return 2 * fabs(vv[2]); }
    fp area() const   { return vv[0] * fabs(vv[2]) * sqrt(M_PI / M_LN2); }
};

class FuncLorentzian : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(Lorentzian)
    void do_precomputations(std::vector<Variable*> const &variables);
    bool get_nonzero_range (fp level, fp &left, fp &right) const;  
    bool is_peak() const { return true; } 
    bool knows_peak_properties() const { return true; }
    fp center() const { return vv[0]; }
    fp height() const { return vv[1]; }
    fp fwhm() const   { return 2 * fabs(vv[2]); }
    fp area() const   { return vv[0] * fabs(vv[2]) * M_PI; }
};

class FuncPearson7 : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(Pearson7)
    void do_precomputations(std::vector<Variable*> const &variables); 
    bool get_nonzero_range (fp level, fp &left, fp &right) const;  
    bool is_peak() const { return true; } 
    bool knows_peak_properties() const { return true; }
    fp center() const { return vv[0]; }
    fp height() const { return vv[1]; }
    fp fwhm() const   { return 2 * fabs(vv[2]); }
    fp area() const;
};

class FuncPielaszekCube : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(PielaszekCube)
    bool is_peak() const { return true; } 
};

class FuncValente : public Function
{
    DECLARE_FUNC_OBLIGATORY_METHODS(Valente)
    bool is_peak() const { return true; } 
};



#endif 


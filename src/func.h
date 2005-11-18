// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$
#ifndef FUNCS__H__
#define FUNCS__H__

#include <map>
#include <var.h>


class Function : public VariableUser
{
public:
    struct Multi { 
        int p; int n; fp mult; 
        Multi(int n_, Variable::ParMult const&pm): p(pm.p),n(n_),mult(pm.mult){}
    };
    const std::string type_formula; //eg. Gaussian(a,b,c) = a*(...)
    const std::string type_name;
    const std::string type_rhs;
    fp cutoff_level;

    Function(std::string const &name_, std::vector<std::string> const &vars,
             std::string const &formula_, VariableManager *m);
    virtual ~Function() {}
    static Function* factory(std::string const &name, 
                             std::string const &type_name,
                             std::vector<std::string> const &vars,
                             VariableManager *m);

    static bool statics_initialized;
    static std::map<std::string, std::string> default_variables;

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
    std::vector<std::string> const& get_type_var_names() const 
                                               { return type_var_names; }
    std::string get_info(std::vector<Variable*> const &variables, 
                    std::vector<fp> const &parameters, 
                    bool extended=false) const;
    std::string get_current_formula(std::string const& x = "x") const;
protected:
    std::vector<std::string> type_var_names;
    std::vector<fp> vv; /// current variable values
    std::vector<Multi> multi;
    VariableManager const *const mgr; //TODO is it neccessary
private:
    static std::vector<fp> calc_val_xx, calc_val_yy;

};


class FuncConstant : public Function
{
    FuncConstant (std::string const &name, std::vector<std::string> const &vars,
                  VariableManager *m)
        : Function(name, vars, formula, m) {}
    friend class Function;
public:
    static const char *formula; 
    void calculate_value(std::vector<fp> const &x, std::vector<fp> &y) const; 
    void calculate_value_deriv(std::vector<fp> const &x, 
                               std::vector<fp> &y, std::vector<fp> &dy_da,
                               bool in_dx=false) const; 
};
    

class FuncGaussian : public Function
{
    static const char *formula; 
    FuncGaussian (std::string const &name, std::vector<std::string> const &vars,
                  VariableManager *m)
        : Function (name, vars, formula, m) {}  
    FuncGaussian (const FuncGaussian&);
    friend class Function;
public:

    void calculate_value(std::vector<fp> const &x, std::vector<fp> &y) const; 
    void calculate_value_deriv(std::vector<fp> const &x, 
                               std::vector<fp> &y, std::vector<fp> &dy_da,
                               bool in_dx=false) const; 
    //void do_precomputations() { /*if (fabs(av[2]) < TINY) av[2] = TINY;*/ }
    bool get_nonzero_range (fp level, fp &left, fp &right) const;  
    bool is_peak() const { return true; } 
    bool knows_peak_properties() const { return true; }
    fp center() const { return vv[0]; }
    fp height() const { return vv[1]; }
    fp fwhm() const   { return 2 * fabs(vv[2]); }
    fp area() const   { return vv[0] * fabs(vv[2]) * sqrt(M_PI / M_LN2); }
};


#endif //FUNCS__H__


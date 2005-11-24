// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#include "common.h"
#include "func.h"
#include "var.h"
#include "voigt.h"
#include "ui.h"
#include <memory>
#include <ctype.h>
#include <boost/spirit/core.hpp>

using namespace std; 
using namespace boost::spirit;

std::vector<fp> Function::calc_val_xx(1); 
std::vector<fp> Function::calc_val_yy(1);

Function::Function (string const &name_, vector<string> const &vars,
                    string const &formula_)
    : VariableUser(name_, "%", vars), type_formula(formula_),
      type_name(get_typename_from_formula(formula_)),
      type_var_names(get_varnames_from_formula(formula_)),
      type_var_eq(get_varnames_from_formula(formula_, true)),
      type_rhs(get_rhs_from_formula(formula_)),
      nv(vars.size()), cutoff_level(0.), vv(vars.size())
{
    // parsing formula (above) for every instance of the class is not effective 
    // but the overhead is negligible
    // the ease of adding new built-in function types is more important
    // is there a better way to do it? (MW)

    if (type_var_names.size() != vars.size())
        throw ExecuteError("Function " + type_name + " requires " 
                           + S(type_var_names.size()) + " parameters.");
}

vector<string> Function::get_varnames_from_formula(string const &formula, 
                                                   bool get_eq)
{
    int lb = formula.find('('),
        rb = formula.find(')');
    string all_names(formula, lb+1, rb-lb-1);
    vector<string> nd = split_string(all_names, ',');
    vector<string> names, defaults;
    for (vector<string>::const_iterator i = nd.begin(); i != nd.end(); ++i) {
        string::size_type eq = i->find('=');
        if (eq == string::npos) {
            names.push_back(strip_string(*i));
            defaults.push_back(string());
        }
        else {
            names.push_back(strip_string(string(*i, 0, eq)));
            defaults.push_back(strip_string(string(*i, eq+1)));
        }
    }
    return get_eq ? defaults : names;
}

Function* Function::factory (string const &name_, string const &type_name,
                             vector<string> const &vars) 
{
    string name = name_.empty() ? Function::next_auto_name() : name_; 
    if (name[0] == '%')
        name = string(name, 1);
    if (type_name == "Constant")
        return new FuncConstant(name, vars);
    else if (type_name == "Gaussian")
        return new FuncGaussian(name, vars);
#if 0
    else if (type_name == "Lorentzian")
        return new fLorentz(name, vars);
    else if (type_name == "PearsonVII")
        return new fPearson(name, vars);
    else if (type_name == "PseudoVoigt")
        return new fPsVoigt(name, vars);
    else if (type_name == "Voigt")
        return new fVoigt(name, vars);
    else if (type_name == "Polynomial5")
        return new fPolynomial5(name, vars);
#endif
    else if (type_name == "PielaszekCube")
        return new FuncPielaszekCube(name, vars);
    else 
        throw ExecuteError("Undefined type of function: " + type_name);
}

const char* builtin_formulas[] = {
    FuncConstant::formula,
    FuncGaussian::formula,
    FuncPielaszekCube::formula
};

vector<string> Function::get_all_types()
{
    /*
    const char* builtin[] = {
        "Constant", "Linear", "Polynomial3", "Polynomial4", "Polynomial5",
        "Gaussian", "Lorentzian", "PearsonVII", "PseudoVoigt",
        "Voigt"
    };
    */
    vector<string> types;
    int nb = sizeof(builtin_formulas)/sizeof(builtin_formulas[0]);
    for (int i = 0; i < nb; ++i)
        types.push_back(get_typename_from_formula(builtin_formulas[i]));
    return types;
}

string Function::get_formula(string const& type)
{
    int nb = sizeof(builtin_formulas)/sizeof(builtin_formulas[0]);
    for (int i = 0; i < nb; ++i)
        if (get_typename_from_formula(builtin_formulas[i]) == type)
            return builtin_formulas[i];
    return "";
}

void Function::do_precomputations(vector<Variable*> const &variables)
{
    //precondition: recalculate() for all variables
    multi.clear();
    for (int i = 0; i < size(var_idx); ++i) {
        Variable const *v = variables[var_idx[i]];
        vv[i] = v->get_value(); 
        vector<Variable::ParMult> const &pm = v->get_recursive_derivatives();
        for (vector<Variable::ParMult>::const_iterator j = pm.begin();
                j != pm.end(); ++j)
            multi.push_back(Multi(i, *j));
    }
}

void Function::get_nonzero_idx_range(std::vector<fp> const &x,
                                     int &first, int &last) const
{
    //precondition: x is sorted
    fp left, right;
    bool r = get_nonzero_range(cutoff_level, left, right);
    if (r) {
        first = lower_bound(x.begin(), x.end(), left) - x.begin(); 
        last = upper_bound(x.begin(), x.end(), right) - x.begin(); 
    }
    else {
        first = 0; 
        last = x.size();
    }
}

fp Function::calculate_value(fp x) const
{
    calc_val_xx[0] = x;
    calc_val_yy[0] = 0.;
    calculate_value(calc_val_xx, calc_val_yy);
    return calc_val_yy[0];
}

string Function::get_info(vector<Variable*> const &variables, 
                          vector<fp> const &parameters, 
                          bool extended) const 
{ 
    vector<string> xvarnames; 
    for (vector<int>::const_iterator i = var_idx.begin(); 
            i != var_idx.end(); ++i)
        xvarnames.push_back(variables[*i]->xname);
    string s = xname+" = "+type_name+ "(" + join_vector(xvarnames, ", ") + ")";
    if (extended) {
        s += "\n" + type_formula;
        for (vector<int>::const_iterator i = var_idx.begin(); 
                i != var_idx.end(); ++i)
            s += "\n" + variables[*i]->get_info(parameters);
    }
    return s;
} 

string Function::get_current_formula(string const& x) const
{
    string t = type_rhs;
    for (int i = 0; i < size(type_var_names); ++i) 
        replace_words(t, type_var_names[i], S(vv[i]));
    replace_words(t, "x", x);
    return t;
}

int Function::unnamed_counter = 0;

///////////////////////////////////////////////////////////////////////
#define PUT_DERIVATIVES_AND_VALUE(VAL) \
        if (!in_dx) { \
            y[i] += (VAL); \
            for (vector<Multi>::const_iterator j = multi.begin(); \
                    j != multi.end(); ++j) \
                dy_da[dyn*i+j->p] += dy_dv[j->n] * j->mult;\
            dy_da[dyn*i+dyn-1] += dy_dx;\
        }\
        else {  \
            for (vector<Multi>::const_iterator j = multi.begin(); \
                    j != multi.end(); ++j) \
                dy_da[dyn*i+j->p] += dy_da[dyn*i+dyn-1] * dy_dv[j->n]*j->mult;\
        }
///////////////////////////////////////////////////////////////////////

const char *FuncConstant::formula 
= "Constant(a) = a"; 

void FuncConstant::calculate_value(vector<fp> const &/*x*/, vector<fp> &y) const
{
    for (vector<fp>::iterator i = y.begin(); i != y.end(); ++i)
        *i += vv[0];
}

void FuncConstant::calculate_value_deriv(vector<fp> const &x, 
                                         vector<fp> &y, 
                                         vector<fp> &dy_da,
                                         bool in_dx) const
{
    // dy_da.size() == x.size() * (parameters.size()+1)
    int dyn = dy_da.size() / x.size();
    vector<fp> dy_dv(vv.size());
    for (int i = 0; i < size(y); ++i) {
        dy_dv[0] = 1.;
        fp dy_dx = 0;
        PUT_DERIVATIVES_AND_VALUE(vv[0]);
    }
}

///////////////////////////////////////////////////////////////////////

const char *FuncGaussian::formula 
= "Gaussian(height, center, hwhm) = "
                        "height*exp(-ln(2)*((x-center)/hwhm)^2)"; 


void FuncGaussian::calculate_value(vector<fp> const &x, vector<fp> &y) const
{
    int first, last;
    get_nonzero_idx_range(x, first, last);
    for (int i = first; i < last; ++i) {
        fp xa1a2 = (x[i] - vv[1]) / vv[2];
        fp ex = exp_ (- M_LN2 * xa1a2 * xa1a2);
        y[i] += vv[0] * ex;
    }
}

void FuncGaussian::calculate_value_deriv(vector<fp> const &x, 
                                         vector<fp> &y, 
                                         vector<fp> &dy_da,
                                         bool in_dx) const
{
    int first, last;
    get_nonzero_idx_range(x, first, last);
    int dyn = dy_da.size() / x.size();
    vector<fp> dy_dv(vv.size());
    for (int i = first; i < last; ++i) {
        fp xa1a2 = (x[i] - vv[1]) / vv[2];
        fp ex = exp_ (- M_LN2 * xa1a2 * xa1a2);
        dy_dv[0] = ex;
        fp dcenter = 2 * M_LN2 * vv[0] * ex * xa1a2 / vv[2];
        dy_dv[1] = dcenter;
        dy_dv[2] = dcenter * xa1a2;
        fp dy_dx = -dcenter;
        PUT_DERIVATIVES_AND_VALUE(vv[0]*ex);
    }
}

bool FuncGaussian::get_nonzero_range (fp level, fp &left, fp &right) const
{  
    if (level <= 0)
        return false;
    else if (level >= vv[0])
        left = right = 0;
    else {
        fp w = sqrt (log (vv[0] / level) / M_LN2) * vv[2]; 
        left = vv[1] - w;             
        right = vv[1] + w;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////

const char *FuncPielaszekCube::formula 
= "PielaszekCube(a=height*0.016, center, r=300, s=150) = ..."; 


void FuncPielaszekCube::calculate_value(vector<fp> const &x, 
                                        vector<fp> &y) const
{
    fp height = vv[0];
    fp center = vv[1];
    fp R = vv[2];
    fp s = vv[3];
    fp s2 = s*s;
    fp s4 = s2*s2;
    fp R2 = R*R;
    for (int i = 0; i < size(x); ++i) {
        fp q = (x[i]-center);
        fp q2 = q*q;
        y[i] += height * 
        (-3*R*(-1 - (R2*(-1 +
                              pow(1 + (q2*s4)/R2, 1.5 - R2/(2.*s2))
                              * cos(2*(-1.5 + R2/(2.*s2)) * atan((q*s2)/R))))/
               (2.*q2*(-1.5 + R2/(2.*s2))* (-1 + R2/(2.*s2))*s4)))/
          (sqrt(2*M_PI)*q2*(-0.5 + R2/(2.*s2))* s2);
    }
}

void FuncPielaszekCube::calculate_value_deriv(vector<fp> const &x, 
                                         vector<fp> &y, 
                                         vector<fp> &dy_da,
                                         bool in_dx) const
{
    int dyn = dy_da.size() / x.size();
    vector<fp> dy_dv(vv.size());
    fp height = vv[0];
    fp center = vv[1];
    fp R = vv[2];
    fp s = vv[3];
    fp s2 = s*s;
    fp s3 = s*s2;
    fp s4 = s2*s2;
    fp R2 = R*R;
    fp R4 = R2*R2;
    fp R3 = R*R2;
    for (int i = 0; i < size(x); ++i) {
        fp q = (x[i]-center);
        fp q2 = q*q;
        fp t = (-3*R*(-1 - (R2*(-1 +
                              pow(1 + (q2*s4)/R2, 1.5 - R2/(2.*s2))
                              * cos(2*(-1.5 + R2/(2.*s2)) * atan((q*s2)/R))))/
               (2.*q2*(-1.5 + R2/(2.*s2))* (-1 + R2/(2.*s2))*s4)))/
          (sqrt(2*M_PI)*q2*(-0.5 + R2/(2.*s2))* s2);

        fp dcenter = height * (
            (3*sqrt(2/M_PI)*R*(-1 - 
                        (R2* (-1 + pow(1 + (q2*s4)/R2, 1.5 - R2/(2.*s2))*
             cos(2*(-1.5 + R2/(2.*s2))* atan((q*s2)/R))))/
        (2.*q2*(-1.5 + R2/(2.*s2))* (-1 + R2/(2.*s2))*s4)))/
   (q*q2*(-0.5 + R2/(2.*s2))*s2) - (3*R*((R2*(-1 + 
            pow(1 + (q2*s4)/R2, 1.5 - R2/(2.*s2))*
             cos(2*(-1.5 + R2/(2.*s2))* atan((q*s2)/R))))/
        (q*q2*(-1.5 + R2/(2.*s2))* (-1 + R2/(2.*s2))*s4) - 
       (R2*((2*q*(1.5 - R2/(2.*s2))* s4*
               pow(1 + (q2*s4)/R2, 0.5 - R2/(2.*s2))*
               cos(2*(-1.5 + R2/(2.*s2))* atan((q*s2)/R)))/R2 - 
            (2*(-1.5 + R2/(2.*s2))*s2* pow(1 + (q2*s4)/R2,
                0.5 - R2/(2.*s2))* sin(2*(-1.5 + R2/(2.*s2))*
                 atan((q*s2)/R)))/R))/
        (2.*q2*(-1.5 + R2/(2.*s2))* (-1 + R2/(2.*s2))*s4)))/
   (sqrt(2*M_PI)*q2*(-0.5 + R2/(2.*s2))* s2));

        fp dR = height * (
        (3*R2*(-1 - (R2* (-1 + pow(1 + (q2*s4)/R2,
              1.5 - R2/(2.*s2))* cos(2*(-1.5 + R2/(2.*s2))*
               atan((q*s2)/R))))/ (2.*q2*(-1.5 + R2/(2.*s2))*
          (-1 + R2/(2.*s2))*s4)))/ (sqrt(2*M_PI)*q2*pow(-0.5 + R2/(2.*s2),2)*
     s4) - (3*(-1 - (R2*(-1 + pow(1 + (q2*s4)/R2,
              1.5 - R2/(2.*s2))* cos(2*(-1.5 + R2/(2.*s2))*
               atan((q*s2)/R))))/ (2.*q2*(-1.5 + R2/(2.*s2))*
          (-1 + R2/(2.*s2))*s4)))/ (sqrt(2*M_PI)*q2*(-0.5 + R2/(2.*s2))*
     s2) - (3*R*((R3* (-1 + pow(1 + (q2*s4)/R2,
              1.5 - R2/(2.*s2))* cos(2*(-1.5 + R2/(2.*s2))*
               atan((q*s2)/R))))/ (2.*q2*(-1.5 + R2/(2.*s2))*
          pow(-1 + R2/(2.*s2),2)*s4*s2) + (R3*(-1 + pow(1 + (q2*s4)/R2,
              1.5 - R2/(2.*s2))* cos(2*(-1.5 + R2/(2.*s2))* atan((q*s2)/R))))/
        (2.*q2*pow(-1.5 + R2/(2.*s2),2)* (-1 + R2/(2.*s2))*(s4*s2)) - 
       (R*(-1 + pow(1 + (q2*s4)/R2, 1.5 - R2/(2.*s2))*
             cos(2*(-1.5 + R2/(2.*s2))* atan((q*s2)/R))))/
        (q2*(-1.5 + R2/(2.*s2))* (-1 + R2/(2.*s2))*s4) - 
       (R2*(pow(1 + (q2*s4)/R2, 1.5 - R2/(2.*s2))*
             cos(2*(-1.5 + R2/(2.*s2))* atan((q*s2)/R))*
             ((-2*q2*(1.5 - R2/(2.*s2))* s4)/ (R3*
                  (1 + (q2*s4)/R2)) - (R*log(1 + (q2*s4)/R2))/ s2) + 
            pow(1 + (q2*s4)/R2, 1.5 - R2/(2.*s2))* ((2*q*(-1.5 + R2/(2.*s2))*
                  s2)/ (R2* (1 + (q2*s4)/R2)) - (2*R*atan((q*s2)/R))/s2)*
             sin(2*(-1.5 + R2/(2.*s2))* atan((q*s2)/R))))/
        (2.*q2*(-1.5 + R2/(2.*s2))* (-1 + R2/(2.*s2))*s4)))/
   (sqrt(2*M_PI)*q2*(-0.5 + R2/(2.*s2))* s2));

        fp ds = height * (
            (-3*R3*(-1 - (R2* (-1 + pow(1 + (q2*s4)/R2,
              1.5 - R2/(2.*s2))* cos(2*(-1.5 + R2/(2.*s2))*
               atan((q*s2)/R))))/ (2.*q2*(-1.5 + R2/(2.*s2))*
          (-1 + R2/(2.*s2))*s4)))/
   (sqrt(2*M_PI)*q2*pow(-0.5 + R2/(2.*s2),2)* (s4*s)) + (3*sqrt(2/M_PI)*R*
     (-1 - (R2*(-1 + pow(1 + (q2*s4)/R2, 1.5 - R2/(2.*s2))*
             cos(2*(-1.5 + R2/(2.*s2))* atan((q*s2)/R))))/
        (2.*q2*(-1.5 + R2/(2.*s2))* (-1 + R2/(2.*s2))*s4)))/
   (q2*(-0.5 + R2/(2.*s2))*s3) - (3*R*(-(R4*(-1 + 
             pow(1 + (q2*s4)/R2, 1.5 - R2/(2.*s2))*
              cos(2*(-1.5 + R2/(2.*s2))* atan((q*s2)/R))))/
        (2.*q2*(-1.5 + R2/(2.*s2))* pow(-1 + R2/(2.*s2),2)*(s4*s3)) - 
       (R4*(-1 + pow(1 + (q2*s4)/R2, 1.5 - R2/(2.*s2))*
             cos(2*(-1.5 + R2/(2.*s2))* atan((q*s2)/R))))/
        (2.*q2*pow(-1.5 + R2/(2.*s2),2)* (-1 + R2/(2.*s2))*(s4*s3)) + 
       (2*R2*(-1 + pow(1 + (q2*s4)/R2,
              1.5 - R2/(2.*s2))* cos(2*(-1.5 + R2/(2.*s2))*
               atan((q*s2)/R))))/ (q2*(-1.5 + R2/(2.*s2))*
          (-1 + R2/(2.*s2))*(s4*s)) - (R2*(pow(1 + (q2*s4)/R2,
              1.5 - R2/(2.*s2))* cos(2*(-1.5 + R2/(2.*s2))*
               atan((q*s2)/R))* ((4*q2*(1.5 - R2/(2.*s2))* s3)/
                (R2* (1 + (q2*s4)/R2)) + (R2*log(1 + 
                    (q2*s4)/R2))/ s3) + 
            pow(1 + (q2*s4)/R2, 1.5 - R2/(2.*s2))*
             ((-4*q*(-1.5 + R2/(2.*s2))*s)/ (R*(1 + (q2*s4)/R2)) + 
               (2*R2*atan((q*s2)/R))/ s3)*
             sin(2*(-1.5 + R2/(2.*s2))* atan((q*s2)/R))))/
        (2.*q2*(-1.5 + R2/(2.*s2))* (-1 + R2/(2.*s2))*s4)))/
   (sqrt(2*M_PI)*q2*(-0.5 + R2/(2.*s2))* s2));

        dy_dv[0] = t;
        dy_dv[1] = -dcenter;
        dy_dv[2] = dR;
        dy_dv[3] = ds;
        fp dy_dx = dcenter;
        PUT_DERIVATIVES_AND_VALUE(height*t);

    }
}




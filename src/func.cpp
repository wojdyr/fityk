// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#include "common.h"
#include "func.h"
#include "var.h"
#include "voigt.h"
#include "ui.h"
#include "numfuncs.h"
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
      nv(vars.size()), cutoff_level(0.), 
      center_idx(contains_element(type_var_names, "center") 
                 ? find(type_var_names.begin(), type_var_names.end(), "center")
                                                      - type_var_names.begin()
                 : -1),
      vv(vars.size())
{
    // parsing formula (above) for every instance of the class is not effective 
    // but the overhead is negligible
    // the ease of adding new built-in function types is more important
    // is there a better way to do it? (MW)

    if (type_var_names.size() != vars.size())
        throw ExecuteError("Function " + type_name + " requires " 
                           + S(type_var_names.size()) + " parameters.");
}

/// returns type variable names if !get_eq
///      or type variable default values if get_eq
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

#define FACTORY_FUNC(NAME) \
    if (type_name == #NAME) \
        return new Func##NAME(name, vars);

    FACTORY_FUNC(Constant)
    FACTORY_FUNC(Linear)
    FACTORY_FUNC(Quadratic)
    FACTORY_FUNC(Cubic)
    FACTORY_FUNC(Polynomial4)
    FACTORY_FUNC(Polynomial5)
    FACTORY_FUNC(Polynomial6)
    FACTORY_FUNC(Linear)
    FACTORY_FUNC(Gaussian)
    FACTORY_FUNC(SplitGaussian)
    FACTORY_FUNC(Lorentzian)
    FACTORY_FUNC(Pearson7)
    FACTORY_FUNC(SplitPearson7)
    FACTORY_FUNC(PseudoVoigt)
    FACTORY_FUNC(Voigt)
    FACTORY_FUNC(Valente)
    FACTORY_FUNC(PielaszekCube)
    else 
        throw ExecuteError("Undefined type of function: " + type_name);
}

const char* builtin_formulas[] = {
    FuncConstant::formula,
    FuncLinear::formula,
    FuncQuadratic::formula,
    FuncCubic::formula,
    FuncPolynomial4::formula,
    FuncPolynomial5::formula,
    FuncPolynomial6::formula,
    FuncGaussian::formula,
    FuncSplitGaussian::formula,
    FuncLorentzian::formula,
    FuncPearson7::formula,
    FuncSplitPearson7::formula,
    FuncPseudoVoigt::formula,
    FuncVoigt::formula,
    FuncValente::formula,
    FuncPielaszekCube::formula
};

vector<string> Function::get_all_types()
{
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

void Function::erased_parameter(int k)
{
    for (vector<Multi>::iterator i = multi.begin(); i != multi.end(); ++i)
        if (i->p > k)
            -- i->p;
}


void Function::get_nonzero_idx_range(std::vector<fp> const &xx,
                                     int &first, int &last) const
{
    //precondition: xx is sorted
    fp left, right;
    bool r = get_nonzero_range(cutoff_level, left, right);
    if (r) {
        first = lower_bound(xx.begin(), xx.end(), left) - xx.begin(); 
        last = upper_bound(xx.begin(), xx.end(), right) - xx.begin(); 
    }
    else {
        first = 0; 
        last = xx.size();
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
        for (int i = 0; i < size(var_idx); ++i)
            s += "\n" + type_var_names[i] + " = "
                + variables[var_idx[i]]->get_info(parameters);
        if (this->is_peak()) {
            if (!contains_element(type_var_names, string("center")))
                s += "\nCenter: " + S(center());
            if (!contains_element(type_var_names, string("height")))
                s += "\nHeight: " + S(height());
            if (!contains_element(type_var_names, string("fwhm")))
                s += "\nFWHM: " + S(fwhm());
            if (!contains_element(type_var_names, string("area")))
                s += "\nArea: " + S(area());
        }
    }
    return s;
} 

string Function::get_current_definition(vector<Variable*> const &variables,
                                        vector<fp> const &parameters) const
{
    vector<string> vv; 
    assert(type_var_names.size() == var_idx.size());
    for (int i = 0; i < size(var_idx); ++i) {
        Variable const* v = variables[var_idx[i]];
        string t = type_var_names[i] + "=" 
                   + (v->is_simple() ? v->get_formula(parameters) : v->xname);
        vv.push_back(t);
    }
    return xname + " = " + type_name + "(" + join_vector(vv, ", ") + ")";
}

string Function::get_current_formula(string const& x) const
{
    string t = type_rhs;
    for (int i = 0; i < size(type_var_names); ++i) 
        replace_words(t, type_var_names[i], S(vv[i]));
    replace_words(t, "x", x);
    return t;
}

int Function::find_param_nr(std::string const& param) const
{
    vector<string>::const_iterator i = find(type_var_names.begin(), 
                                            type_var_names.end(), param);
    if (i == type_var_names.end())
        throw ExecuteError("function " + xname + " has no parameter: " + param);
    return i - type_var_names.begin();
}

int Function::unnamed_counter = 0;

///////////////////////////////////////////////////////////////////////
#define DEFINE_FUNC_CALCULATE_VALUE_BEGIN(NAME) \
void Func##NAME::calculate_value(vector<fp> const &xx, vector<fp> &yy) const\
{\
    int first, last; \
    get_nonzero_idx_range(xx, first, last); \
    for (int i = first; i < last; ++i) {\
        fp x = xx[i]; 


#define DEFINE_FUNC_CALCULATE_VALUE_END(VAL) \
        yy[i] += (VAL);\
    }\
}

#define DEFINE_FUNC_CALCULATE_VALUE(NAME, VAL) \
    DEFINE_FUNC_CALCULATE_VALUE_BEGIN(NAME) \
    DEFINE_FUNC_CALCULATE_VALUE_END(VAL)


#define PUT_DERIVATIVES_AND_VALUE(VAL) \
        if (!in_dx) { \
            yy[i] += (VAL); \
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


#define DEFINE_FUNC_CALCULATE_VALUE_DERIV_BEGIN(NAME) \
void Func##NAME::calculate_value_deriv(vector<fp> const &xx, \
                                         vector<fp> &yy, \
                                         vector<fp> &dy_da, \
                                         bool in_dx) const \
{ \
    int first, last; \
    get_nonzero_idx_range(xx, first, last); \
    int dyn = dy_da.size() / xx.size(); \
    vector<fp> dy_dv(nv); \
    for (int i = first; i < last; ++i) { \
        fp x = xx[i]; \
        fp dy_dx;


#define DEFINE_FUNC_CALCULATE_VALUE_DERIV_END(VAL) \
        PUT_DERIVATIVES_AND_VALUE(VAL)  \
    } \
}




///////////////////////////////////////////////////////////////////////

const char *FuncConstant::formula 
= "Constant(a=height) = a"; 

void FuncConstant::calculate_value(vector<fp> const&/*xx*/, vector<fp>&yy) const
{
    for (vector<fp>::iterator i = yy.begin(); i != yy.end(); ++i)
        *i += vv[0];
}

void FuncConstant::calculate_value_deriv(vector<fp> const &xx, 
                                         vector<fp> &yy, 
                                         vector<fp> &dy_da,
                                         bool in_dx) const
{
    // dy_da.size() == xx.size() * (parameters.size()+1)
    int dyn = dy_da.size() / xx.size();
    vector<fp> dy_dv(nv);
    for (int i = 0; i < size(yy); ++i) {
        dy_dv[0] = 1.;
        fp dy_dx = 0;
        PUT_DERIVATIVES_AND_VALUE(vv[0]);
    }
}

///////////////////////////////////////////////////////////////////////

const char *FuncLinear::formula 
= "Linear(a0=height,a1=0) = a0 + a1 * x"; 


DEFINE_FUNC_CALCULATE_VALUE(Linear, vv[0] + x*vv[1])

DEFINE_FUNC_CALCULATE_VALUE_DERIV_BEGIN(Linear)
    dy_dv[0] = 1.;
    dy_dv[1] = x;
    dy_dx = vv[1];
DEFINE_FUNC_CALCULATE_VALUE_DERIV_END(vv[0] + x*vv[1])

///////////////////////////////////////////////////////////////////////

const char *FuncQuadratic::formula 
= "Quadratic(a0=height,a1=0, a2=0) = a0 + a1*x + a2*x^2"; 


DEFINE_FUNC_CALCULATE_VALUE(Quadratic, vv[0] + x*vv[1] + x*x*vv[2])

DEFINE_FUNC_CALCULATE_VALUE_DERIV_BEGIN(Quadratic)
    dy_dv[0] = 1.;
    dy_dv[1] = x;
    dy_dv[2] = x*x;
    dy_dx = vv[1] + 2*x*vv[2];
DEFINE_FUNC_CALCULATE_VALUE_DERIV_END(vv[0] + x*vv[1] + x*x*vv[2])

///////////////////////////////////////////////////////////////////////

const char *FuncCubic::formula 
= "Cubic(a0=height,a1=0, a2=0, a3=0) = a0 + a1*x + a2*x^2 + a3*x^3"; 


DEFINE_FUNC_CALCULATE_VALUE(Cubic, vv[0] + x*vv[1] + x*x*vv[2] + x*x*x*vv[3])

DEFINE_FUNC_CALCULATE_VALUE_DERIV_BEGIN(Cubic)
    dy_dv[0] = 1.;
    dy_dv[1] = x;
    dy_dv[2] = x*x;
    dy_dv[3] = x*x*x;
    dy_dx = vv[1] + 2*x*vv[2] + 3*x*x*vv[3];
DEFINE_FUNC_CALCULATE_VALUE_DERIV_END(vv[0] + x*vv[1] + x*x*vv[2] + x*x*x*vv[3])

///////////////////////////////////////////////////////////////////////

const char *FuncPolynomial4::formula 
= "Polynomial4(a0=height,a1=0, a2=0, a3=0, a4=0) = "
                                 "a0 + a1*x + a2*x^2 + a3*x^3 + a4*x^4"; 


DEFINE_FUNC_CALCULATE_VALUE(Polynomial4, vv[0] + x*vv[1] + x*x*vv[2] 
                                         + x*x*x*vv[3] + x*x*x*x*vv[4])

DEFINE_FUNC_CALCULATE_VALUE_DERIV_BEGIN(Polynomial4)
    dy_dv[0] = 1.;
    dy_dv[1] = x;
    dy_dv[2] = x*x;
    dy_dv[3] = x*x*x;
    dy_dv[4] = x*x*x*x;
    dy_dx = vv[1] + 2*x*vv[2] + 3*x*x*vv[3] + 4*x*x*x*vv[4];
DEFINE_FUNC_CALCULATE_VALUE_DERIV_END(vv[0] + x*vv[1] + x*x*vv[2] + x*x*x*vv[3]
                                      + x*x*x*x*vv[4])

///////////////////////////////////////////////////////////////////////

const char *FuncPolynomial5::formula 
= "Polynomial5(a0=height,a1=0, a2=0, a3=0, a4=0, a5=0) = "
                             "a0 + a1*x + a2*x^2 + a3*x^3 + a4*x^4 + a5*x^5"; 


DEFINE_FUNC_CALCULATE_VALUE(Polynomial5, vv[0] + x*vv[1] + x*x*vv[2] 
                               + x*x*x*vv[3] + x*x*x*x*vv[4] + x*x*x*x*x*vv[5])

DEFINE_FUNC_CALCULATE_VALUE_DERIV_BEGIN(Polynomial5)
    dy_dv[0] = 1.;
    dy_dv[1] = x;
    dy_dv[2] = x*x;
    dy_dv[3] = x*x*x;
    dy_dv[4] = x*x*x*x;
    dy_dv[5] = x*x*x*x*x;
    dy_dx = vv[1] + 2*x*vv[2] + 3*x*x*vv[3] + 4*x*x*x*vv[4] + 5*x*x*x*x*vv[5];
DEFINE_FUNC_CALCULATE_VALUE_DERIV_END(vv[0] + x*vv[1] + x*x*vv[2] 
                               + x*x*x*vv[3] + x*x*x*x*vv[4] + x*x*x*x*x*vv[5])

///////////////////////////////////////////////////////////////////////

const char *FuncPolynomial6::formula 
= "Polynomial6(a0=height,a1=0, a2=0, a3=0, a4=0, a5=0, a6=0) = "
                     "a0 + a1*x + a2*x^2 + a3*x^3 + a4*x^4 + a5*x^5 + a6*x^6"; 


DEFINE_FUNC_CALCULATE_VALUE(Polynomial6, vv[0] + x*vv[1] + x*x*vv[2] 
                               + x*x*x*vv[3] + x*x*x*x*vv[4] + x*x*x*x*x*vv[5]
                               + x*x*x*x*x*x*vv[6])

DEFINE_FUNC_CALCULATE_VALUE_DERIV_BEGIN(Polynomial6)
    dy_dv[0] = 1.;
    dy_dv[1] = x;
    dy_dv[2] = x*x;
    dy_dv[3] = x*x*x;
    dy_dv[4] = x*x*x*x;
    dy_dv[5] = x*x*x*x*x;
    dy_dv[6] = x*x*x*x*x*x;
    dy_dx = vv[1] + 2*x*vv[2] + 3*x*x*vv[3] + 4*x*x*x*vv[4] + 5*x*x*x*x*vv[5]
            + 6*x*x*x*x*x*vv[6];
DEFINE_FUNC_CALCULATE_VALUE_DERIV_END(vv[0] + x*vv[1] + x*x*vv[2] 
                               + x*x*x*vv[3] + x*x*x*x*vv[4] + x*x*x*x*x*vv[5]
                               + x*x*x*x*x*x*vv[6])

///////////////////////////////////////////////////////////////////////

const char *FuncGaussian::formula 
= "Gaussian(height, center, hwhm1=hwhm, hwhm2=hwhm) = "
               "height*exp(-ln(2)*((x-center)/hwhm)^2)"; 

void FuncGaussian::do_precomputations(vector<Variable*> const &variables) 
{ 
    Function::do_precomputations(variables);
    if (fabs(vv[2]) < EPSILON) 
        vv[2] = EPSILON; 
}

DEFINE_FUNC_CALCULATE_VALUE_BEGIN(Gaussian)
    fp xa1a2 = (x - vv[1]) / vv[2];
    fp ex = exp_ (- M_LN2 * xa1a2 * xa1a2);
DEFINE_FUNC_CALCULATE_VALUE_END(vv[0] * ex)

DEFINE_FUNC_CALCULATE_VALUE_DERIV_BEGIN(Gaussian)
    fp xa1a2 = (x - vv[1]) / vv[2];
    fp ex = exp_ (- M_LN2 * xa1a2 * xa1a2);
    dy_dv[0] = ex;
    fp dcenter = 2 * M_LN2 * vv[0] * ex * xa1a2 / vv[2];
    dy_dv[1] = dcenter;
    dy_dv[2] = dcenter * xa1a2;
    dy_dx = -dcenter;
DEFINE_FUNC_CALCULATE_VALUE_DERIV_END(vv[0]*ex)

bool FuncGaussian::get_nonzero_range (fp level, fp &left, fp &right) const
{  
    if (level == 0)
        return false;
    else if (fabs(level) >= fabs(vv[0]))
        left = right = 0;
    else {
        fp w = sqrt (log (fabs(vv[0]/level)) / M_LN2) * vv[2]; 
        left = vv[1] - w;             
        right = vv[1] + w;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////

const char *FuncSplitGaussian::formula 
= "SplitGaussian(height, center, hwhm1=hwhm, hwhm2=hwhm) = "
                   "height*exp(-ln(2)*((x-center)/(x<center?hwhm1:hwhm2))^2)"; 

void FuncSplitGaussian::do_precomputations(vector<Variable*> const &variables) 
{ 
    Function::do_precomputations(variables);
    if (fabs(vv[2]) < EPSILON) 
        vv[2] = EPSILON; 
    if (fabs(vv[3]) < EPSILON) 
        vv[3] = EPSILON; 
}

DEFINE_FUNC_CALCULATE_VALUE_BEGIN(SplitGaussian)
    fp hwhm = (x < vv[1] ? vv[2] : vv[3]);
    fp xa1a2 = (x - vv[1]) / hwhm;
    fp ex = exp_ (- M_LN2 * xa1a2 * xa1a2);
DEFINE_FUNC_CALCULATE_VALUE_END(vv[0] * ex)

DEFINE_FUNC_CALCULATE_VALUE_DERIV_BEGIN(SplitGaussian)
    fp hwhm = (x < vv[1] ? vv[2] : vv[3]);
    fp xa1a2 = (x - vv[1]) / hwhm;
    fp ex = exp_ (- M_LN2 * xa1a2 * xa1a2);
    dy_dv[0] = ex;
    fp dcenter = 2 * M_LN2 * vv[0] * ex * xa1a2 / hwhm;
    dy_dv[1] = dcenter;
    if (x < vv[1]) {
        dy_dv[2] = dcenter * xa1a2;
        dy_dv[3] = 0;
    }
    else {
        dy_dv[2] = 0;
        dy_dv[3] = dcenter * xa1a2;
    }
    dy_dx = -dcenter;
DEFINE_FUNC_CALCULATE_VALUE_DERIV_END(vv[0]*ex)

bool FuncSplitGaussian::get_nonzero_range (fp level, fp &left, fp &right) const
{  
    if (level == 0)
        return false;
    else if (fabs(level) >= fabs(vv[0]))
        left = right = 0;
    else {
        fp w1 = sqrt (log (fabs(vv[0]/level)) / M_LN2) * vv[2]; 
        fp w2 = sqrt (log (fabs(vv[0]/level)) / M_LN2) * vv[3]; 
        left = vv[1] - w1;             
        right = vv[1] + w2;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////

const char *FuncLorentzian::formula 
= "Lorentzian(height, center, hwhm) = "
                        "height/(1+((x-center)/hwhm)^2)"; 


void FuncLorentzian::do_precomputations(vector<Variable*> const &variables)
{ 
    Function::do_precomputations(variables);
    if (fabs(vv[2]) < EPSILON) 
        vv[2] = EPSILON; 
}

DEFINE_FUNC_CALCULATE_VALUE_BEGIN(Lorentzian)
    fp xa1a2 = (x - vv[1]) / vv[2];
    fp inv_denomin = 1. / (1 + xa1a2 * xa1a2);
DEFINE_FUNC_CALCULATE_VALUE_END(vv[0] * inv_denomin)

DEFINE_FUNC_CALCULATE_VALUE_DERIV_BEGIN(Lorentzian)
    fp xa1a2 = (x - vv[1]) / vv[2];
    fp inv_denomin = 1. / (1 + xa1a2 * xa1a2);
    dy_dv[0] = inv_denomin;
    fp dcenter = 2 * vv[0] * xa1a2 / vv[2] * inv_denomin * inv_denomin;
    dy_dv[1] = dcenter;
    dy_dv[2] = dcenter * xa1a2;
    dy_dx = -dcenter;
DEFINE_FUNC_CALCULATE_VALUE_DERIV_END(vv[0] * inv_denomin)

bool FuncLorentzian::get_nonzero_range (fp level, fp &left, fp &right) const
{  
    if (level == 0)
        return false;
    else if (fabs(level) >= fabs(vv[0]))
        left = right = 0;
    else {
        fp w = sqrt (fabs(vv[0]/level) - 1) * vv[2];
        left = vv[1] - w;             
        right = vv[1] + w;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////

const char *FuncPearson7::formula 
= "Pearson7(height, center, hwhm, shape=2) = "
                   "height/(1+((x-center)/hwhm)^2*(2^(1/shape)-1))^shape"; 

void FuncPearson7::do_precomputations(vector<Variable*> const &variables)
{ 
    Function::do_precomputations(variables);
    if (fabs(vv[2]) < EPSILON) 
        vv[2] = EPSILON; 
    if (vv.size() != 5)
        vv.resize(5);
    // not checking for vv[3]>0.5 nor even >0
    vv[4] = pow(2, 1. / vv[3]) - 1;
}

DEFINE_FUNC_CALCULATE_VALUE_BEGIN(Pearson7)
    fp xa1a2 = (x - vv[1]) / vv[2];
    fp xa1a2sq = xa1a2 * xa1a2;
    fp pow_2_1_a3_1 = vv[4]; //pow (2, 1. / a3) - 1;
    fp denom_base = 1 + xa1a2sq * pow_2_1_a3_1;
    fp inv_denomin = pow (denom_base, - vv[3]);
DEFINE_FUNC_CALCULATE_VALUE_END(vv[0] * inv_denomin)

DEFINE_FUNC_CALCULATE_VALUE_DERIV_BEGIN(Pearson7)
    fp xa1a2 = (x - vv[1]) / vv[2];
    fp xa1a2sq = xa1a2 * xa1a2;
    fp pow_2_1_a3_1 = vv[4]; //pow (2, 1. / a3) - 1;
    fp denom_base = 1 + xa1a2sq * pow_2_1_a3_1;
    fp inv_denomin = pow (denom_base, - vv[3]);
    dy_dv[0] = inv_denomin;
    fp dcenter = 2 * vv[0] * vv[3] * pow_2_1_a3_1 * xa1a2 * inv_denomin /
                                                      (denom_base * vv[2]);
    dy_dv[1] = dcenter;
    dy_dv[2] = dcenter * xa1a2;
    dy_dv[3] = vv[0] * inv_denomin * (M_LN2 * (pow_2_1_a3_1 + 1)
                       * xa1a2sq / (denom_base * vv[3]) - log(denom_base));
    dy_dx = -dcenter;
DEFINE_FUNC_CALCULATE_VALUE_DERIV_END(vv[0] * inv_denomin)


bool FuncPearson7::get_nonzero_range (fp level, fp &left, fp &right) const
{  
    if (level == 0)
        return false;
    else if (fabs(level) >= fabs(vv[0]))
        left = right = 0;
    else {
        fp t = (pow(fabs(vv[0]/level), 1./vv[3]) - 1) / (pow (2, 1./vv[3]) - 1);
        fp w = sqrt(t) * vv[2];
        left = vv[1] - w;             
        right = vv[1] + w;
    }
    return true;
}

fp FuncPearson7::area() const
{
    if (vv[3] <= 0.5)
        return +INF;
    fp g = exp_ (LnGammaE(vv[3] - 0.5) - LnGammaE(vv[3]));
    return vv[0] * 2 * fabs(vv[2])
        * sqrt(M_PI) * g / (2 * sqrt (vv[4]));
    //in f_val_precomputations(): vv[4] = pow (2, 1. / a3) - 1;
}

///////////////////////////////////////////////////////////////////////

const char *FuncSplitPearson7::formula 
= "SplitPearson7(height, center, hwhm1=hwhm, hwhm2=hwhm, shape1=2, shape2=2) = "
    "height/(1+((x-center)/(x<center?hwhm1:hwhm2))^2"
               "*(2^(1/(x<center?shape1:shape2))-1))^(x<center?shape1:shape2)";

void FuncSplitPearson7::do_precomputations(vector<Variable*> const &variables)
{ 
    Function::do_precomputations(variables);
    if (fabs(vv[2]) < EPSILON) 
        vv[2] = EPSILON; 
    if (fabs(vv[3]) < EPSILON) 
        vv[3] = EPSILON; 
    if (vv.size() != 8)
        vv.resize(8);
    // not checking for vv[3]>0.5 nor even >0
    vv[6] = pow(2, 1. / vv[4]) - 1;
    vv[7] = pow(2, 1. / vv[5]) - 1;
}

DEFINE_FUNC_CALCULATE_VALUE_BEGIN(SplitPearson7)
    int lr = x < vv[1] ? 0 : 1;
    fp xa1a2 = (x - vv[1]) / vv[2+lr];
    fp xa1a2sq = xa1a2 * xa1a2;
    fp pow_2_1_a3_1 = vv[6+lr]; //pow(2, 1./shape) - 1;
    fp denom_base = 1 + xa1a2sq * pow_2_1_a3_1;
    fp inv_denomin = pow(denom_base, - vv[4+lr]);
DEFINE_FUNC_CALCULATE_VALUE_END(vv[0] * inv_denomin)

DEFINE_FUNC_CALCULATE_VALUE_DERIV_BEGIN(SplitPearson7)
    int lr = x < vv[1] ? 0 : 1;
    fp hwhm = vv[2+lr];
    fp shape = vv[4+lr];
    fp xa1a2 = (x - vv[1]) / hwhm;
    fp xa1a2sq = xa1a2 * xa1a2;
    fp pow_2_1_a3_1 = vv[6+lr]; //pow(2, 1./shape) - 1;
    fp denom_base = 1 + xa1a2sq * pow_2_1_a3_1;
    fp inv_denomin = pow (denom_base, -shape);
    dy_dv[0] = inv_denomin;
    fp dcenter = 2 * vv[0] * shape * pow_2_1_a3_1 * xa1a2 * inv_denomin /
                                                      (denom_base * hwhm);
    dy_dv[1] = dcenter;
    dy_dv[2] = dy_dv[3] = dy_dv[4] = dy_dv[5] = 0;
    dy_dv[2+lr] = dcenter * xa1a2;
    dy_dv[4+lr] = vv[0] * inv_denomin * (M_LN2 * (pow_2_1_a3_1 + 1)
                           * xa1a2sq / (denom_base * shape) - log(denom_base));
    dy_dx = -dcenter;
DEFINE_FUNC_CALCULATE_VALUE_DERIV_END(vv[0] * inv_denomin)


bool FuncSplitPearson7::get_nonzero_range (fp level, fp &left, fp &right) const
{  
    if (level == 0)
        return false;
    else if (fabs(level) >= fabs(vv[0]))
        left = right = 0;
    else {
        fp t1 = (pow(fabs(vv[0]/level), 1./vv[4]) - 1) / (pow(2, 1./vv[4]) - 1);
        fp w1 = sqrt(t1) * vv[2];
        fp t2 = (pow(fabs(vv[0]/level), 1./vv[5]) - 1) / (pow(2, 1./vv[5]) - 1);
        fp w2 = sqrt(t2) * vv[3];
        left = vv[1] - w1;             
        right = vv[1] + w2;
    }
    return true;
}

fp FuncSplitPearson7::area() const
{
    if (vv[4] <= 0.5 || vv[5] <= 0.5)
        return +INF;
    fp g1 = exp_ (LnGammaE(vv[4] - 0.5) - LnGammaE(vv[4]));
    fp g2 = exp_ (LnGammaE(vv[5] - 0.5) - LnGammaE(vv[5]));
    return vv[0] * fabs(vv[2]) * sqrt(M_PI) * g1 / (2 * sqrt (vv[6]))
         + vv[0] * fabs(vv[3]) * sqrt(M_PI) * g2 / (2 * sqrt (vv[7]));
}

///////////////////////////////////////////////////////////////////////

const char *FuncPseudoVoigt::formula 
= "PseudoVoigt(height, center, hwhm, shape=0.5) = "
                        "height*((1-shape)*exp(-ln(2)*((x-center)/hwhm)^2)"
                                 "+shape/(1+((x-center)/hwhm)^2)"; 

void FuncPseudoVoigt::do_precomputations(vector<Variable*> const &variables) 
{ 
    Function::do_precomputations(variables);
    if (fabs(vv[2]) < EPSILON) 
        vv[2] = EPSILON; 
}

DEFINE_FUNC_CALCULATE_VALUE_BEGIN(PseudoVoigt)
    fp xa1a2 = (x - vv[1]) / vv[2];
    fp ex = exp_ (- M_LN2 * xa1a2 * xa1a2);
    fp lor = 1. / (1 + xa1a2 * xa1a2);
    fp without_height =  (1-vv[3]) * ex + vv[3] * lor;
DEFINE_FUNC_CALCULATE_VALUE_END(vv[0] * without_height)

DEFINE_FUNC_CALCULATE_VALUE_DERIV_BEGIN(PseudoVoigt)
    fp xa1a2 = (x - vv[1]) / vv[2];
    fp ex = exp_ (- M_LN2 * xa1a2 * xa1a2);
    fp lor = 1. / (1 + xa1a2 * xa1a2);
    fp without_height =  (1-vv[3]) * ex + vv[3] * lor;
    dy_dv[0] = without_height;
    fp dcenter = 2 * vv[0] * xa1a2 / vv[2]
                    * (vv[3]*lor*lor + (1-vv[3])*M_LN2*ex);
    dy_dv[1] = dcenter;
    dy_dv[2] = dcenter * xa1a2;
    dy_dv[3] =  vv[0] * (lor - ex);
    dy_dx = -dcenter;
DEFINE_FUNC_CALCULATE_VALUE_DERIV_END(vv[0] * without_height)

bool FuncPseudoVoigt::get_nonzero_range (fp level, fp &left, fp &right) const
{  
    if (level == 0)
        return false;
    else if (fabs(level) >= fabs(vv[0]))
        left = right = 0;
    else {
        // neglecting Gaussian part and adding 4.0 to compensate it
        fp w = (sqrt (vv[3] * fabs(vv[0]/level) - 1) + 4.) * vv[2];
        left = vv[1] - w;             
        right = vv[1] + w;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////

const char *FuncVoigt::formula 
= "Voigt(height, center, gwidth=hwhm*0.9, shape=0.1) ="
                            " convolution of Gaussian and Lorentzian";

void FuncVoigt::do_precomputations(vector<Variable*> const &variables) 
{ 
    Function::do_precomputations(variables);
    if (vv.size() != 6)
        vv.resize(6);
    float k, l, dkdx, dkdy;
    humdev(0, fabs(vv[3]), k, l, dkdx, dkdy);
    vv[4] = 1. / k;
    vv[5] = dkdy / k;

    if (fabs(vv[2]) < EPSILON) 
        vv[2] = EPSILON; 
}

DEFINE_FUNC_CALCULATE_VALUE_BEGIN(Voigt)
    // humdev/humlik routines require with y (a3 here) parameter >0.
    float k;
    fp xa1a2 = (x - vv[1]) / vv[2];
    k = humlik(xa1a2, fabs(vv[3]));
DEFINE_FUNC_CALCULATE_VALUE_END(vv[0] * vv[4] * k)

DEFINE_FUNC_CALCULATE_VALUE_DERIV_BEGIN(Voigt)
    // humdev/humlik routines require with y (a3 here) parameter >0.
    // here fabs(vv[3]) is used, and dy_dv[3] is negated if vv[3]<0.
    float k;
    fp xa1a2 = (x-vv[1]) / vv[2];
    fp a0a4 = vv[0] * vv[4];
    float l, dkdx, dkdy;
    humdev(xa1a2, fabs(vv[3]), k, l, dkdx, dkdy);
    dy_dv[0] = vv[4] * k;
    fp dcenter = -a0a4 * dkdx / vv[2];
    dy_dv[1] = dcenter;
    dy_dv[2] = dcenter * xa1a2;
    dy_dv[3] = a0a4 * (dkdy - k * vv[5]);
    if (vv[3] < 0)
        dy_dv[3] = -dy_dv[3];
    dy_dx = -dcenter;
DEFINE_FUNC_CALCULATE_VALUE_DERIV_END(a0a4 * k)

bool FuncVoigt::get_nonzero_range (fp level, fp &left, fp &right) const
{  
    if (level == 0)
        return false;
    else if (fabs(level) >= fabs(vv[0]))
        left = right = 0;
    else {
        //TODO
        return false; 
    }
    return true;
}

///estimation according to
///http://en.wikipedia.org/w/index.php?title=Voigt_profile&oldid=29250968
fp FuncVoigt::fwhm() const 
{ 
    fp gauss_fwhm = 2 * fabs(vv[2]);
    fp const c0 = 2.0056;
    fp const c1 = 1.0593;
    fp phi = vv[3];
    return gauss_fwhm * (1 - c0*c1 + sqrt(phi*phi + 2*c1*phi + c0*c0*c1*c1));
}

fp FuncVoigt::area() const
{
    return fabs(vv[0] * vv[2] * sqrt(M_PI) / humlik(0, fabs(vv[3])));
                                            //* vv[4];//TODO
}
                                                            

///////////////////////////////////////////////////////////////////////

const char *FuncValente::formula 
= "Valente(amp=height, a=7.6, b=7.6, c=center, d=-6.9, n=0) ="
"amp*exp(-((exp(a)-exp(b)) * (x-c) + sqrt((exp(a)+exp(b))^2 * (x-c)^2" 
                    "+ 4*exp(d)))^exp(n))";

DEFINE_FUNC_CALCULATE_VALUE_BEGIN(Valente)
fp amp=vv[0], a=vv[1], b=vv[2], c=vv[3], d=vv[4], n=vv[5];
fp xc = x-c;
fp a_b = exp(a)-exp(b);
fp apb = exp(a)+exp(b);
fp base = (a_b * xc + sqrt(apb*apb * xc*xc + 4*exp(d)));
fp t = exp(-pow(base,exp(n)));
DEFINE_FUNC_CALCULATE_VALUE_END(amp*t)

DEFINE_FUNC_CALCULATE_VALUE_DERIV_BEGIN(Valente)
fp amp=vv[0], a=vv[1], b=vv[2], c=vv[3], d=vv[4], n=vv[5];
fp xc = x-c;
fp a_b = exp(a)-exp(b);
fp apb = exp(a)+exp(b);
fp base = (a_b * xc + sqrt(apb*apb * xc*xc + 4*exp(d)));
fp t = exp(-pow(base,exp(n)));
dy_dv[0] = t;
dy_dv[1] = amp*t*(-(exp(a+n)*(-c+x)*pow(sqrt(4*exp(d)+pow(exp(a)+exp(
b),2)*pow(c-x,2))+(exp(a)-exp(b))*(-c+x),-1+exp(n))*(1+((exp(a)+exp(
b))*(-c+x))/sqrt(4*exp(d)+pow(exp(a)+exp(b),2)*pow(x-c,2)))));
dy_dv[2] = amp*t*(-(exp(b+n)*(-c+x)*pow(sqrt(4*exp(d)+pow(exp(a)+exp(
b),2)*pow(x-c,2))+(exp(a)-exp(b))*(-c+x),-1+exp(n))*(-1+((exp(a)+exp(
b))*(-c+x))/sqrt(4*exp(d)+pow(exp(a)+exp(b),2)*pow(c-x,2)))));
dy_dv[3] = amp*t*(-(exp(n)*(-exp(a)+exp(b)+(pow(exp(a)+exp(b),2)*(c-
x))/sqrt(4*exp(d)+pow(exp(a)+exp(b),2)*pow(c-x,2)))*pow(sqrt(4*exp(d)+
pow(exp(a)+exp(b),2)*pow(c-x,2))+(exp(a)-exp(b))*(-c+x),-1+exp(n))));
dy_dv[4] = amp*t*((-2*exp(d+n)*pow(sqrt(4*exp(d)+pow(exp(a)+exp(
b),2)*pow(x-c,2))+(exp(a)-exp(b))*(-c+x),-1+exp(n)))/sqrt(4*exp(d)+
pow(exp(a)+exp(b),2)*pow(x-c,2)));
dy_dv[5] = amp*t*(-(exp(n)*pow(sqrt(4*exp(d)+pow(exp(a)+exp(b),2)*pow(c-x,2))+
(exp(a)-exp(b))*(-c+x),exp(n))*log(sqrt(4*exp(d)+pow(exp(a)+exp(
b),2)*pow(c-x,2))+(exp(a)-exp(b))*(-c+x))));
dy_dx = - dy_dv[3];
DEFINE_FUNC_CALCULATE_VALUE_DERIV_END(amp*t)
///////////////////////////////////////////////////////////////////////

const char *FuncPielaszekCube::formula 
= "PielaszekCube(a=height*0.016, center, r=300, s=150) = ..."; 


void FuncPielaszekCube::calculate_value(vector<fp> const &xx, 
                                        vector<fp> &yy) const
{
    fp height = vv[0];
    fp center = vv[1];
    fp R = vv[2];
    fp s = vv[3];
    fp s2 = s*s;
    fp s4 = s2*s2;
    fp R2 = R*R;
    for (int i = 0; i < size(xx); ++i) {
        fp q = (xx[i]-center);
        fp q2 = q*q;
        yy[i] += height * 
        (-3*R*(-1 - (R2*(-1 +
                              pow(1 + (q2*s4)/R2, 1.5 - R2/(2.*s2))
                              * cos(2*(-1.5 + R2/(2.*s2)) * atan((q*s2)/R))))/
               (2.*q2*(-1.5 + R2/(2.*s2))* (-1 + R2/(2.*s2))*s4)))/
          (sqrt(2*M_PI)*q2*(-0.5 + R2/(2.*s2))* s2);
    }
}

void FuncPielaszekCube::calculate_value_deriv(vector<fp> const &xx, 
                                              vector<fp> &yy, 
                                              vector<fp> &dy_da,
                                              bool in_dx) const
{
    int dyn = dy_da.size() / xx.size();
    vector<fp> dy_dv(nv);
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
    for (int i = 0; i < size(xx); ++i) {
        fp q = (xx[i]-center);
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




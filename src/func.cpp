// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#include "common.h"
#include "func.h"
#include "var.h"
#include "voigt.h"
#include "ui.h"
#include "numfuncs.h"
#include "settings.h"
#include "calc.h"
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
      type_rhs(get_rhs_from_formula(formula_)),
      nv(vars.size()), settings(getSettings()), 
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
/// can be used also for eg. Foo(3+$bleh, area/fwhm/sqrt(pi/ln(2)))
vector<string> Function::get_varnames_from_formula(string const &formula, 
                                                   bool get_eq)
{
    int lb = formula.find('(');
    int open_bracket_counter = 1;
    int rb = lb;
    while (open_bracket_counter > 0) {
        ++rb;
        char ch = formula[rb];
        if (ch == ')')
            --open_bracket_counter;
        else if (ch == '(')
            ++open_bracket_counter;
        assert(ch);
    }
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
    string name = name_[0] == '%' ? string(name_, 1) : name;

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
    FACTORY_FUNC(Gaussian)
    FACTORY_FUNC(SplitGaussian)
    FACTORY_FUNC(Lorentzian)
    FACTORY_FUNC(Pearson7)
    FACTORY_FUNC(SplitPearson7)
    FACTORY_FUNC(PseudoVoigt)
    FACTORY_FUNC(Voigt)
    FACTORY_FUNC(VoigtA)
    FACTORY_FUNC(EMG)
    FACTORY_FUNC(DoniachSunjic)
    FACTORY_FUNC(Valente)
    FACTORY_FUNC(PielaszekCube)
    else if (CompoundFunction::is_defined(type_name))
        return new CompoundFunction(name, type_name, vars);
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
    FuncVoigtA::formula,
    FuncEMG::formula,
    FuncDoniachSunjic::formula,
    //FuncValente::formula,
    FuncPielaszekCube::formula
};

vector<string> Function::get_all_types()
{
    vector<string> types;
    int nb = sizeof(builtin_formulas)/sizeof(builtin_formulas[0]);
    for (int i = 0; i < nb; ++i)
        types.push_back(get_typename_from_formula(builtin_formulas[i]));
    vector<string> const& cff = CompoundFunction::get_formulae();
    for (vector<string>::const_iterator i = cff.begin(); i != cff.end(); ++i)
        types.push_back(get_typename_from_formula(*i));
    return types;
}

string Function::get_formula(string const& type)
{
    int nb = sizeof(builtin_formulas)/sizeof(builtin_formulas[0]);
    for (int i = 0; i < nb; ++i)
        if (get_typename_from_formula(builtin_formulas[i]) == type)
            return builtin_formulas[i];
    vector<string> const& cff = CompoundFunction::get_formulae();
    for (vector<string>::const_iterator i = cff.begin(); i != cff.end(); ++i)
        if (get_typename_from_formula(*i) == type)
            return *i;
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
    bool r = get_nonzero_range(settings->get_cut_level(), left, right);
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

void Function::calculate_values_with_params(vector<fp> const& x, 
                                            vector<fp> &y,
                                            vector<fp> const& alt_vv) const
{
    vector<fp> backup_vv = vv;
    for (int i = 0; i < min(size(alt_vv), size(vv)); ++i)
        const_cast<Function*>(this)->vv[i] = alt_vv[i];
    calculate_value(x, y);
    const_cast<Function*>(this)->vv = backup_vv;
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
        if (this->has_center()) 
            if (!contains_element(type_var_names, string("center")))
                s += "\nCenter: " + S(center());
        if (this->has_height()) 
            if (!contains_element(type_var_names, string("height")))
                s += "\nHeight: " + S(height());
        if (this->has_fwhm()) 
            if (!contains_element(type_var_names, string("fwhm")))
                s += "\nFWHM: " + S(fwhm());
        if (this->has_area()) 
            if (!contains_element(type_var_names, string("area")))
                s += "\nArea: " + S(area());
    }
    return s;
} 

string Function::get_current_definition(vector<Variable*> const &variables,
                                        vector<fp> const &parameters) const
{
    vector<string> vs; 
    assert(type_var_names.size() == var_idx.size());
    for (int i = 0; i < size(var_idx); ++i) {
        Variable const* v = variables[var_idx[i]];
        string t = type_var_names[i] + "=" 
                   + (v->is_simple() ? v->get_formula(parameters) : v->xname);
        vs.push_back(t);
    }
    return xname + " = " + type_name + "(" + join_vector(vs, ", ") + ")";
}

string Function::get_current_formula(string const& x) const
{
    string t = type_rhs;
    for (int i = 0; i < size(type_var_names); ++i) 
        replace_words(t, type_var_names[i], S(get_var_value(i)));
    replace_words(t, "x", x);
    return t;
}

int Function::get_param_nr(string const& param) const
{
    vector<string>::const_iterator i = find(type_var_names.begin(), 
                                            type_var_names.end(), param);
    if (i == type_var_names.end())
        throw ExecuteError("function " + xname + " has no parameter: " + param);
    return i - type_var_names.begin();
}

fp Function::get_param_value(string const& param) const
{
    if (param.empty())
        throw ExecuteError("Empty parameter name??");
    if (islower(param[0]))
        return get_var_value(get_param_nr(param));
    else if (param == "Center" && has_center()) {
        return center();
    }
    else if (param == "Height" && has_height()) {
        return height();
    }
    else if (param == "FWHM" && has_fwhm()) {
        return fwhm();
    }
    else if (param == "Area" && has_area()) {
        return area();
    }
    else
        throw ExecuteError("Function " + xname + " (" + type_name 
                           + ") has no parameter " + param);
}

fp Function::numarea(fp x1, fp x2, int nsteps) const
{
    if (nsteps <= 1)
        return 0.;
    fp xmin = min(x1, x2);
    fp xmax = max(x1, x2);
    fp h = (xmax - xmin) / (nsteps-1);
    vector<fp> xx(nsteps), yy(nsteps); 
    for (int i = 0; i < nsteps; ++i)
        xx[i] = xmin + i*h;
    calculate_value(xx, yy);
    fp a = (yy[0] + yy[nsteps-1]) / 2.;
    for (int i = 1; i < nsteps-1; ++i)
        a += yy[i];
    return a*h;
}

/// search x in [x1, x2] for which %f(x)==val, 
/// x1, x2, val: f(x1) <= val <= f(x2) or f(x2) <= val <= f(x1)
/// bisection + Newton-Raphson
fp Function::find_x_with_value(fp x1, fp x2, fp val, 
                               fp xacc, int max_iter) const
{
    fp y1 = calculate_value(x1) - val;
    fp y2 = calculate_value(x2) - val;
    if (y1 > 0 && y2 > 0 || y1 < 0 && y2 < 0)
        throw ExecuteError("Value " + S(val) + " is not bracketed by "
                           + S(x1) + "(" + S(y1+val) + ") and " 
                           + S(x2) + "(" + S(y2+val) + ").");
    int n = 0;
    for (vector<Multi>::const_iterator j = multi.begin(); j != multi.end(); ++j)
        n = max(j->p + 1, n);
    vector<fp> dy_da(n+1);
    if (y1 == 0)
        return x1;
    if (y2 == 0)
        return x2;
    if (y1 > 0)
        Swap(x1, x2);
    fp t = (x1 + x2) / 2.;
    for (int i = 0; i < max_iter; ++i) {
        //check if converged
        if (fabs(x1-x2) < xacc)
            return (x1+x2) / 2.;

        // calculate f and df
        calc_val_xx[0] = t;
        calc_val_yy[0] = 0;
        dy_da.back() = 0;
        calculate_value_deriv(calc_val_xx, calc_val_yy, dy_da);
        fp f = calc_val_yy[0] - val;
        fp df = dy_da.back();

        // narrow range
        if (f == 0.)
            return t;
        else if (f < 0)
            x1 = t;
        else // f > 0
            x2 = t;

        // select new guess point
        fp dx = -f/df * 1.02; // 1.02 is to jump to the other side of point
        if (t+dx > x2 && t+dx > x1 || t+dx < x2 && t+dx < x1  // outside
                            || i % 20 == 19) {                 // precaution
            //bisection
            t = (x1 + x2) / 2.;
        }
        else {
            t += dx;
        }
    }
    throw ExecuteError("The search has not converged in " + S(max_iter) 
                       + " steps");
}

/// finds root of derivative, using bisection method
fp Function::find_extremum(fp x1, fp x2, fp xacc, int max_iter) const
{
    int n = 0;
    for (vector<Multi>::const_iterator j = multi.begin(); j != multi.end(); ++j)
        n = max(j->p + 1, n);
    vector<fp> dy_da(n+1);

    // calculate df
    calc_val_xx[0] = x1;
    dy_da.back() = 0;
    calculate_value_deriv(calc_val_xx, calc_val_yy, dy_da);
    fp y1 = dy_da.back();

    calc_val_xx[0] = x2;
    dy_da.back() = 0;
    calculate_value_deriv(calc_val_xx, calc_val_yy, dy_da);
    fp y2 = dy_da.back();

    if (y1 > 0 && y2 > 0 || y1 < 0 && y2 < 0)
        throw ExecuteError("Derivatives at " + S(x1) + " and " + S(x2) 
                           + " have the same sign.");
    if (y1 == 0)
        return x1;
    if (y2 == 0)
        return x2;
    if (y1 > 0)
        Swap(x1, x2);
    for (int i = 0; i < max_iter; ++i) {

        fp t = (x1 + x2) / 2.;

        // calculate df
        calc_val_xx[0] = t;
        dy_da.back() = 0;
        calculate_value_deriv(calc_val_xx, calc_val_yy, dy_da);
        fp df = dy_da.back();

        // narrow range
        if (df == 0.)
            return t;
        else if (df < 0)
            x1 = t;
        else // df > 0
            x2 = t;

        //check if converged
        if (fabs(x1-x2) < xacc)
            return (x1+x2) / 2.;
    }
    throw ExecuteError("The search has not converged in " + S(max_iter) 
                       + " steps");
}

///////////////////////////////////////////////////////////////////////

// when changing, change also CompoundFunction::harddef_count
vector<string> CompoundFunction::formulae = split_string(
"GaussianA(area, center, hwhm) = Gaussian(area/hwhm/sqrt(pi/ln(2)), center, hwhm)\n"
"LorentzianA(area, center, fwhm) = Lorentzian(area/fwhm/pi, center, fwhm)\n"
"PseudoVoigtA(area, center, fwhm, shape) = GaussianA(area*(1-shape), center, fwhm) + LorentzianA(area*shape, center, fwhm)",
  "\n");

void CompoundFunction::define(std::string const &formula)
{
    string type = get_typename_from_formula(formula);
    vector<string> lhs_vars = get_varnames_from_formula(formula);
    if (contains_element(lhs_vars, "x"))
        throw ExecuteError("x should not be given explicitly as "
                           "function type parameters.");
    vector<string> rhs_funcs = split_string(get_rhs_from_formula(formula), "+");
    for (vector<string>::const_iterator i = rhs_funcs.begin(); 
            i != rhs_funcs.end(); ++i) {
        string t = get_typename_from_formula(*i);
        string tf = Function::get_formula(t);
        if (tf.empty())
            throw ExecuteError("definition based on undefined function `" 
                               + t + "'");
        vector<string> tvars = get_varnames_from_formula(tf);
        vector<string> gvars = get_varnames_from_formula(*i);
        if (tvars.size() != gvars.size())
            throw ExecuteError("Function `" + t + "' requires " 
                                          + S(tvars.size()) + " parameters.");
        for (vector<string>::const_iterator j = gvars.begin(); 
                                                       j != gvars.end(); ++j){
            tree_parse_info<> info = ast_parse(j->c_str(), FuncG, space_p);
            assert(info.full);
            vector<string> vars = find_tokens_in_ptree(FuncGrammar::variableID,
                                                       info);
            if (contains_element(vars, "x"))
                throw ExecuteError("variable can not depend on x.");
            for (vector<string>::const_iterator k = vars.begin(); 
                                                    k != vars.end(); ++k)
                if ((*k)[0] != '~' && (*k)[0] != '{' && (*k)[0] != '$' 
                        && (*k)[0] != '%' && !contains_element(lhs_vars, *k))
                    throw ExecuteError("Improper variable given in parameter " 
                                       + S(k-vars.begin()+1) + " of "+ t + ": "
                                       + *k);
        }
    }
    if (is_defined(type, true)) {
        undefine(type);
    }
    else if (!Function::get_formula(type).empty()) { //not UDF -> built-in
        throw ExecuteError("Built-in functions can't be redefined.");
    }
    formulae.push_back(formula);
}

void CompoundFunction::undefine(std::string const &type)
{
    for (vector<string>::iterator i=formulae.begin(); i != formulae.end(); ++i)
        if (get_typename_from_formula(*i) == type) {
            if (i - formulae.begin() < harddef_count)
                throw ExecuteError("Built-in compound function "
                                                    "can't be undefined.");
            formulae.erase(i);
            return;
        }
    throw ExecuteError("Can not undefine function `" + type 
                        + "' which is not defined");
}

bool CompoundFunction::is_defined(std::string const &type, bool only_udf)
{
    int start = only_udf ? harddef_count : 0; 
    for (vector<string>::const_iterator i = formulae.begin() + start; 
            i != formulae.end(); ++i)
        if (get_typename_from_formula(*i) == type)
            return true;
    return false;
}

std::string const& CompoundFunction::get_formula(std::string const& type)
{
    for (vector<string>::const_iterator i = formulae.begin(); 
            i != formulae.end(); ++i)
        if (get_typename_from_formula(*i) == type) 
            return *i;
    throw ExecuteError("Function `" + type + "' is not defined.");
}

CompoundFunction::CompoundFunction(string const &name, string const &type,
                                   vector<string> const &vars)
    : Function(name, vars, get_formula(type)) 
{
    vmgr.silent = true;
    for (int j = 0; j != nv; ++j) 
        vmgr.assign_variable(varnames[j], "0");//fake vars to be replaced later
    vector<string> funforms = split_string(Function::type_rhs, "+");
    for (vector<string>::iterator i=funforms.begin(); i != funforms.end(); ++i){
        for (int j = 0; j != nv; ++j) {
            replace_words(*i, type_var_names[j], vmgr.get_variable(j)->xname);
        }
        vector<string> newv = get_varnames_from_formula(*i);
        vmgr.assign_func("", get_typename_from_formula(*i), newv);
    }
}

void CompoundFunction::do_precomputations(vector<Variable*> const &variables_)
{ 
    Function::do_precomputations(variables_); 

    for (int i = 0; i < nv; ++i) //replace fake variables
        vmgr.set_recalculated_variable(i, *variables_[get_var_idx(i)]);
    vmgr.use_parameters();
}

void CompoundFunction::calculate_value(vector<fp> const &xx, 
                                       vector<fp> &yy) const
{
    vector<Function*> const& functions = vmgr.get_functions();
    for (vector<Function*>::const_iterator i = functions.begin();
            i != functions.end(); ++i)
        (*i)->calculate_value(xx, yy);
}

void CompoundFunction::calculate_value_deriv(vector<fp> const &xx, 
                                             vector<fp> &yy, vector<fp> &dy_da,
                                             bool in_dx) const
{
    vector<Function*> const& functions = vmgr.get_functions();
    for (vector<Function*>::const_iterator i = functions.begin();
            i != functions.end(); ++i)
        (*i)->calculate_value_deriv(xx, yy, dy_da, in_dx);
}

string CompoundFunction::get_current_formula(string const& x) const
{
    string t;
    vector<Function*> const& functions = vmgr.get_functions();
    for (vector<Function*>::const_iterator i = functions.begin();
            i != functions.end(); ++i) {
        if (i != functions.begin())
            t += "+";
        t += (*i)->get_current_formula(x);
    }
    return t;
}

bool CompoundFunction::has_center() const 
{ 
    vector<Function*> const& ff = vmgr.get_functions();
    for (size_t i = 0; i < ff.size(); ++i) {
        if (!ff[i]->has_center()
                || i > 0 && ff[i-1]->center() != ff[i]->center())
            return false;
    }
    return true;
}

/// if consists of >1 functions, if centers are in the same place
///  height is a sum of heights
bool CompoundFunction::has_height() const
{
    vector<Function*> const& ff = vmgr.get_functions();
    if (ff.size() == 1) 
        return ff[0]->has_center();
    for (size_t i = 0; i < ff.size(); ++i) {
        if (!ff[i]->has_center() || !ff[i]->has_height()
                || i > 0 && ff[i-1]->center() != ff[i]->center())
            return false;
    }
    return true;
}

fp CompoundFunction::height() const
{
    fp height = 0;
    vector<Function*> const& ff = vmgr.get_functions();
    for (size_t i = 0; i < ff.size(); ++i) {
        height += ff[i]->height();
    }
    return height;
}

bool CompoundFunction::has_fwhm() const
{
    vector<Function*> const& ff = vmgr.get_functions();
    return ff.size() == 1 && ff[0]->has_fwhm();
}

fp CompoundFunction::fwhm() const
{
    return vmgr.get_function(0)->fwhm();
}

bool CompoundFunction::has_area() const
{
    vector<Function*> const& ff = vmgr.get_functions();
    for (size_t i = 0; i < ff.size(); ++i) 
        if (!ff[i]->has_area())
            return false;
    return true;
}

fp CompoundFunction::area() const
{
    fp a = 0;
    vector<Function*> const& ff = vmgr.get_functions();
    for (size_t i = 0; i < ff.size(); ++i) {
        a += ff[i]->area();
    }
    return a;
}

bool CompoundFunction::get_nonzero_range(fp level, fp& left, fp& right) const
{ 
    vector<Function*> const& ff = vmgr.get_functions();
    if (ff.size() == 1) 
        return ff[0]->get_nonzero_range(level, left, right);
    else
        return false; 
}

///////////////////////////////////////////////////////////////////////
#define FUNC_CALCULATE_VALUE_BEGIN(NAME) \
void Func##NAME::calculate_value(vector<fp> const &xx, vector<fp> &yy) const\
{\
    int first, last; \
    get_nonzero_idx_range(xx, first, last); \
    for (int i = first; i < last; ++i) {\
        fp x = xx[i]; 


#define FUNC_CALCULATE_VALUE_END(VAL) \
        yy[i] += (VAL);\
    }\
}

#define FUNC_CALCULATE_VALUE(NAME, VAL) \
    FUNC_CALCULATE_VALUE_BEGIN(NAME) \
    FUNC_CALCULATE_VALUE_END(VAL)


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


#define FUNC_CALCULATE_VALUE_DERIV_BEGIN(NAME) \
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


#define FUNC_CALCULATE_VALUE_DERIV_END(VAL) \
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


FUNC_CALCULATE_VALUE(Linear, vv[0] + x*vv[1])

FUNC_CALCULATE_VALUE_DERIV_BEGIN(Linear)
    dy_dv[0] = 1.;
    dy_dv[1] = x;
    dy_dx = vv[1];
FUNC_CALCULATE_VALUE_DERIV_END(vv[0] + x*vv[1])

///////////////////////////////////////////////////////////////////////

const char *FuncQuadratic::formula 
= "Quadratic(a0=height,a1=0, a2=0) = a0 + a1*x + a2*x^2"; 


FUNC_CALCULATE_VALUE(Quadratic, vv[0] + x*vv[1] + x*x*vv[2])

FUNC_CALCULATE_VALUE_DERIV_BEGIN(Quadratic)
    dy_dv[0] = 1.;
    dy_dv[1] = x;
    dy_dv[2] = x*x;
    dy_dx = vv[1] + 2*x*vv[2];
FUNC_CALCULATE_VALUE_DERIV_END(vv[0] + x*vv[1] + x*x*vv[2])

///////////////////////////////////////////////////////////////////////

const char *FuncCubic::formula 
= "Cubic(a0=height,a1=0, a2=0, a3=0) = a0 + a1*x + a2*x^2 + a3*x^3"; 


FUNC_CALCULATE_VALUE(Cubic, vv[0] + x*vv[1] + x*x*vv[2] + x*x*x*vv[3])

FUNC_CALCULATE_VALUE_DERIV_BEGIN(Cubic)
    dy_dv[0] = 1.;
    dy_dv[1] = x;
    dy_dv[2] = x*x;
    dy_dv[3] = x*x*x;
    dy_dx = vv[1] + 2*x*vv[2] + 3*x*x*vv[3];
FUNC_CALCULATE_VALUE_DERIV_END(vv[0] + x*vv[1] + x*x*vv[2] + x*x*x*vv[3])

///////////////////////////////////////////////////////////////////////

const char *FuncPolynomial4::formula 
= "Polynomial4(a0=height,a1=0, a2=0, a3=0, a4=0) = "
                                 "a0 + a1*x + a2*x^2 + a3*x^3 + a4*x^4"; 


FUNC_CALCULATE_VALUE(Polynomial4, vv[0] + x*vv[1] + x*x*vv[2] 
                                         + x*x*x*vv[3] + x*x*x*x*vv[4])

FUNC_CALCULATE_VALUE_DERIV_BEGIN(Polynomial4)
    dy_dv[0] = 1.;
    dy_dv[1] = x;
    dy_dv[2] = x*x;
    dy_dv[3] = x*x*x;
    dy_dv[4] = x*x*x*x;
    dy_dx = vv[1] + 2*x*vv[2] + 3*x*x*vv[3] + 4*x*x*x*vv[4];
FUNC_CALCULATE_VALUE_DERIV_END(vv[0] + x*vv[1] + x*x*vv[2] + x*x*x*vv[3]
                                      + x*x*x*x*vv[4])

///////////////////////////////////////////////////////////////////////

const char *FuncPolynomial5::formula 
= "Polynomial5(a0=height,a1=0, a2=0, a3=0, a4=0, a5=0) = "
                             "a0 + a1*x + a2*x^2 + a3*x^3 + a4*x^4 + a5*x^5"; 


FUNC_CALCULATE_VALUE(Polynomial5, vv[0] + x*vv[1] + x*x*vv[2] 
                               + x*x*x*vv[3] + x*x*x*x*vv[4] + x*x*x*x*x*vv[5])

FUNC_CALCULATE_VALUE_DERIV_BEGIN(Polynomial5)
    dy_dv[0] = 1.;
    dy_dv[1] = x;
    dy_dv[2] = x*x;
    dy_dv[3] = x*x*x;
    dy_dv[4] = x*x*x*x;
    dy_dv[5] = x*x*x*x*x;
    dy_dx = vv[1] + 2*x*vv[2] + 3*x*x*vv[3] + 4*x*x*x*vv[4] + 5*x*x*x*x*vv[5];
FUNC_CALCULATE_VALUE_DERIV_END(vv[0] + x*vv[1] + x*x*vv[2] 
                               + x*x*x*vv[3] + x*x*x*x*vv[4] + x*x*x*x*x*vv[5])

///////////////////////////////////////////////////////////////////////

const char *FuncPolynomial6::formula 
= "Polynomial6(a0=height,a1=0, a2=0, a3=0, a4=0, a5=0, a6=0) = "
                     "a0 + a1*x + a2*x^2 + a3*x^3 + a4*x^4 + a5*x^5 + a6*x^6"; 


FUNC_CALCULATE_VALUE(Polynomial6, vv[0] + x*vv[1] + x*x*vv[2] 
                               + x*x*x*vv[3] + x*x*x*x*vv[4] + x*x*x*x*x*vv[5]
                               + x*x*x*x*x*x*vv[6])

FUNC_CALCULATE_VALUE_DERIV_BEGIN(Polynomial6)
    dy_dv[0] = 1.;
    dy_dv[1] = x;
    dy_dv[2] = x*x;
    dy_dv[3] = x*x*x;
    dy_dv[4] = x*x*x*x;
    dy_dv[5] = x*x*x*x*x;
    dy_dv[6] = x*x*x*x*x*x;
    dy_dx = vv[1] + 2*x*vv[2] + 3*x*x*vv[3] + 4*x*x*x*vv[4] + 5*x*x*x*x*vv[5]
            + 6*x*x*x*x*x*vv[6];
FUNC_CALCULATE_VALUE_DERIV_END(vv[0] + x*vv[1] + x*x*vv[2] 
                               + x*x*x*vv[3] + x*x*x*x*vv[4] + x*x*x*x*x*vv[5]
                               + x*x*x*x*x*x*vv[6])

///////////////////////////////////////////////////////////////////////

const char *FuncGaussian::formula 
= "Gaussian(height, center, hwhm) = "
               "height*exp(-ln(2)*((x-center)/hwhm)^2)"; 

void FuncGaussian::do_precomputations(vector<Variable*> const &variables) 
{ 
    Function::do_precomputations(variables);
    if (fabs(vv[2]) < EPSILON) 
        vv[2] = EPSILON; 
}

FUNC_CALCULATE_VALUE_BEGIN(Gaussian)
    fp xa1a2 = (x - vv[1]) / vv[2];
    fp ex = exp_ (- M_LN2 * xa1a2 * xa1a2);
FUNC_CALCULATE_VALUE_END(vv[0] * ex)

FUNC_CALCULATE_VALUE_DERIV_BEGIN(Gaussian)
    fp xa1a2 = (x - vv[1]) / vv[2];
    fp ex = exp_ (- M_LN2 * xa1a2 * xa1a2);
    dy_dv[0] = ex;
    fp dcenter = 2 * M_LN2 * vv[0] * ex * xa1a2 / vv[2];
    dy_dv[1] = dcenter;
    dy_dv[2] = dcenter * xa1a2;
    dy_dx = -dcenter;
FUNC_CALCULATE_VALUE_DERIV_END(vv[0]*ex)

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
= "SplitGaussian(height, center, hwhm1=fwhm*0.5, hwhm2=fwhm*0.5) = "
                   "height*exp(-ln(2)*((x-center)/(x<center?hwhm1:hwhm2))^2)"; 

void FuncSplitGaussian::do_precomputations(vector<Variable*> const &variables) 
{ 
    Function::do_precomputations(variables);
    if (fabs(vv[2]) < EPSILON) 
        vv[2] = EPSILON; 
    if (fabs(vv[3]) < EPSILON) 
        vv[3] = EPSILON; 
}

FUNC_CALCULATE_VALUE_BEGIN(SplitGaussian)
    fp hwhm = (x < vv[1] ? vv[2] : vv[3]);
    fp xa1a2 = (x - vv[1]) / hwhm;
    fp ex = exp_ (- M_LN2 * xa1a2 * xa1a2);
FUNC_CALCULATE_VALUE_END(vv[0] * ex)

FUNC_CALCULATE_VALUE_DERIV_BEGIN(SplitGaussian)
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
FUNC_CALCULATE_VALUE_DERIV_END(vv[0]*ex)

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

FUNC_CALCULATE_VALUE_BEGIN(Lorentzian)
    fp xa1a2 = (x - vv[1]) / vv[2];
    fp inv_denomin = 1. / (1 + xa1a2 * xa1a2);
FUNC_CALCULATE_VALUE_END(vv[0] * inv_denomin)

FUNC_CALCULATE_VALUE_DERIV_BEGIN(Lorentzian)
    fp xa1a2 = (x - vv[1]) / vv[2];
    fp inv_denomin = 1. / (1 + xa1a2 * xa1a2);
    dy_dv[0] = inv_denomin;
    fp dcenter = 2 * vv[0] * xa1a2 / vv[2] * inv_denomin * inv_denomin;
    dy_dv[1] = dcenter;
    dy_dv[2] = dcenter * xa1a2;
    dy_dx = -dcenter;
FUNC_CALCULATE_VALUE_DERIV_END(vv[0] * inv_denomin)

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

FUNC_CALCULATE_VALUE_BEGIN(Pearson7)
    fp xa1a2 = (x - vv[1]) / vv[2];
    fp xa1a2sq = xa1a2 * xa1a2;
    fp pow_2_1_a3_1 = vv[4]; //pow (2, 1. / a3) - 1;
    fp denom_base = 1 + xa1a2sq * pow_2_1_a3_1;
    fp inv_denomin = pow (denom_base, - vv[3]);
FUNC_CALCULATE_VALUE_END(vv[0] * inv_denomin)

FUNC_CALCULATE_VALUE_DERIV_BEGIN(Pearson7)
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
FUNC_CALCULATE_VALUE_DERIV_END(vv[0] * inv_denomin)


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
= "SplitPearson7(height, center, hwhm1=fwhm*0.5, hwhm2=fwhm*0.5, "
                                                        "shape1=2, shape2=2) = "
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

FUNC_CALCULATE_VALUE_BEGIN(SplitPearson7)
    int lr = x < vv[1] ? 0 : 1;
    fp xa1a2 = (x - vv[1]) / vv[2+lr];
    fp xa1a2sq = xa1a2 * xa1a2;
    fp pow_2_1_a3_1 = vv[6+lr]; //pow(2, 1./shape) - 1;
    fp denom_base = 1 + xa1a2sq * pow_2_1_a3_1;
    fp inv_denomin = pow(denom_base, - vv[4+lr]);
FUNC_CALCULATE_VALUE_END(vv[0] * inv_denomin)

FUNC_CALCULATE_VALUE_DERIV_BEGIN(SplitPearson7)
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
FUNC_CALCULATE_VALUE_DERIV_END(vv[0] * inv_denomin)


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

FUNC_CALCULATE_VALUE_BEGIN(PseudoVoigt)
    fp xa1a2 = (x - vv[1]) / vv[2];
    fp ex = exp_ (- M_LN2 * xa1a2 * xa1a2);
    fp lor = 1. / (1 + xa1a2 * xa1a2);
    fp without_height =  (1-vv[3]) * ex + vv[3] * lor;
FUNC_CALCULATE_VALUE_END(vv[0] * without_height)

FUNC_CALCULATE_VALUE_DERIV_BEGIN(PseudoVoigt)
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
FUNC_CALCULATE_VALUE_DERIV_END(vv[0] * without_height)

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
= "Voigt(height, center, gwidth=fwhm*0.4, shape=0.1) ="
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

FUNC_CALCULATE_VALUE_BEGIN(Voigt)
    // humdev/humlik routines require with y (a3 here) parameter >0.
    float k;
    fp xa1a2 = (x - vv[1]) / vv[2];
    k = humlik(xa1a2, fabs(vv[3]));
FUNC_CALCULATE_VALUE_END(vv[0] * vv[4] * k)

FUNC_CALCULATE_VALUE_DERIV_BEGIN(Voigt)
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
FUNC_CALCULATE_VALUE_DERIV_END(a0a4 * k)

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
    return vv[0] * fabs(vv[2] * sqrt(M_PI) * vv[4]);
}
                                                            

///////////////////////////////////////////////////////////////////////

const char *FuncVoigtA::formula 
= "VoigtA(area, center, gwidth=fwhm*0.4, shape=0.1) = "
                            "convolution of Gaussian and Lorentzian";

void FuncVoigtA::do_precomputations(vector<Variable*> const &variables) 
{ 
    Function::do_precomputations(variables);
    if (vv.size() != 6)
        vv.resize(6);
    vv[4] = 1. / humlik(0, fabs(vv[3]));

    if (fabs(vv[2]) < EPSILON) 
        vv[2] = EPSILON; 
}

FUNC_CALCULATE_VALUE_BEGIN(VoigtA)
    // humdev/humlik routines require with y (a3 here) parameter >0.
    float k;
    fp xa1a2 = (x - vv[1]) / vv[2];
    k = humlik(xa1a2, fabs(vv[3]));
FUNC_CALCULATE_VALUE_END(vv[0] / (sqrt(M_PI) * vv[2]) * k)

FUNC_CALCULATE_VALUE_DERIV_BEGIN(VoigtA)
    // humdev/humlik routines require with y (a3 here) parameter >0.
    // here fabs(vv[3]) is used, and dy_dv[3] is negated if vv[3]<0.
    float k;
    fp xa1a2 = (x-vv[1]) / vv[2];
    fp f = vv[0] / (sqrt(M_PI) * vv[2]);
    float l, dkdx, dkdy;
    humdev(xa1a2, fabs(vv[3]), k, l, dkdx, dkdy);
    dy_dv[0] = k / (sqrt(M_PI) * vv[2]);
    fp dcenter = -f * dkdx / vv[2];
    dy_dv[1] = dcenter;
    dy_dv[2] = dcenter * xa1a2 - f * k / vv[2];
    dy_dv[3] = f * dkdy;
    if (vv[3] < 0)
        dy_dv[3] = -dy_dv[3];
    dy_dx = -dcenter;
FUNC_CALCULATE_VALUE_DERIV_END(f * k)

bool FuncVoigtA::get_nonzero_range (fp level, fp &left, fp &right) const
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
fp FuncVoigtA::fwhm() const 
{ 
    fp gauss_fwhm = 2 * fabs(vv[2]);
    fp const c0 = 2.0056;
    fp const c1 = 1.0593;
    fp phi = vv[3];
    return gauss_fwhm * (1 - c0*c1 + sqrt(phi*phi + 2*c1*phi + c0*c0*c1*c1));
}

fp FuncVoigtA::height() const
{
    return vv[0] / fabs(vv[2] * sqrt(M_PI) * vv[4]);
}
                                                            

///////////////////////////////////////////////////////////////////////

const char *FuncEMG::formula 
= "EMG(a=height, b=center, c=fwhm*0.4, d=fwhm*0.04) ="
                " a*c*(2*pi)^0.5/(2*d) * exp((b-x)/d + c^2/(2*d^2))"
                " * (sign(d) - erf((b-x)/(2^0.5*c) + c/(2^0.5*d)))";

void FuncEMG::do_precomputations(vector<Variable*> const &variables) 
{ 
    Function::do_precomputations(variables);
}

bool FuncEMG::get_nonzero_range(fp/*level*/, fp&/*left*/, fp&/*right*/) const
    { return false; }

FUNC_CALCULATE_VALUE_BEGIN(EMG)
    fp a = vv[0];
    fp bx = vv[1] - x;
    fp c = vv[2];
    fp d = vv[3];
    fp sqrt2 = sqrt(2.);
    fp fact = a*c*sqrt(2*M_PI)/(2*d);
    fp ex = exp(bx/d + c*c/(2*d*d));
    fp erf_arg = bx/(sqrt2*c) + c/(sqrt2*d);
    fp t = fact * ex * (d >= 0 ? erfc(erf_arg) : -erfc(-erf_arg));
    //fp t = fact * ex * (d >= 0 ? 1-erf(erf_arg) : -1-erf(erf_arg));
FUNC_CALCULATE_VALUE_END(t)

FUNC_CALCULATE_VALUE_DERIV_BEGIN(EMG)
    fp a = vv[0];
    fp bx = vv[1] - x;
    fp c = vv[2];
    fp d = vv[3];
    fp sqrt2 = sqrt(2.);
    fp cs2d = c/(sqrt2*d);
    fp cc = c*sqrt(M_PI/2)/d;
    fp ex = exp(bx/d + cs2d*cs2d); //==exp((c^2+2bd-2dx) / 2d^2)
    fp bx2c = bx/(sqrt2*c);
    fp erf_arg = bx2c + cs2d; //== (c*c+b*d-d*x)/(sqrt2*c*d);
    //fp er = erf(erf_arg);
    //fp d_sign = d >= 0 ? 1 : -1;
    //fp ser = d_sign - er;
    fp ser = (d >= 0 ? erfc(erf_arg) : -erfc(-erf_arg));
    fp t = cc * ex * ser;
    fp eee = exp(erf_arg*erf_arg);
    dy_dv[0] = t;
    dy_dv[1] = -a/d * ex / eee + a*t/d;
    dy_dv[2] = - a/(2*c*d*d*d)*exp(-bx2c*bx2c) 
                  * (2*d*(c*c-bx*d) - sqrt(2*M_PI)*c*(c*c+d*d) * eee * ser);
    //dy_dv[3] = a*c/(d*d*d)*ex * (c/eee 
    //                             - d_sign * (c*cc + sqrt(M_PI/2)*(b+d-x))
    //                             + sqrt(M_PI/2) * (c*c/d + b+d-x) * er);
    dy_dv[3] = a*c/(d*d*d)*ex * (c/eee - ser * (c*cc + sqrt(M_PI/2)*(bx+d)));
    dy_dx = - dy_dv[1];
FUNC_CALCULATE_VALUE_DERIV_END(a*t)

///////////////////////////////////////////////////////////////////////

const char *FuncDoniachSunjic::formula 
= "DoniachSunjic(h=height, a=0.1, F=1, E=center) ="
    "h * cos(pi*a/2 + (1-a)*atan((x-E)/F)) / (F^2+(x-E)^2)^((1-a)/2)";

bool FuncDoniachSunjic::get_nonzero_range(fp/*level*/, fp&/*left*/, 
                                          fp&/*right*/) const
{ return false; }

FUNC_CALCULATE_VALUE_BEGIN(DoniachSunjic)
    fp h = vv[0];
    fp a = vv[1];
    fp F = vv[2];
    fp xE = x - vv[3];
    fp t = h * cos(M_PI*a/2 + (1-a)*atan(xE/F)) / pow(F*F+xE*xE, (1-a)/2);
FUNC_CALCULATE_VALUE_END(t)

FUNC_CALCULATE_VALUE_DERIV_BEGIN(DoniachSunjic)
    fp h = vv[0];
    fp a = vv[1];
    fp F = vv[2];
    fp xE = x - vv[3];
    fp fe2 = F*F+xE*xE;
    fp ac = 1-a;
    fp p = pow(fe2, -ac/2);
    fp at = atan(xE/F);
    fp cos_arg = M_PI*a/2 + ac*at;
    fp co = cos(cos_arg);
    fp si = sin(cos_arg);
    fp t = co * p;
    dy_dv[0] = t;
    dy_dv[1] = h * p * (co/2 * log(fe2) + (at-M_PI/2) * si);
    dy_dv[2] = h * ac*p/fe2 * (xE*si - F*co);
    dy_dv[3] = h * ac*p/fe2 * (xE*co + si*F);
    dy_dx = -dy_dv[3];
FUNC_CALCULATE_VALUE_DERIV_END(h*t)
///////////////////////////////////////////////////////////////////////

const char *FuncValente::formula 
= "Valente(amp=height, a=7.6, b=7.6, c=center, d=-6.9, n=0) ="
"amp*exp(-((exp(a)-exp(b)) * (x-c) + sqrt((exp(a)+exp(b))^2 * (x-c)^2" 
                    "+ 4*exp(d)))^exp(n))";

FUNC_CALCULATE_VALUE_BEGIN(Valente)
fp amp=vv[0], a=vv[1], b=vv[2], c=vv[3], d=vv[4], n=vv[5];
fp xc = x-c;
fp a_b = exp(a)-exp(b);
fp apb = exp(a)+exp(b);
fp base = (a_b * xc + sqrt(apb*apb * xc*xc + 4*exp(d)));
fp t = exp(-pow(base,exp(n)));
FUNC_CALCULATE_VALUE_END(amp*t)

FUNC_CALCULATE_VALUE_DERIV_BEGIN(Valente)
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
FUNC_CALCULATE_VALUE_DERIV_END(amp*t)
///////////////////////////////////////////////////////////////////////

const char *FuncPielaszekCube::formula 
= "PielaszekCube(a=height*0.016, center, r=300, s=150) = ..."; 


FUNC_CALCULATE_VALUE_BEGIN(PielaszekCube)
    fp height = vv[0];
    fp center = vv[1];
    fp R = vv[2];
    fp s = vv[3];
    fp s2 = s*s;
    fp s4 = s2*s2;
    fp R2 = R*R;

    fp q = (x-center);
    fp q2 = q*q;
    fp t = height * (-3*R*(-1 - (R2*(-1 +
                          pow(1 + (q2*s4)/R2, 1.5 - R2/(2.*s2))
                          * cos(2*(-1.5 + R2/(2.*s2)) * atan((q*s2)/R))))/
           (2.*q2*(-1.5 + R2/(2.*s2))* (-1 + R2/(2.*s2))*s4)))/
      (sqrt(2*M_PI)*q2*(-0.5 + R2/(2.*s2))* s2);
FUNC_CALCULATE_VALUE_END(t)

FUNC_CALCULATE_VALUE_DERIV_BEGIN(PielaszekCube)
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

    fp q = (x-center);
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
    dy_dx = dcenter;
FUNC_CALCULATE_VALUE_DERIV_END(height*t);



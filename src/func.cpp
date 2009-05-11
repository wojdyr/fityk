// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License version 2 or (at your option) 3
// $Id$

#include "common.h"
#include "func.h"
#include "bfunc.h"
#include "var.h"
#include "ui.h"
#include "settings.h"
#include "logic.h"
#include "calc.h"
#include "ast.h"

#include <memory>
#include <ctype.h>
#include <boost/spirit/core.hpp>

using namespace std;
using namespace boost::spirit;

std::vector<fp> Function::calc_val_xx(1);
std::vector<fp> Function::calc_val_yy(1);

Function::Function (Ftk const* F_,
                    string const &name_,
                    vector<string> const &vars,
                    string const &formula_)
    : VariableUser(name_, "%", vars),
      type_formula(formula_),
      type_name(get_typename_from_formula(formula_)),
      type_var_names(get_varnames_from_formula(formula_)),
      type_rhs(get_rhs_from_formula(formula_)),
      nv(vars.size()),
      F(F_),
      center_idx(find_center_in_typevars()),
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

/// returns type variable names
/// can be used also for eg. Foo(3+$bleh, area/fwhm/sqrt(pi/ln(2)))
vector<string> Function::get_varnames_from_formula(string const& formula)
{
    vector<string> names;
    int lb = formula.find('(');
    int rb = find_matching_bracket(formula, lb);
    string all_names(formula, lb+1, rb-lb-1);
    if (strip_string(all_names).empty())
        return names;
    vector<string> nd = split_string(all_names, ',');
    for (vector<string>::const_iterator i = nd.begin(); i != nd.end(); ++i) {
        string::size_type eq = i->find('=');
        if (eq == string::npos)
            names.push_back(strip_string(*i));
        else
            names.push_back(strip_string(string(*i, 0, eq)));
    }
    return names;
}

/// returns type variable default values
vector<string> Function::get_defvalues_from_formula(string const& formula)
{
    int lb = formula.find('(');
    int rb = find_matching_bracket(formula, lb);
    string all_names(formula, lb+1, rb-lb-1);
    vector<string> nd = split_string(all_names, ',');
    vector<string> defaults;
    for (vector<string>::const_iterator i = nd.begin(); i != nd.end(); ++i) {
        string::size_type eq = i->find('=');
        if (eq == string::npos)
            defaults.push_back(string());
        else
            defaults.push_back(strip_string(string(*i, eq+1)));
    }
    return defaults;
}

int Function::find_center_in_typevars() const
{
      if (contains_element(type_var_names, "center"))
          return find(type_var_names.begin(), type_var_names.end(), "center")
                                                      - type_var_names.begin();
      else
          return -1;
}


Function* Function::factory (Ftk const* F,
                             string const &name_, string const &type_name,
                             vector<string> const &vars)
{
    string name = name_[0] == '%' ? string(name_, 1) : name_;

#define FACTORY_FUNC(NAME) \
    if (type_name == #NAME) \
        return new Func##NAME(F, name, vars);

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
    FACTORY_FUNC(PielaszekCube)
    FACTORY_FUNC(LogNormal)
    else if (UdfContainer::is_defined(type_name)) {
        UdfContainer::UDF const* udf = UdfContainer::get_udf(type_name);
        if (udf->is_compound)
            return new CompoundFunction(F, name, type_name, vars);
        else
            return new CustomFunction(F, name, type_name, vars,
                                      udf->op_trees);
    }
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
    FuncPielaszekCube::formula,
    FuncLogNormal::formula
};

vector<string> Function::get_all_types()
{
    vector<string> types;
    int nb = sizeof(builtin_formulas)/sizeof(builtin_formulas[0]);
    for (int i = 0; i < nb; ++i)
        types.push_back(get_typename_from_formula(builtin_formulas[i]));
    vector<UdfContainer::UDF> const& uff = UdfContainer::get_udfs();
    for (vector<UdfContainer::UDF>::const_iterator i = uff.begin();
                                                        i != uff.end(); ++i)
        types.push_back(i->name);
    return types;
}

string Function::get_formula(int n)
{
    assert (n >= 0);
    int nb = sizeof(builtin_formulas) / sizeof(builtin_formulas[0]);
    if (n < nb)
        return builtin_formulas[n];
    UdfContainer::UDF const* udf = UdfContainer::get_udf(n - nb);
    if (udf)
        return udf->formula;
    else
        return "";
}

string Function::get_formula(string const& type)
{
    int nb = sizeof(builtin_formulas) / sizeof(builtin_formulas[0]);
    for (int i = 0; i < nb; ++i)
        if (get_typename_from_formula(builtin_formulas[i]) == type)
            return builtin_formulas[i];
    UdfContainer::UDF const* udf = UdfContainer::get_udf(type);
    if (udf)
        return udf->formula;
    return "";
}

/// returns: 2 if built-in, in C++, 1 if buit-in UDF, 0 if defined by user
int Function::is_builtin(int n)
{
    int nb = sizeof(builtin_formulas) / sizeof(builtin_formulas[0]);
    assert (n >= 0 && n < nb + size(UdfContainer::udfs));
    if (n < nb)
        return 2;
    else if (UdfContainer::udfs[n-nb].is_builtin)
        return 1;
    else
        return 0;
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
    this->more_precomputations();
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
    bool r = get_nonzero_range(F->get_settings()->get_cut_level(), left, right);
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
    Function* this_ = const_cast<Function*>(this);
    for (int i = 0; i < min(size(alt_vv), size(vv)); ++i)
        this_->vv[i] = alt_vv[i];
    this_->precomputations_for_alternative_vv();
    calculate_value(x, y);
    this_->vv = backup_vv;
    this_->more_precomputations();
}

bool Function::has_other_prop(std::string const& name)
{
    return contains_element(get_other_prop_names(), name);
}

std::string Function::other_props_str() const
{
    string r;
    vector<string> v = get_other_prop_names();
    for (vector<string>::const_iterator i = v.begin(); i != v.end(); ++i)
        r += (i == v.begin() ? "" : "\n") + *i + ": " + S(other_prop(*i));
    return r;
}

string Function::get_info(vector<Variable*> const &variables,
                          vector<fp> const &parameters,
                          bool extended) const
{
    string s = get_basic_assignment();
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
        if (this->has_other_props())
            s += "\n" + other_props_str();
    }
    return s;
}

/// return sth like: Linear($foo, $_2)
string Function::get_basic_assignment() const
{
    vector<string> xvarnames;
    for (vector<string>::const_iterator i = varnames.begin();
            i != varnames.end(); ++i)
        xvarnames.push_back("$" + *i);
    return xname + " = " + type_name+ "(" + join_vector(xvarnames, ", ") + ")";
}

/// return sth like: Linear(a0=$foo, a1=~3.5)
string Function::get_current_assignment(vector<Variable*> const &variables,
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
    if (contains_element(t, '#')) {
        vector<fp> values(vv.begin(), vv.begin() + nv);
        t = type_name + "(" + join_vector(values, ", ") + ")";
    }
    else {
        for (size_t i = 0; i < type_var_names.size(); ++i)
            replace_words(t, type_var_names[i], S(get_var_value(i)));
    }

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

bool Function::get_param_value_safe(string const& param, fp &value) const
{
    vector<string>::const_iterator i = find(type_var_names.begin(),
                                            type_var_names.end(), param);
    if (i == type_var_names.end())
        return false;
    value = get_var_value(i - type_var_names.begin());
    return true;
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
fp Function::find_x_with_value(fp x1, fp x2, fp val, int max_iter) const
{
    fp y1 = calculate_value(x1) - val;
    fp y2 = calculate_value(x2) - val;
    if ((y1 > 0 && y2 > 0) || (y1 < 0 && y2 < 0))
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
        swap(x1, x2);
    fp t = (x1 + x2) / 2.;
    for (int i = 0; i < max_iter; ++i) {
        //check if converged
        if (is_eq(x1, x2))
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
        if ((t+dx > x2 && t+dx > x1) || (t+dx < x2 && t+dx < x1)  // outside
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
fp Function::find_extremum(fp x1, fp x2, int max_iter) const
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

    if ((y1 > 0 && y2 > 0) || (y1 < 0 && y2 < 0))
        throw ExecuteError("Derivatives at " + S(x1) + " and " + S(x2)
                           + " have the same sign.");
    if (y1 == 0)
        return x1;
    if (y2 == 0)
        return x2;
    if (y1 > 0)
        swap(x1, x2);
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
        if (is_eq(x1, x2))
            return (x1+x2) / 2.;
    }
    throw ExecuteError("The search has not converged in " + S(max_iter)
                       + " steps");
}

///////////////////////////////////////////////////////////////////////

namespace UdfContainer {

vector<UDF> udfs;

void initialize_udfs()
{
    vector<string> formulae = split_string(
"GaussianA(area, center, hwhm) = Gaussian(area/hwhm/sqrt(pi/ln(2)), center, hwhm)\n"
"LorentzianA(area, center, hwhm) = Lorentzian(area/hwhm/pi, center, hwhm)\n"
"Pearson7A(area, center, hwhm, shape=2) = Pearson7(area/(hwhm*exp(lgamma(shape-0.5)-lgamma(shape))*sqrt(pi/(2^(1/shape)-1))), center, hwhm, shape)\n"
"PseudoVoigtA(area, center, hwhm, shape=0.5) = GaussianA(area*(1-shape), center, hwhm) + LorentzianA(area*shape, center, hwhm)\n"
"ExpDecay(a=0, t=1) = a*exp(-x/t)\n"
"LogNormalA(area, center, width=fwhm, asym=0.1) = LogNormal(sqrt(ln(2)/pi)*(2*area/width)*exp(-asym^2/4/ln(2)), center, width, asym)",
  "\n");
    udfs.clear();
    for (vector<string>::const_iterator i = formulae.begin();
                                                    i != formulae.end(); ++i)
        udfs.push_back(UDF(*i, true));
}

bool is_compounded(string const& formula)
{
    string::size_type t = formula.rfind('=');
    assert(t != string::npos);
    t = formula.find_first_not_of(" \t\r\n", t+1);
    assert(t != string::npos);
    return isupper(formula[t]);
}

vector<OpTree*> make_op_trees(string const& formula)
{
    string rhs = Function::get_rhs_from_formula(formula);
    tree_parse_info<> info = ast_parse(rhs.c_str(), FuncG >> end_p, space_p);
    assert(info.full);
    vector<string> vars = find_tokens_in_ptree(FuncGrammar::variableID, info);
    vector<string> lhs_vars = Function::get_varnames_from_formula(formula);
    lhs_vars.push_back("x");
    for (vector<string>::const_iterator i = vars.begin(); i != vars.end(); i++)
        if (find(lhs_vars.begin(), lhs_vars.end(), *i) == lhs_vars.end())
            throw ExecuteError("variable `" + *i
                                           + "' only at the right hand side.");
    vector<OpTree*> op_trees = calculate_deriv(info.trees.begin(), lhs_vars);
    return op_trees;
}

void check_fudf_rhs(string const& rhs, vector<string> const& lhs_vars)
{
    if (rhs.empty())
        throw ExecuteError("No formula");
    tree_parse_info<> info = ast_parse(rhs.c_str(), FuncG >> end_p, space_p);
    if (!info.full)
        throw ExecuteError("Syntax error in formula");
    vector<string> vars = find_tokens_in_ptree(FuncGrammar::variableID, info);
    for (vector<string>::const_iterator i = vars.begin(); i != vars.end(); ++i)
        if (*i != "x" && !contains_element(lhs_vars, *i)) {
            throw ExecuteError("Unexpected parameter in formula: " + *i);
        }
    for (vector<string>::const_iterator i = lhs_vars.begin();
                                                    i != lhs_vars.end(); ++i)
        if (!contains_element(vars, *i)) {
            throw ExecuteError("Unused parameter in formula: " + *i);
        }
}

void check_rhs(string const& rhs, vector<string> const& lhs_vars)
{
    string::size_type t = rhs.find_first_not_of(" \t\r\n");
    if (t != string::npos && isupper(rhs[t])) { //compound
        parse_info<> info
            = parse(rhs.c_str(),
                    ((lexeme_d[(upper_p >> +alnum_p)]
                      >> '(' >> (no_actions_d[FuncG]  % ',') >> ')'
                     ) % '+'
                    ) >> end_p,
                    space_p);
        if (!info.full)
            throw ExecuteError("Syntax error in compound formula.");
        vector<string> rf = get_cpd_rhs_components(rhs, false);
        for (vector<string>::const_iterator i = rf.begin(); i != rf.end(); ++i)
            check_cpd_rhs_function(*i, lhs_vars);
    }
    else { //udf, not compound
        check_fudf_rhs(rhs, lhs_vars);
    }
}

void define(std::string const &formula)
{
    string type = Function::get_typename_from_formula(formula);
    vector<string> lhs_vars = Function::get_varnames_from_formula(formula);
    for (vector<string>::const_iterator i = lhs_vars.begin();
                                                    i != lhs_vars.end(); i++) {
        if (*i == "x")
            throw ExecuteError("x should not be given explicitly as "
                               "function type parameters.");
        else if (!islower((*i)[0]))
            throw ExecuteError("Improper variable: " + *i);
    }
    check_rhs(Function::get_rhs_from_formula(formula), lhs_vars);
    if (is_defined(type) && !get_udf(type)->is_builtin) {
        //defined, but can be undefined; don't undefine function implicitely
        throw ExecuteError("Function `" + type + "' is already defined. "
                           "You can try to undefine it.");
    }
    else if (!Function::get_formula(type).empty()) { //not UDF -> built-in
        throw ExecuteError("Built-in functions can't be redefined.");
    }
    udfs.push_back(UDF(formula));
}

void undefine(std::string const &type)
{
    for (vector<UDF>::iterator i = udfs.begin(); i != udfs.end(); ++i)
        if (i->name == type) {
            if (i->is_builtin)
                throw ExecuteError("Built-in compound function "
                                                    "can't be undefined.");
            //check if other definitions depend on it
            for (vector<UDF>::const_iterator j = udfs.begin();
                                                  j != udfs.end(); ++j) {
                if (!j->is_builtin)
                    continue;
                vector<string> rf = get_cpd_rhs_components(j->formula, true);
                for (vector<string>::const_iterator k = rf.begin();
                                                          k != rf.end(); ++k) {
                    if (Function::get_typename_from_formula(*k) == type)
                        throw ExecuteError("Can not undefine function `" + type
                                            + "', because function `" + j->name
                                            + "' depends on it.");
                }
            }
            udfs.erase(i);
            return;
        }
    throw ExecuteError("Can not undefine function `" + type
                        + "' which is not defined");
}

UDF const* get_udf(std::string const &type)
{
    for (vector<UDF>::const_iterator i = udfs.begin(); i != udfs.end(); ++i)
        if (i->name == type)
            return &(*i);
    return 0;
}


///check for errors in function at RHS
void check_cpd_rhs_function(std::string const& fun,
                                          vector<string> const& lhs_vars)
{
    //check if component function is known
    string t = Function::get_typename_from_formula(fun);
    string tf = Function::get_formula(t);
    if (tf.empty())
        throw ExecuteError("definition based on undefined function `" + t +"'");
    //...and if it has proper number of parameters
    vector<string> tvars = Function::get_varnames_from_formula(tf);
    vector<string> gvars = Function::get_varnames_from_formula(fun);
    if (tvars.size() != gvars.size())
        throw ExecuteError("Function `" + t + "' requires "
                                      + S(tvars.size()) + " parameters.");
    // ... and check if these parameters are ok
    for (vector<string>::const_iterator j=gvars.begin(); j != gvars.end(); ++j){
        tree_parse_info<> info = ast_parse(j->c_str(), FuncG >> end_p, space_p);
        assert(info.full);
        vector<string> vars=find_tokens_in_ptree(FuncGrammar::variableID, info);
        if (contains_element(vars, "x"))
            throw ExecuteError("variable can not depend on x.");
        for (vector<string>::const_iterator k = vars.begin();
                                                        k != vars.end(); ++k)
            if ((*k)[0] != '~' && (*k)[0] != '{' && (*k)[0] != '$'
                    && (*k)[0] != '%' && !contains_element(lhs_vars, *k))
                throw ExecuteError("Improper variable given in parameter "
                              + S(j-gvars.begin()+1) + " of "+ t + ": " + *k);
    }
}

/// find components of RHS (split sum "A() + B() + ...")
vector<string> get_cpd_rhs_components(string const &formula, bool full)
{
    vector<string> result;
    string::size_type pos = (full ? formula.rfind('=') + 1 : 0),
                      rpos = 0;
    while (pos != string::npos) {
        rpos = find_matching_bracket(formula, formula.find('(', pos));
        if (rpos == string::npos)
            break;
        string fun = string(formula, pos, rpos-pos+1);
        pos = formula.find_first_not_of(" +", rpos+1);
        result.push_back(fun);
    }
    return result;
}

} // namespace UdfContainer

///////////////////////////////////////////////////////////////////////

CompoundFunction::CompoundFunction(Ftk const* F,
                                   string const &name,
                                   string const &type,
                                   vector<string> const &vars)
    : Function(F, name, vars, get_formula(type)),
      vmgr(F)
{
    vmgr.silent = true;
    for (int j = 0; j != nv; ++j)
        vmgr.assign_variable(varnames[j], ""); // mirror variables

    vector<string> rf = UdfContainer::get_cpd_rhs_components(type_formula,true);
    for (vector<string>::iterator i = rf.begin(); i != rf.end(); ++i) {
        for (int j = 0; j != nv; ++j) {
            replace_words(*i, type_var_names[j], vmgr.get_variable(j)->xname);
        }
        vmgr.assign_func("", get_typename_from_formula(*i),
                             get_varnames_from_formula(*i));
    }
}

void CompoundFunction::set_var_idx(vector<Variable*> const& variables)
{
    VariableUser::set_var_idx(variables);
    for (int i = 0; i < nv; ++i)
        vmgr.get_variable(i)->set_original(variables[get_var_idx(i)]);
}

/// vv was changed, but not variables, mirror variables in vmgr must be frozen
void CompoundFunction::precomputations_for_alternative_vv()
{
    vector<Variable const*> backup(nv);
    for (int i = 0; i < nv; ++i) {
        //prevent change in use_parameters()
        backup[i] = vmgr.get_variable(i)->freeze_original(vv[i]);
    }
    vmgr.use_parameters();
    for (int i = 0; i < nv; ++i) {
        vmgr.get_variable(i)->set_original(backup[i]);
    }
}

void CompoundFunction::more_precomputations()
{
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
                || (i > 0 && ff[i-1]->center() != ff[i]->center()))
            return false;
    }
    return true;
}

/// if consists of >1 functions and centers are in the same place
///  height is a sum of heights
bool CompoundFunction::has_height() const
{
    vector<Function*> const& ff = vmgr.get_functions();
    if (ff.size() == 1)
        return ff[0]->has_center();
    for (size_t i = 0; i < ff.size(); ++i) {
        if (!ff[i]->has_center() || !ff[i]->has_height()
                || (i > 0 && ff[i-1]->center() != ff[i]->center()))
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

CustomFunction::CustomFunction(Ftk const* F,
                               string const &name,
                               string const &type,
                               vector<string> const &vars,
                               vector<OpTree*> const& op_trees)
    : Function(F, name, vars, get_formula(type)),
      derivatives(nv+1),
      afo(op_trees, value, derivatives)
{
}


void CustomFunction::set_var_idx(vector<Variable*> const& variables)
{
    VariableUser::set_var_idx(variables);
    afo.tree_to_bytecode(var_idx.size());
}


void CustomFunction::more_precomputations()
{
    afo.prepare_optimized_codes(vv);
}

void CustomFunction::calculate_value(vector<fp> const &xx,
                                     vector<fp> &yy) const
{
    //int first, last;
    //get_nonzero_idx_range(xx, first, last);
    //for (int i = first; i < last; ++i) {
    for (size_t i = 0; i < xx.size(); ++i) {
        yy[i] += afo.run_vm_val(xx[i]);
    }
}

void CustomFunction::calculate_value_deriv(vector<fp> const &xx,
                                           vector<fp> &yy, vector<fp> &dy_da,
                                           bool in_dx) const
{
    int dyn = dy_da.size() / xx.size();
    for (size_t i = 0; i < yy.size(); ++i) {
        afo.run_vm_der(xx[i]);

        if (!in_dx) {
            yy[i] += value;
            for (vector<Multi>::const_iterator j = multi.begin();
                    j != multi.end(); ++j)
                dy_da[dyn*i+j->p] += derivatives[j->n] * j->mult;
            dy_da[dyn*i+dyn-1] += derivatives.back();
        }
        else {
            for (vector<Multi>::const_iterator j = multi.begin();
                    j != multi.end(); ++j)
                dy_da[dyn*i+j->p] += dy_da[dyn*i+dyn-1]
                                       * derivatives[j->n] * j->mult;
        }
    }
}




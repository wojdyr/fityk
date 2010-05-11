// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
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
#include <boost/spirit/include/classic_core.hpp>

using namespace std;
using namespace boost::spirit::classic;

std::vector<fp> Function::calc_val_xx(1);
std::vector<fp> Function::calc_val_yy(1);

static
string::size_type find_outer_comma(string const& s, string::size_type pos)
{
    while (1) {
        string::size_type c = s.find(',', pos);
        if (c == string::npos)
            return string::npos;
        if (count(s.begin() + pos, s.begin() + c, '(')
                == count(s.begin() + pos, s.begin() + c, ')'))
            return c;
        pos = c + 1;
    }
}

string Function::get_rhs_from_formula(string const &formula)
{
    string::size_type v = formula.find(" where ");
    string::size_type rhs_start = formula.rfind('=', v) + 1;

    if (v == string::npos) // no substitutions
        return strip_string(formula.substr(rhs_start));

    // substitude variables that go after "where"
    string rhs(formula, rhs_start, v - rhs_start);
    v += 7; // strlen(" where ");
    while (1) {
        string::size_type eq = formula.find('=', v);
        string var = strip_string(formula.substr(v, eq-v));
        if (var.empty())
            throw ExecuteError("Wrong syntax in formula after `where'");
        string::size_type comma = find_outer_comma(formula, eq + 1);
        string value(formula, eq + 1,
                     comma == string::npos ? string::npos : comma - (eq+1));
        replace_words(rhs, var, value);
        if (comma == string::npos)
            break;
        v = comma + 1;
    }
    return strip_string(rhs);
}

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
        if (udf->type == UdfContainer::kCompound) {
            CompoundFunction* f= new CompoundFunction(F, name, type_name, vars);
            f->init();
            return f;
        }
        else if (udf->type == UdfContainer::kSplit) {
            SplitFunction* f = new SplitFunction(F, name, type_name, vars);
            f->init();
            return f;
        }
        else if (udf->type == UdfContainer::kCustom) {
            return new CustomFunction(F, name, type_name, vars, udf->op_trees);
        }
        else
            assert(!"unexpected udf type");
    }
    else
        throw ExecuteError("Undefined type of function: " + type_name);
    return NULL; // to avoid warnings
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

Function::HowDefined Function::how_defined(int n)
{
    int nb = sizeof(builtin_formulas) / sizeof(builtin_formulas[0]);
    assert (n >= 0 && n < nb + size(UdfContainer::udfs));
    if (n < nb)
        return kCoded;
    else if (UdfContainer::udfs[n-nb].builtin)
        return kInterpreted;
    else
        return kUserDefined;
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


void Function::calculate_value(std::vector<fp> const &x,
                               std::vector<fp> &y) const
{
    fp left, right;
    bool r = get_nonzero_range(F->get_settings()->get_cut_level(), left, right);
    if (r) {
        int first = lower_bound(x.begin(), x.end(), left) - x.begin();
        int last = upper_bound(x.begin(), x.end(), right) - x.begin();
        this->calculate_value_in_range(x, y, first, last);
    }
    else
        this->calculate_value_in_range(x, y, 0, x.size());
}

fp Function::calculate_value(fp x) const
{
    calc_val_xx[0] = x;
    calc_val_yy[0] = 0.;
    calculate_value_in_range(calc_val_xx, calc_val_yy, 0, 1);
    return calc_val_yy[0];
}

void Function::calculate_value_deriv(std::vector<fp> const &x,
                                     std::vector<fp> &y,
                                     std::vector<fp> &dy_da,
                                     bool in_dx) const
{
    fp left, right;
    bool r = get_nonzero_range(F->get_settings()->get_cut_level(), left, right);
    if (r) {
        int first = lower_bound(x.begin(), x.end(), left) - x.begin();
        int last = upper_bound(x.begin(), x.end(), right) - x.begin();
        this->calculate_value_deriv_in_range(x, y, dy_da, in_dx, first, last);
    }
    else
        this->calculate_value_deriv_in_range(x, y, dy_da, in_dx, 0, x.size());
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

string Function::get_info(VariableManager const* mgr, bool extended) const
{
    string s = get_basic_assignment();
    if (extended) {
        s += "\n" + type_formula;
        for (int i = 0; i < size(var_idx); ++i)
            s += "\n" + type_var_names[i] + " = "
                + mgr->get_variable_info(mgr->get_variable(var_idx[i]), false);
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

const char* default_udfs[] = {
"ExpDecay(a=0, t=1) = "
    "a*exp(-x/t)",
"GaussianA(area, center, hwhm) = "
    "Gaussian(area/hwhm/sqrt(pi/ln(2)), center, hwhm)",
"LogNormalA(area, center, width=fwhm, asym=0.1) = "
    "LogNormal(sqrt(ln(2)/pi)*(2*area/width)*exp(-asym^2/4/ln(2)), center, width, asym)",
"LorentzianA(area, center, hwhm) = "
    "Lorentzian(area/hwhm/pi, center, hwhm)",
"Pearson7A(area, center, hwhm, shape=2) = "
    "Pearson7(area/(hwhm*exp(lgamma(shape-0.5)-lgamma(shape))*sqrt(pi/(2^(1/shape)-1))), center, hwhm, shape)",
"PseudoVoigtA(area, center, hwhm, shape=0.5) = "
    "GaussianA(area*(1-shape), center, hwhm) + "
    "LorentzianA(area*shape, center, hwhm)",
"SplitLorentzian(height, center, hwhm1=fwhm*0.5, hwhm2=fwhm*0.5) = "
    "x < center ? Lorentzian(height, center, hwhm1)"
    " : Lorentzian(height, center, hwhm2)",
"SplitPseudoVoigt(height, center, hwhm1=fwhm*0.5, hwhm2=fwhm*0.5, shape1=0.5, shape2=0.5) = "
    "x < center ? PseudoVoigt(height, center, hwhm1, shape1)"
    " : PseudoVoigt(height, center, hwhm2, shape2)",
};

vector<UDF> udfs;

void initialize_udfs()
{
    udfs.clear();
    int n = sizeof(default_udfs) / sizeof(default_udfs[0]);
    for (int i = 0; i != n; ++i)
        udfs.push_back(UDF(default_udfs[i], true));
}

// formula can be either a full formula or only RHS
UdfType get_udf_type(string const& formula)
{
    // locate the start of RHS (search for the last '=', because default
    // parameters also can have '=')
    string::size_type t = formula.rfind('=');
    // the string formula may contain only RHS
    if (t == string::npos)
        t = 0;
    else
        ++t;

    // skip blanks
    t = formula.find_first_not_of(" \t\r\n", t);
    if (t == string::npos)
        throw ExecuteError("Empty definition.");

    // we don't parse the formula here, we just determine type of the UDF
    // assuming that the formula is correct
    if (isupper(formula[t]))
        return kCompound;
    else if (formula.find('?', t) != string::npos)
        return kSplit;
    else
        return kCustom;
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

UDF::UDF(std::string const& formula_, bool is_builtin_)
    : formula(formula_),
      builtin(is_builtin_)
{
    name = Function::get_typename_from_formula(formula_);
    type = get_udf_type(formula_);
    if (type == kCustom)
        op_trees = make_op_trees(formula);
}

// that's oversimplified atm,
// we require spaces, because words if/then/else can be part of parameter name:
// "if ", " then " and " else ".
vector<string> get_if_then_else_parts(string const &formula, bool full)
{
    vector<string> parts;
    size_t pos = (full ? formula.rfind('=') + 1 : 0);
    size_t qmark_pos = formula.find('?', pos);
    if (qmark_pos == string::npos)
        throw ExecuteError("Wrong syntax of the formula.");
    size_t colon_pos = formula.find(':', qmark_pos);
    if (colon_pos == string::npos)
        throw ExecuteError("Wrong syntax of the formula: '?' requires ':'");
    parts.push_back(formula.substr(pos, qmark_pos - pos));
    parts.push_back(formula.substr(qmark_pos + 1, colon_pos - qmark_pos - 1));
    parts.push_back(formula.substr(colon_pos + 1));
    return parts;
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
    UdfType type = get_udf_type(rhs);
    if (type == kCompound) {
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
    else if (type == kSplit) {
        vector<string> parts = get_if_then_else_parts(rhs, false);
        assert(parts.size() == 3);

        check_cpd_rhs_function(parts[1], lhs_vars);
        check_cpd_rhs_function(parts[2], lhs_vars);

        tree_parse_info<> info = ast_parse(parts[0].c_str(),
                                           str_p("x") >> "<" >> FuncG >> end_p,
                                           space_p);
        if (!info.full)
            throw ExecuteError("Syntax error in the the split condition.");
        vector<string> vars = find_tokens_in_ptree(FuncGrammar::variableID,
                                                   info);
        for (vector<string>::const_iterator i = vars.begin();
                                                        i != vars.end(); ++i)
            if (!contains_element(lhs_vars, *i))
                throw ExecuteError(
                        "Unexpected parameter in the condition: " + *i);
    }
    else if (type == kCustom) {
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
    if (is_defined(type) && !get_udf(type)->builtin) {
        //defined, but can be undefined; don't undefine function implicitely
        throw ExecuteError("Function `" + type + "' is already defined. "
                           "You can try to undefine it.");
    }
    else if (!Function::get_formula(type).empty()) { //not UDF -> built-in
        throw ExecuteError("Built-in functions can't be redefined.");
    }
    udfs.push_back(UDF(formula));
}

bool is_definition_dependend_on(UDF const& udf, string const& type)
{
    if (udf.type == kCompound) {
        vector<string> rf = get_cpd_rhs_components(udf.formula, true);
        for (vector<string>::const_iterator k = rf.begin(); k != rf.end(); ++k){
            if (Function::get_typename_from_formula(*k) == type)
                return true;
        }
        return false;
    }
    else if (udf.type == kSplit) {
        vector<string> rf = get_if_then_else_parts(udf.formula, true);
        return Function::get_typename_from_formula(rf[1]) == type ||
               Function::get_typename_from_formula(rf[2]) == type;
    }
    else
        return false;
}

void undefine(std::string const &type)
{
    for (vector<UDF>::iterator i = udfs.begin(); i != udfs.end(); ++i)
        if (i->name == type) {
            if (i->builtin)
                throw ExecuteError("Built-in compound function "
                                                    "can't be undefined.");
            //check if other definitions depend on it
            for (vector<UDF>::const_iterator j = udfs.begin();
                                                  j != udfs.end(); ++j) {
                // built-in function can't depend on user-defined
                if (!j->builtin && is_definition_dependend_on(*j, type))
                    throw ExecuteError("Can not undefine function `" + type
                                        + "', because function `" + j->name
                                        + "' depends on it.");
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
    // ... and check these parameters
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

/// find components of RHS (split sum "A(...) + B(...) + ...")
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
}

void CompoundFunction::init()
{
    vector<string> rf = UdfContainer::get_cpd_rhs_components(type_formula,true);
    init_components(rf);
}

void CompoundFunction::init_components(vector<string>& rf)
{
    vmgr.silent = true;
    for (int j = 0; j != nv; ++j)
        vmgr.assign_variable(varnames[j], ""); // mirror variables

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

void CompoundFunction::calculate_value_in_range(vector<fp> const &xx,
                                                vector<fp> &yy,
                                                int first, int last) const
{
    vector<Function*> const& functions = vmgr.get_functions();
    for (vector<Function*>::const_iterator i = functions.begin();
            i != functions.end(); ++i)
        (*i)->calculate_value_in_range(xx, yy, first, last);
}

void CompoundFunction::calculate_value_deriv_in_range(
                                             vector<fp> const &xx,
                                             vector<fp> &yy, vector<fp> &dy_da,
                                             bool in_dx,
                                             int first, int last) const
{
    vector<Function*> const& functions = vmgr.get_functions();
    for (vector<Function*>::const_iterator i = functions.begin();
            i != functions.end(); ++i)
        (*i)->calculate_value_deriv_in_range(xx, yy, dy_da, in_dx, first, last);
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

void CustomFunction::calculate_value_in_range(vector<fp> const &xx,
                                              vector<fp> &yy,
                                              int first, int last) const
{
    for (int i = first; i < last; ++i) {
        yy[i] += afo.run_vm_val(xx[i]);
    }
}

void CustomFunction::calculate_value_deriv_in_range(
                                           vector<fp> const &xx,
                                           vector<fp> &yy, vector<fp> &dy_da,
                                           bool in_dx,
                                           int first, int last) const
{
    int dyn = dy_da.size() / xx.size();
    for (int i = first; i < last; ++i) {
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

///////////////////////////////////////////////////////////////////////

SplitFunction::SplitFunction(Ftk const* F,
                             string const &name,
                             string const &type,
                             vector<string> const &vars)
    : CompoundFunction(F, name, type, vars)
{
}

void SplitFunction::init()
{
    vector<string> rf = UdfContainer::get_if_then_else_parts(type_formula,true);
    string split_expr = rf[0].substr(rf[0].find('<') + 1);
    rf.erase(rf.begin());
    init_components(rf);
    for (int j = 0; j != nv; ++j)
        replace_words(split_expr, type_var_names[j],
                                  vmgr.get_variable(j)->xname);
    vmgr.assign_variable("", split_expr);
}

void SplitFunction::calculate_value_in_range(vector<fp> const &xx,
                                             vector<fp> &yy,
                                             int first, int last) const
{
    double xsplit = vmgr.get_variables().back()->get_value();
    int t = lower_bound(xx.begin(), xx.end(), xsplit) - xx.begin();
    vmgr.get_function(0)->calculate_value_in_range(xx, yy, first, t);
    vmgr.get_function(1)->calculate_value_in_range(xx, yy, t, last);
}

void SplitFunction::calculate_value_deriv_in_range(
                                          vector<fp> const &xx,
                                          vector<fp> &yy, vector<fp> &dy_da,
                                          bool in_dx,
                                          int first, int last) const
{
    double xsplit = vmgr.get_variables().back()->get_value();
    int t = lower_bound(xx.begin(), xx.end(), xsplit) - xx.begin();
    vmgr.get_function(0)->
        calculate_value_deriv_in_range(xx, yy, dy_da, in_dx, first, t);
    vmgr.get_function(1)->
        calculate_value_deriv_in_range(xx, yy, dy_da, in_dx, t, last);
}

string SplitFunction::get_current_formula(string const& x) const
{
    double xsplit = vmgr.get_variables().back()->get_value();
    return "if x < " + S(xsplit)
        + " then " + vmgr.get_function(0)->get_current_formula(x)
        + " else " + vmgr.get_function(1)->get_current_formula(x);
}

bool SplitFunction::get_nonzero_range(fp level, fp& left, fp& right) const
{
    vector<Function*> const& ff = vmgr.get_functions();
    fp dummy;
    return ff[0]->get_nonzero_range(level, left, dummy) &&
           ff[0]->get_nonzero_range(level, dummy, right);
}

bool SplitFunction::has_height() const
{
    vector<Function*> const& ff = vmgr.get_functions();
    return ff[0]->has_height() && ff[1]->has_height() &&
        is_eq(ff[0]->height(), ff[1]->height());
}

bool SplitFunction::has_center() const
{
    vector<Function*> const& ff = vmgr.get_functions();
    return ff[0]->has_center() && ff[1]->has_center() &&
        is_eq(ff[0]->center(), ff[1]->center());
}


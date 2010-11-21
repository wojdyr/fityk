// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
// $Id$

#include "func.h"
#include "common.h"
#include "bfunc.h"
#include "settings.h"
#include "logic.h"
#include "udf.h"

using namespace std;

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
        replace_words(rhs, var, "("+value+")");
        if (comma == string::npos)
            break;
        v = comma + 1;
    }
    return strip_string(rhs);
}

// Parsing formula (like in initialization below) for every instance
// of the class is not effective, but the overhead is negligible.
// The ease of adding new built-in function types is more important.
Function::Function (Ftk const* F,
                    string const &name,
                    vector<string> const &vars,
                    string const &formula_)
    : VariableUser(name, "%", vars),
      type_formula(formula_),
      type_name(get_typename_from_formula(formula_)),
      type_rhs(get_rhs_from_formula(formula_)),
      F_(F),
      vv_(vars.size())
{
}

void Function::init()
{
    type_params_ = get_varnames_from_formula(type_formula);
    center_idx_ = index_of_element(type_params_, "center");

    if (vv_.size() != type_params_.size())
        throw ExecuteError("Function " + type_name + " requires "
                           + S(type_params_.size()) + " parameters.");
}

void VarArgFunction::init()
{
    for (size_t i = 0; i != vv_.size(); ++i) {
        // so far all the va functions have parameters x1,y1,x2,y2,...
        string p = (i%2 == 0 ? "x" : "y") + S(i/2 + 1);
        type_params_.push_back(p);
    }
    center_idx_ = -1;
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
    vector_foreach (string, i, nd) {
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
    vector_foreach (string, i, nd) {
        string::size_type eq = i->find('=');
        if (eq == string::npos)
            defaults.push_back(string());
        else
            defaults.push_back(strip_string(string(*i, eq+1)));
    }
    return defaults;
}

Function* Function::factory (Ftk const* F,
                             string const &name, string const &type_name,
                             vector<string> const &vars)
{
    Function* f = NULL;

    if (false) {}

#define FACTORY_FUNC(NAME) \
    else if (type_name == #NAME) \
        f = new Func##NAME(F, name, vars);

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
    FACTORY_FUNC(Spline)
    FACTORY_FUNC(Polyline)

    else if (UdfContainer::is_defined(type_name)) {
        UdfContainer::UDF const* udf = UdfContainer::get_udf(type_name);
        if (udf->type == UdfContainer::kCompound) {
            f = new CompoundFunction(F, name, type_name, vars);
        }
        else if (udf->type == UdfContainer::kSplit) {
            f = new SplitFunction(F, name, type_name, vars);
        }
        else if (udf->type == UdfContainer::kCustom) {
            f = new CustomFunction(F, name, type_name, vars, udf->op_trees);
        }
        else
            assert(!"unexpected udf type");
    }
    else
        throw ExecuteError("Undefined type of function: " + type_name);
    f->init();
    return f; // to avoid warnings
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
    FuncLogNormal::formula,
    FuncSpline::formula,
    FuncPolyline::formula
};

vector<string> Function::get_all_types()
{
    vector<string> types;
    int nb = sizeof(builtin_formulas)/sizeof(builtin_formulas[0]);
    for (int i = 0; i < nb; ++i)
        types.push_back(get_typename_from_formula(builtin_formulas[i]));
    vector<UdfContainer::UDF> const& uff = UdfContainer::get_udfs();
    vector_foreach (UdfContainer::UDF, i, uff)
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
    multi_.clear();
    for (int i = 0; i < size(var_idx); ++i) {
        Variable const *v = variables[var_idx[i]];
        vv_[i] = v->get_value();
        vector_foreach (Variable::ParMult, j, v->recursive_derivatives())
            multi_.push_back(Multi(i, *j));
    }
    this->more_precomputations();
}

void Function::erased_parameter(int k)
{
    vectorm_foreach (Multi, i, multi_)
        if (i->p > k)
            -- i->p;
}


void Function::calculate_value(std::vector<fp> const &x,
                               std::vector<fp> &y) const
{
    fp left, right;
    double cut_level = F_->get_settings()->get_cut_level();
    bool r = get_nonzero_range(cut_level, left, right);
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
    double cut_level = F_->get_settings()->get_cut_level();
    bool r = get_nonzero_range(cut_level, left, right);
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
    vector<fp> backup_vv = vv_;
    Function* this_ = const_cast<Function*>(this);
    for (int i = 0; i < min(size(alt_vv), size(vv_)); ++i)
        this_->vv_[i] = alt_vv[i];
    this_->precomputations_for_alternative_vv();
    calculate_value(x, y);
    this_->vv_ = backup_vv;
    this_->more_precomputations();
}

bool Function::get_center(fp* a) const
{
    if (center_idx_ != -1) {
        *a = vv_[center_idx_];
        return true;
    }
    return false;
}

bool Function::get_iwidth(fp* a) const
{
    fp area, height;
    if (this->get_area(&area) && this->get_height(&height)) {
        *a = height != 0. ? area / height : 0.;
        return true;
    }
    return false;
}

string Function::get_par_info(VariableManager const* mgr) const
{
    string s = type_formula;
    for (int i = 0; i < size(var_idx); ++i) {
        s += "\n" + type_params_[i] + " = ";
        s += mgr->get_variable_info(mgr->get_variable(var_idx[i]));
    }
    fp a;
    if (this->get_center(&a))
        if (!contains_element(type_params_, string("center")))
            s += "\nCenter: " + S(a);
    if (this->get_height(&a))
        if (!contains_element(type_params_, string("height")))
            s += "\nHeight: " + S(a);
    if (this->get_fwhm(&a))
        if (!contains_element(type_params_, string("fwhm")))
            s += "\nFWHM: " + S(a);
    if (this->get_area(&a))
        if (!contains_element(type_params_, string("area")))
            s += "\nArea: " + S(a);
    vector_foreach (string, i, this->get_other_prop_names())
        s += "\n" + *i + ": " + S(get_other_prop(*i));
    return s;
}

/// return sth like: Linear($foo, $_2)
string Function::get_basic_assignment() const
{
    vector<string> xvarnames;
    vector_foreach (string, i, varnames)
        xvarnames.push_back("$" + *i);
    return xname + " = " + type_name+ "(" + join_vector(xvarnames, ", ") + ")";
}

/// return sth like: Linear(a0=$foo, a1=~3.5)
string Function::get_current_assignment(vector<Variable*> const &variables,
                                        vector<fp> const &parameters) const
{
    vector<string> vs;
    assert(type_params_.size() == var_idx.size());
    for (int i = 0; i < size(var_idx); ++i) {
        Variable const* v = variables[var_idx[i]];
        string t = type_params_[i] + "="
            + (v->is_simple() ? v->get_formula(parameters) : v->xname);
        vs.push_back(t);
    }
    return xname + " = " + type_name + "(" + join_vector(vs, ", ") + ")";
}

string Function::get_current_formula(string const& x) const
{
    string t = type_rhs;
    if (contains_element(t, '#')) {
        vector<fp> values(vv_.begin(), vv_.begin() + nv());
        t = type_name + "(" + join_vector(values, ", ") + ")";
    }
    else {
        for (size_t i = 0; i < type_params_.size(); ++i)
            replace_words(t, type_params_[i], S(get_var_value(i)));
    }

    replace_words(t, "x", x);
    return t;
}

int Function::get_param_nr(string const& param) const
{
    int n = get_param_nr_nothrow(param);
    if (n == -1)
        throw ExecuteError("function " + xname + " has no parameter: " + param);
    return n;
}

int Function::get_param_nr_nothrow(string const& param) const
{
    return index_of_element(type_params_, param);
}

bool Function::get_param_value_nothrow(string const& param, fp &value) const
{
    vector<string>::const_iterator i = find(type_params_.begin(),
                                            type_params_.end(), param);
    if (i == type_params_.end())
        return false;
    value = get_var_value(i - type_params_.begin());
    return true;
}

fp Function::get_param_value(string const& param) const
{
    if (param.empty())
        throw ExecuteError("Empty parameter name??");
    fp a;
    if (islower(param[0]))
        return get_var_value(get_param_nr(param));
    else if (param == "Center" && get_center(&a)) {
        return a;
    }
    else if (param == "Height" && get_height(&a)) {
        return a;
    }
    else if (param == "FWHM" && get_fwhm(&a)) {
        return a;
    }
    else if (param == "Area" && get_area(&a)) {
        return a;
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
    vector_foreach (Multi, j, multi_)
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
    vector_foreach (Multi, j, multi_)
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


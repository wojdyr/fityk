// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
// $Id: $

#include "udf.h"
#include "ast.h"

using namespace std;


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
"SplitLorentzian(height, center, hwhm1=fwhm/2, hwhm2=fwhm/2) = "
    "x < center ? Lorentzian(height, center, hwhm1)"
    " : Lorentzian(height, center, hwhm2)",
"SplitPseudoVoigt(height, center, hwhm1=fwhm*0.5, hwhm2=fwhm/2, shape1=0.5, shape2=0.5) = "
    "x < center ? PseudoVoigt(height, center, hwhm1, shape1)"
    " : PseudoVoigt(height, center, hwhm2, shape2)",
"SplitVoigt(height, center, hwhm1=fwhm/2, hwhm2=fwhm/2, shape1=0.5, shape2=0.5) = "
    "x < center ? Voigt(height, center, hwhm1, shape1)"
    " : Voigt(height, center, hwhm2, shape2)",
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
    return NULL;
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
      vmgr_(F)
{
}

void CompoundFunction::init()
{
    Function::init();
    vector<string> rf = UdfContainer::get_cpd_rhs_components(type_formula,true);
    init_components(rf);
}

void CompoundFunction::init_components(vector<string>& rf)
{
    vmgr_.silent = true;
    for (int j = 0; j != nv(); ++j)
        vmgr_.assign_variable(varnames[j], ""); // mirror variables

    for (vector<string>::iterator i = rf.begin(); i != rf.end(); ++i) {
        for (int j = 0; j != nv(); ++j) {
            replace_words(*i, type_params[j], vmgr_.get_variable(j)->xname);
        }
        vmgr_.assign_func("", get_typename_from_formula(*i),
                              get_varnames_from_formula(*i));
    }
}

void CompoundFunction::set_var_idx(vector<Variable*> const& variables)
{
    VariableUser::set_var_idx(variables);
    for (int i = 0; i < nv(); ++i)
        vmgr_.get_variable(i)->set_original(variables[get_var_idx(i)]);
}

/// vv was changed, but not variables, mirror variables in vmgr_ must be frozen
void CompoundFunction::precomputations_for_alternative_vv()
{
    vector<Variable const*> backup(nv());
    for (int i = 0; i < nv(); ++i) {
        //prevent change in use_parameters()
        backup[i] = vmgr_.get_variable(i)->freeze_original(vv[i]);
    }
    vmgr_.use_parameters();
    for (int i = 0; i < nv(); ++i) {
        vmgr_.get_variable(i)->set_original(backup[i]);
    }
}

void CompoundFunction::more_precomputations()
{
    vmgr_.use_parameters();
}

void CompoundFunction::calculate_value_in_range(vector<fp> const &xx,
                                                vector<fp> &yy,
                                                int first, int last) const
{
    vector<Function*> const& functions = vmgr_.get_functions();
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
    vector<Function*> const& functions = vmgr_.get_functions();
    for (vector<Function*>::const_iterator i = functions.begin();
            i != functions.end(); ++i)
        (*i)->calculate_value_deriv_in_range(xx, yy, dy_da, in_dx, first, last);
}

string CompoundFunction::get_current_formula(string const& x) const
{
    string t;
    vector<Function*> const& functions = vmgr_.get_functions();
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
    vector<Function*> const& ff = vmgr_.get_functions();
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
    vector<Function*> const& ff = vmgr_.get_functions();
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
    vector<Function*> const& ff = vmgr_.get_functions();
    for (size_t i = 0; i < ff.size(); ++i) {
        height += ff[i]->height();
    }
    return height;
}

bool CompoundFunction::has_fwhm() const
{
    vector<Function*> const& ff = vmgr_.get_functions();
    return ff.size() == 1 && ff[0]->has_fwhm();
}

fp CompoundFunction::fwhm() const
{
    return vmgr_.get_function(0)->fwhm();
}

bool CompoundFunction::has_area() const
{
    vector<Function*> const& ff = vmgr_.get_functions();
    for (size_t i = 0; i < ff.size(); ++i)
        if (!ff[i]->has_area())
            return false;
    return true;
}

fp CompoundFunction::area() const
{
    fp a = 0;
    vector<Function*> const& ff = vmgr_.get_functions();
    for (size_t i = 0; i < ff.size(); ++i) {
        a += ff[i]->area();
    }
    return a;
}

bool CompoundFunction::get_nonzero_range(fp level, fp& left, fp& right) const
{
    vector<Function*> const& ff = vmgr_.get_functions();
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
      // don't use nv() here, it's not set until init()
      derivatives_(vars.size()+1),
      afo_(op_trees, value_, derivatives_)
{
}


void CustomFunction::set_var_idx(vector<Variable*> const& variables)
{
    VariableUser::set_var_idx(variables);
    afo_.tree_to_bytecode(var_idx.size());
}


void CustomFunction::more_precomputations()
{
    afo_.prepare_optimized_codes(vv);
}

void CustomFunction::calculate_value_in_range(vector<fp> const &xx,
                                              vector<fp> &yy,
                                              int first, int last) const
{
    for (int i = first; i < last; ++i) {
        yy[i] += afo_.run_vm_val(xx[i]);
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
        afo_.run_vm_der(xx[i]);

        if (!in_dx) {
            yy[i] += value_;
            for (vector<Multi>::const_iterator j = multi.begin();
                    j != multi.end(); ++j)
                dy_da[dyn*i+j->p] += derivatives_[j->n] * j->mult;
            dy_da[dyn*i+dyn-1] += derivatives_.back();
        }
        else {
            for (vector<Multi>::const_iterator j = multi.begin();
                    j != multi.end(); ++j)
                dy_da[dyn*i+j->p] += dy_da[dyn*i+dyn-1]
                                       * derivatives_[j->n] * j->mult;
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
    Function::init();
    vector<string> rf = UdfContainer::get_if_then_else_parts(type_formula,true);
    string split_expr = rf[0].substr(rf[0].find('<') + 1);
    rf.erase(rf.begin());
    init_components(rf);
    for (int j = 0; j != nv(); ++j)
        replace_words(split_expr, type_params[j],
                                  vmgr_.get_variable(j)->xname);
    vmgr_.assign_variable("", split_expr);
}

void SplitFunction::calculate_value_in_range(vector<fp> const &xx,
                                             vector<fp> &yy,
                                             int first, int last) const
{
    double xsplit = vmgr_.get_variables().back()->get_value();
    int t = lower_bound(xx.begin(), xx.end(), xsplit) - xx.begin();
    vmgr_.get_function(0)->calculate_value_in_range(xx, yy, first, t);
    vmgr_.get_function(1)->calculate_value_in_range(xx, yy, t, last);
}

void SplitFunction::calculate_value_deriv_in_range(
                                          vector<fp> const &xx,
                                          vector<fp> &yy, vector<fp> &dy_da,
                                          bool in_dx,
                                          int first, int last) const
{
    double xsplit = vmgr_.get_variables().back()->get_value();
    int t = lower_bound(xx.begin(), xx.end(), xsplit) - xx.begin();
    vmgr_.get_function(0)->
        calculate_value_deriv_in_range(xx, yy, dy_da, in_dx, first, t);
    vmgr_.get_function(1)->
        calculate_value_deriv_in_range(xx, yy, dy_da, in_dx, t, last);
}

string SplitFunction::get_current_formula(string const& x) const
{
    double xsplit = vmgr_.get_variables().back()->get_value();
    return "x < " + S(xsplit)
        + " ? " + vmgr_.get_function(0)->get_current_formula(x)
        + " : " + vmgr_.get_function(1)->get_current_formula(x);
}

bool SplitFunction::get_nonzero_range(fp level, fp& left, fp& right) const
{
    vector<Function*> const& ff = vmgr_.get_functions();
    fp dummy;
    return ff[0]->get_nonzero_range(level, left, dummy) &&
           ff[0]->get_nonzero_range(level, dummy, right);
}

bool SplitFunction::has_height() const
{
    vector<Function*> const& ff = vmgr_.get_functions();
    return ff[0]->has_height() && ff[1]->has_height() &&
        is_eq(ff[0]->height(), ff[1]->height());
}

bool SplitFunction::has_center() const
{
    vector<Function*> const& ff = vmgr_.get_functions();
    return ff[0]->has_center() && ff[1]->has_center() &&
        is_eq(ff[0]->center(), ff[1]->center());
}


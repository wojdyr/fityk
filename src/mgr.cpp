// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
// $Id$

#include "mgr.h"
#include "common.h"
#include "var.h"
#include "datatrans.h"
#include "ast.h"
#include "ui.h"
#include "func.h"
#include "model.h"
#include "settings.h"
#include "logic.h" //VariableManager::get_or_make_variable() handles @0.F[1].a

#include <stdlib.h>
#include <ctype.h>
#include <boost/spirit/include/classic_core.hpp>
#include <algorithm>
#include <memory>

using namespace std;

VariableManager::~VariableManager()
{
    purge_all_elements(functions);
    purge_all_elements(variables);
}

void VariableManager::unregister_model(Model const *s)
{
    vector<Model*>::iterator k = find(models.begin(), models.end(), s);
    assert (k != models.end());
    models.erase(k);
}

void VariableManager::sort_variables()
{
    for (vector<Variable*>::iterator i = variables.begin();
            i != variables.end(); ++i)
        (*i)->set_var_idx(variables);
    int pos = 0;
    while (pos < size(variables)) {
        int M = variables[pos]->get_max_var_idx();
        if (M > pos) {
            swap(variables[pos], variables[M]);
            for (vector<Variable*>::iterator i = variables.begin();
                    i != variables.end(); ++i)
                (*i)->set_var_idx(variables);
        }
        else
            ++pos;
    }
}


/// takes string parsable by FuncGrammar and:
///  if the string refers to one variable -- returns its name
///  else makes variable and returns its name
string VariableManager::get_or_make_variable(string const& func)
{
    string ret;
    assert(!func.empty());
    string tmp1, tmp2;
    if (parse(func.c_str(), VariableLhsG).full) // $foo
        ret = string(func, 1);
    else if (parse(func.c_str(),
                   (FunctionLhsG
                   | !lexeme_d['@' >> uint_p >> '.']
                     >> (str_p("F[")|"Z[") >> int_p >> ch_p(']')
                   ) [assign_a(tmp1)]
                   >> '.' >>
                   lexeme_d[alpha_p >> *(alnum_p|'_')][assign_a(tmp2)]
                  ).full) {                     // %bar.bleh
        const Function* f = F->find_function_any(tmp1);
        ret = f->get_var_name(f->get_param_nr(tmp2));
    }
    else                                       // anything else
        ret = assign_variable("", func);
    return ret;
}


namespace {

void parse_and_set_domain(Variable *var, string const& domain_str)
{
    string::size_type lb = domain_str.find('[');
    string::size_type pm = domain_str.find("+-");
    string::size_type rb = domain_str.find(']');
    string ctr_str = strip_string(string(domain_str, lb+1, pm-(lb+1)));
    string sigma_str(domain_str, pm+2, rb-(pm+2));
    fp sigma = strtod(sigma_str.c_str(), 0);
    if (!ctr_str.empty()) {
        fp ctr = strtod(ctr_str.c_str(), 0);
        var->domain.set(ctr, sigma);
    }
    else
        var->domain.set_sigma(sigma);
}

string::size_type skip_variable_value(string const& s, string::size_type pos)
{
    string::size_type new_pos;
    if (s[pos] == '{')
        new_pos = s.find('}', pos) + 1;
    else {
        char const* s_c = s.c_str();
        char *endptr;
        strtod(s_c+pos, &endptr);
        new_pos = endptr - s_c;
    }
    while (new_pos < s.size() && isspace(s[new_pos]))
        ++new_pos;
    return new_pos;
}

inline
string strip_tilde_variable(string s)
{
    string::size_type pos = 0;
    while ((pos = s.find('~', pos)) != string::npos) {
        s.erase(pos, 1);
        assert(pos < s.size());
        pos = skip_variable_value(s, pos);
        if (pos < s.size()) {
            if (s[pos] == '[') {
                string::size_type right_b = s.find(']', pos);
                assert(right_b != string::npos);
                s.erase(pos, right_b-pos+1);
            }
        }
    }
    return s;
}

} //anonymous namespace


string VariableManager::assign_variable(string const &name, string const &rhs)
{
    Variable *var = 0;
    string nonempty_name = name.empty() ? next_var_name() : name;

    if (rhs.empty()) {// mirror-variable
        var = new Variable(nonempty_name, -2);
        return put_into_variables(var);
    }

    tree_parse_info<> info = ast_parse(rhs.c_str(), FuncG >> end_p, space_p);
    assert(info.full);
    const_tm_iter_t const &root = info.trees.begin();
    if (root->value.id() == FuncGrammar::variableID
            && *root->value.begin() == '~') { //simple variable
        string val_str = string(root->value.begin()+1, root->value.end());
        string domain_str;
        string::size_type pos = skip_variable_value(val_str, 0);
        if (pos < val_str.size() && val_str[pos] == '[') {
            domain_str = string(val_str, pos);
            val_str.erase(pos);
        }
        fp val = get_constant_value(val_str);
        int nr;

        // avoid changing order of parameters in case of "$_1 = ~1.23"
        int old_pos = find_variable_nr(name);
        if (old_pos != -1 && variables[old_pos]->is_simple()) {
            nr = variables[old_pos]->get_nr();
            parameters[nr] = val; //variable at old_pos will be deleted soon
        }
        else {
            nr = parameters.size();
            parameters.push_back(val);
        }
        var = new Variable(nonempty_name, nr);
        if (!domain_str.empty())
            parse_and_set_domain(var, domain_str);
    }
    else {
        vector<string> vars=find_tokens_in_ptree(FuncGrammar::variableID, info);
        if (contains_element(vars, "x"))
            throw ExecuteError("variable can't depend on x.");
        for (vector<string>::const_iterator i = vars.begin();
                                                i != vars.end(); i++)
            if ((*i)[0]!='~' && (*i)[0]!='{' && (*i)[0]!='$' && (*i)[0]!='%'
                && (*i)[0]!='@'
                && (((*i)[0]!='F' && (*i)[0]!='Z')
                                        || i->size() < 2 || (*i)[1]!='['))
                throw ExecuteError("`" + *i + "' can't be used as variable.");
        vector<OpTree*> op_trees = calculate_deriv(root, vars);
        // ~14.3 -> $var4
        for (vector<string>::iterator i = vars.begin(); i != vars.end(); ++i) {
            *i = get_or_make_variable(*i);
        }
        var = new Variable(nonempty_name, vars, op_trees);
    }
    return put_into_variables(var);
}

bool VariableManager::is_variable_referred(int i,
                                           vector<string> const &ignore_vars,
                                           string *first_referrer)
{
    for (int j = i+1; j < size(variables); ++j) {
        if (variables[j]->is_directly_dependent_on(i)
                    && !contains_element(ignore_vars, variables[j]->name)) {
            if (first_referrer)
                *first_referrer = variables[j]->xname;
            return true;
        }
    }
    for (vector<Function*>::iterator j = functions.begin();
            j != functions.end(); ++j) {
        if ((*j)->is_directly_dependent_on(i)) {
            if (first_referrer)
                *first_referrer = (*j)->xname;
            return true;
        }
    }
    return false;
}

vector<string>
VariableManager::get_variable_references(string const &name) const
{
    int idx = find_variable_nr(name);
    vector<string> refs;
    for (vector<Variable*>::const_iterator i = variables.begin();
            i != variables.end(); ++i)
        if ((*i)->is_directly_dependent_on(idx))
            refs.push_back((*i)->xname);
    for (vector<Function*>::const_iterator i = functions.begin();
            i != functions.end(); ++i)
        for (int j = 0; j < (*i)->get_vars_count(); ++j)
            if ((*i)->get_var_idx(j) == idx)
                refs.push_back((*i)->xname + "." + (*i)->get_param(j));
    return refs;
}

void VariableManager::remove_unreferred()
{
    // remove auto-delete marked variables, which are not referred by others
    for (int i = variables.size()-1; i >= 0; --i)
        if (variables[i]->is_auto_delete()) {
            if (!is_variable_referred(i)) {
                delete variables[i];
                variables.erase(variables.begin() + i);
            }
        }
    // re-index all functions and variables (in any case)
    for (vector<Variable*>::iterator i = variables.begin();
            i != variables.end(); ++i)
        (*i)->set_var_idx(variables);
    for (vector<Function*>::iterator i = functions.begin();
            i != functions.end(); ++i) {
        (*i)->set_var_idx(variables);
    }
    // remove unreferred parameters
    for (int i = size(parameters)-1; i >= 0; --i) {
        bool del=true;
        for (int j = 0; j < size(variables); ++j)
            if (variables[j]->get_nr() == i) {
                del=false;
                break;
            }
        if (del) {
            parameters.erase(parameters.begin() + i);
            // take care about parameter indices in variables and functions
            for (vector<Variable*>::iterator j = variables.begin();
                    j != variables.end(); ++j)
                (*j)->erased_parameter(i);
            for (vector<Function*>::iterator j = functions.begin();
                    j != functions.end(); ++j)
                (*j)->erased_parameter(i);
        }
    }
}

string VariableManager::get_variable_info(Variable const* v,
                                          bool extended) const
{
    string s = v->xname + " = " + v->get_formula(parameters) + " = "
               + F->get_settings()->format_double(v->get_value());
    if (v->domain.is_set())
        s += "  " + v->domain.str();
    if (v->is_auto_delete())
        s += "  [auto]";
    if (extended && v->get_nr() == -1) {
        vector<string> vn = concat_pairs("$", v->get_varnames());
        for (int i = 0; i < v->get_vars_count(); ++i)
            s += "\nd(" + v->xname + ")/d($" + v->get_var_name(i) + "): "
              + v->get_op_trees()[i]->str(&vn) + " == "
              + F->get_settings()->format_double(v->get_derivative(i));
    }
    return s;
}

/// puts Variable into "variables" vector, checking dependencies
string VariableManager::put_into_variables(Variable* new_var)
{
    auto_ptr<Variable> var(new_var);
    string var_name = var->name;
    var->set_var_idx(variables);
    int old_pos = find_variable_nr(var->name);
    if (old_pos == -1) {
        variables.push_back(var.release());
    }
    else {
        if (var->is_dependent_on(old_pos, variables)) { //check for loops
            throw ExecuteError("detected loop in variable dependencies of "
                               + var->xname);
        }
        delete variables[old_pos];
        variables[old_pos] = var.release();
        if (variables[old_pos]->get_max_var_idx() > old_pos) {
            sort_variables();
        }
        remove_unreferred();
    }
    return var_name;
}

string VariableManager::assign_variable_copy(string const& name,
                                             Variable const* orig,
                                             map<int,string> const& varmap)
{
    Variable *var=0;
    assert(!name.empty());
    if (orig->is_simple()) {
        fp val = orig->get_value();
        parameters.push_back(val);
        int nr = parameters.size() - 1;
        var = new Variable(name, nr);
    }
    else {
        vector<string> vars;
        for (int i = 0; i != orig->get_vars_count(); ++i) {
            assert(varmap.count(orig->get_var_idx(i)));
            vars.push_back(varmap.find(orig->get_var_idx(i))->second);
        }
        vector<OpTree*> new_op_trees;
        for (vector<OpTree*>::const_iterator i = orig->get_op_trees().begin();
                                          i != orig->get_op_trees().end(); ++i)
            new_op_trees.push_back((*i)->copy());
        var = new Variable(name, vars, new_op_trees);
    }
    return put_into_variables(var);
}

void VariableManager::delete_variables(vector<string> const &names)
{
    const int n = names.size();
    vector<int> nrs (n);
    for (int i = 0; i < n; ++i) {
        int k = find_variable_nr(names[i]);
        if (k == -1)
            throw ExecuteError("undefined variable: $" + names[i]);
        string first_referrer;
        if (is_variable_referred(k, names, &first_referrer))
            throw ExecuteError("can't delete $" + names[i] + " because "
                               + first_referrer + " depends on it.");
        nrs[i] = k;
    }
    sort(nrs.begin(), nrs.end());
    for (int i = n-1; i >= 0; --i) {
        int k = nrs[i];
        delete variables[k];
        variables.erase(variables.begin() + k);
    }
    remove_unreferred();
}

void VariableManager::delete_funcs(vector<string> const &names)
{
    if (names.empty())
        return;
    for (vector<string>::const_iterator i=names.begin(); i != names.end(); ++i){
        int k = find_function_nr(*i);
        if (k == -1)
            throw ExecuteError("undefined function: %" + *i);
        delete functions[k];
        functions.erase(functions.begin() + k);
    }
    remove_unreferred();
    for (vector<Model*>::iterator i = models.begin(); i != models.end(); ++i)
        (*i)->find_function_indices();
}

bool VariableManager::is_function_referred(int n) const
{
    for (vector<Model*>::const_iterator i = models.begin();
                                                    i != models.end(); ++i) {
        if (contains_element((*i)->get_ff_idx(), n)
                || contains_element((*i)->get_zz_idx(), n))
            return true;
    }
    return false;
}

void VariableManager::auto_remove_functions()
{
    int func_size = functions.size();
    for (int i = func_size - 1; i >= 0; --i)
        if (functions[i]->is_auto_delete() && !is_function_referred(i)) {
            delete functions[i];
            functions.erase(functions.begin() + i);
        }
    if (func_size != size(functions)) {
        remove_unreferred();
        for (vector<Model*>::iterator i = models.begin();
                                                    i != models.end(); ++i)
            (*i)->find_function_indices();
    }
}

void VariableManager::delete_funcs_and_vars(vector<string> const &xnames)
{
    vector<string> vars, funcs;
    for (vector<string>::const_iterator i = xnames.begin();
            i != xnames.end(); ++i) {
        if ((*i)[0] == '$')
            vars.push_back(string(*i, 1));
        else if ((*i)[0] == '%')
            funcs.push_back(string(*i, 1));
        else
            assert(0);
    }
    delete_funcs(funcs);
    delete_variables(vars);
}


int VariableManager::find_function_nr(string const &name) const
{
    string only_name = !name.empty() && name[0]=='%' ? string(name,1) : name;
    for (int i = 0; i < size(functions); ++i)
        if (functions[i]->name == only_name)
            return i;
    return -1;
}

const Function* VariableManager::find_function(string const &name) const
{
    int n = find_function_nr(name);
    if (n == -1)
        throw ExecuteError("undefined function: "
                                       + (name[0]=='%' ? name : "%"+name));
    return functions[n];
}

int VariableManager::find_variable_nr(string const &name) const
{
    for (int i = 0; i < size(variables); ++i)
        if (variables[i]->name == name)
            return i;
    return -1;
}

Variable const* VariableManager::find_variable(string const &name) const
{
    int n = find_variable_nr(name);
    if (n == -1)
        throw ExecuteError("undefined variable: $" + name);
    return variables[n];
}

int VariableManager::find_nr_var_handling_param(int p) const
{
    assert(p >= 0 && p < size(parameters));
    for (size_t i = 0; i < variables.size(); ++i)
        if (variables[i]->get_nr() == p)
            return i;
    assert(0);
    return 0;
}

int VariableManager::find_parameter_variable(int par) const
{
    for (int i = 0; i < size(variables); ++i)
        if (variables[i]->get_nr() == par)
            return i;
    return -1;
}

void VariableManager::use_parameters()
{
    use_external_parameters(parameters);
}

void VariableManager::use_external_parameters(vector<fp> const &ext_param)
{
    for (vector<Variable*>::iterator i = variables.begin();
                i != variables.end(); ++i)
        (*i)->recalculate(variables, ext_param);
    for (vector<Function*>::iterator i = functions.begin();
            i != functions.end(); ++i)
        (*i)->do_precomputations(variables);
}

void VariableManager::put_new_parameters(vector<fp> const &aa)
{
    for (size_t i = 0; i < min(aa.size(), parameters.size()); ++i)
        parameters[i] = aa[i];
    use_parameters();
}

vector<string> VariableManager::get_vars_from_kw(string const &function,
                                                 vector<string> const &vars)
{
    string formula = Function::get_formula(function);
    if (formula.empty())
        throw ExecuteError("Undefined type of function: " + function);
    vector<string> tnames = Function::get_varnames_from_formula(formula);
    vector<string> tvalues = Function::get_defvalues_from_formula(formula);
    int n = tnames.size();
    size_t vsize = vars.size();
    vector<string> vars_names(vsize), vars_rhs(vsize);
    for (size_t i = 0; i < vsize; ++i) {
        string::size_type eq = vars[i].find('=');
        assert(eq != string::npos);
        vars_names[i] = string(vars[i], 0, eq);
        vars_rhs[i] = string(vars[i], eq+1);
    }
    vector<string> vv(n);
    for (int i = 0; i < n; ++i) {
        string const& tname = tnames[i];
        // (1st try) variables given in vars
        int tname_idx = index_of_element(vars_names, tname);
        if (tname_idx != -1) {
            vv[i] = vars_rhs[tname_idx];
            continue;
        }
        // (2nd try) use default parameter value
        if (!tvalues[i].empty()) {
            for (size_t j = 0; j < vsize; ++j)
                replace_words(tvalues[i], vars_names[j], vars_rhs[j]);
            try {
                fp v = get_transform_expression_value(
                                      strip_tilde_variable(tvalues[i]), 0);
                vv[i] = "~" + S(v);
                continue;
            }
            catch (ExecuteError &) {} //nothing
        }
        // (3rd try) name
        else if (tname == "hwhm") {
            int fwhm_idx = index_of_element(vars_names, "fwhm");
            if (fwhm_idx != -1) {
                fp v = get_transform_expression_value("0.5*"
                               + strip_tilde_variable(vars_rhs[fwhm_idx]), 0);
                vv[i] = "~" + S(v);
                continue;
            }
        }

        throw ExecuteError("Can't create function " + function
                               + " because " + tname + " is unknown.");
    }
    return vv;
}

string VariableManager::assign_func(string const &name, string const &function,
                                    vector<string> const &vars)
{
    Function *func = 0;
    try {
        string func_name = name.empty() ? next_func_name() : name;

        vector<string> varnames;
        bool has_eq = (vars.empty() || vars[0].find('=') != string::npos);
        for (vector<string>::const_iterator i = vars.begin();
                                                        i != vars.end(); ++i)
            if ((i->find('=') != string::npos) != has_eq)
                throw ExecuteError("Either use keywords for all parameters"
                                   " or for none");
        vector<string> vv = (has_eq ? get_vars_from_kw(function, vars) : vars);
        for (size_t i = 0; i < vv.size(); ++i)
            varnames.push_back(get_or_make_variable(vv[i]));

        func = Function::factory(F, func_name, function, varnames);
    } catch (ExecuteError &) {
        remove_unreferred();
        throw;
    }
    return do_assign_func(func);
}

string VariableManager::do_assign_func(Function* func)
{
    func->set_var_idx(variables);
    //if there is already function with the same name -- replace
    bool found = false;
    for (int i = 0; i < size(functions); ++i) {
        if (functions[i]->name == func->name) {
            delete functions[i];
            functions[i] = func;
            if (!silent)
                F->msg("New function %" + func->name +" replaced the old one.");
            remove_unreferred();
            found = true;
            break;
        }
    }
    if (!found) {
        functions.push_back(func);
        if (!silent)
            F->msg("New function %" + func->name + " was created.");
    }
    return func->name;
}

string VariableManager::make_var_copy_name(Variable const* v)
{
    if (v->name[0] == '_')
        return next_var_name();

    //for other names append "01" or increase the last two digits in name
    int vs = v->name.size();
    int appendix = 0;
    string core = v->name;
    if (vs > 2 && is_int(string(v->name, vs-2, 2))) { // foo02
        appendix = atoi(v->name.c_str()+vs-2);
        core.resize(vs-2);
    }
    while (true) {
        ++appendix;
        string new_varname = core + S(appendix/10) + S(appendix%10);
        if (find_variable_nr(new_varname) == -1)
            return new_varname;
    }
}

string VariableManager::assign_func_copy(string const &name, string const &orig)
{
    Function const* of = find_function(orig);
    map<int,string> varmap;
    for (int i = 0; i < size(variables); ++i) {
        if (!of->is_dependent_on(i, variables))
            continue;
        Variable const* var_orig = variables[i];
        string new_varname = make_var_copy_name(var_orig);
        assign_variable_copy(new_varname, var_orig, varmap);
        varmap[i] = new_varname;
    }
    vector<string> varnames;
    for (int i = 0; i != of->get_vars_count(); ++i) {
        assert(varmap.count(of->get_var_idx(i)));
        varnames.push_back(varmap[of->get_var_idx(i)]);
    }

    string func_name = name.empty() ? next_func_name() : name;
    Function *func = Function::factory(F, func_name, of->type_name, varnames);
    return do_assign_func(func);
}

void VariableManager::substitute_func_param(string const &name,
                                            string const &param,
                                            string const &var)
{
    int nr = find_function_nr(name);
    if (nr == -1)
        throw ExecuteError("undefined function: %" + name);
    Function* k = functions[nr];
    k->substitute_param(k->get_param_nr(param), get_or_make_variable(var));
    k->set_var_idx(variables);
    remove_unreferred();
}

fp VariableManager::variation_of_a (int n, fp variat) const
{
    assert (0 <= n && n < size(get_parameters()));
    Domain const& dom = get_variable(n)->domain;
    fp ctr = dom.is_ctr_set() ? dom.get_ctr() : parameters[n];
    fp sgm = dom.is_set() ? dom.get_sigma()
            : ctr * F->get_settings()->get_f("variable-domain-percent") / 100.;
    return ctr + sgm * variat;
}

string VariableManager::next_var_name()
{
    while (1) {
        string t = "_" + S(++var_autoname_counter);
        if (find_variable_nr(t) == -1)
            return t;
    }
}

string VariableManager::next_func_name()
{
    while (1) {
        string t = "_" + S(++func_autoname_counter);
        if (find_function_nr(t) == -1)
            return t;
    }
}

//TODO: remove it, use dtor+ctor
void VariableManager::do_reset()
{
    var_autoname_counter = 0;
    func_autoname_counter = 0;
    purge_all_elements(functions);
    purge_all_elements(variables);
    parameters.clear();
    //don't delete models, they should unregister itself
    for (vector<Model*>::iterator i = models.begin(); i != models.end(); ++i)
        (*i)->find_function_indices();
}


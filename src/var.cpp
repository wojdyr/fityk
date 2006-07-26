// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#include "var.h"
#include "common.h"
#include "datatrans.h"
#include "calc.h"
#include "ast.h"
#include "ui.h"
#include "func.h"
#include "sum.h"
#include "numfuncs.h"
#include <stdlib.h>
#include <ctype.h>
#include <boost/spirit/core.hpp>
#include <algorithm>
#include <memory>

using namespace std;
using namespace boost::spirit;



bool VariableUser::is_directly_dependent_on(int idx) {
    return count(var_idx.begin(), var_idx.end(), idx);
}

/// checks if *this depends (directly or indirectly) on variable with index idx
bool VariableUser::is_dependent_on(int idx, 
                                   vector<Variable*> const &variables) const
{
    for (vector<int>::const_iterator i = var_idx.begin(); 
            i != var_idx.end(); ++i)
        if (*i == idx || variables[*i]->is_dependent_on(idx, variables))
            return true;
    return false;
}

void VariableUser::set_var_idx(vector<Variable*> const &variables)
{
    const int n = varnames.size();
    var_idx.resize(n);
    for (int v = 0; v < n; ++v) {
        bool found = false;
        for (int i = 0; i < size(variables); ++i) {
            if (varnames[v] == variables[i]->name) {
                var_idx[v] = i;
                found = true;
                break;
            }
        }
        if (!found)
            throw ExecuteError("Undefined variable: $" + varnames[v]);
    }
}

int VariableUser::get_max_var_idx() 
{
    if (var_idx.empty()) 
        return -1; 
    else
       return *max_element(var_idx.begin(), var_idx.end());
}

////////////////////////////////////////////////////////////////////////////


// ctor for simple variables and mirror variables
Variable::Variable(std::string const &name_, int nr_)
    : VariableUser(name_, "$"), 
      auto_delete(name_[0] == '_'),
      nr(nr_), af(value, derivatives)
{
    assert(!name_.empty());
    if (nr != -2) {
        ParMult pm;
        pm.p = nr_;
        pm.mult = 1;
        recursive_derivatives.push_back(pm);
    }
}

// ctor for compound variables
Variable::Variable(std::string const &name_, vector<string> const &vars_,
                   vector<OpTree*> const &op_trees_)
    : VariableUser(name_, "$", vars_), 
      auto_delete(name_[0] == '_'),
      nr(-1), derivatives(vars_.size()),
      af(op_trees_, value, derivatives) 
{
    assert(!name_.empty());
}

void Variable::set_var_idx(vector<Variable*> const& variables)
{
    VariableUser::set_var_idx(variables); 
    if (nr == -1)
        af.tree_to_bytecode(var_idx); 
}


string Variable::get_formula(vector<fp> const &parameters) const
{
    return nr == -1 ? get_op_trees().back()->str(&varnames) 
                    : "~" + S(parameters[nr]);
}

string Variable::get_info(vector<fp> const &parameters, bool extended) const 
{ 
    string s = xname + " = " + get_formula(parameters) + " = " + S(value);
    if (domain.is_set())
        s += domain.str();
    if (auto_delete)
        s += "  [auto]";
    if (extended && nr == -1) {
        for (unsigned int i = 0; i < varnames.size(); ++i)
            s += "\nd(" + xname + ")/d($" + varnames[i] + "): " 
              + get_op_trees()[i]->str(&varnames) + " == " + S(derivatives[i]);
    }
    return s;
} 

void Variable::recalculate(vector<Variable*> const &variables, 
                           vector<fp> const &parameters)
{
  if (nr == -1) {
      af.run_vm(variables);
      recursive_derivatives.clear();
      for (int i = 0; i < size(derivatives); ++i) {
          Variable *v = variables[var_idx[i]];
          vector<ParMult> const &pm = v->get_recursive_derivatives();
          for (vector<ParMult>::const_iterator j=pm.begin(); j!=pm.end(); ++j) {
              recursive_derivatives.push_back(*j);
              recursive_derivatives.back().mult *= derivatives[i];
          }
      }
  }
  else if (nr == -2)
      ; //do nothing, it must be set with set_mirror()
  else {
      value = parameters[nr];
      if (!derivatives.empty()) {
          derivatives.clear();
      }
  }
}

void Variable::erased_parameter(int k)
{
    if (nr != -1 && nr > k) 
            --nr; 
    for (vector<ParMult>::iterator i = recursive_derivatives.begin(); 
                                        i != recursive_derivatives.end(); ++i)
        if (i->p > k)
            -- i->p;
}

////////////////////////////////////////////////////////////////////////////

VariableManager::~VariableManager()
{
    purge_all_elements(functions);
    purge_all_elements(variables);
}

void VariableManager::unregister_sum(Sum const *s)
{ 
    vector<Sum*>::iterator k = find(sums.begin(), sums.end(), s);
    assert (k != sums.end());
    sums.erase(k);
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
    assert(!func.empty());
    string tmp1, tmp2;
    if (parse(func.c_str(), VariableLhsG).full) // $foo
        return string(func, 1);
    else if (parse(func.c_str(), 
                   FunctionLhsG [assign_a(tmp1)] 
                       >> '[' >> 
                        lexeme_d[alpha_p >> *(alnum_p|'_')][assign_a(tmp2)]
                       >> ']')
                  .full) {                     // %bar[bleh]
        return get_func_param(tmp1, tmp2);
    }
    else                                       // anything else
        return assign_variable("", func);
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
        return do_assign_variable(var);
    }

    tree_parse_info<> info = ast_parse(rhs.c_str(), FuncG, space_p);
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
        parameters.push_back(val);
        int nr = parameters.size() - 1;
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
            if ((*i)[0]!='~' && (*i)[0]!='{' && (*i)[0]!='$' && (*i)[0]!='%')
                throw ExecuteError("`" + *i + "' can't be used as variable.");
        vector<OpTree*> op_trees = calculate_deriv(root, vars);
        // ~14.3 -> $var4
        for (vector<string>::iterator i = vars.begin(); i != vars.end(); ++i) {
            *i = get_or_make_variable(*i);
        }
        var = new Variable(nonempty_name, vars, op_trees);
    }
    return do_assign_variable(var);
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

vector<string> VariableManager::get_variable_references(string const &name)
{
    int idx = find_variable_nr(name);
    vector<string> refs;
    for (vector<Variable*>::const_iterator i = variables.begin(); 
            i != variables.end(); ++i)
        if ((*i)->is_directly_dependent_on(idx)) 
            refs.push_back((*i)->xname);
    for (vector<Function*>::const_iterator i = functions.begin(); 
            i != functions.end(); ++i)
        if ((*i)->is_directly_dependent_on(idx)) 
            refs.push_back((*i)->xname);
    return refs;
}

void VariableManager::remove_unreferred() 
{
    // remove auto-delete marked variables, which are not referred by others
    for (int i = variables.size()-1; i >= 0; --i)
        if (variables[i]->auto_delete) {
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
    // remove unreffered parameters
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


string VariableManager::do_assign_variable(Variable* new_var)
{
    auto_ptr<Variable> var(new_var);
    string var_name = var->name;
    var->set_var_idx(variables);
    int old_pos = find_variable_nr(var->name);
    if (old_pos == -1) {
        var->recalculate(variables, parameters);
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
    use_parameters();
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
    return do_assign_variable(var);
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
    for (vector<Sum*>::iterator i = sums.begin(); i != sums.end(); ++i)
        (*i)->find_function_indices();
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


int VariableManager::find_function_nr(string const &name) {
    string only_name = !name.empty() && name[0]=='%' ? string(name,1) : name;
    for (int i = 0; i < size(functions); ++i)
        if (functions[i]->name == only_name)
            return i;
    return -1;
}

const Function* VariableManager::find_function(string const &name) {
    int n = find_function_nr(name);
    if (n == -1)
        throw ExecuteError("undefined function: " 
                                       + (name[0]=='%' ? name : "%"+name));
    return functions[n];
}

int VariableManager::find_variable_nr(string const &name) {
    for (int i = 0; i < size(variables); ++i)
        if (variables[i]->name == name)
            return i;
    return -1;
}

Variable const* VariableManager::find_variable(string const &name) {
    int n = find_variable_nr(name);
    if (n == -1)
        throw ExecuteError("undefined variable: $" + name);
    return variables[n];
}

int VariableManager::find_nr_var_handling_param(int p)
{
    assert(p >= 0 && p < size(parameters));
    for (size_t i = 0; i < variables.size(); ++i)
        if (variables[i]->get_nr() == p)
            return i;
    assert(0);
    return 0;
}

int VariableManager::find_parameter_variable(int par)
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

void VariableManager::put_new_parameters(vector<fp> const &aa, 
                                         string const &/*method*/,
                                         bool change)
{
    //TODO history
    if (change)
        parameters = aa;
    use_parameters();
}

vector<string> VariableManager::get_vars_from_kw(string const &function,
                                                 vector<string> const &vars)
{
    string formula = Function::get_formula(function);
    if (formula.empty())
        throw ExecuteError("Undefined type of function: " + function);
    vector<string> tnames 
        = Function::get_varnames_from_formula(formula, false);
    vector<string> tvalues 
        = Function::get_varnames_from_formula(formula, true);
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
            catch (ExecuteError &e) {} //nothing
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

vector<string> VariableManager::make_varnames(string const &function,
                                              vector<string> const &vars) 
{
    vector<string> varnames;
    if (vars.empty())
        return varnames;
    bool has_eq = (vars[0].find('=') != string::npos);
    for (vector<string>::const_iterator i = vars.begin(); i != vars.end(); ++i) 
        if ((i->find('=') != string::npos) != has_eq)
            throw ExecuteError("Either use keywords for all parameters"
                               " or for none");
    vector<string> vv = (!has_eq ? vars : get_vars_from_kw(function, vars));
    for (int i = 0; i < size(vv); ++i) 
        varnames.push_back(get_or_make_variable(vv[i]));
    return varnames;
}

string VariableManager::assign_func(string const &name, string const &function, 
                                    vector<string> const &vars, bool parse_vars)
{
    Function *func = 0;
    try {
        func = Function::factory(name.empty() ? next_func_name() : name, 
                                 function, 
                                 parse_vars ? make_varnames(function, vars) 
                                            : vars);
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
                mesg("New function %" + func->name + " replaced the old one.");
            remove_unreferred();
            found = true;
            break;
        }
    }
    if (!found) {
        functions.push_back(func);
        if (!silent)
            info("New function %" + func->name + " was created.");
    }
    func->do_precomputations(variables);
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
    return assign_func(name, of->type_name, varnames, false);
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
    k->do_precomputations(variables);
    remove_unreferred();
}

string VariableManager::get_func_param(string const &name, string const &param)
{
    Function const* k = find_function(name);
    return k->get_var_name(k->get_param_nr(param));
}

fp VariableManager::variation_of_a (int n, fp variat) const
{
    assert (0 <= n && n < size(get_parameters()));
    Domain const& dom = get_variable(n)->domain;
    fp ctr = dom.is_ctr_set() ? dom.get_ctr() : parameters[n];
    fp sgm = dom.is_set() ? dom.get_sigma() : 0.5 * ctr;
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


template <typename ScannerT>
FuncGrammar::definition<ScannerT>::definition(FuncGrammar const& /*self*/)
{
    //  Start grammar definition
    real_const  =  leaf_node_d[   real_p 
                               |  as_lower_d[str_p("pi")] 
                               | '{' >> +~ch_p('}') >> '}' 
                              ];

    //"x" only in functions
    //all expressions but the last are for variables and functions
    //the last is for function types
    variable    = leaf_node_d[lexeme_d['$' >> +(alnum_p | '_')]]
                | leaf_node_d[lexeme_d['~' >> real_p] 
                              >> !('[' >> !real_p >> "+-" >> real_p >> ']')]
                | leaf_node_d["~{" >> +~ch_p('}') >> '}']
                // using FunctionLhsG causes crash 
                | leaf_node_d[lexeme_d["%" >> +(alnum_p | '_')] //FunctionLhsG 
                              >> '[' >> lexeme_d[alpha_p >> *(alnum_p | '_')] 
                              >> ']'] 
                | leaf_node_d[lexeme_d[alpha_p >> *(alnum_p | '_')]]
                ;

    exptoken    =  real_const
                |  inner_node_d[ch_p('(') >> expression >> ')']
                |  root_node_d[ as_lower_d[ str_p("sqrt") 
                                          | "exp" | "log10" | "ln" 
                                          | "sin" | "cos" | "tan" 
                                          | "atan" | "asin" | "acos"
                                          | "lgamma"
                                          ] ]
                   >>  inner_node_d[ch_p('(') >> expression >> ')']
                |  variable
                ;

    signarg     =  exptoken >>
                   *(  (root_node_d[ch_p('^')] >> exptoken)
                    );

    factor      =  root_node_d[ch_p('-')] >> signarg
                |  discard_node_d[!ch_p('+')] >> signarg
                ;

    term        =  factor >>
                   *(  (root_node_d[ch_p('*')] >> factor)
                     | (root_node_d[ch_p('/')] >> factor)
                    );

    expression  =  term >>
                   *(  (root_node_d[ch_p('+')] >> term)
                     | (root_node_d[ch_p('-')] >> term)
                    );
}

// explicit template instantiation -- to accelerate compilation 
template FuncGrammar::definition<scanner<char const*, scanner_policies<skipper_iteration_policy<iteration_policy>, match_policy, no_actions_action_policy<action_policy> > > >::definition(FuncGrammar const&);

template FuncGrammar::definition<scanner<char const*, scanner_policies<skipper_iteration_policy<iteration_policy>, match_policy, no_actions_action_policy<no_actions_action_policy<action_policy> > > > >::definition(FuncGrammar const&);


/// small but slow utility function 
/// uses calculate_deriv() to simplify formulae
std::string simplify_formula(std::string const &formula)
{
    tree_parse_info<> info = ast_parse(formula.c_str(), FuncG, space_p);
    assert(info.full);
    const_tm_iter_t const &root = info.trees.begin();
    vector<string> vars(1, "x");
    vector<OpTree*> results = calculate_deriv(root, vars);
    string simplified = results.back()->str(&vars);
    // simplied formula has $x instead of x
    replace_all(simplified, "$x", "x");
    purge_all_elements(results);
    return simplified;
}


FuncGrammar FuncG;
VariableLhsGrammar VariableLhsG;
FunctionLhsGrammar  FunctionLhsG;



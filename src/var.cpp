// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#include "var.h"
#include "common.h"
//#include "datatrans.h"
#include "calc.h"
#include "ui.h"
#include "func.h"
#include "sum.h"
#include <stdlib.h>
#include <boost/spirit/core.hpp>
#include <algorithm>
#include <memory>

using namespace std;
using namespace boost::spirit;

const int stack_size = 8192;  //should be enough, 
                              //there are no checks for stack overflow  
vector<double> stack(stack_size);



bool VariableUser::is_directly_dependent_on(int idx) {
    return count(var_idx.begin(), var_idx.end(), idx);
}

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



Variable::Variable(std::string const &name_, int nr_, 
                   bool auto_delete_, bool hidden_)
    : VariableUser(name_, "$"),
      auto_delete(auto_delete_), hidden(hidden_), nr(nr_), 
      recursive_derivatives(1)
{
    recursive_derivatives[0].p = nr_;
    recursive_derivatives[0].mult = 1;
}

Variable::Variable(std::string const &name_, vector<string> const &vars_,
                   vector<OpTree*> const &op_trees_, 
                   bool auto_delete_, bool hidden_)
    : VariableUser(name_, "$", vars_),
      auto_delete(auto_delete_), hidden(hidden_), nr(-1), 
      derivatives(vars_.size()),
      op_trees(op_trees_) 
{
}


string Variable::get_formula(vector<fp> const &parameters) const
{
    return nr == -1 ? op_trees.back()->str(&varnames) 
                    : "~" + S(parameters[nr]);
}

string Variable::get_info(vector<fp> const &parameters, 
                          bool extended) const 
{ 
    string s = xname + " = "+ get_formula(parameters) + " = " + S(value);
    if (extended && nr == -1) {
        if (auto_delete)
            s += "  [auto]";
        for (unsigned int i = 0; i < varnames.size(); ++i)
            s += "\nd(" + xname + ")/d($" + varnames[i] + "): " 
                    + op_trees[i]->str(&varnames) + " == " + S(derivatives[i]);
    }
    return s;
} 

void Variable::run_vm(vector<Variable*> const &variables)
{
    vector<double>::iterator stackPtr = stack.begin() - 1;//will be ++'ed first
    for (vector<int>::const_iterator i = vmcode.begin(); i!=vmcode.end(); i++) {
        switch (*i) {
            //unary operators
            case OP_NEG:
                *stackPtr = - *stackPtr;
                break;
            case OP_SQRT:
                *stackPtr = sqrt(*stackPtr);
                break;
            case OP_EXP:
                *stackPtr = exp(*stackPtr);
                break;
            case OP_LOG10:
                *stackPtr = log10(*stackPtr); 
                break;
            case OP_LN:
                *stackPtr = log(*stackPtr); 
                break;
            case OP_SIN:
                *stackPtr = sin(*stackPtr);
                break;
            case OP_COS:
                *stackPtr = cos(*stackPtr);
                break;
            case OP_TAN:
                *stackPtr = tan(*stackPtr); 
                break;
            case OP_ATAN:
                *stackPtr = atan(*stackPtr); 
                break;
            case OP_ASIN:
                *stackPtr = asin(*stackPtr); 
                break;
            case OP_ACOS:
                *stackPtr = acos(*stackPtr); 
                break;

            //binary operators
            case OP_ADD:
                stackPtr--;
                *stackPtr += *(stackPtr+1);
                break;
            case OP_SUB:
                stackPtr--;
                *stackPtr -= *(stackPtr+1);
                break;
            case OP_MUL:
                stackPtr--;
                *stackPtr *= *(stackPtr+1);
                break;
            case OP_DIV:
                stackPtr--;
                *stackPtr /= *(stackPtr+1);
                break;
            case OP_POW:
                stackPtr--;
                *stackPtr = pow(*stackPtr, *(stackPtr+1));
                break;

            // putting-number-to-stack-operators
            // stack overflow not checked
            case OP_CONSTANT:
                stackPtr++;
                i++;
                *stackPtr = vmdata[*i];
                break;
            case OP_VARIABLE:
                stackPtr++;
                i++;
                *stackPtr = variables[*i]->get_value();
                break;
            //assignment-operators
            case OP_PUT_VAL:
                value = *stackPtr;
                stackPtr--; 
                break;
            case OP_PUT_DERIV:
                i++;
                derivatives[*i] = *stackPtr;
                stackPtr--; 
                break;

            default:
                assert(0); //("Unknown operator in VM code: " + S(*i))
        }
    }
    assert(stackPtr == stack.begin() - 1);
}

void Variable::recalculate(vector<Variable*> const &variables, 
                           vector<fp> const &parameters)
{
  if (nr == -1) {
      run_vm(variables);
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
      ;
  else {
      value = parameters[nr];
      if (!derivatives.empty()) {
          derivatives.clear();
      }
  }
}

void Variable::tree_to_bytecode()
{
    if (nr == -1) {
        assert(var_idx.size() + 1 == op_trees.size()); 
        int n = var_idx.size();
        vmcode.clear();
        vmdata.clear();
        add_calc_bytecode(op_trees.back(), var_idx, vmcode, vmdata);
        vmcode.push_back(OP_PUT_VAL);
        for (int i = 0; i < n; ++i) {
            add_calc_bytecode(op_trees[i], var_idx, vmcode, vmdata);
            vmcode.push_back(OP_PUT_DERIV);
            vmcode.push_back(i);
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
    if (parse(func.c_str(), VariableLhsG).full)
        return string(func, 1);
    else if (parse(func.c_str(), 
                   FunctionLhsG [assign_a(tmp1)]
                       >> '[' >> (alpha_p >> *(alnum_p|'_'))[assign_a(tmp2)]
                       >> ']')
                  .full) {
        return get_func_param(tmp1, tmp2);
    }
    else
        return assign_variable("", func);
}

string VariableManager::assign_variable(string const &name, string const &rhs)
{
    Variable *var = 0;
    bool auto_del = name.empty();
    string nonempty_name = name.empty() ? Variable::next_auto_name() : name; 
    tree_parse_info<> info = ast_parse(rhs.c_str(), FuncG, space_p);
    assert(info.full);
    const_tm_iter_t const &root = info.trees.begin();
    if (root->value.id() == FuncGrammar::variableID
            && *root->value.begin() == '~') { 
        string val_str = string(root->value.begin()+1, root->value.end());
        fp val = get_constant_value(val_str);
        parameters.push_back(val);
        int nr = parameters.size() - 1;
        var = new Variable(nonempty_name, nr, auto_del);
    }
    else {
        vector<string> vars=find_tokens_in_ptree(FuncGrammar::variableID, info);
        if (find(vars.begin(), vars.end(), "x") != vars.end())
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
        var = new Variable(nonempty_name, vars, op_trees, auto_del);
    }
    return do_assign_variable(var);
}

bool VariableManager::is_variable_referred(int i, 
                                           vector<string> const &ignore_vars,
                                           string *first_referrer)
{
    for (int j = i+1; j < size(variables); ++j) {
        if (variables[j]->is_directly_dependent_on(i) 
            && find(ignore_vars.begin(), ignore_vars.end(), variables[j]->name) 
               == ignore_vars.end()) {
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
            throw ExecuteError("detected loop in variable dependencies");
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
        var = new Variable(name, nr, orig->auto_delete);
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
        var = new Variable(name, vars, new_op_trees, orig->auto_delete);
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
    return n == -1 ? 0 : functions[n];
}

int VariableManager::find_variable_nr(string const &name) {
    for (int i = 0; i < size(variables); ++i)
        if (variables[i]->name == name)
            return i;
    return -1;
}

Variable const* VariableManager::find_variable(string const &name) {
    int n = find_variable_nr(name);
    return n == -1 ? 0 : variables[n];
}

Variable const* VariableManager::find_variable_handling_param(int p)
{
    assert(p >= 0 && p < size(parameters));
    for (vector<Variable*>::const_iterator i = variables.begin(); 
            i != variables.end(); ++i)
        if ((*i)->get_nr() == p)
            return *i;
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

string VariableManager::get_var_from_expression(string const& expr,
                                                vector<string> const& vars)
{
    // `keyword*real_number' values defined in function type definitions
    string::size_type ax = expr.find('*');
    bool has_mult = (ax != string::npos);
    string dname = has_mult ? string(expr, 0, ax) : expr;
    for (vector<string>::const_iterator k = vars.begin(); k != vars.end(); ++k){
        string::size_type eq = k->find('=');
        string name = string(*k, 0, eq);
        if (name == dname) {
            string value = string(*k, eq+1);
            if (has_mult) {
                string multip = string(expr, ax+1);
                // change "~1.2 * 2.5" to "~3.0"
                if (value[0] == '~' && is_double(string(value,1)))
                    return "~" + S(strtod(value.c_str()+1, 0) 
                                    * strtod(multip.c_str(), 0));
                else
                    return value + "*" + multip;
            }
            else {
                return value;
            }
        }
    }
    return "";
}

string VariableManager::get_variable_from_kw(string const& function,
                                             string const& tname, 
                                             string const& tvalue, 
                                             vector<string> const& vars)
{
    // variables given in vars
    for (vector<string>::const_iterator k = vars.begin(); 
                                                k != vars.end(); ++k) {
        string::size_type eq = k->find('=');
        assert(eq != string::npos);
        string name = string(*k, 0, eq);
        if (name == tname) 
            return string(*k, eq+1);
    }
    // numeric values defined in function type definitions
    if (is_double(tvalue)) {
        return "~" + tvalue;
    }
    // `keyword*real_number' values defined in function type definitions
    string r;
    if (!tvalue.empty()) 
        r = get_var_from_expression(tvalue, vars);
    if (!r.empty())
        return r;
    // special case: hwhm=fwhm*0.5
    if (tname == "hwhm")
        r = get_var_from_expression("fwhm*0.5", vars);
    if (!r.empty())
        return r;

    throw ExecuteError("Can't create function " + function
                           + " because " + tname + " is unknown.");
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
    vector<string> vv(n);
    for (int i = 0; i < n; ++i) {
        vv[i] = get_variable_from_kw(function, tnames[i], tvalues[i], vars);
    }
    return vv;
}

vector<string> VariableManager::make_varnames(string const &function,
                                              vector<string> const &vars) 
{
    vector<string> varnames;
    if (vars.empty())
        return vars;
    vector<string> vv = (vars[0].find('=') == string::npos ? vars
                                           : get_vars_from_kw(function, vars));
    for (int i = 0; i < size(vv); ++i) 
        varnames.push_back(get_or_make_variable(vv[i]));
    return varnames;
}

string VariableManager::assign_func(string const &name, string const &function, 
                                    vector<string> const &vars)
{
    Function *func = 0;
    try {
        vector<string> varnames = make_varnames(function, vars);
        func = Function::factory(name, function, varnames);
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
        return Variable::next_auto_name(); 

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
    Function* cf = Function::factory(name, of->type_name, varnames);
    return do_assign_func(cf);
}

void VariableManager::substitute_func_param(string const &name, 
                                            string const &param,
                                            string const &var)
{
    int nr = find_function_nr(name);
    if (nr == -1)
        throw ExecuteError("undefined function: %" + name);
    Function* k = functions[nr];
    k->substitute_param(k->find_param_nr(param), get_or_make_variable(var)); 
    k->set_var_idx(variables);
    k->do_precomputations(variables);
    remove_unreferred();
}

string VariableManager::get_func_param(string const &name, string const &param)
{
    Function const* k = find_function(name);
    if (!k)
        throw ExecuteError("undefined function: " + name);
    return k->get_var_name(k->find_param_nr(param));
}

fp VariableManager::variation_of_a (int n, fp variat) const
{
    assert (0 <= n && n < size(get_parameters()));
    //TODO domain
#if 0
    const Domain& dom = get_domain(n);
    fp ctr = dom.is_ctr_set() ? dom.Ctr() : parameters[n];
    fp sgm = dom.is_set() ? dom.Sigma() 
                          : my_sum->get_def_rel_domain_width() * ctr;
#endif
    fp ctr = parameters[n];
    fp sgm = 0.1;

    return ctr + sgm * variat;
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
                | leaf_node_d[lexeme_d['~' >> real_p]]
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
                                          ] ]
                   >>  inner_node_d[ch_p('(') >> expression >> ')']
                |  (root_node_d[ch_p('-')] >> exptoken)
                |  variable
                ;

    factor      =  exptoken >>
                   *(  (root_node_d[ch_p('^')] >> exptoken)
                    );

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
/// uses calculate_deriv() to simplify formulea
std::string simplify_formula(std::string const &formula)
{
    tree_parse_info<> info = ast_parse(formula.c_str(), FuncG, space_p);
    assert(info.full);
    const_tm_iter_t const &root = info.trees.begin();
    vector<string> vars(1, "x");
    vector<OpTree*> results = calculate_deriv(root, vars);
    string simplified = results.back()->str(&vars);
    purge_all_elements(results);
    return simplified;
}


FuncGrammar FuncG;
VariableLhsGrammar VariableLhsG;
FunctionLhsGrammar  FunctionLhsG;
int Variable::unnamed_counter = 0;



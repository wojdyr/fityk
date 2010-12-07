// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
// $Id$

#include "mgr.h"
#include "common.h"
#include "var.h"
#include "ast.h"
#include "ui.h"
#include "tplate.h"
#include "func.h"
#include "model.h"
#include "settings.h"
#include "logic.h" //VariableManager::get_or_make_variable() handles @0.F[1].a
#include "lexer.h"
#include "eparser.h"

#include <stdlib.h>
#include <ctype.h>
#include <algorithm>
#include <memory>
#include <set>

using namespace std;

VariableManager::VariableManager(const Ftk* F)
    : silent(false),
      F_(F),
      var_autoname_counter_(0),
      func_autoname_counter_(0)
{
}

VariableManager::~VariableManager()
{
    purge_all_elements(functions_);
    purge_all_elements(variables_);
}

void VariableManager::unregister_model(const Model *s)
{
    vector<Model*>::iterator k = find(models_.begin(), models_.end(), s);
    assert (k != models_.end());
    models_.erase(k);
}

void VariableManager::sort_variables()
{
    for (vector<Variable*>::iterator i = variables_.begin();
            i != variables_.end(); ++i)
        (*i)->set_var_idx(variables_);
    int pos = 0;
    while (pos < size(variables_)) {
        int M = variables_[pos]->get_max_var_idx();
        if (M > pos) {
            swap(variables_[pos], variables_[M]);
            for (vector<Variable*>::iterator i = variables_.begin();
                    i != variables_.end(); ++i)
                (*i)->set_var_idx(variables_);
        }
        else
            ++pos;
    }
}

static
string parse_and_find_fz_idx(const Ftk* F, string const &fstr)
{
    int pos = 0;
    int pref = -1;
    if (fstr[0] == '@') {
        pos = fstr.find(".") + 1;
        pref = strtol(fstr.c_str()+1, 0, 10);
    }
    vector<string> const &names = F->get_model(pref)->get_fz(fstr[pos]).names;
    int idx_ = strtol(fstr.c_str()+pos+2, 0, 10);
    int idx = (idx_ >= 0 ? idx_ : idx_ + names.size());
    if (!is_index(idx, names))
        throw ExecuteError("There is no item with index " + S(idx_));
    return names[idx];
}

/// takes string parsable by FuncGrammar and:
///  if the string refers to one variable -- returns its name
///  else makes variable and returns its name
string VariableManager::get_or_make_variable(const string& func)
{
    string ret;
    assert(!func.empty());
    string tmp1, tmp2;
    if (parse(func.c_str(), lexeme_d["$" >> +(alnum_p | '_')]).full) // $foo
        ret = string(func, 1);
    else if (parse(func.c_str(),
                   ( lexeme_d["%" >> +(alnum_p | '_')]
                   | !lexeme_d['@' >> uint_p >> '.']
                     >> (str_p("F[")|"Z[") >> int_p >> ch_p(']')
                   ) [assign_a(tmp1)]
                   >> '.' >>
                   lexeme_d[alpha_p >> *(alnum_p|'_')][assign_a(tmp2)]
                  ).full) {                     // %bar.bleh
        string name = parse_and_find_fz_idx(F_, tmp1);
        const Function* f = F_->find_function(name);
        ret = f->get_var_name(f->get_param_nr(tmp2));
    }
    else {                                     // anything else
        ret = next_var_name();
        assign_variable(ret, func);
    }
    return ret;
}


int VariableManager::assign_variable(const string &name, const string &rhs)
{
    Variable *var = 0;
    assert(!name.empty());

    if (rhs.empty()) {// mirror-variable
        var = new Variable(name, -2);
        return add_variable(var);
    }

    tree_parse_info<> info = ast_parse(rhs.c_str(), FuncG >> end_p, space_p);
    assert(info.full);
    const_tm_iter_t const &root = info.trees.begin();
    if (root->value.id() == FuncGrammar::variableID
            && *root->value.begin() == '~') { //simple variable
        string val_str = string(root->value.begin()+1, root->value.end());
        /*
        string domain_str;
        string::size_type pos = skip_variable_value(val_str, 0);
        if (pos < val_str.size() && val_str[pos] == '[') {
            domain_str = string(val_str, pos);
            val_str.erase(pos);
        }
        */
        fp val = get_constant_value(val_str);
        int nr;

        // avoid changing order of parameters in case of "$_1 = ~1.23"
        int old_pos = find_variable_nr(name);
        if (old_pos != -1 && variables_[old_pos]->is_simple()) {
            nr = variables_[old_pos]->get_nr();
            parameters_[nr] = val; //variable at old_pos will be deleted soon
        }
        else {
            nr = parameters_.size();
            parameters_.push_back(val);
        }
        var = new Variable(name, nr);
        /*
        if (!domain_str.empty())
            parse_and_set_domain(var, domain_str);
        */
    }
    else {
        vector<string> vars=find_tokens_in_ptree(FuncGrammar::variableID, info);
        if (contains_element(vars, "x"))
            throw ExecuteError("variable can't depend on x.");
        v_foreach (string, i, vars)
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
        var = new Variable(name, vars, op_trees);
    }
    return add_variable(var);
}

bool VariableManager::is_variable_referred(int i, string *first_referrer)
{
    // A variable can be referred only by variables with larger index.
    for (int j = i+1; j < size(variables_); ++j) {
        if (variables_[j]->is_directly_dependent_on(i)) {
            if (first_referrer)
                *first_referrer = variables_[j]->xname;
            return true;
        }
    }
    for (vector<Function*>::iterator j = functions_.begin();
            j != functions_.end(); ++j) {
        if ((*j)->is_directly_dependent_on(i)) {
            if (first_referrer)
                *first_referrer = (*j)->xname;
            return true;
        }
    }
    return false;
}

vector<string>
VariableManager::get_variable_references(const string &name) const
{
    int idx = find_variable_nr(name);
    vector<string> refs;
    v_foreach (Variable*, i, variables_)
        if ((*i)->is_directly_dependent_on(idx))
            refs.push_back((*i)->xname);
    v_foreach (Function*, i, functions_)
        for (int j = 0; j < (*i)->get_vars_count(); ++j)
            if ((*i)->get_var_idx(j) == idx)
                refs.push_back((*i)->xname + "." + (*i)->get_param(j));
    return refs;
}

// set indices corresponding to variable names in all functions and variables
void VariableManager::reindex_all()
{
    for (vector<Variable*>::iterator i = variables_.begin();
            i != variables_.end(); ++i)
        (*i)->set_var_idx(variables_);
    for (vector<Function*>::iterator i = functions_.begin();
            i != functions_.end(); ++i) {
        (*i)->set_var_idx(variables_);
    }
}

void VariableManager::remove_unreferred()
{
    // remove auto-delete marked variables, which are not referred by others
    for (int i = variables_.size()-1; i >= 0; --i)
        if (variables_[i]->is_auto_delete()) {
            if (!is_variable_referred(i)) {
                delete variables_[i];
                variables_.erase(variables_.begin() + i);
            }
        }

    // re-index all functions and variables (in any case)
    reindex_all();

    // remove unreferred parameters
    for (int i = size(parameters_)-1; i >= 0; --i) {
        bool del=true;
        for (int j = 0; j < size(variables_); ++j)
            if (variables_[j]->get_nr() == i) {
                del=false;
                break;
            }
        if (del) {
            parameters_.erase(parameters_.begin() + i);
            // take care about parameter indices in variables and functions
            for (vector<Variable*>::iterator j = variables_.begin();
                    j != variables_.end(); ++j)
                (*j)->erased_parameter(i);
            for (vector<Function*>::iterator j = functions_.begin();
                    j != functions_.end(); ++j)
                (*j)->erased_parameter(i);
        }
    }
}

string VariableManager::get_variable_info(const Variable* v) const
{
    string s = v->xname + " = " + v->get_formula(parameters_) + " = "
               + F_->get_settings()->format_double(v->get_value());
    if (v->domain.is_set())
        s += "  " + v->domain.str();
    if (v->is_auto_delete())
        s += "  [auto]";
    return s;
}

/// puts Variable into `variables_' vector, checking dependencies
int VariableManager::add_variable(Variable* new_var)
{
    auto_ptr<Variable> var(new_var);
    var->set_var_idx(variables_);
    int pos = find_variable_nr(var->name);
    if (pos == -1) {
        pos = variables_.size();
        variables_.push_back(var.release());
    }
    else {
        if (var->is_dependent_on(pos, variables_)) { //check for loops
            throw ExecuteError("loop in dependencies of " + var->xname);
        }
        delete variables_[pos];
        variables_[pos] = var.release();
        if (variables_[pos]->get_max_var_idx() > pos)
            sort_variables();
        remove_unreferred();
    }
    return pos;
}

void VariableManager::assign_variable_copy(const string& name,
                                           const Variable* orig,
                                           const map<int,string>& varmap)
{
    Variable *var=0;
    assert(!name.empty());
    if (orig->is_simple()) {
        fp val = orig->get_value();
        parameters_.push_back(val);
        int nr = parameters_.size() - 1;
        var = new Variable(name, nr);
    }
    else {
        vector<string> vars;
        for (int i = 0; i != orig->get_vars_count(); ++i) {
            assert(varmap.count(orig->get_var_idx(i)));
            vars.push_back(varmap.find(orig->get_var_idx(i))->second);
        }
        vector<OpTree*> new_op_trees;
        v_foreach (OpTree*, i, orig->get_op_trees())
            new_op_trees.push_back((*i)->copy());
        var = new Variable(name, vars, new_op_trees);
    }
    add_variable(var);
}

// names can contains '*' wildcards
void VariableManager::delete_variables(const vector<string> &names)
{
    if (names.empty())
        return;

    set<int> nn;
    // find indices of variables_, expanding wildcards
    v_foreach (string, i, names) {
        if (i->find('*') == string::npos) {
            int k = find_variable_nr(*i);
            if (k == -1)
                throw ExecuteError("undefined variable: $" + *i);
            nn.insert(k);
        }
        else
            for (size_t j = 0; j != variables_.size(); ++j)
                if (match_glob(variables_[j]->name.c_str(), i->c_str()))
                    nn.insert(j);
    }

    // Delete variables_. The descending index order is required to make
    // is_variable_referred() and variables_.erase() work properly.
    for (set<int>::const_reverse_iterator i = nn.rbegin(); i != nn.rend(); ++i){
        // Check for dependencies.
        string first_referrer;
        if (is_variable_referred(*i, &first_referrer)) {
            reindex_all();
            remove_unreferred(); // post-delete
            throw ExecuteError("can't delete $" + get_variable(*i)->name +
                             " because " + first_referrer + " depends on it.");
        }

        delete variables_[*i];
        variables_.erase(variables_.begin() + *i);
    }

    // post-delete
    reindex_all();
    remove_unreferred();
}

void VariableManager::delete_funcs(const vector<string>& names)
{
    if (names.empty())
        return;

    set<int> nn;
    // find indices of functions, expanding wildcards
    v_foreach (string, i, names) {
        if (i->find('*') == string::npos) {
            int k = find_function_nr(*i);
            if (k == -1)
                throw ExecuteError("undefined function: %" + *i);
            nn.insert(k);
        }
        else
            for (size_t j = 0; j != functions_.size(); ++j)
                if (match_glob(functions_[j]->name.c_str(), i->c_str()))
                    nn.insert(j);
    }

    // Delete functions. The descending index order is needed by .erase().
    for (set<int>::const_reverse_iterator i = nn.rbegin(); i != nn.rend(); ++i){
        delete functions_[*i];
        functions_.erase(functions_.begin() + *i);
    }

    // post-delete
    remove_unreferred();
    update_indices_in_models();
}

bool VariableManager::is_function_referred(int n) const
{
    v_foreach (Model*, i, models_) {
        if (contains_element((*i)->get_ff().idx, n)
                || contains_element((*i)->get_zz().idx, n))
            return true;
    }
    return false;
}

void VariableManager::auto_remove_functions()
{
    int func_size = functions_.size();
    for (int i = func_size - 1; i >= 0; --i)
        if (functions_[i]->is_auto_delete() && !is_function_referred(i)) {
            delete functions_[i];
            functions_.erase(functions_.begin() + i);
        }
    if (func_size != size(functions_)) {
        remove_unreferred();
        update_indices_in_models();
    }
}

int VariableManager::find_function_nr(const string &name) const
{
    for (int i = 0; i < size(functions_); ++i)
        if (functions_[i]->name == name)
            return i;
    return -1;
}

const Function* VariableManager::find_function(const string &name) const
{
    int n = find_function_nr(name);
    if (n == -1)
        throw ExecuteError("undefined function: %" + name);
    return functions_[n];
}

int VariableManager::find_variable_nr(const string &name) const
{
    for (int i = 0; i < size(variables_); ++i)
        if (variables_[i]->name == name)
            return i;
    return -1;
}

const Variable* VariableManager::find_variable(const string &name) const
{
    int n = find_variable_nr(name);
    if (n == -1)
        throw ExecuteError("undefined variable: $" + name);
    return variables_[n];
}

int VariableManager::find_nr_var_handling_param(int p) const
{
    assert(p >= 0 && p < size(parameters_));
    for (size_t i = 0; i < variables_.size(); ++i)
        if (variables_[i]->get_nr() == p)
            return i;
    assert(0);
    return 0;
}

int VariableManager::find_parameter_variable(int par) const
{
    for (int i = 0; i < size(variables_); ++i)
        if (variables_[i]->get_nr() == par)
            return i;
    return -1;
}

void VariableManager::use_parameters()
{
    use_external_parameters(parameters_);
}

void VariableManager::use_external_parameters(const vector<fp> &ext_param)
{
    vm_foreach (Variable*, i, variables_)
        (*i)->recalculate(variables_, ext_param);
    vm_foreach (Function*, i, functions_)
        (*i)->do_precomputations(variables_);
}

void VariableManager::put_new_parameters(const vector<fp> &aa)
{
    for (size_t i = 0; i < min(aa.size(), parameters_.size()); ++i)
        parameters_[i] = aa[i];
    use_parameters();
}

int VariableManager::assign_func(const string &name, Tplate::Ptr tp,
                                 const vector<string> &args)
{
    Function *func = 0;
    assert(tp);
    vector<string> varnames;
    v_foreach (string, j, args)
        varnames.push_back(get_or_make_variable(*j));
    func = (*tp->create)(F_, name, tp, varnames);
    func->init();
    return add_func(func);
}

int VariableManager::add_func(Function* func)
{
    func->set_var_idx(variables_);
    // if there is already function with the same name -- replace
    int nr = find_function_nr(func->name);
    if (nr != -1) {
        delete functions_[nr];
        functions_[nr] = func;
        remove_unreferred();
        if (!silent)
            F_->msg("%" + func->name + " replaced.");
    }
    else {
        nr = functions_.size();
        functions_.push_back(func);
        if (!silent)
            F_->msg("%" + func->name + " created.");
    }
    return nr;
}

string VariableManager::make_var_copy_name(const Variable* v)
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

int VariableManager::assign_func_copy(const string &name, const string &orig)
{
    assert(!name.empty());
    const Function* of = find_function(orig);
    map<int,string> varmap;
    for (int i = 0; i < size(variables_); ++i) {
        if (!of->is_dependent_on(i, variables_))
            continue;
        const Variable* var_orig = variables_[i];
        string new_varname = make_var_copy_name(var_orig);
        assign_variable_copy(new_varname, var_orig, varmap);
        varmap[i] = new_varname;
    }
    vector<string> varnames;
    for (int i = 0; i != of->get_vars_count(); ++i) {
        assert(varmap.count(of->get_var_idx(i)));
        varnames.push_back(varmap[of->get_var_idx(i)]);
    }

    Tplate::Ptr tp = of->tp();
    Function* func = (*tp->create)(F_, name, tp, varnames);
    func->init();
    return add_func(func);
}

void VariableManager::substitute_func_param(const string &name,
                                            const string &param,
                                            const string &var)
{
    int nr = find_function_nr(name);
    if (nr == -1)
        throw ExecuteError("undefined function: %" + name);
    Function* k = functions_[nr];
    k->substitute_param(k->get_param_nr(param), get_or_make_variable(var));
    k->set_var_idx(variables_);
    remove_unreferred();
}

fp VariableManager::variation_of_a (int n, fp variat) const
{
    assert (0 <= n && n < size(parameters()));
    const Domain& dom = get_variable(n)->domain;
    fp ctr = dom.is_ctr_set() ? dom.get_ctr() : parameters_[n];
    fp sgm = dom.is_set() ? dom.get_sigma()
            : ctr * F_->get_settings()->get_f("variable_domain_percent") / 100.;
    return ctr + sgm * variat;
}

string VariableManager::next_var_name()
{
    while (1) {
        string t = "_" + S(++var_autoname_counter_);
        if (find_variable_nr(t) == -1)
            return t;
    }
}

string VariableManager::next_func_name()
{
    while (1) {
        string t = "_" + S(++func_autoname_counter_);
        if (find_function_nr(t) == -1)
            return t;
    }
}

void VariableManager::do_reset()
{
    purge_all_elements(functions_);
    purge_all_elements(variables_);
    var_autoname_counter_ = 0;
    func_autoname_counter_ = 0;
    parameters_.clear();
    //don't delete models, they should unregister itself
    update_indices_in_models();
}

void VariableManager::update_indices(FunctionSum& sum)
{
    sum.idx.clear();
    size_t i = 0;
    while (i < sum.names.size()) {
        int k = find_function_nr(sum.names[i]);
        if (k == -1)
            sum.names.erase(sum.names.begin() + i);
        else {
            sum.idx.push_back(k);
            ++i;
        }
    }
}

void VariableManager::update_indices_in_models()
{
    for (vector<Model*>::iterator i = models_.begin(); i != models_.end(); ++i){
        update_indices((*i)->get_ff());
        update_indices((*i)->get_zz());
    }
}

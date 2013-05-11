// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#define BUILDING_LIBFITYK
#include "mgr.h"

#include <stdlib.h>
#include <ctype.h>
#include <algorithm>
#include <memory>
#include <set>

#include "common.h"
#include "var.h"
#include "ast.h"
#include "ui.h"
#include "tplate.h"
#include "func.h"
#include "model.h"
#include "settings.h"
#include "logic.h"
#include "lexer.h"
#include "eparser.h"

using namespace std;

namespace fityk {

ModelManager::ModelManager(const BasicContext* ctx)
    : ctx_(ctx),
      var_autoname_counter_(0),
      func_autoname_counter_(0)
{
}

ModelManager::~ModelManager()
{
    purge_all_elements(functions_);
    purge_all_elements(variables_);
}

void ModelManager::unregister_model(const Model *s)
{
    vector<Model*>::iterator k = find(models_.begin(), models_.end(), s);
    assert (k != models_.end());
    models_.erase(k);
}

void ModelManager::sort_variables()
{
    for (vector<Variable*>::iterator i = variables_.begin();
            i != variables_.end(); ++i)
        (*i)->set_var_idx(variables_);
    int pos = 0;
    while (pos < size(variables_)) {
        int M = variables_[pos]->used_vars().get_max_idx();
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

/*
/// takes expression string and:
///  if the string refers to existing variable -- returns its name
///  otherwise -- creates a new variable and returns its name
string ModelManager::get_or_make_variable(const VMData* vd)
{
    int ret;
    if (vd->index != -1) {
        ret = vd->index;
    }
    else {
        ret = next_var_name();
        assign_variable(ret, func);
    }
    assert(is_index(ret, variables_));
    return ret;
}
*/

Variable* make_compound_variable(const string &name, VMData* vd,
                                 const vector<Variable*>& all_variables)
{
    //printf("make_compound_variable: %s\n", vm2str(*vd).c_str());
    if (vd->has_op(OP_X))
        throw ExecuteError("variable can't depend on x.");

    // re-index variables
    vector<string> used_vars;
    vm_foreach (int, i, vd->get_mutable_code()) {
        if (*i == OP_SYMBOL) {
            ++i;
            const string& name = all_variables[*i]->name;
            int idx = index_of_element(used_vars, name);
            if (idx == -1) {
                idx = used_vars.size();
                used_vars.push_back(name);
            }
            *i = idx;
        }
        else if (VMData::has_idx(*i))
            ++i;
    }

    vector<OpTree*> op_trees = prepare_ast_with_der(*vd, used_vars.size());
    return new Variable(name, used_vars, op_trees);
}

int ModelManager::make_variable(const string &name, VMData* vd)
{
    Variable *var;
    assert(!name.empty());
    const std::vector<int>& code = vd->code();

    // simple variable [OP_TILDE OP_NUMBER idx]
    if (code.size() == 3 && code[0] == OP_TILDE && code[1] == OP_NUMBER) {
        realt val = vd->numbers()[code[2]];
        // avoid changing order of parameters in case of "$var = ~1.23"
        int old_pos = find_variable_nr(name);
        if (old_pos != -1 && variables_[old_pos]->is_simple()) {
            int gpos = variables_[old_pos]->gpos();
            // variable at old_pos will be deleted soon
            parameters_[gpos] = val;
            return old_pos;
        }

        var = new Variable(name, parameters_.size());
        parameters_.push_back(val);
    }

    // compound variable
    else {
        // OP_TILDE -> new variable
        vector<int>& code = vd->get_mutable_code();
        vm_foreach (int, op, code) {
            if (*op == OP_TILDE) {
                *op = OP_SYMBOL;
                ++op;
                assert(*op == OP_NUMBER);
                *op = variables_.size();
                int num_index = *(op+1);
                double value = vd->numbers()[num_index];
                code.erase(op+1);
                string tname = next_var_name();
                Variable *tilde_var = new Variable(tname, parameters_.size());
                parameters_.push_back(value);
                variables_.push_back(tilde_var);
            }
            else if (VMData::has_idx(*op))
                ++op;
        }

        var = make_compound_variable(name, vd, variables_);
    }
    return add_variable(var);
}

bool ModelManager::is_variable_referred(int i, string *first_referrer)
{
    // A variable can be referred only by variables with larger index.
    for (int j = i+1; j < size(variables_); ++j) {
        if (variables_[j]->used_vars().has_idx(i)) {
            if (first_referrer)
                *first_referrer = "$" + variables_[j]->name;
            return true;
        }
    }
    for (vector<Function*>::iterator j = functions_.begin();
            j != functions_.end(); ++j) {
        if ((*j)->used_vars().has_idx(i)) {
            if (first_referrer)
                *first_referrer = "%" + (*j)->name;
            return true;
        }
    }
    return false;
}

vector<string>
ModelManager::get_variable_references(const string &name) const
{
    int idx = find_variable_nr(name);
    vector<string> refs;
    v_foreach (Variable*, i, variables_)
        if ((*i)->used_vars().has_idx(idx))
            refs.push_back("$" + (*i)->name);
    v_foreach (Function*, i, functions_)
        for (int j = 0; j < (*i)->used_vars().get_count(); ++j)
            if ((*i)->used_vars().get_idx(j) == idx)
                refs.push_back("%" + (*i)->name + "." + (*i)->get_param(j));
    return refs;
}

// set indices corresponding to variable names in all functions and variables
void ModelManager::reindex_all()
{
    for (vector<Variable*>::iterator i = variables_.begin();
            i != variables_.end(); ++i)
        (*i)->set_var_idx(variables_);
    for (vector<Function*>::iterator i = functions_.begin();
            i != functions_.end(); ++i) {
        (*i)->update_var_indices(variables_);
    }
}

void ModelManager::remove_unreferred()
{
    // remove auto-delete marked variables, which are not referred by others
    for (int i = variables_.size()-1; i >= 0; --i)
        if (is_auto(variables_[i]->name) && !is_variable_referred(i)) {
            delete variables_[i];
            variables_.erase(variables_.begin() + i);
        }

    // re-index all functions and variables (in any case)
    reindex_all();

    // remove unreferred parameters
    for (int i = size(parameters_)-1; i >= 0; --i) {
        bool del=true;
        for (int j = 0; j < size(variables_); ++j)
            if (variables_[j]->gpos() == i) {
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

/// puts Variable into `variables_' vector, checking dependencies
int ModelManager::add_variable(Variable* new_var)
{
    auto_ptr<Variable> var(new_var);
    var->set_var_idx(variables_);
    int pos = find_variable_nr(var->name);
    if (pos == -1) {
        pos = variables_.size();
        variables_.push_back(var.release());
    }
    else {
        if (var->used_vars().depends_on(pos, variables_)) { //check for loops
            throw ExecuteError("loop in dependencies of $" + var->name);
        }
        delete variables_[pos];
        variables_[pos] = var.release();
        if (variables_[pos]->used_vars().get_max_idx() > pos)
            sort_variables();
        remove_unreferred();
    }
    return pos;
}

string ModelManager::assign_variable_copy(const Variable* orig,
                                             const map<int,string>& varmap)
{
    string name = name_var_copy(orig);
    Variable *var;
    if (orig->is_simple()) {
        realt val = orig->value();
        parameters_.push_back(val);
        int gpos = parameters_.size() - 1;
        var = new Variable(name, gpos);
    }
    else {
        vector<string> vars;
        for (int i = 0; i != orig->used_vars().get_count(); ++i) {
            int v_idx = orig->used_vars().get_idx(i);
            assert(varmap.count(v_idx));
            vars.push_back(varmap.find(v_idx)->second);
        }
        vector<OpTree*> new_op_trees;
        v_foreach (OpTree*, i, orig->get_op_trees())
            new_op_trees.push_back((*i)->clone());
        var = new Variable(name, vars, new_op_trees);
    }
    add_variable(var);
    return name;
}

// names can contains '*' wildcards
void ModelManager::delete_variables(const vector<string> &names)
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

void ModelManager::delete_funcs(const vector<string>& names)
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

bool ModelManager::is_function_referred(int n) const
{
    v_foreach (Model*, i, models_) {
        if (contains_element((*i)->get_ff().idx, n)
                || contains_element((*i)->get_zz().idx, n))
            return true;
    }
    return false;
}

// post: call update_indices_in_models()
void ModelManager::auto_remove_functions()
{
    int func_size = functions_.size();
    for (int i = func_size - 1; i >= 0; --i)
        if (is_auto(functions_[i]->name) && !is_function_referred(i)) {
            delete functions_[i];
            functions_.erase(functions_.begin() + i);
        }
    if (func_size != size(functions_)) {
        remove_unreferred();
    }
}

int ModelManager::find_function_nr(const string &name) const
{
    for (int i = 0; i < size(functions_); ++i)
        if (functions_[i]->name == name)
            return i;
    return -1;
}

const Function* ModelManager::find_function(const string &name) const
{
    int n = find_function_nr(name);
    if (n == -1)
        throw ExecuteError("undefined function: %" + name);
    return functions_[n];
}

int ModelManager::find_variable_nr(const string &name) const
{
    for (int i = 0; i < size(variables_); ++i)
        if (variables_[i]->name == name)
            return i;
    return -1;
}

const Variable* ModelManager::find_variable(const string &name) const
{
    int n = find_variable_nr(name);
    if (n == -1)
        throw ExecuteError("undefined variable: $" + name);
    return variables_[n];
}

int ModelManager::gpos_to_vpos(int gpos) const
{
    assert(gpos >= 0 && gpos < size(parameters_));
    for (size_t i = 0; i < variables_.size(); ++i)
        if (variables_[i]->gpos() == gpos)
            return i;
    assert(0);
    return 0;
}

void ModelManager::use_parameters()
{
    use_external_parameters(parameters_);
}

void ModelManager::use_external_parameters(const vector<realt> &ext_param)
{
    vm_foreach (Variable*, i, variables_)
        (*i)->recalculate(variables_, ext_param);
    vm_foreach (Function*, i, functions_)
        (*i)->do_precomputations(variables_);
}

void ModelManager::put_new_parameters(const vector<realt> &aa)
{
    for (size_t i = 0; i < min(aa.size(), parameters_.size()); ++i)
        parameters_[i] = aa[i];
    use_parameters();
}

void ModelManager::set_domain(int n, const RealRange& domain)
{
    variables_[n]->domain = domain;
}

int ModelManager::assign_func(const string &name, Tplate::Ptr tp,
                                 vector<VMData*> &args)
{
    assert(tp);
    vector<string> varnames;
    vm_foreach (VMData*, j, args) {
        int idx = (*j)->single_symbol() ? (*j)->code()[1]
                                        : make_variable(next_var_name(), *j);
        varnames.push_back(variables_[idx]->name);
    }
    Function *func = (*tp->create)(ctx_->get_settings(), name, tp, varnames);
    func->init();
    return add_func(func);
}

int ModelManager::assign_func_copy(const string &name, const string &orig)
{
    assert(!name.empty());
    const Function* of = find_function(orig);
    map<int,string> var_copies;
    for (int i = 0; i < size(variables_); ++i) {
        if (of->used_vars().depends_on(i, variables_)) {
            const Variable* var_orig = variables_[i];
            var_copies[i] = assign_variable_copy(var_orig, var_copies);
        }
    }
    vector<string> varnames;
    for (int i = 0; i != of->used_vars().get_count(); ++i) {
        int v_idx = of->used_vars().get_idx(i);
        assert(var_copies.count(v_idx));
        varnames.push_back(var_copies[v_idx]);
    }

    Tplate::Ptr tp = of->tp();
    Function* func = (*tp->create)(ctx_->get_settings(), name, tp, varnames);
    func->init();
    return add_func(func);
}

int ModelManager::add_func(Function* func)
{
    func->update_var_indices(variables_);
    // if there is already function with the same name -- replace
    int nr = find_function_nr(func->name);
    if (nr != -1) {
        delete functions_[nr];
        functions_[nr] = func;
        remove_unreferred();
        ctx_->msg("%" + func->name + " replaced.");
    }
    else {
        nr = functions_.size();
        functions_.push_back(func);
        ctx_->msg("%" + func->name + " created.");
    }
    return nr;
}

string ModelManager::name_var_copy(const Variable* v)
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

void ModelManager::substitute_func_param(const string &name,
                                            const string &param,
                                            VMData* vd)
{
    int nr = find_function_nr(name);
    if (nr == -1)
        throw ExecuteError("undefined function: %" + name);
    Function* k = functions_[nr];
    int v_idx = vd->single_symbol() ? vd->code()[1]
                                    : make_variable(next_var_name(), vd);
    k->set_param_name(k->get_param_nr(param), variables_[v_idx]->name);
    k->update_var_indices(variables_);
    remove_unreferred();
}

realt ModelManager::variation_of_a(int n, realt variat) const
{
    assert (0 <= n && n < size(parameters()));
    const Variable* v = get_variable(n);
    double lo = v->domain.lo, hi = v->domain.hi;
    double percent = ctx_->get_settings()->domain_percent;
    if (v->domain.lo_inf())
        lo = v->value() * (1 - 0.01 * percent);
    if (v->domain.hi_inf())
        hi = v->value() * (1 + 0.01 * percent);
    // return lower bound for variat=-1 and upper bound for variat=1
    return lo + 0.5 * (variat + 1) * (hi - lo);
}

string ModelManager::next_var_name()
{
    while (1) {
        string t = "_" + S(++var_autoname_counter_);
        if (find_variable_nr(t) == -1)
            return t;
    }
}

string ModelManager::next_func_name()
{
    while (1) {
        string t = "_" + S(++func_autoname_counter_);
        if (find_function_nr(t) == -1)
            return t;
    }
}

void ModelManager::do_reset()
{
    purge_all_elements(functions_);
    purge_all_elements(variables_);
    var_autoname_counter_ = 0;
    func_autoname_counter_ = 0;
    parameters_.clear();
    //don't delete models, they should unregister itself
    update_indices_in_models();
}

void ModelManager::update_indices(FunctionSum& sum)
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

void ModelManager::update_indices_in_models()
{
    for (vector<Model*>::iterator i = models_.begin(); i != models_.end(); ++i){
        update_indices((*i)->get_ff());
        update_indices((*i)->get_zz());
    }
}

} // namespace fityk

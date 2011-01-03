// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#include "udf.h"
#include "ast.h"

using namespace std;


Function* init_component(const string& func_name, const Tplate::Component& c,
                         vector<Variable*>& variables, const Settings* settings)
{
    assert(c.p);
    vector<string> varnames;
    v_foreach (VMData, j, c.cargs) {
        string var_name;
        if (j->single_symbol()) {
            int idx = j->code()[1];
            var_name = variables[idx]->name;
        }
        else {
            var_name = "_i" + S(variables.size() + 1);
            //TODO handle OP_TILDE
            VMData vm = *j;
            Variable *v = make_compound_variable(var_name, &vm, variables);
            v->set_var_idx(variables);
            variables.push_back(v);
        }
        varnames.push_back(var_name);
    }
    Function *func = (*c.p->create)(settings, func_name, c.p, varnames);
    func->init();
    func->set_var_idx(variables);
    return func;
}


CompoundFunction::CompoundFunction(const Settings* settings,
                                   const string &name,
                                   const Tplate::Ptr tp,
                                   const vector<string> &vars)
    : Function(settings, name, tp, vars)
{
}

CompoundFunction::~CompoundFunction()
{
    purge_all_elements(intern_functions_);
    purge_all_elements(intern_variables_);
}

void CompoundFunction::init()
{
    Function::init();

    // add mirror-variables
    for (int j = 0; j != nv(); ++j) {
        Variable* var = new Variable(varnames[j], -2);
        intern_variables_.push_back(var);
    }

    v_foreach (Tplate::Component, i, tp_->components) {
        string func_name = "_i" + S(intern_functions_.size() + 1);
        Function *func = init_component(func_name, *i, intern_variables_,
                                        settings_);
        intern_functions_.push_back(func);
    }
}

void CompoundFunction::set_var_idx(vector<Variable*> const& variables)
{
    VariableUser::set_var_idx(variables);
    for (int i = 0; i < nv(); ++i) {
        const Variable* orig = variables[get_var_idx(i)];
        intern_variables_[i]->set_original(orig);
    }
}

void CompoundFunction::more_precomputations()
{
    vm_foreach (Variable*, i, intern_variables_)
        (*i)->recalculate(intern_variables_, vector<double>());
    vm_foreach (Function*, i, intern_functions_)
        (*i)->do_precomputations(intern_variables_);
}

void CompoundFunction::calculate_value_in_range(const vector<fp> &xx,
                                                vector<fp> &yy,
                                                int first, int last) const
{
    v_foreach (Function*, i, intern_functions_)
        (*i)->calculate_value_in_range(xx, yy, first, last);
}

void CompoundFunction::calculate_value_deriv_in_range(
                                             const vector<fp> &xx,
                                             vector<fp> &yy, vector<fp> &dy_da,
                                             bool in_dx,
                                             int first, int last) const
{
    v_foreach (Function*, i, intern_functions_)
        (*i)->calculate_value_deriv_in_range(xx, yy, dy_da, in_dx, first, last);
}

string CompoundFunction::get_current_formula(const string& x) const
{
    string t;
    v_foreach (Function*, i, intern_functions_) {
        if (!t.empty())
            t += "+";
        t += (*i)->get_current_formula(x);
    }
    return t;
}

bool CompoundFunction::get_center(fp* a) const
{
    vector<Function*> const& ff = intern_functions_;
    bool r = ff[0]->get_center(a);
    if (!r)
        return false;
    for (size_t i = 1; i < ff.size(); ++i) {
        fp b;
        r = ff[i]->get_center(&b);
        if (!r || is_neq(*a, b))
            return false;
    }
    return true;
}

/// if consists of >1 functions and centers are in the same place
///  height is a sum of heights
bool CompoundFunction::get_height(fp* a) const
{
    vector<Function*> const& ff = intern_functions_;
    if (ff.size() == 1)
        return ff[0]->get_height(a);
    fp ctr;
    if (!get_center(&ctr))
        return false;
    fp sum = 0;
    for (size_t i = 0; i < ff.size(); ++i) {
        if (!ff[i]->get_height(a))
            return false;
        sum += *a;
    }
    *a = sum;
    return true;
}

bool CompoundFunction::get_fwhm(fp* a) const
{
    vector<Function*> const& ff = intern_functions_;
    if (ff.size() == 1)
        return ff[0]->get_fwhm(a);
    return false;
}

bool CompoundFunction::get_area(fp* a) const
{
    vector<Function*> const& ff = intern_functions_;
    fp sum = 0;
    for (size_t i = 0; i < ff.size(); ++i)
        if (ff[i]->get_area(a))
            sum += *a;
        else
            return false;
    *a = sum;
    return true;
}

bool CompoundFunction::get_nonzero_range(fp level, fp& left, fp& right) const
{
    vector<Function*> const& ff = intern_functions_;
    if (ff.size() == 1)
        return ff[0]->get_nonzero_range(level, left, right);
    else
        return false;
}

///////////////////////////////////////////////////////////////////////

CustomFunction::CustomFunction(const Settings* settings,
                               const string &name,
                               const Tplate::Ptr tp,
                               const vector<string> &vars)
    : Function(settings, name, tp, vars),
      // don't use nv() here, it's not set until init()
      derivatives_(vars.size()+1), op_trees_(tp->op_trees)
{
}

CustomFunction::~CustomFunction()
{
    purge_all_elements(op_trees_);
}

void CustomFunction::set_var_idx(const vector<Variable*>& variables)
{
    VariableUser::set_var_idx(variables);

    assert(var_idx.size() + 2 == op_trees_.size());
    // we put function's parameter index rather than variable index after
    //  OP_SYMBOL, it is handled in this way in more_precomputations()
    vector<int> symbol_map = range_vector(0, var_idx.size());
    vm_.clear_data();
    int n = op_trees_.size() - 1;
    for (int i = 0; i < n; ++i) {
        add_bytecode_from_tree(op_trees_[i], symbol_map, vm_);
        vm_.append_code(OP_PUT_DERIV);
        vm_.append_code(i);
    }
    value_offset_ = vm_.code().size();
    add_bytecode_from_tree(op_trees_.back(), symbol_map, vm_);
}

void CustomFunction::more_precomputations()
{
    substituted_vm_ = vm_;
    substituted_vm_.replace_symbols(av_);
}

void CustomFunction::calculate_value_in_range(const vector<fp> &xx,
                                              vector<fp> &yy,
                                              int first, int last) const
{
    for (int i = first; i < last; ++i)
        yy[i] += run_code_for_custom_func_value(substituted_vm_, xx[i],
                                                value_offset_);
}

void CustomFunction::calculate_value_deriv_in_range(
                                           const vector<fp> &xx,
                                           vector<fp> &yy, vector<fp> &dy_da,
                                           bool in_dx,
                                           int first, int last) const
{
    int dyn = dy_da.size() / xx.size();
    for (int i = first; i < last; ++i) {
        run_code_for_custom_func(substituted_vm_, xx[i], derivatives_);

        if (!in_dx) {
            yy[i] += value_;
            v_foreach (Multi, j, multi_)
                dy_da[dyn*i+j->p] += derivatives_[j->n] * j->mult;
            dy_da[dyn*i+dyn-1] += derivatives_.back();
        }
        else {
            v_foreach (Multi, j, multi_)
                dy_da[dyn*i+j->p] += dy_da[dyn*i+dyn-1]
                                       * derivatives_[j->n] * j->mult;
        }
    }
}

string CustomFunction::get_bytecode() const
{
    const VMData& s = substituted_vm_;
    vector<int> der_code(s.code().begin(), s.code().begin() + value_offset_);
    vector<int> val_code(s.code().begin() + value_offset_, s.code().end());
    return "code with symbols: " + vm2str(vm_)
        + "\nderivatives: " + vm2str(der_code, s.numbers())
        + "\nvalue: " + vm2str(val_code, s.numbers());
}

///////////////////////////////////////////////////////////////////////

SplitFunction::SplitFunction(const Settings* settings,
                             const string &name,
                             const Tplate::Ptr tp,
                             const vector<string> &vars)
    : Function(settings, name, tp, vars)
{
}

SplitFunction::~SplitFunction()
{
    delete left_;
    delete right_;
    purge_all_elements(intern_variables_);
}

void SplitFunction::init()
{
    Function::init();

    // add mirror-variables
    for (int j = 0; j != nv(); ++j) {
        Variable* var = new Variable(varnames[j], -2);
        intern_variables_.push_back(var);
    }

    left_ = init_component("l", tp_->components[1], intern_variables_,
                           settings_);
    right_ = init_component("r", tp_->components[2], intern_variables_,
                            settings_);

    VMData vm = tp_->components[0].cargs[0];
    Variable *v = make_compound_variable("split", &vm, intern_variables_);
    v->set_var_idx(intern_variables_);
    intern_variables_.push_back(v);
}

void SplitFunction::set_var_idx(vector<Variable*> const& variables)
{
    VariableUser::set_var_idx(variables);
    for (int i = 0; i < nv(); ++i) {
        const Variable* orig = variables[get_var_idx(i)];
        intern_variables_[i]->set_original(orig);
    }
}

void SplitFunction::more_precomputations()
{
    vm_foreach (Variable*, i, intern_variables_)
        (*i)->recalculate(intern_variables_, vector<double>());
    left_->do_precomputations(intern_variables_);
    right_->do_precomputations(intern_variables_);
}

void SplitFunction::calculate_value_in_range(const vector<fp> &xx,
                                             vector<fp> &yy,
                                             int first, int last) const
{
    double xsplit = intern_variables_.back()->get_value();
    int t = lower_bound(xx.begin(), xx.end(), xsplit) - xx.begin();
    left_->calculate_value_in_range(xx, yy, first, t);
    right_->calculate_value_in_range(xx, yy, t, last);
}

void SplitFunction::calculate_value_deriv_in_range(
                                          const vector<fp> &xx,
                                          vector<fp> &yy, vector<fp> &dy_da,
                                          bool in_dx,
                                          int first, int last) const
{
    double xsplit = intern_variables_.back()->get_value();
    int t = lower_bound(xx.begin(), xx.end(), xsplit) - xx.begin();
    left_-> calculate_value_deriv_in_range(xx, yy, dy_da, in_dx, first, t);
    right_-> calculate_value_deriv_in_range(xx, yy, dy_da, in_dx, t, last);
}

string SplitFunction::get_current_formula(const string& x) const
{
    double xsplit = intern_variables_.back()->get_value();
    return "x < " + S(xsplit) + " ? " + left_->get_current_formula(x)
                              + " : " + right_->get_current_formula(x);
}

bool SplitFunction::get_height(fp* a) const
{
    fp h2;
    return left_->get_height(a) && right_->get_height(&h2) && is_eq(*a, h2);
}

bool SplitFunction::get_center(fp* a) const
{
    fp c2;
    return left_->get_center(a) && right_->get_center(&c2) && is_eq(*a, c2);
}

bool SplitFunction::get_nonzero_range(fp level, fp& left, fp& right) const
{
    fp dummy;
    return left_->get_nonzero_range(level, left, dummy) &&
           right_->get_nonzero_range(level, dummy, right);
}


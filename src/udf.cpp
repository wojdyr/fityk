// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
// $Id: $

#include "udf.h"
#include "ast.h"

using namespace std;


CompoundFunction::CompoundFunction(const Settings* settings,
                                   const string &name,
                                   const Tplate::Ptr tp,
                                   const vector<string> &vars)
    : Function(settings, name, tp, vars)
{
}

void CompoundFunction::init()
{
    Function::init();
    init_components();
}

void CompoundFunction::init_components()
{
    for (int j = 0; j != nv(); ++j) {
        // mirror variables
        Variable* var = new Variable(varnames[j], -2);
        intern_variables_.push_back(var);
    }

    v_foreach (Tplate::Component, i, tp_->components) {
        assert(i->p);
        vector<string> varnames;
        v_foreach (VMData, j, i->cargs) {
            string var_name;
            if (j->single_symbol()) {
                int idx = j->code()[1];
                var_name = intern_variables_[idx]->name;
            }
            else {
                var_name = "_i" + S(intern_variables_.size() + 1);
                //TODO handle OP_TILDE
                VMData vm = *j;
                Variable *v = make_compound_variable(var_name, &vm,
                                                     intern_variables_);
                v->set_var_idx(intern_variables_);
                intern_variables_.push_back(v);
            }
            varnames.push_back(var_name);
        }
        string func_name = "_i" + S(intern_functions_.size() + 1);
        Function *func = (*i->p->create)(settings_, func_name, i->p, varnames);
        func->init();
        func->set_var_idx(intern_variables_);
        intern_functions_.push_back(func);
        /*
        if (i->p)
            vmgr_.assign_func(vmgr_.next_func_name(), i->p, args);
        else {
            assert (args.size() == 1);
            // TODO: use op_trees
            vmgr_.make_variable(vmgr_.next_var_name(), args[0]);
        }
        */
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
      derivatives_(vars.size()+1),
      afo_(tp->op_trees, value_, derivatives_)
{
}


void CustomFunction::set_var_idx(const vector<Variable*>& variables)
{
    VariableUser::set_var_idx(variables);
    afo_.treex_to_bytecode(var_idx.size());
}


void CustomFunction::more_precomputations()
{
    afo_.prepare_optimized_codes(av_);
}

void CustomFunction::calculate_value_in_range(const vector<fp> &xx,
                                              vector<fp> &yy,
                                              int first, int last) const
{
    for (int i = first; i < last; ++i) {
        yy[i] += afo_.run_vm_val(xx[i]);
    }
}

void CustomFunction::calculate_value_deriv_in_range(
                                           const vector<fp> &xx,
                                           vector<fp> &yy, vector<fp> &dy_da,
                                           bool in_dx,
                                           int first, int last) const
{
    int dyn = dy_da.size() / xx.size();
    for (int i = first; i < last; ++i) {
        afo_.run_vm_der(xx[i]);

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

///////////////////////////////////////////////////////////////////////

SplitFunction::SplitFunction(const Settings* settings,
                             const string &name,
                             const Tplate::Ptr tp,
                             const vector<string> &vars)
    : Function(settings, name, tp, vars)
{
}

void SplitFunction::init()
{
    Function::init();
#if 0
    init_components();
#endif
}

void SplitFunction::calculate_value_in_range(const vector<fp> &xx,
                                             vector<fp> &yy,
                                             int first, int last) const
{
#if 0
    double xsplit = vmgr_.variables().back()->get_value();
    int t = lower_bound(xx.begin(), xx.end(), xsplit) - xx.begin();
    vmgr_.get_function(0)->calculate_value_in_range(xx, yy, first, t);
    vmgr_.get_function(1)->calculate_value_in_range(xx, yy, t, last);
#endif
}

void SplitFunction::calculate_value_deriv_in_range(
                                          const vector<fp> &xx,
                                          vector<fp> &yy, vector<fp> &dy_da,
                                          bool in_dx,
                                          int first, int last) const
{
#if 0
    double xsplit = vmgr_.variables().back()->get_value();
    int t = lower_bound(xx.begin(), xx.end(), xsplit) - xx.begin();
    vmgr_.get_function(0)->
        calculate_value_deriv_in_range(xx, yy, dy_da, in_dx, first, t);
    vmgr_.get_function(1)->
        calculate_value_deriv_in_range(xx, yy, dy_da, in_dx, t, last);
#endif
}

string SplitFunction::get_current_formula(const string& x) const
{
#if 0
    double xsplit = vmgr_.variables().back()->get_value();
    return "x < " + S(xsplit)
        + " ? " + vmgr_.get_function(0)->get_current_formula(x)
        + " : " + vmgr_.get_function(1)->get_current_formula(x);
#endif
}

bool SplitFunction::get_height(fp* a) const
{
#if 0
    vector<Function*> const& ff = vmgr_.functions();
    fp h2;
    return ff[0]->get_height(a) && ff[1]->get_height(&h2) && is_eq(*a, h2);
#endif
}

bool SplitFunction::get_center(fp* a) const
{
#if 0
    vector<Function*> const& ff = vmgr_.functions();
    fp c2;
    return ff[0]->get_center(a) && ff[1]->get_center(&c2) && is_eq(*a, c2);
#endif
}

bool SplitFunction::get_nonzero_range(fp level, fp& left, fp& right) const
{
#if 0
    vector<Function*> const& ff = vmgr_.functions();
    fp dummy;
    return ff[0]->get_nonzero_range(level, left, dummy) &&
           ff[0]->get_nonzero_range(level, dummy, right);
#endif
}


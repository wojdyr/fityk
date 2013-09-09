// This file is part of fityk program. Copyright 2001-2013 Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#define BUILDING_LIBFITYK
#include "var.h"
#include "common.h"
#include "ast.h"

#include <stdlib.h>
#include <algorithm>

using namespace std;

namespace fityk {

/// checks if *this depends (directly or indirectly) on variable with index idx
bool IndexedVars::depends_on(int idx, vector<Variable*> const &variables) const
{
    v_foreach (int, i, indices_)
        if (*i == idx || variables[*i]->used_vars().depends_on(idx, variables))
            return true;
    return false;
}

void IndexedVars::update_indices(vector<Variable*> const &variables)
{
    const int n = names_.size();
    indices_.resize(n);
    for (int v = 0; v < n; ++v) {
        bool found = false;
        for (int i = 0; i < size(variables); ++i) {
            if (names_[v] == variables[i]->name) {
                indices_[v] = i;
                found = true;
                break;
            }
        }
        if (!found)
            throw ExecuteError("Undefined variable: $" + names_[v]);
    }
}

int IndexedVars::get_max_idx() const
{
    if (indices_.empty())
        return -1;
    else
       return *max_element(indices_.begin(), indices_.end());
}

////////////////////////////////////////////////////////////////////////////

// ctor for simple variables and mirror variables
Variable::Variable(string const &name_, int gpos)
    : Var(name_, gpos), original_(NULL)
{
    assert(!name_.empty());
    if (gpos_ != -2) {
        ParMult pm;
        pm.p = gpos_;
        pm.mult = 1;
        recursive_derivatives_.push_back(pm);
    }
}

// ctor for compound variables
Variable::Variable(string const &name_, vector<string> const &vars,
                   vector<OpTree*> const &op_trees)
    : Var(name_, -1), used_vars_(vars),
      derivatives_(vars.size()), op_trees_(op_trees), original_(NULL)
{
    assert(!name_.empty());
}

Variable::~Variable()
{
    purge_all_elements(op_trees_);
}

void Variable::set_var_idx(vector<Variable*> const& variables)
{
    used_vars_.update_indices(variables);
    if (gpos_ == -1) {
        /// (re-)create bytecode, required after update_indices()
        assert(used_vars_.indices().size() + 1 == op_trees_.size());
        vm_.clear_data();
        int n = op_trees_.size() - 1;
        for (int i = 0; i < n; ++i) {
            add_bytecode_from_tree(op_trees_[i], used_vars_.indices(), vm_);
            vm_.append_code(OP_PUT_DERIV);
            vm_.append_code(i);
        }
        add_bytecode_from_tree(op_trees_.back(), used_vars_.indices(), vm_);
        //printf("Variable::set_var_idx: %s\n", vm2str(vm_).c_str());
    }
}

string Variable::get_formula(vector<realt> const &parameters) const
{
    assert(gpos_ >= -1);
    vector<string> vn;
    v_foreach (string, i, used_vars_.names())
        vn.push_back("$" + *i);
    const char* num_format = "%.12g";
    OpTreeFormat fmt = { num_format, &vn };
    return gpos_ == -1 ? get_op_trees().back()->str(fmt)
                    : "~" + eS(parameters[gpos_]);
}

void Variable::recalculate(vector<Variable*> const &variables,
                           vector<realt> const &parameters)
{
    if (gpos_ >= 0) {
        assert (gpos_ < (int) parameters.size());
        value_ = parameters[gpos_];
        assert(derivatives_.empty());
    } else if (gpos_ == -1) {
        value_ = run_code_for_variable(vm_, variables, derivatives_);
        recursive_derivatives_.clear();
        for (int i = 0; i < size(derivatives_); ++i) {
            Variable *v = variables[used_vars_.get_idx(i)];
            v_foreach (ParMult, j, v->recursive_derivatives()) {
                recursive_derivatives_.push_back(*j);
                recursive_derivatives_.back().mult *= derivatives_[i];
            }
        }
    } else if (gpos_ == -2) {
        if (original_) {
            value_ = original_->value_;
            recursive_derivatives_ = original_->recursive_derivatives_;
        }
    } else
        assert(0);
}

void Variable::erased_parameter(int k)
{
    if (gpos_ != -1 && gpos_ > k)
            --gpos_;
    for (vector<ParMult>::iterator i = recursive_derivatives_.begin();
                                        i != recursive_derivatives_.end(); ++i)
        if (i->p > k)
            -- i->p;
}

bool Variable::is_constant() const
{
    return gpos_ == -1 && op_trees_.back()->op == 0;
}

} // namespace fityk

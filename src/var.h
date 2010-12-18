// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
// $Id$

#ifndef FITYK__VAR__H__
#define FITYK__VAR__H__

#include "common.h"
#include "vm.h"

struct OpTree;
class Variable;
class Function;
class Sum;

class VariableUser
{
public:
    const std::string name;
    const std::string prefix;

    VariableUser(const std::string &name_, std::string const &prefix_,
              const std::vector<std::string> &vars = std::vector<std::string>())
        : name(name_), prefix(prefix_), varnames(vars) {}
    virtual ~VariableUser() {}
    bool is_auto_delete() const { return name.size() > 0 && name[0] == '_'; }

    bool is_dependent_on(int idx, const std::vector<Variable*> &variables)const;
    bool is_directly_dependent_on(int idx) const
                                  { return contains_element(var_idx, idx); }

    virtual void set_var_idx(const std::vector<Variable*>& variables);
    int get_var_idx(int n) const
             { assert(n >= 0 && n < size(var_idx)); return var_idx[n]; }
    int get_max_var_idx();
    int get_vars_count() const { return varnames.size(); }
    const std::vector<std::string>& get_varnames() const { return varnames; }
    std::string get_var_name(int n) const
             { assert(n >= 0 && n < size(varnames)); return varnames[n]; }
    void substitute_param(int n, const std::string &new_p)
             { assert(n >= 0 && n < size(varnames)); varnames[n] = new_p; }
    std::string get_debug_idx_info() const;

protected:
    std::vector<std::string> varnames; // variable names
    /// var_idx is set after initialization (in derived class)
    /// and modified after variable removal or change
    std::vector<int> var_idx;
};


/// domain of variable, used _only_ for randomization of the variable
class Domain
{
    bool ok, ctr_set;
    fp ctr, sigma;

public:
    Domain() : ok(false), ctr_set(false) {}
    bool is_set() const { return ok; }
    bool is_ctr_set() const { return ctr_set; }
    fp get_ctr() const { assert(ok && ctr_set); return ctr; }
    fp get_sigma() const { assert(ok); return sigma; }
    void set(fp c, fp s) { ok=true; ctr_set=true; ctr=c; sigma=s; }
    void set_sigma(fp s) { ok=true; sigma=s; }
    std::string str() const
    {
        if (ok)
            return "[" + (ctr_set ? S(ctr) : S()) + " +- " + S(sigma) + "]";
        else
            return std::string();
    }
};


/// the variable is either simple-variable and nr_ is the index in vector
/// of parameters, or it is "compound variable" and has nr_==-1.
/// third special case: nr_==-2 - it is mirror-variable (such variable
///        is not recalculated but copied)
/// In second case, the value and derivatives are calculated
/// in following steps:
///  0. string is parsed by Spirit parser to Spirit AST representation,
///     and then expression is simplified and derivates are
///     calculated using calculate_deriv() function.
///     It results in struct-OpTree-based trees (for value and all derivatives)
///     That's before creating the Variable
///  1  set_var_idx() finds indices of variables in variables vector
///      (references to variables are kept using names of the variables
///       is has to be called when indices of referred variables change
///     and prepares bytecode from trees and var_idx
///  3. recalculate() calculates (using run_code_for_variable()) value
///     and derivatives for current parameter value
class Variable : public VariableUser
{
public:
    Domain domain;

    struct ParMult { int p; fp mult; };
    Variable(const std::string &name_, int nr_);
    Variable(std::string const &name_, std::vector<std::string> const &vars_,
             std::vector<OpTree*> const &op_trees_);
    void recalculate(std::vector<Variable*> const &variables,
                     std::vector<fp> const &parameters);

    int get_nr() const { return nr_; };
    void erased_parameter(int k);
    fp get_value() const { return value_; };
    std::string get_formula(std::vector<fp> const &parameters) const;
    bool is_visible() const { return true; } //for future use
    void set_var_idx(std::vector<Variable*> const& variables);
    std::vector<ParMult> const& recursive_derivatives() const
                                            { return recursive_derivatives_; }
    bool is_simple() const { return nr_ != -1; }
    bool is_constant() const;

    std::vector<OpTree*> const& get_op_trees() const { return op_trees_; }
    void set_original(Variable const* orig) { assert(nr_==-2); original_=orig; }
    fp get_derivative(int n) const { return derivatives_[n]; }

private:
    int nr_; /// see description of this class in .h
    fp value_;
    std::vector<fp> derivatives_;
    std::vector<ParMult> recursive_derivatives_;
    std::vector<OpTree*> op_trees_;
    VMData vm_;
    Variable const* original_;
};

#endif

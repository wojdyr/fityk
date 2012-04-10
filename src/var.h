// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#ifndef FITYK__VAR__H__
#define FITYK__VAR__H__

#include "common.h"
#include "vm.h"

struct OpTree;
class Variable;
class Function;
class Sum;

class IndexedVars
{
public:
    IndexedVars() {}
    IndexedVars(const std::vector<std::string> &vars) : names_(vars) {}

    const std::vector<std::string>& names() const { return names_; }
    const std::vector<int>& indices() const { return indices_; }

    int get_count() const { return names_.size(); }
    const std::string& get_name(int n) const
                     { assert(is_index(n, names_)); return names_[n]; }

    int get_idx(int n) const
                     { assert(is_index(n, indices_)); return indices_[n]; }
    int get_max_idx() const;
    bool has_idx(int idx) const { return contains_element(indices_, idx); }
    bool depends_on(int idx, const std::vector<Variable*> &variables) const;
    std::string get_debug_idx_info() const;

    void set_name(int n, const std::string &new_p)
                         { assert(is_index(n, names_)); names_[n] = new_p; }
    void update_indices(const std::vector<Variable*>& variables);

private:
    // variable names
    std::vector<std::string> names_;
    // corresponding indices; set after initialization (in outer class)
    // and modified after variable removal or change
    std::vector<int> indices_;

    DISALLOW_COPY_AND_ASSIGN(IndexedVars);
};

/// the variable is either simple-variable and nr_ is the index in vector
/// of parameters, or it is "compound variable" and has nr_==-1.
/// third special case: nr_==-2 - it is mirror-variable (such variable
///        is not recalculated but copied)
/// In the second case, the value and derivatives are calculated:
/// -  string is parsed by eparser to VMData representation,
///    and then it is transformed to AST (struct OpTree), calculating derivates
///    at the same time (calculate_deriv()).
/// -  set_var_idx() finds positions of variables in variables vector
///    (references to variables are kept using names) and creates bytecode
///    (VMData, again) that will be used to calculate value and derivatives.
/// -  recalculate() calculates (using run_code_for_variable()) value
///    and derivatives for current parameter value
class Variable
{
public:
    const std::string name;
    RealRange domain;

    struct ParMult { int p; realt mult; };
    Variable(const std::string &name_, int nr);
    Variable(const std::string &name_, const std::vector<std::string> &vars,
             const std::vector<OpTree*> &op_trees);
    ~Variable();
    void recalculate(const std::vector<Variable*> &variables,
                     const std::vector<realt> &parameters);

    int get_nr() const { return nr_; };
    void erased_parameter(int k);
    realt get_value() const { return value_; };
    std::string get_formula(const std::vector<realt> &parameters) const;
    bool is_visible() const { return true; } //for future use
    void set_var_idx(const std::vector<Variable*> &variables);
    const std::vector<ParMult>& recursive_derivatives() const
                                            { return recursive_derivatives_; }
    bool is_simple() const { return nr_ != -1; }
    bool is_constant() const;

    std::vector<OpTree*> const& get_op_trees() const { return op_trees_; }
    void set_original(const Variable* orig) { assert(nr_==-2); original_=orig; }
    realt get_derivative(int n) const { return derivatives_[n]; }
    const IndexedVars& used_vars() const { return used_vars_; }

private:
    int nr_; /// see description of this class in .h
    realt value_;
    IndexedVars used_vars_;
    std::vector<realt> derivatives_;
    std::vector<ParMult> recursive_derivatives_;
    std::vector<OpTree*> op_trees_;
    VMData vm_;
    Variable const* original_;
};

#endif

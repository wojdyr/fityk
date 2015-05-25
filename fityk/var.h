// This file is part of fityk program. Copyright 2001-2013 Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#ifndef FITYK_VAR_H_
#define FITYK_VAR_H_

#include <assert.h>
#include "common.h"
#include "vm.h"

namespace fityk {
struct OpTree;
class Variable;

class FITYK_API IndexedVars
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

/// the variable can be one of:
/// * simple-variable; gpos_ is its index in the global array of parameters,
/// * compound-variable; gpos_ == -1,
/// * mirror-variable; gpos_ == -2 (such variable is copied, not recalculated)
///
/// The value and derivatives of compound-variable are calculated in this way:
/// -  string is parsed by eparser to VMData representation,
///    and then it is transformed to AST (struct OpTree), calculating derivates
///    at the same time (calculate_deriv()).
/// -  set_var_idx() finds positions of variables in variables vector
///    (references to variables are kept using names) and creates bytecode
///    (VMData, again) that will be used to calculate value and derivatives.
/// -  recalculate() calculates (using run_code_for_variable()) value
///    and derivatives for current parameter value

class FITYK_API Variable : public Var
{
public:
    struct ParMult { int p; realt mult; };
    Variable(const std::string &name_, int gpos);
    Variable(const std::string &name_, const std::vector<std::string> &vars,
             const std::vector<OpTree*> &op_trees);
    ~Variable();
    bool is_constant() const;
    std::string get_formula(const std::vector<realt> &parameters) const;
    void recalculate(const std::vector<Variable*> &variables,
                     const std::vector<realt> &parameters);

    void erased_parameter(int k);
    bool is_visible() const { return true; } //for future use
    void set_var_idx(const std::vector<Variable*> &variables);
    const std::vector<ParMult>& recursive_derivatives() const
                                            { return recursive_derivatives_; }
    std::vector<OpTree*> const& get_op_trees() const { return op_trees_; }
    void set_original(const Variable* orig)
                                 { assert(gpos_ == -2); original_ = orig; }
    realt get_derivative(int n) const { return derivatives_[n]; }
    const IndexedVars& used_vars() const { return used_vars_; }

private:
    IndexedVars used_vars_;
    std::vector<realt> derivatives_;
    std::vector<ParMult> recursive_derivatives_;
    std::vector<OpTree*> op_trees_;
    VMData vm_;
    Variable const* original_;
};

} // namespace fityk
#endif // FITYK_VAR_H_

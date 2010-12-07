// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
// $Id$

#ifndef FITYK__CALC__H__
#define FITYK__CALC__H__

#include "common.h"

struct OpTree;
class Variable;


class AnyFormula
{
public:
    AnyFormula(fp &value, std::vector<fp>& derivatives)
        : value_(value), derivatives_(derivatives) {}
    AnyFormula(std::vector<OpTree*> const &op_trees,
               fp &value, std::vector<fp>& derivatives)
        : value_(value), derivatives_(derivatives), op_trees_(op_trees) {}
    /// (re-)create bytecode, required after ::set_var_idx()
    void tree_to_bytecode(std::vector<int> const& var_idx);
    void run_vm(std::vector<Variable*> const &variables) const;
    std::vector<OpTree*> const& get_op_trees() const { return op_trees_; }
    /// check for the simplest case, just constant number
    bool is_constant() const;

protected:
    // these are recalculated every time parameters or variables are changed
    mutable fp &value_;
    mutable std::vector<fp> &derivatives_;

    std::vector<OpTree*> op_trees_;
    std::vector<int> vmcode; //OP_PUT_DERIV, OP_PUT_VAL, OP_VAR, OP_SIN, etc.
    std::vector<fp> vmdata;

    void exec_vm_op_action(std::vector<int>::const_iterator &i,
                           std::vector<double>::iterator &stackPtr) const;
};

class AnyFormulaO : public AnyFormula
{
public:
    AnyFormulaO(std::vector<OpTree*> const &op_trees_,
                fp &value, std::vector<fp>& derivatives)
        : AnyFormula(op_trees_, value, derivatives) {}
    void tree_to_bytecode(size_t var_idx_size);
    void prepare_optimized_codes(std::vector<fp> const& vv);
    fp run_vm_val(fp x) const;
    void run_vm_der(fp x) const;
    std::string get_vmcode_info() const;
private:
    int vmdata_size;
    std::vector<int> vmcode_val;
    std::vector<int> vmcode_der;
    void run_vm(); //disable
};

#endif

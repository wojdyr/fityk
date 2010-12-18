// This file is part of fityk program. Copyright (C) 2005 Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
// $Id$

#include "calc.h"
#include "ast.h"
#include "var.h"
#include "numfuncs.h"
#include "voigt.h"

#include <boost/math/special_functions/gamma.hpp>
#include <boost/math/special_functions/digamma.hpp>

using namespace std;

namespace {

const int stack_size = 8192;  //should be enough,
                              //there are no checks for stack overflow
vector<double> stack(stack_size);


void add_calc_bytecode(const OpTree* tree, const vector<int> &vmvar_idx,
                       VMData& vm)
{
    int op = tree->op;
    if (op < 0) {
        size_t n = -op-1;
        if (n == vmvar_idx.size()) {
            vm.append_code(OP_X);
        }
        else {
            assert(is_index(n, vmvar_idx));
            int var_idx = vmvar_idx[n];
            vm.append_code(OP_SYMBOL);
            vm.append_code(var_idx);
        }
    }
    else if (op == 0) {
        vm.append_number(tree->val);
    }
    else if (op >= OP_ONE_ARG && op < OP_TWO_ARG) { //one argument
        add_calc_bytecode(tree->c1, vmvar_idx, vm);
        vm.append_code(op);
    }
    else if (op >= OP_TWO_ARG) { //two arguments
        add_calc_bytecode(tree->c1, vmvar_idx, vm);
        add_calc_bytecode(tree->c2, vmvar_idx, vm);
        vm.append_code(op);
    }
}

} //anonymous namespace

////////////////////////////////////////////////////////////////////////////

void AnyFormula::exec_vm_op_action(const vector<double>& numbers,
                                   vector<int>::const_iterator &i,
                                   vector<double>::iterator &stackPtr) const
{
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
            case OP_ERFC:
                *stackPtr = erfc(*stackPtr);
                break;
            case OP_ERF:
                *stackPtr = erf(*stackPtr);
                break;
            case OP_LOG10:
                *stackPtr = log10(*stackPtr);
                break;
            case OP_LN:
                *stackPtr = log(*stackPtr);
                break;
            case OP_SINH:
                *stackPtr = sinh(*stackPtr);
                break;
            case OP_COSH:
                *stackPtr = cosh(*stackPtr);
                break;
            case OP_TANH:
                *stackPtr = tanh(*stackPtr);
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
            case OP_LGAMMA:
                *stackPtr = boost::math::lgamma(*stackPtr);
                break;
            case OP_DIGAMMA:
                *stackPtr = boost::math::digamma(*stackPtr);
                break;
            case OP_ABS:
                *stackPtr = fabs(*stackPtr);
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
            case OP_VOIGT:
                stackPtr--;
                *stackPtr = humlik(*stackPtr, *(stackPtr+1)) / sqrt(M_PI);
                break;
            case OP_DVOIGT_DX:
                stackPtr--;
                *stackPtr = humdev_dkdx(*stackPtr, *(stackPtr+1)) / sqrt(M_PI);
                break;
            case OP_DVOIGT_DY:
                stackPtr--;
                *stackPtr = humdev_dkdy(*stackPtr, *(stackPtr+1)) / sqrt(M_PI);
                break;

            // putting-number-to-stack-operators
            // stack overflow not checked
            case OP_NUMBER:
                stackPtr++;
                i++; // OP_NUMBER opcode is always followed by index
                *stackPtr = numbers[*i];
                break;

            //assignment-operators
            case OP_PUT_VAL:
                value_ = *stackPtr;
                stackPtr--;
                break;
            case OP_PUT_DERIV:
                i++;
                // the OP_PUT_DERIV opcode is followed by a number n,
                // the derivative is calculated with respect to n'th variable
                assert(*i < (int) derivatives_.size());
                derivatives_[*i] = *stackPtr;
                stackPtr--;
                break;

            default:
                //printf("op:%d\n", *i);
                assert(0);
        }
}

/// executes VM code, what sets this->value_ and this->derivatives_
void AnyFormula::run_vm(vector<Variable*> const &variables) const
{
    vector<double>::iterator stackPtr = stack.begin() - 1;//will be ++'ed first
    v_foreach (int, i, vm_.code()) {
        if (*i == OP_SYMBOL) {
            ++stackPtr;
            ++i; // skip the next one
            *stackPtr = variables[*i]->get_value();
        }
        else
            exec_vm_op_action(vm_.numbers(), i, stackPtr);
    }
    assert(stackPtr == stack.begin() - 1);
}

void AnyFormula::tree_to_bytecode(vector<int> const& var_idx)
{
    //assert(var_idx.size() + 1 == op_trees_.size());
    // it's +2, if also dy/dx is in op_trees_
    vm_.clear_data();
    add_calc_bytecode(op_trees_.back(), var_idx, vm_);
    vm_.append_code(OP_PUT_VAL);
    int n = op_trees_.size() - 1;
    for (int i = 0; i < n; ++i) {
        add_calc_bytecode(op_trees_[i], var_idx, vm_);
        vm_.append_code(OP_PUT_DERIV);
        vm_.append_code(i);
    }
    //printf("tree_to_bytecode: %s\n", vm2str(vm_).c_str());
}

bool AnyFormula::is_constant() const
{
    return op_trees_.back()->op == 0;
}

////////////////////////////////////////////////////////////////////////////

void AnyFormulaO::treex_to_bytecode(size_t var_idx_size)
{
    assert(var_idx_size + 2 == op_trees_.size());
    // we put function's parameter index rather than variable index after
    //  OP_SYMBOL, it is handled in this way in prepare_optimized_codes()
    AnyFormula::tree_to_bytecode(range_vector(0, var_idx_size));
}

void AnyFormulaO::prepare_optimized_codes(const vector<fp>& vv)
{
    vm_der_ = vm_;
    vm_der_.replace_symbols(vv);

    // find OP_PUT_VAL
    vector<int>::const_iterator value_it = vm_der_.code().begin();
    v_foreach (int, i, vm_der_.code()) {
        if (*i == OP_PUT_VAL) {
            value_it = i;
            break;
        }
        else if (*i == OP_NUMBER || *i == OP_PUT_DERIV)
            ++i;
    }

    vmcode_val_ = vector<int>(vm_der_.code().begin(), value_it);
}

void AnyFormulaO::run_vm_der(fp x) const
{
    vector<double>::iterator stackPtr = stack.begin() - 1;//will be ++'ed first
    v_foreach (int, i, vm_der_.code()) {
        if (*i == OP_X) {
            stackPtr++;
            *stackPtr = x;
        }
        else
            exec_vm_op_action(vm_der_.numbers(), i, stackPtr);
    }
    assert(stackPtr == stack.begin() - 1);
}

fp AnyFormulaO::run_vm_val(fp x) const
{
    vector<double>::iterator stackPtr = stack.begin() - 1;//will be ++'ed first
    v_foreach (int, i, vmcode_val_) {
        if (*i == OP_X) {
            stackPtr++;
            *stackPtr = x;
        }
        else
            exec_vm_op_action(vm_der_.numbers(), i, stackPtr);
    }
    return *stackPtr;
}


string AnyFormulaO::get_vmcode_info() const
{
    return "not optimized code: " + vm2str(vm_)
        + "\noptimized code: " + vm2str(vm_der_)
        + "\nonly-value code: " + vm2str(vmcode_val_, vm_der_.numbers());
}



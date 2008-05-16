// This file is part of fityk program. Copyright (C) 2005 Marcin Wojdyr
// Licence: GNU General Public License version 2
// $Id$

#include "calc.h"
#include "ast.h"
#include "var.h"
#include "numfuncs.h"
#include "voigt.h"

using namespace std;

namespace {

const int stack_size = 8192;  //should be enough, 
                              //there are no checks for stack overflow  
vector<double> stack(stack_size);


void add_calc_bytecode(const OpTree* tree, const vector<int> &vmvar_idx,
                       vector<int> &vmcode, vector<fp> &vmdata)
{
    int op = tree->op;
    if (op < 0) {
        size_t ii = -op-1;
        if (ii == vmvar_idx.size()) {
            vmcode.push_back(OP_X);
        }
        else {
            int var_idx = vmvar_idx[ii];
            vmcode.push_back(OP_VARIABLE);
            vmcode.push_back(var_idx);
        }
    }
    else if (op == 0) {
        vmcode.push_back(OP_CONSTANT);
        vmcode.push_back(vmdata.size());
        vmdata.push_back(tree->val);
    }
    else if (op >= OP_ONE_ARG && op < OP_TWO_ARG) { //one argument
        add_calc_bytecode(tree->c1, vmvar_idx, vmcode, vmdata);
        vmcode.push_back(op);
    }
    else if (op >= OP_TWO_ARG) { //two arguments
        add_calc_bytecode(tree->c1, vmvar_idx, vmcode, vmdata);
        add_calc_bytecode(tree->c2, vmvar_idx, vmcode, vmdata);
        vmcode.push_back(op);
    }
}

/// debuging utility
#define OP_(x) \
    case OP_##x: return #x;
string ast_op(int op)
{
    switch (op) {
        OP_(CONSTANT)
        OP_(VARIABLE)  
        OP_(X)
        OP_(PUT_VAL)
        OP_(PUT_DERIV)
        OP_(NEG)
        OP_(EXP)
        OP_(ERFC)
        OP_(ERF)
        OP_(SINH)       
        OP_(COSH)       
        OP_(TANH)       
        OP_(SIN)
        OP_(COS)
        OP_(ATAN)
        OP_(TAN)
        OP_(ASIN)
        OP_(ACOS)
        OP_(LOG10)
        OP_(LN)
        OP_(SQRT)
        OP_(LGAMMA)
        OP_(DIGAMMA)
        OP_(VOIGT)
        OP_(DVOIGT_DX)
        OP_(DVOIGT_DY)
        OP_(POW)
        OP_(MUL)
        OP_(DIV)
        OP_(ADD)
        OP_(SUB)
        default: return S(op);
    }
};

string vmcode2str(vector<int> const& code, vector<fp> const& data)
{
    string s;
    for (vector<int>::const_iterator i = code.begin(); i != code.end(); ++i) {
        s += ast_op(*i);
        if (*i == OP_CONSTANT) {
            ++i;
            assert (*i >= 0 && *i < size(data));
            s += "[" + S(*i) + "](" + S(data[*i]) + ")";
        }
        else if (*i == OP_VARIABLE || *i == OP_PUT_DERIV)
            s += "[" + S(*++i) + "]";
        s += " ";
    }
    return s;

}

} //anonymous namespace

////////////////////////////////////////////////////////////////////////////

void AnyFormula::exec_vm_op_action(vector<int>::const_iterator &i,
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
                *stackPtr = lgammafn(*stackPtr); 
                break;
            case OP_DIGAMMA:
                *stackPtr = digamma(*stackPtr); 
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
            case OP_CONSTANT:
                stackPtr++;
                i++;
                *stackPtr = vmdata[*i];
                break;

            //assignment-operators
            case OP_PUT_VAL:
                value = *stackPtr;
                stackPtr--; 
                break;
            case OP_PUT_DERIV:
                i++;
                derivatives[*i] = *stackPtr;
                stackPtr--; 
                break;

            default:
                assert(0);
        }
}

/// executes VM code, what sets this->value and this->derivatives
void AnyFormula::run_vm(vector<Variable*> const &variables) const
{
    vector<double>::iterator stackPtr = stack.begin() - 1;//will be ++'ed first
    for (vector<int>::const_iterator i = vmcode.begin(); i!=vmcode.end(); i++) {
        if (*i == OP_VARIABLE) {
            ++stackPtr;
            ++i;
            *stackPtr = variables[*i]->get_value();
        }
        else
            exec_vm_op_action(i, stackPtr);
    }
    assert(stackPtr == stack.begin() - 1);
}

void AnyFormula::tree_to_bytecode(vector<int> const& var_idx)
{
    //assert(var_idx.size() + 1 == op_trees.size()); 
    // it's +2, if also dy/dx is in op_trees
    vmcode.clear();
    vmdata.clear();
    add_calc_bytecode(op_trees.back(), var_idx, vmcode, vmdata);
    vmcode.push_back(OP_PUT_VAL);
    int n = op_trees.size() - 1;
    for (int i = 0; i < n; ++i) {
        add_calc_bytecode(op_trees[i], var_idx, vmcode, vmdata);
        vmcode.push_back(OP_PUT_DERIV);
        vmcode.push_back(i);
    }
}

bool AnyFormula::is_constant() const 
{ 
    return op_trees.back()->op == 0; 
}

////////////////////////////////////////////////////////////////////////////

void AnyFormulaO::tree_to_bytecode(size_t var_idx_size) 
{
    assert(var_idx_size + 2 == op_trees.size()); 
    // we put function's parameter index rather than variable index after 
    //  OP_VARIABLE, it is handled in this way in prepare_optimized_codes()
    AnyFormula::tree_to_bytecode(range_vector(0, var_idx_size));
    vmdata_size = vmdata.size();
}

void AnyFormulaO::prepare_optimized_codes(vector<fp> const& vv)
{
    vmdata.resize(vmdata_size);
    vmcode_der = vmcode;
    vector<int>::iterator value_it = vmcode_der.begin();
    for (vector<int>::iterator i = vmcode_der.begin(); 
                                        i != vmcode_der.end(); ++i) {
        if (*i == OP_CONSTANT || *i == OP_PUT_DERIV) 
            ++i;
        else if (*i == OP_VARIABLE) {
            *i = OP_CONSTANT;
            ++i;
            fp value = vv[*i]; //see AnyFormulaO::tree_to_bytecode()
            int data_idx = -1;
            for (size_t j = 0; j != vmdata.size(); ++j)
                if (vmdata[j] == value) {
                    data_idx = j;
                }
            if (data_idx == -1) {
                data_idx = vmdata.size();
                vmdata.push_back(value);
            }
            else
                vmdata[data_idx] = value;
            *i = data_idx;
        }
        else if (*i == OP_PUT_VAL) {
            value_it = i;
        }
    }
    vmcode_val = vector<int>(vmcode_der.begin(), value_it);
}

void AnyFormulaO::run_vm_der(fp x) const
{
    vector<double>::iterator stackPtr = stack.begin() - 1;//will be ++'ed first
    for (vector<int>::const_iterator i = vmcode_der.begin(); 
                                             i != vmcode_der.end(); i++) {
        if (*i == OP_X) {
            stackPtr++;
            *stackPtr = x;
        }
        else
            exec_vm_op_action(i, stackPtr);
    }
    assert(stackPtr == stack.begin() - 1);
}

fp AnyFormulaO::run_vm_val(fp x) const
{
    vector<double>::iterator stackPtr = stack.begin() - 1;//will be ++'ed first
    for (vector<int>::const_iterator i = vmcode_val.begin(); 
                                                i != vmcode_val.end(); i++) {
        if (*i == OP_X) {
            stackPtr++;
            *stackPtr = x;
        }
        else
            exec_vm_op_action(i, stackPtr);
    }
    return *stackPtr;
}


string AnyFormulaO::get_vmcode_info() const
{
    return "not optimized code: " + vmcode2str(vmcode, vmdata)
        + "\n value code: " + vmcode2str(vmcode_val, vmdata)
        + "\n value+derivatives code: " + vmcode2str(vmcode_der, vmdata);
}



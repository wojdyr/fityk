// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#ifndef FITYK__CALC__H__
#define FITYK__CALC__H__

#include <boost/spirit/core.hpp>
#include <boost/spirit/tree/ast.hpp>
using namespace boost::spirit;

/// used for functions and variables
/// there is a different set of opcodes for data transformation
enum 
{
    OP_CONSTANT=0,
    OP_VARIABLE,  // 1
    OP_PUT_VAL,   // 2
    OP_PUT_DERIV, // 3
    OP_ONE_ARG,   // 4
    OP_NEG,       // 5
    OP_EXP,       // 6
    OP_SIN,       // 7
    OP_COS,       // 8
    OP_ATAN,      // 9
    OP_TAN,       //10
    OP_ASIN,      //11
    OP_ACOS,      //12
    OP_LOG10,     //13
    OP_LN,        //14
    OP_SQRT,      //15
    OP_TWO_ARG,   //16
    OP_POW,       //17
    OP_MUL,       //18
    OP_DIV,       //19
    OP_ADD,       //20
    OP_SUB,       //21
    OP_END        //22
};


/// Node in abstract syntax tree (AST)
struct OpTree
{
    int op;   /// op < 0: variable (n=-op-1)
              /// op == 0: constant
              /// op > 0: operator
    OpTree *c1, 
           *c2;
    fp val;

    explicit OpTree(fp v) : op(0), c1(0), c2(0), val(v) {}
    explicit OpTree(int n, const std::string &/*s*/) 
                            : op(-n-1), c1(0), c2(0), val(0.) {}
    explicit OpTree(int n, OpTree *arg1);
    explicit OpTree(int n, OpTree *arg1, OpTree *arg2);

    ~OpTree() { delete c1; delete c2; }
    std::string str(const std::vector<std::string> *vars=0); 
    std::string str_b(bool b=true, const std::vector<std::string> *vars=0) 
                            { return b ? "(" + str(vars) + ")" : str(vars); } 
    std::string ascii_tree(int width=64, int start=0, 
                           const std::vector<std::string> *vars=0);
    OpTree* copy() const;
    //void swap_args() { assert(c1 && c2); OpTree *t=c1; c1=c2; c2=t; }
    OpTree* remove_c1() { OpTree *t=c1; c1=0; return t; }
    OpTree* remove_c2() { OpTree *t=c2; c2=0; return t; }
    void change_op(int op_) { op=op_; }
    bool operator==(const OpTree &t) { 
        return op == t.op && val == t.val 
               && (c1 == t.c1 || (c1 && t.c1 && *c1 == *t.c1)) 
               && (c2 == t.c2 || (c2 && t.c2 && *c2 == *t.c2));
    }
};

std::vector<std::string> 
find_tokens_in_ptree(int tokenID, const tree_parse_info<> &info);

typedef tree_match<char const*>::const_tree_iterator const_tm_iter_t;
std::vector<OpTree*> calculate_deriv(const_tm_iter_t const &i,
                                     std::vector<std::string> const &vars);
void add_calc_bytecode(const OpTree* tree, const std::vector<int> &vmvar_idx,
                       std::vector<int> &vmcode, std::vector<fp> &vmdata);
fp get_constant_value(std::string const &s);

#endif


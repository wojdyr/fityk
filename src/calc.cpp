// This file is part of fityk program. Copyright (C) 2005 Marcin Wojdyr
// $Id$

//  Based on ast_calc example from Boost::Spirit by Daniel Nuffer

// this file can be compiled to stand-alone test program:
// $ g++ -I../3rdparty -DSTANDALONE_DF calc.cpp -o calc
// $ ./calc

//TODO:
// white characters in expression (and then check if "sin" != "s in")
// -(x*y) -> don't needs brackets
// simplify_terms()
// CSE in tree (or in VM code?)
// new op: SQR? DUP? STORE/WRITE?
// output VM code for tests

#define STANDALONE_DF


#include <boost/spirit/core.hpp>
#include <boost/spirit/tree/ast.hpp>

#include <iostream>
#include <sstream>
#include <stack>
#include <functional>
#include <string>
#include <vector>
#include <cassert>
#include <cstdlib>
#include <cmath>

#include "calc.h"

////////////////////////////////////////////////////////////////////////////
using namespace std;
using namespace boost::spirit;

typedef char const*         iterator_t;
typedef tree_match<iterator_t> parse_tree_match_t;
typedef parse_tree_match_t::tree_iterator iter_t;
typedef parse_tree_match_t::const_tree_iterator const_iter_t;

////////////////////////////////////////////////////////////////////////////
string join_strings(const vector<string>& v, const string& sep)
{
    if (v.empty())
        return "";
    string s = v[0];
    for (vector<string>::const_iterator i = v.begin() + 1; i != v.end(); ++i)
        s += sep + *i;
    return s;
}

/// for vector<T*> - delete object and erase pointer
template<typename T>
void purge_element(std::vector<T*> &vec, int n)
{
    assert(n >= 0 && n < size(vec));
    delete vec[n];
    vec.erase(vec.begin() + n);
}

/// delete all objects handled by pointers and clear vector
template<typename T>
void purge_all_elements(std::vector<T*> &vec)
{
    for (typename std::vector<T*>::iterator i=vec.begin(); i!=vec.end(); ++i) 
        delete *i;
    vec.clear();
}

/// S() converts to string
template <typename T>
inline std::string S(T k) {
    return static_cast<std::ostringstream&>(std::ostringstream() << k).str();
}

////////////////////////////////////////////////////////////////////////////

enum CompoundVarOperator
{
    OP_CONSTANT=0,
    OP_ONE_ARG=1,
    OP_NEG,   OP_EXP,   OP_SIN,   OP_COS,  OP_ATAN,  
    OP_TAN, OP_ASIN, OP_ACOS, OP_LOG10, OP_LN,  OP_SQRT,  
    OP_TWO_ARG,
    OP_POW, OP_MUL, OP_DIV, OP_ADD, OP_SUB   
};


struct OpTree
{
    int op;   // op < 0: variable (n=-op-1)
              // op == 0: constant
              // op > 0: operator
    OpTree *c1, 
           *c2;
    double val;
    string var_name;

    explicit OpTree(double v) : op(0), c1(0), c2(0), val(v) {}
    explicit OpTree(int n, string s) 
                            : op(-n-1), c1(0), c2(0), val(0.), var_name(s) {}
    explicit OpTree(int n, OpTree *arg1) : op(n), c1(arg1), c2(0), val(0.) 
                                  { assert(OP_ONE_ARG < n && n < OP_TWO_ARG); }
    explicit OpTree(int n, OpTree *arg1, OpTree *arg2) 
        : op(n), c1(arg1), c2(arg2), val(0.)   { assert(n > OP_TWO_ARG); }

    ~OpTree() { delete c1; delete c2; }
    string str(); 
    string str_b(bool b=true) { return b ? "(" + str() + ")" : str(); } 
    string ascii_tree(int width=64, int start=0);
    OpTree *copy();
    //void swap_args() { assert(c1 && c2); OpTree *t=c1; c1=c2; c2=t; }
    OpTree* remove_c1() { OpTree *t=c1; c1=0; return t; }
    OpTree* remove_c2() { OpTree *t=c2; c2=0; return t; }
    bool operator==(const OpTree &t) { 
        return op == t.op && val == t.val && var_name == t.var_name 
               && (c1 == t.c1 || (c1 && t.c1 && *c1 == *t.c1)) 
               && (c2 == t.c2 || (c2 && t.c2 && *c2 == *t.c2));
    }
};

string OpTree::str()
{
    if (op < 0)
        return var_name; //"var"+S(-op-1);
    switch (op) {
        case 0:       return S(val);
        case OP_NEG:  return "-" + c1->str_b(c1->op >= OP_POW);
        case OP_EXP:  return "exp(" + c1->str() + ")";
        case OP_SIN:  return "sin(" + c1->str() + ")";
        case OP_COS:  return "cos(" + c1->str() + ")";
        case OP_ATAN: return "atan("+ c1->str() + ")";
        case OP_TAN:  return "tan(" + c1->str() + ")";
        case OP_ASIN: return "asin("+ c1->str() + ")";
        case OP_ACOS: return "acos("+ c1->str() + ")";
        case OP_LOG10:return "log10("+c1->str() + ")";
        case OP_LN:   return "ln("  + c1->str() + ")";
        case OP_SQRT: return "sqrt("+ c1->str() + ")";
        case OP_POW:  return c1->str_b(c1->op >= OP_POW) 
                             + "^" + c2->str_b(c2->op >= OP_POW);
        case OP_ADD:  return c1->str() + "+" + c2->str();
        case OP_SUB:  return c1->str() + "-" + c2->str_b(c2->op >= OP_ADD);
        case OP_MUL:  return c1->str_b(c1->op >= OP_ADD) 
                             + "*" + c2->str_b(c2->op >= OP_ADD);
        case OP_DIV:  return c1->str_b(c1->op >= OP_ADD) 
                             + "/" + c2->str_b(c2->op >= OP_MUL);
        default: assert(0); return "";
    }
}

string OpTree::ascii_tree(int width, int start)
{
    string node = "???";
    if (op < 0)
        node = var_name;
    else
        switch (op) {
            case 0:       node = S(val); break;
            case OP_NEG:  node = "NEG";  break;
            case OP_EXP:  node = "EXP";  break;
            case OP_SIN:  node = "SIN";  break;
            case OP_COS:  node = "COS";  break;
            case OP_ATAN: node = "ATAN"; break;
            case OP_TAN:  node = "TAN";  break;
            case OP_ASIN: node = "ASIN"; break;
            case OP_ACOS: node = "ACOS"; break;
            case OP_LOG10:node = "LOG";  break;
            case OP_LN:   node = "LN";   break;
            case OP_SQRT: node = "SQRT"; break;
            case OP_POW:  node = "POW";  break;
            case OP_ADD:  node = "ADD";  break;
            case OP_SUB:  node = "SUB";  break;
            case OP_MUL:  node = "MUL";  break;
            case OP_DIV:  node = "DIV";  break;
        }
    int n = (int(node.size()) < width ? start + (width-node.size())/2
                                 : start);
    node = string(n, ' ') + node + "\n";
    if (c1)
        node += c1->ascii_tree(width/2, start);
    if (c2)
        node += c2->ascii_tree(width/2, start+width/2);
    return node;
}

OpTree* OpTree::copy()
{ 
    OpTree *t = new OpTree(*this); 
    if (c1) t->c1 = c1->copy();
    if (c2) t->c2 = c2->copy();
    return t;
}

////////////////////////////////////////////////////////////////////////////

void do_find_variables(const_iter_t const &i, vector<string> &vars)
{
    for (const_iter_t j = i->children.begin(); j != i->children.end(); ++j) {
        if (j->value.id() == FuncGrammar::variableID) {
            string v(j->value.begin(), j->value.end());
            if (find(vars.begin(), vars.end(), v) == vars.end())
                vars.push_back(v);
        }
        else
            do_find_variables(j, vars);
    }
}

vector<string> find_variables(tree_parse_info<> info)
{
    vector<string> vars;
    const_iter_t const &root = info.trees.begin();
    if (root->value.id() == FuncGrammar::variableID) //special case: "x"
        vars.push_back(string(root->value.begin(), root->value.end()));
    else
        do_find_variables(root, vars);
    return vars;
}

////////////////////////////////////////////////////////////////////////////
OpTree* do_add(OpTree *a, OpTree *b);
OpTree* do_sub(OpTree *a, OpTree *b);

void get_factors(OpTree *a, vector<OpTree*>& u, vector<OpTree*>& b)
{
    assert (a->op == OP_MUL || a->op == OP_DIV || a->op == OP_NEG);
    if (a->c1->op == OP_MUL || a->c1->op == OP_DIV || a->c1->op == OP_NEG) 
        get_factors(a->c1, u, b);
    else
        u.push_back(a->remove_c1());
    if (a->op == OP_MUL) {
        if (a->c2->op == OP_MUL || a->c2->op == OP_DIV || a->c2->op == OP_NEG) 
            get_factors(a->c2, u, b);
        else
            u.push_back(a->remove_c2());
    }
    else if (a->op == OP_DIV) { 
        if (a->c2->op == OP_MUL || a->c2->op == OP_DIV || a->c2->op == OP_NEG) 
            get_factors(a->c2, b, u);
        else
            b.push_back(a->remove_c2());
    }
    else if (a->op == OP_NEG) {
        u.push_back(new OpTree(-1.));
    }
}

OpTree* simplify_factors(OpTree *a)
{
    //cout << "before simplify_factors(): " << a->str() << endl;
    assert (a->op == OP_MUL || a->op == OP_DIV || a->op == OP_NEG);
    vector<OpTree*> u, b;
    //              \product_i u_i 
    //    tree ->   -------------- 
    //              \product_i b_i
    get_factors(a, u, b);
    delete a;
    // tan -> sin/cos
    for (vector<OpTree*>::iterator i = u.begin(); i != u.end(); ++i) 
        if ((*i)->op == OP_TAN) {
            (*i)->op = OP_SIN;
            b.push_back(new OpTree(OP_COS, (*i)->c1->copy()));
        }
    for (vector<OpTree*>::iterator i = b.begin(); i != b.end(); ++i) 
        if ((*i)->op == OP_TAN) {
            (*i)->op = OP_SIN;
            u.push_back(new OpTree(OP_COS, (*i)->c1->copy()));
        }
    // reduce (x / x)
    for (vector<OpTree*>::iterator i = u.begin(); i != u.end(); ++i) 
        for (vector<OpTree*>::iterator j = b.begin(); j != b.end(); ++j) 
            if (*i && *j && **j == **i) {
                *i = *j = 0;
                break;
            }
    // reduce (x^n / x^m) -> x^(m-n)
    for (vector<OpTree*>::iterator i = u.begin(); i != u.end(); ++i) {
        if (*i && (*i)->op == OP_POW) {
            for (vector<OpTree*>::iterator j = i+1; j != u.end(); ++j) 
                if (*j && (*j)->op == OP_POW && *(*j)->c1 == *(*i)->c1) {
                    (*i)->c2 = do_add((*i)->remove_c2(), (*j)->remove_c2());
                    delete *j;
                    *j = 0;
                }
                else if (*j && **j == *(*i)->c1) {
                    (*i)->c2 = do_add((*i)->remove_c2(), new OpTree(1.));
                    delete *j;
                    *j = 0;
                }
            for (vector<OpTree*>::iterator j = b.begin(); j != b.end(); ++j) 
                if (*j && (*j)->op == OP_POW && *(*j)->c1 == *(*i)->c1) {
                    (*i)->c2 = do_sub((*i)->remove_c2(), (*j)->remove_c2());
                    delete *j;
                    *j = 0;
                }
                else if (*j && **j == *(*i)->c1) {
                    (*i)->c2 = do_sub((*i)->remove_c2(), new OpTree(1.));
                    delete *j;
                    *j = 0;
                }
        }
    }
    for (vector<OpTree*>::iterator i = b.begin(); i != b.end(); ++i) {
        if (*i && (*i)->op == OP_POW) {
            for (vector<OpTree*>::iterator j = i+1; j != b.end(); ++j) {
                if (*j && (*j)->op == OP_POW && *(*j)->c1 == *(*i)->c1) {
                    (*i)->c2 = do_add((*i)->remove_c2(), (*j)->remove_c2());
                    delete *j;
                    *j = 0;
                }
                else if (*j && **j == *(*i)->c1) {
                    (*i)->c2 = do_add((*i)->remove_c2(), new OpTree(1.));
                    delete *j;
                    *j = 0;
                }
            }
        }
    }
    //sin/cos -> tan
    for (vector<OpTree*>::iterator i = u.begin(); i != u.end(); ++i) {
        if (*i == 0)
            continue;
        if ((*i)->op == OP_SIN)
            for (vector<OpTree*>::iterator j = b.begin(); j != b.end(); ++j) {
                if (*j && (*j)->op == OP_COS && *(*j)->c1 == *(*i)->c1) {
                    (*i)->op = OP_TAN;
                    *j = 0;
                    break;
                }
            }
        else if ((*i)->op == OP_COS)
            for (vector<OpTree*>::iterator j = b.begin(); j != b.end(); ++j) {
                if (*j && (*j)->op == OP_SIN && *(*j)->c1 == *(*i)->c1) {
                    (*j)->op = OP_TAN;
                    *i = 0;
                    break;
                }
            }
    }
    // multiply constants
    double val = 1.;
    for (vector<OpTree*>::iterator i = u.begin(); i != u.end(); ++i) {
        if (*i == 0)
            continue;
        else if ((*i)->op == 0) {
            val *= (*i)->val;
            delete *i;
            *i = 0;
        }
        else
            assert((*i)->op != OP_NEG);
    }
    for (vector<OpTree*>::iterator i = b.begin(); i != b.end(); ++i) {
        if (*i == 0)
            continue;
        if ((*i)->op == 0) {
            val /= (*i)->val;
            *i = 0;
        }
        else if ((*i)->op == OP_NEG)
            val = -val;
    }
    // -> tree
    OpTree *tu = 0;
    for (vector<OpTree*>::iterator i = u.begin(); i != u.end(); ++i) {
        if (*i == 0)
            continue;
        if (!tu)
            tu = *i;
        else
            tu = new OpTree(OP_MUL, tu, *i);
    }
    OpTree *tb = 0;
    for (vector<OpTree*>::iterator i = b.begin(); i != b.end(); ++i) {
        if (*i == 0)
            continue;
        if (!tb)
            tb = *i;
        else
            tb = new OpTree(OP_MUL, tb, *i);
    }
    if (tu && val != 1.) {
        tu = (val == -1. ? new OpTree(OP_NEG, tu)
                         : new OpTree(OP_MUL, new OpTree(val), tu));
    }
    else if (!tu)
        tu = new OpTree(val);
    OpTree *ret = tb ? new OpTree(OP_DIV, tu, tb) : tu;
    //cout << "after simplify_factors(): " << ret->str() << endl;
    return ret;
}

////////////////////////////////////////////////////////////////////////////

OpTree* simplify_terms(OpTree *a)
{
    //TODO x+3+x -> 2*x+3 ; 3*y+y -> 4y ; 
    //handle cases like (x*x*x+(x*x+2*x*x)*x)
    return a;
}

////////////////////////////////////////////////////////////////////////////
OpTree* do_multiply(OpTree *a, OpTree *b);

OpTree* do_change_sign(OpTree *a)
{
    if (a->op == 0) {
        double val = - a->val;
        delete a;
        return new OpTree(val);
    }
    else if (a->op == OP_NEG) {
        OpTree *t = a->c1->copy();
        delete a;
        return t;
    }
    else
        return new OpTree(OP_NEG, a);
}

OpTree* do_add(int op, OpTree *a, OpTree *b)
{
    if (a->op == 0 && a->val == 0.) {
        delete a;
        if (op == OP_ADD)
            return b;
        else
            return do_change_sign(b);
    }
    else if (b->op == 0 && b->val == 0.) {
        delete b;
        return a;
    }
    else if (a->op == 0 && b->op == 0) {
        double val = (op == OP_ADD ? a->val + b->val : a->val - b->val);
        delete a;
        delete b;
        return new OpTree(val);
    }
    else if (b->op == OP_NEG) {
        OpTree *t = b->remove_c1();
        delete b;
        return do_add(op == OP_ADD ? OP_SUB : OP_ADD, a, t);
    }
    else if ((b->op == OP_MUL || b->op == OP_DIV) 
             && b->c1->op == 0  && b->c1->val < 0) {
        b->c1->val = - b->c1->val;
        return do_add(op == OP_ADD ? OP_SUB : OP_ADD, a, b);
    }
    else if (*a == *b) {
        delete b;
        if (op == OP_ADD)
            return do_multiply(new OpTree(2.), a);
        else {
            delete a;
            return new OpTree(0.);
        }
    }
    else
        return simplify_terms(new OpTree(op, a, b));
}

OpTree* do_add(OpTree *a, OpTree *b)
{
    return do_add(OP_ADD, a, b);
}

OpTree* do_sub(OpTree *a, OpTree *b)
{
    return do_add(OP_SUB, a, b);
}

OpTree* do_multiply(OpTree *a, OpTree *b)
{
    if ((a->op == 0 && b->op == 0)
        || (a->op == 0 && a->val == 0.)
        || (b->op == 0 && b->val == 0.))
    {
        double val = a->val * b->val;
        delete a;
        delete b;
        return new OpTree(val);
    }
    else if (a->op == 0 && a->val == 1.) {
        delete a;
        return b;
    }
    else if (b->op == 0 && b->val == 1.) {
        delete b;
        return a;
    }
    else {
        //return new OpTree(OP_MUL, a, b);
        return simplify_factors(new OpTree(OP_MUL, a, b));
    }
}

OpTree* do_divide(OpTree *a, OpTree *b)
{
    if (a->op == 0 && a->val == 0.) {
        delete a;
        delete b;
        return new OpTree(0.);
    }
    else if ((a->op == 0 && b->op == 0) || (b->op == 0 && b->val == 0.))
    {
        double val = a->val / b->val;
        delete a;
        delete b;
        return new OpTree(val);
    }
    else if (b->op == 0 && b->val == 1.) {
        delete b;
        return a;
    }
    else {
        //return new OpTree(OP_DIV, a, b);
        return simplify_factors(new OpTree(OP_DIV, a, b));
    }
}

OpTree *do_sqr(OpTree *a)
{
    return do_multiply(a, a->copy());
    //return new OpTree(OP_MUL, a, a->copy());
}

OpTree *do_oneover(OpTree *a)
{
    return do_divide(new OpTree(1.), a);
}


OpTree* do_exp(OpTree *a)
{
    if (a->op == 0) {
        double val = exp(a->val);
        delete a;
        return new OpTree(val);
    }
    else
        return new OpTree(OP_EXP, a);
}

OpTree* do_sqrt(OpTree *a)
{
    if (a->op == 0) {
        double val = sqrt(a->val);
        delete a;
        return new OpTree(val);
    }
    else
        return new OpTree(OP_SQRT, a);
}

OpTree* do_log10(OpTree *a)
{
    if (a->op == 0) {
        double val = log10(a->val);
        delete a;
        return new OpTree(val);
    }
    else
        return new OpTree(OP_LOG10, a);
}

OpTree* do_ln(OpTree *a)
{
    if (a->op == 0) {
        double val = log(a->val);
        delete a;
        return new OpTree(val);
    }
    else
        return new OpTree(OP_LN, a);
}

OpTree* do_sin(OpTree *a)
{
    if (a->op == 0) {
        double val = sin(a->val);
        delete a;
        return new OpTree(val);
    }
    else
        return new OpTree(OP_SIN, a);
}

OpTree* do_cos(OpTree *a)
{
    if (a->op == 0) {
        double val = cos(a->val);
        delete a;
        return new OpTree(val);
    }
    else
        return new OpTree(OP_COS, a);
}

OpTree* do_tan(OpTree *a)
{
    if (a->op == 0) {
        double val = tan(a->val);
        delete a;
        return new OpTree(val);
    }
    else
        return new OpTree(OP_TAN, a);
}

OpTree* do_atan(OpTree *a)
{
    if (a->op == 0) {
        double val = atan(a->val);
        delete a;
        return new OpTree(val);
    }
    else
        return new OpTree(OP_ATAN, a);
}

OpTree* do_asin(OpTree *a)
{
    if (a->op == 0) {
        double val = asin(a->val);
        delete a;
        return new OpTree(val);
    }
    else
        return new OpTree(OP_ASIN, a);
}

OpTree* do_acos(OpTree *a)
{
    if (a->op == 0) {
        double val = acos(a->val);
        delete a;
        return new OpTree(val);
    }
    else
        return new OpTree(OP_ACOS, a);
}

OpTree* do_pow(OpTree *a, OpTree *b)
{
    if (a->op == 0 && a->val == 0.) {
        delete a;
        delete b;
        return new OpTree(0.);
    }
    if ((b->op == 0 && b->val == 0.) 
        || (a->op == 0 && a->val == 1.)) {
        delete a;
        delete b;
        return new OpTree(1.);
    }
    else if (b->op == 0 && b->val == 1.) {
        delete b;
        return a;
    }
    if (a->op == 0 && b->op == 0) {
        double val = pow(a->val, b->val);
        delete a;
        delete b;
        return new OpTree(val);
    }
    else {
        return new OpTree(OP_POW, a, b);
    }
}

////////////////////////////////////////////////////////////////////////////

vector<OpTree*> calculate_deriv(const_iter_t const &i,
                                vector<string> const &vars)
{
    int len = vars.size();
    vector<OpTree*> results(len + 1);
    string s(i->value.begin(), i->value.end());

    if (i->value.id() == FuncGrammar::real_constID)
    {
        for (int k = 0; k < len; ++k)
            results[k] = new OpTree(0.); 
        double v = strtod(s.c_str(), 0);
        results[len] = new OpTree(v);
    }

    else if (i->value.id() == FuncGrammar::variableID)
    {
        for (int k = 0; k < len; ++k)
            if (s == vars[k]) {
                results[k] = new OpTree(1.); 
                results[len] = new OpTree(k, s);
            }
            else
                results[k] = new OpTree(0.); 
    }

    else if (i->value.id() == FuncGrammar::exptokenID)
    {
        vector<OpTree*> arg = calculate_deriv(i->children.begin(), vars);
        if (s == "-")
            for (int k = 0; k < len+1; ++k) 
                results[k] = do_change_sign(arg[k]);
        else {
            OpTree* (* do_op)(OpTree *) = 0;
            OpTree* der = 0;
            OpTree* larg = arg.back()->copy();
            if (s == "sqrt") {
                der = do_divide(new OpTree(0.5), do_sqrt(larg));
                do_op = do_sqrt;
            }
            else if (s == "exp") {
                der = do_exp(larg);
                do_op = do_exp;
            }
            else if (s == "log10") {
                OpTree *ln_10 = do_ln(new OpTree(10.));
                der = do_oneover(do_multiply(larg, ln_10));
                do_op = do_log10;
            }
            else if (s == "ln") {
                der = do_oneover(larg);
                do_op = do_ln;
            }
            else if (s == "sin") {
                der = do_cos(larg);
                do_op = do_sin;
            }
            else if (s == "cos") {
                der = do_change_sign(do_sin(larg));
                do_op = do_cos;
            }
            else if (s == "tan") {
                der = do_oneover(do_sqr(do_cos(larg)));
                do_op = do_tan;
            }
            else if (s == "atan") {
                der = do_oneover(do_add(new OpTree(1.), do_sqr(larg)));
                do_op = do_atan;
            }
            else if (s == "asin") {
                OpTree *root_arg = do_sub(new OpTree(1.), do_sqr(larg));
                der = do_oneover(do_sqrt(root_arg));
                do_op = do_asin;
            }
            else if (s == "acos") {
                OpTree *root_arg = do_sub(new OpTree(1.), do_sqr(larg));
                der = do_divide(new OpTree(-1.), do_sqrt(root_arg));
                do_op = do_acos;
            }
            else
                assert(0);
            for (int k = 0; k < len; ++k) 
                results[k] = do_multiply(der->copy(), arg[k]);
            delete der;
            results[len] = (*do_op)(arg[len]);
        }
    }

    else if (i->value.id() == FuncGrammar::factorID)
    {
        assert(i->children.size() == 2);
        assert(s == "^");
        vector<OpTree*> left = calculate_deriv(i->children.begin(), vars);
        vector<OpTree*> right = calculate_deriv(i->children.begin() + 1, vars);
        for (int k = 0; k < len; ++k) {
            OpTree *a = left[len],
                   *b = right[len],
                   *ap = left[k],
                   *bp = right[k];
            //        b(x)            b(x)  b(x) a'(x)
            //    (a(x)   )'    = a(x)     (---------- + ln(a(x)) b'(x))
            //                                 a(x)
            OpTree *pow_a_b = do_pow(a->copy(), b->copy());
            OpTree *term1 = do_divide(do_multiply(b->copy(), ap), a->copy());
            OpTree *term2 = do_multiply(do_ln(a->copy()), bp);
            results[k] = do_multiply(pow_a_b, do_add(term1, term2));
        }
        results[len] = do_pow(left[len], right[len]);
    }

    else if (i->value.id() == FuncGrammar::termID)
    {
        assert(i->children.size() == 2);
        assert(s == "*" || s == "/");
        int op = (s == "*" ? OP_MUL : OP_DIV);
        vector<OpTree*> left = calculate_deriv(i->children.begin(), vars);
        vector<OpTree*> right = calculate_deriv(i->children.begin() + 1, vars);
        for (int k = 0; k < len; ++k) {
            OpTree *a = left[len],
                   *b = right[len],
                   *ap = left[k],
                   *bp = right[k];
            if (op == OP_MUL) { // a*b' + a'*b
                results[k] = do_add(do_multiply(a->copy(), bp),
                                    do_multiply(ap,  b->copy()));
            }
            else { //OP_DIV    (a'*b - b'*a) / (b*b)
                OpTree *upper = do_sub(do_multiply(ap, b->copy()),
                                       do_multiply(bp, a->copy()));
                results[k] = do_divide(upper, do_sqr(b->copy()));
            }
        }
        results[len] = (op == OP_MUL ? do_multiply(left[len], right[len])
                                     : do_divide(left[len], right[len]));
    }

    else if (i->value.id() == FuncGrammar::expressionID)
    {
        assert(i->children.size() == 2);
        assert(s == "+" || s == "-");
        vector<OpTree*> left = calculate_deriv(i->children.begin(), vars);
        vector<OpTree*> right = calculate_deriv(i->children.begin() + 1, vars);
        for (int k = 0; k < len+1; ++k) 
            results[k] = (s == "+" ? do_add(left[k], right[k]) 
                                   : do_sub(left[k], right[k]));
    }

    else
        assert(0); // error

    return results;
}


////////////////////////////////////////////////////////////////////////////

#ifdef STANDALONE_DF
int
main()
{
    FuncGrammar gram;

    cout << "Usage example: f = 5*x^3 + exp(x*y) - y*tan(z)" << endl;
    cout << "f = ";

    string str;
    while (getline(cin, str))
    {
        tree_parse_info<> info = ast_parse(str.c_str(), gram);

        if (info.full) {
            cout << "parsing succeeded\n";
            vector<string> vars = find_variables(info);
            vector<OpTree*> results = calculate_deriv(info.trees.begin(), vars);
            assert(results.size() == vars.size() + 1);
            cout << "f("<< join_strings(vars, ", ") << ") = " 
                                              << results.back()->str()  << endl;
            // cout << results.front()->ascii_tree() << endl;
            for (unsigned int i = 0; i < vars.size(); ++i)
                cout << "df/d" << vars[i] << " = " << results[i]->str() << endl;
            purge_all_elements(results);
        }
        else
            cout << "parsing failed\n";
        cout << "\nf = ";
    }
    cout << endl;
    return 0;
}

#endif //STANDALONE_DF


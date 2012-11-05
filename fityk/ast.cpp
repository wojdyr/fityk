// This file is part of fityk program. Copyright (C) 2005 Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#define BUILDING_LIBFITYK
#include "ast.h"

#include <boost/math/special_functions/gamma.hpp>
#include <boost/math/special_functions/digamma.hpp>

#include <string>
#include <vector>
#include <cassert>
#include <cstdlib>
#include <cmath>

#include "common.h"
#include "ui.h"
#include "var.h"
#include "logic.h"
#include "numfuncs.h"
#include "voigt.h"
#include "lexer.h"
#include "eparser.h"

using namespace std;

namespace fityk {

string OpTree::str(const OpTreeFormat& fmt)
{
    if (op < 0) {
        int v_nr = -op-1;
        return fmt.vars == NULL || fmt.vars->empty() ? "var"+S(v_nr)
                                                     : (*fmt.vars)[v_nr];
    }
    switch (op) {
        case OP_NUMBER: return format1<double, 32>(fmt.num_format, val);
        case OP_NEG:  return "-" + c1->str_b(c1->op >= OP_POW, fmt);
        case OP_EXP:  return "exp(" + c1->str(fmt) + ")";
        case OP_ERFC: return "erfc(" + c1->str(fmt) + ")";
        case OP_ERF:  return "erf(" + c1->str(fmt) + ")";
        case OP_SINH:  return "sinh(" + c1->str(fmt) + ")";
        case OP_COSH:  return "cosh(" + c1->str(fmt) + ")";
        case OP_TANH: return "tanh("+ c1->str(fmt) + ")";
        case OP_SIN:  return "sin(" + c1->str(fmt) + ")";
        case OP_COS:  return "cos(" + c1->str(fmt) + ")";
        case OP_TAN:  return "tan(" + c1->str(fmt) + ")";
        case OP_ASIN: return "asin("+ c1->str(fmt) + ")";
        case OP_ACOS: return "acos("+ c1->str(fmt) + ")";
        case OP_ATAN: return "atan("+ c1->str(fmt) + ")";
        case OP_LGAMMA: return "lgamma("+ c1->str(fmt) + ")";
        case OP_DIGAMMA: return "digamma("+ c1->str(fmt) + ")";
        case OP_ABS: return "abs(" + c1->str(fmt) + ")";
        case OP_LOG10:return "log10("+c1->str(fmt) + ")";
        case OP_LN:   return "ln("  + c1->str(fmt) + ")";
        case OP_SQRT: return "sqrt("+ c1->str(fmt) + ")";
        case OP_VOIGT: return "voigt("+ c1->str(fmt) +","+ c2->str(fmt) +")";
        case OP_DVOIGT_DX: return "dvoigt_dx("+ c1->str(fmt)
                                                   + "," + c2->str(fmt) + ")";
        case OP_DVOIGT_DY: return "dvoigt_dy("+ c1->str(fmt)
                                                   + "," + c2->str(fmt) + ")";
        case OP_POW:  return c1->str_b(c1->op >= OP_POW, fmt)
                             + "^" + c2->str_b(c2->op >= OP_POW, fmt);
        case OP_ADD:  return c1->str(fmt) + "+" + c2->str(fmt);
        case OP_SUB:  return c1->str(fmt) + "-"
                                         + c2->str_b(c2->op >= OP_ADD, fmt);
        case OP_MUL:  return c1->str_b(c1->op >= OP_ADD, fmt)
                             + "*" + c2->str_b(c2->op >= OP_ADD, fmt);
        case OP_DIV:  return c1->str_b(c1->op >= OP_ADD, fmt)
                             + "/" + c2->str_b(c2->op >= OP_MUL, fmt);
        default: assert(0); return "";
    }
}


OpTree* OpTree::clone() const
{
    OpTree *t = new OpTree(*this);
    if (c1) t->c1 = c1->clone();
    if (c2) t->c2 = c2->clone();
    return t;
}

////////////////////////////////////////////////////////////////////////////

OpTree* simplify_terms(OpTree *a);
OpTree* do_multiply(OpTree *a, OpTree *b);

OpTree* do_neg(OpTree *a)
{
    if (a->op == 0) {
        realt val = - a->val;
        delete a;
        return new OpTree(val);
    }
    else if (a->op == OP_NEG) {
        OpTree *t = a->c1->clone();
        delete a;
        return t;
    }
    else
        return new OpTree(OP_NEG, a);
}

OpTree* do_add(int op, OpTree *a, OpTree *b)
{
    if (a->op == 0 && b->op == 0) { // p + q
        realt val = (op == OP_ADD ? a->val + b->val : a->val - b->val);
        delete a;
        delete b;
        return new OpTree(val);
    }
    else if (a->op == 0 && is_eq(a->val, 0.)) { // 0 + t
        delete a;
        if (op == OP_ADD)
            return b;
        else
            return do_neg(b);
    }
    else if (b->op == 0 && is_eq(b->val, 0.)) { // t + 0
        delete b;
        return a;
    }
    else if (b->op == OP_NEG) { // t + -u
        OpTree *t = b->remove_c1();
        delete b;
        return do_add(op == OP_ADD ? OP_SUB : OP_ADD, a, t);
    }
    else if ((b->op == OP_MUL || b->op == OP_DIV)
             && b->c1->op == 0  && b->c1->val < 0) { // t + -p*v
        b->c1->val = - b->c1->val;
        return do_add(op == OP_ADD ? OP_SUB : OP_ADD, a, b);
    }
    else if (*a == *b) {
        delete b;
        if (op == OP_ADD) // t + t
            return do_multiply(new OpTree(2.), a);
        else { // t - t
            delete a;
            return new OpTree(0.);
        }
    }
    else
        return new OpTree(op, a, b);
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
    if (a->op == 0 && b->op == 0)  // const * const
    {
        realt val = a->val * b->val;
        delete a;
        delete b;
        return new OpTree(val);
    }
    else if ((a->op == 0 && is_eq(a->val, 0.))   // 0 * ...
             || (b->op == 0 && is_eq(b->val, 0.))) // ... * 0
    {
        delete a;
        delete b;
        return new OpTree(0.);
    }
    else if (a->op == 0 && is_eq(a->val, 1.)) {  // 1 * ...
        delete a;
        return b;
    }
    else if (b->op == 0 && is_eq(b->val, 1.)) {  // ... * 1
        delete b;
        return a;
    }
    else if (a->op == 0 && is_eq(a->val, -1.)) { // -1 * ...
        delete a;
        return do_neg(b);
    }
    else if (b->op == 0 && is_eq(b->val, -1.)) { // ... * -1
        delete b;
        return do_neg(a);
    }
    // const1 * (const2 / ...)
    else if (a->op == 0 && b->op == OP_DIV && b->c1->op == 0) {
        b->c1->val *= a->val;
        delete a;
        return b;
    }
    else {
        return new OpTree(OP_MUL, a, b);
    }
}

OpTree* do_divide(OpTree *a, OpTree *b)
{
    //no check for division by zero
    if (a->op == 0 && b->op == 0)
    {
        realt val = a->val / b->val;
        delete a;
        delete b;
        return new OpTree(val);
    }
    else if (a->op == 0 && is_eq(a->val, 0.)) {
        delete a;
        delete b;
        return new OpTree(0.);
    }
    else if (b->op == 0 && is_eq(b->val, 1.)) {
        delete b;
        return a;
    }
    else {
        return new OpTree(OP_DIV, a, b);
    }
}

OpTree *do_sqr(OpTree *a)
{
    return do_multiply(a, a->clone());
    //return new OpTree(OP_MUL, a, a->clone());
}

OpTree *do_oneover(OpTree *a)
{
    return do_divide(new OpTree(1.), a);
}

/*
template<typename T>
OpTree* one_arg_func(OpTree *a, T func, int op)
{
    if (a->op == 0) {
        realt val = func(a->val);
        delete a;
        return new OpTree(val);
    }
    else
        return new OpTree(op, simplify_terms(a));
}
*/

OpTree* do_exp(OpTree *a)
{
    if (a->op == 0) {
        realt val = exp(a->val);
        delete a;
        return new OpTree(val);
    }
    else
        return new OpTree(OP_EXP, simplify_terms(a));
}

OpTree* do_erf(OpTree *a)
{
    if (a->op == 0) {
        realt val = erf(a->val);
        delete a;
        return new OpTree(val);
    }
    else
        return new OpTree(OP_ERF, simplify_terms(a));
}

OpTree* do_erfc(OpTree *a)
{
    if (a->op == 0) {
        realt val = erfc(a->val);
        delete a;
        return new OpTree(val);
    }
    else
        return new OpTree(OP_ERFC, simplify_terms(a));
}

OpTree* do_sqrt(OpTree *a)
{
    if (a->op == 0) {
        realt val = sqrt(a->val);
        delete a;
        return new OpTree(val);
    }
    else
        return new OpTree(OP_SQRT, a);
}

OpTree* do_log10(OpTree *a)
{
    if (a->op == 0) {
        realt val = log10(a->val);
        delete a;
        return new OpTree(val);
    }
    else
        return new OpTree(OP_LOG10, simplify_terms(a));
}

OpTree* do_ln(OpTree *a)
{
    if (a->op == 0) {
        realt val = log(a->val);
        delete a;
        return new OpTree(val);
    }
    else
        return new OpTree(OP_LN, simplify_terms(a));
}

OpTree* do_sinh(OpTree *a)
{
    if (a->op == 0) {
        realt val = sinh(a->val);
        delete a;
        return new OpTree(val);
    }
    else
        return new OpTree(OP_SINH, simplify_terms(a));
}

OpTree* do_cosh(OpTree *a)
{
    if (a->op == 0) {
        realt val = cosh(a->val);
        delete a;
        return new OpTree(val);
    }
    else
        return new OpTree(OP_COSH, simplify_terms(a));
}

OpTree* do_tanh(OpTree *a)
{
    if (a->op == 0) {
        realt val = tanh(a->val);
        delete a;
        return new OpTree(val);
    }
    else
        return new OpTree(OP_TANH, simplify_terms(a));
}

OpTree* do_sin(OpTree *a)
{
    if (a->op == 0) {
        realt val = sin(a->val);
        delete a;
        return new OpTree(val);
    }
    else
        return new OpTree(OP_SIN, simplify_terms(a));
}

OpTree* do_cos(OpTree *a)
{
    if (a->op == 0) {
        realt val = cos(a->val);
        delete a;
        return new OpTree(val);
    }
    else
        return new OpTree(OP_COS, simplify_terms(a));
}

OpTree* do_tan(OpTree *a)
{
    if (a->op == 0) {
        realt val = tan(a->val);
        delete a;
        return new OpTree(val);
    }
    else
        return new OpTree(OP_TAN, simplify_terms(a));
}

OpTree* do_atan(OpTree *a)
{
    if (a->op == 0) {
        realt val = atan(a->val);
        delete a;
        return new OpTree(val);
    }
    else
        return new OpTree(OP_ATAN, simplify_terms(a));
}

OpTree* do_asin(OpTree *a)
{
    if (a->op == 0) {
        realt val = asin(a->val);
        delete a;
        return new OpTree(val);
    }
    else
        return new OpTree(OP_ASIN, simplify_terms(a));
}

OpTree* do_acos(OpTree *a)
{
    if (a->op == 0) {
        realt val = acos(a->val);
        delete a;
        return new OpTree(val);
    }
    else
        return new OpTree(OP_ACOS, simplify_terms(a));
}

OpTree* do_lgamma(OpTree *a)
{
    if (a->op == 0) {
        realt val = boost::math::lgamma(a->val);
        delete a;
        return new OpTree(val);
    }
    else
        return new OpTree(OP_LGAMMA, simplify_terms(a));
}

OpTree* do_digamma(OpTree *a)
{
    if (a->op == 0) {
        realt val = boost::math::digamma(a->val);
        delete a;
        return new OpTree(val);
    }
    else
        return new OpTree(OP_DIGAMMA, simplify_terms(a));
}

OpTree* do_abs(OpTree *a)
{
    if (a->op == 0) {
        realt val = fabs(a->val);
        delete a;
        return new OpTree(val);
    }
    else
        return new OpTree(OP_ABS, simplify_terms(a));
}

OpTree* do_pow(OpTree *a, OpTree *b)
{
    if (a->op == 0 && b->op == 0) {
        realt val = pow(a->val, b->val);
        delete a;
        delete b;
        return new OpTree(val);
    }
    else if (a->op == 0 && is_eq(a->val, 0.)) {
        delete a;
        delete b;
        return new OpTree(0.);
    }
    else if ((b->op == 0 && is_eq(b->val, 0.))
        || (a->op == 0 && is_eq(a->val, 1.))) {
        delete a;
        delete b;
        return new OpTree(1.);
    }
    else if (b->op == 0 && is_eq(b->val, 1.)) {
        delete b;
        return a;
    }
    else if (b->op == 0 && is_eq(b->val, -1.)) {
        delete b;
        return do_oneover(a);
    }
    else {
        return new OpTree(OP_POW, a, simplify_terms(b));
    }
}

OpTree* do_voigt(OpTree *a, OpTree *b)
{
    if (a->op == 0 && b->op == 0) {
        realt val = humlik(a->val, b->val) / sqrt(M_PI);
        delete a;
        return new OpTree(val);
    }
    else
        return new OpTree(OP_VOIGT, simplify_terms(a), simplify_terms(b));
}

OpTree* do_dvoigt_dx(OpTree *a, OpTree *b)
{
    if (a->op == 0 && b->op == 0) {
        realt val = humdev_dkdx(a->val, b->val) / sqrt(M_PI);
        delete a;
        return new OpTree(val);
    }
    else
        return new OpTree(OP_DVOIGT_DX, simplify_terms(a), simplify_terms(b));
}

OpTree* do_dvoigt_dy(OpTree *a, OpTree *b)
{
    if (a->op == 0 && b->op == 0) {
        realt val = humdev_dkdy(a->val, b->val) / sqrt(M_PI);
        delete a;
        return new OpTree(val);
    }
    else
        return new OpTree(OP_DVOIGT_DY, simplify_terms(a), simplify_terms(b));
}

////////////////////////////////////////////////////////////////////////////

struct MultFactor
{
    // factor (*t)^(*e)
    OpTree *t, *e;
    MultFactor(OpTree *t_, OpTree *e_) : t(t_), e(e_) {}
    void clear() { delete t; delete e; t=e=0; }
};


/// recursively walk though OP_MUL, OP_DIV, OP_NEG, OP_SQRT, OP_POW
/// and builds list of nodes with factors, such that tree a is equal to
/// (v[0]->t)^(v[0]->e) * (v[1]->t)^(v[1]->e) * ...
void get_factors(OpTree *a, OpTree *expo,
                 realt &constant, vector<MultFactor>& v)
{
    if (a->op == OP_ADD || a->op == OP_SUB)
        a = simplify_terms(a);
    if (a->op == 0 && expo->op == 0)
        constant *= pow(a->val, expo->val);
    else if (a->op == OP_MUL) {
        get_factors(a->c1, expo, constant, v);
        get_factors(a->c2, expo, constant, v);
    }
    else if (a->op == OP_DIV) {
        get_factors(a->c1, expo, constant, v);
        OpTree *expo2 = do_neg(expo->clone());
        get_factors(a->c2, expo2, constant, v);
        delete expo2;
    }
    else if (a->op == OP_NEG) {
        get_factors(a->c1, expo, constant, v);
        get_factors(new OpTree(-1.), expo, constant, v);
    }
    else if (a->op == OP_SQRT) {
        OpTree *expo2 = do_multiply(new OpTree(0.5), expo->clone());
        get_factors(a->c1, expo2, constant, v);
        delete expo2;
    }
    else if (a->op == OP_POW) {
        OpTree *expo2 = do_multiply(a->remove_c2(), expo->clone());
        get_factors(a->c1, expo2, constant, v);
        delete expo2;
    }
    else {
        bool found = false;
        for (vector<MultFactor>::iterator i = v.begin(); i != v.end(); ++i)
            if (*i->t == *a) {
                i->e = do_add(i->e, expo->clone());
                found = true;
                break;
            }
            if (!found) {
                v.push_back(MultFactor(a, expo->clone()));
                return; //don't delete a
            }
    }
    //we are here -- MultFactor(a,...) not created
    a->c1 = a->c2 = 0;
    delete a;
}


OpTree* simplify_factors(OpTree *a)
{
#ifdef DEBUG_SIMPLIFY
    cout << "simplify_factors() [<] " << a->str() << endl;
#endif
    vector<MultFactor> v;
    OpTree expo(1.);
    realt constant = 1;
    get_factors(a, &expo, constant, v); //deletes a
#ifdef DEBUG_SIMPLIFY
    cout << "simplify_factors(): [.] {" << constant << "} ";
    for (vector<MultFactor>::iterator i = v.begin(); i != v.end(); ++i)
        cout << "{" << i->t->str() << "|" << i->e->str() << "} ";
    cout << endl;
#endif

    // tan*cos -> sin; tan/sin -> cos
    for (vector<MultFactor>::iterator i = v.begin(); i != v.end(); ++i)
        if (i->t && i->t->op == OP_TAN) {
            for (vector<MultFactor>::iterator j = v.begin(); j != v.end(); ++j){
                if (j->t && j->t->op == OP_COS && *j->e == *i->e) {
                    i->t->change_op(OP_SIN);
                    j->clear();
                }
                if (j->t && j->t->op == OP_SIN
                    && ((j->e->op==0 && i->e->op==0 && j->e->val==-i->e->val)
                        || (j->e->op==OP_NEG && *j->e->c1 == *i->e)
                        || (i->e->op==OP_NEG && *i->e->c1 == *j->e))) {
                    i->t->change_op(OP_COS);
                    j->clear();
                }
            }
        }
    // sin/cos -> tan
    for (vector<MultFactor>::iterator i = v.begin(); i != v.end(); ++i)
        if (i->t && i->t->op == OP_SIN) {
            for (vector<MultFactor>::iterator j = v.begin(); j != v.end(); ++j){
                if (j->t && j->t->op == OP_COS
                    && ((j->e->op==0 && i->e->op==0 && j->e->val==-i->e->val)
                        || (j->e->op==OP_NEG && *j->e->c1 == *i->e)
                        || (i->e->op==OP_NEG && *i->e->c1 == *j->e))) {
                    i->t->change_op(OP_TAN);
                    j->clear();
                }
            }
        }

    // -> tree
    // TODO x^z * y^z -> (x*y)^z  (if z != -1,0,1)
    OpTree *tu = 0, *tb = 0; // preparing expression as (tu / tb)
    for (vector<MultFactor>::iterator i = v.begin(); i != v.end(); ++i)
        if (i->t) {
            if ((i->e->op == 0 && i->e->val < 0) || i->e->op == OP_NEG) {
                OpTree *p = do_pow(i->t, do_neg(i->e));
                tb = (tb == 0 ? p : do_multiply(tb, p));
            }
            else {
                OpTree *p = do_pow(i->t, i->e);
                tu = (tu == 0 ? p : do_multiply(tu, p));
            }
        }
    OpTree *constant_t = new OpTree(constant);
    OpTree *ret = 0;
    if (tu) {
        if (tb)
            ret = do_multiply(constant_t, do_divide(tu, tb));
        else //tu && !tb
            ret = do_multiply(constant_t, tu);
    }
    else {
        if (tb) //!tu && tb
            ret = do_divide(constant_t, tb);
        else //!tu && !tb
            ret = constant_t;
    }
#ifdef DEBUG_SIMPLIFY
    cout << "simplify_factors() [>] " << ret->str() << endl;
#endif
    return ret;
}

////////////////////////////////////////////////////////////////////////////

struct MultTerm
{
    OpTree *t;
    realt k;
    MultTerm(OpTree *t_, realt k_) : t(t_), k(k_) {}
    void clear() { delete t; t=0; }
};

void get_terms(OpTree *a, realt multiplier, vector<MultTerm> &v)
{
    if (a->op == OP_MUL || a->op == OP_DIV || a->op == OP_SQRT
            || a->op == OP_POW)
        a = simplify_factors(a);

    if (a->op == OP_ADD) {  // p + q
        get_terms(a->c1, multiplier, v);
        get_terms(a->c2, multiplier, v);
        a->c1 = a->c2 = 0;
        delete a;
    }
    else if (a->op == OP_SUB) {  // p - q
        get_terms(a->c1, multiplier, v);
        get_terms(a->c2, -multiplier, v);
        a->c1 = a->c2 = 0;
        delete a;
    }
    else if (a->op == OP_NEG) {  // - p
        get_terms(a->c1, -multiplier, v);
        a->c1 = a->c2 = 0;
        delete a;
    }
    else if (a->op == OP_MUL && a->c1->op == 0) { // const * p
        get_terms(a->c2, multiplier*(a->c1->val), v);
        a->c2 = 0;
        delete a;
    }
         // const / p for const != 1 (to avoid loop)
    else if (a->op == OP_DIV && a->c1->op == 0 && a->c1->val != 1.) {
        get_terms(do_oneover(a->c2), multiplier*(a->c1->val), v);
        a->c2 = 0;
        delete a;
    }
    else { // a can't be splitted
        for (vector<MultTerm>::iterator i = v.begin(); i != v.end(); ++i) {
            if (a->op == 0 && i->t && i->t->op == 0) {// number (not the first)
                i->k += multiplier * a->val;
                delete a;
                return;
            }
            if (i->t && *i->t == *a) { //token already in v
                i->k += multiplier;
                delete a;
                return;
            }
        }
        // we are here -- no first token of its kind
        if (a->op == 0) { //add number
            v.push_back(MultTerm(new OpTree(1.), multiplier * a->val));
            delete a;
        }
        else {           // add token
            v.push_back(MultTerm(a, multiplier));
        }
    }
}

OpTree* simplify_terms(OpTree *a)
{
    // not handled:
    //        (x+y) * (x-y) == x^2 - y^2
    //        (x+/-y)^2 == x^2 +/- 2xy + y^2
    if (a->op == OP_MUL || a->op == OP_DIV || a->op == OP_SQRT
            || a->op == OP_POW)
        return simplify_factors(a);
    else if (!(a->op == OP_ADD || a->op == OP_SUB || a->op == OP_NEG))
        return a;
#ifdef DEBUG_SIMPLIFY
    cout << "simplify_terms() [<] " << a->str() << endl;
#endif
    vector<MultTerm> v;
    get_terms(a, 1., v); //deletes a
#ifdef DEBUG_SIMPLIFY
    cout << "simplify_terms() [.] ";
    for (vector<MultTerm>::iterator i = v.begin(); i != v.end(); ++i)
        cout << "{" << i->t->str() << "|" << i->k << "} ";
    cout << endl;
#endif

    // sin^2(x) + cos^2(x) = 1
    realt to_add = 0.;
    for (vector<MultTerm>::iterator i = v.begin(); i != v.end(); ++i)
        if (i->t && i->t->op == OP_POW && i->t->c1->op == OP_SIN
                && i->t->c2->op == 0 && is_eq(i->t->c2->val, 2.))
            for (vector<MultTerm>::iterator j = v.begin(); j != v.end(); ++j)
                if (j->t && j->t->op == OP_POW && j->t->c1->op == OP_COS
                        && j->t->c2->op == 0 && is_eq(j->t->c2->val, 2.)) {
                    realt k = j->k;
                    i->k -= k;
                    j->clear();
                    to_add += k;
                }
    if (to_add)
        get_terms(new OpTree(1.), to_add, v);

    // -> tree
    OpTree *t = 0;
    for (vector<MultTerm>::iterator i = v.begin(); i != v.end(); ++i)
        if (i->t && !is_eq(i->k, 0)) {
            if (!t)
                t = do_multiply(new OpTree(i->k), i->t);
            else if (i->k > 0)
                t = do_add(t, do_multiply(new OpTree(i->k), i->t));
            else //i->k < 0
                t = do_sub(t, do_multiply(new OpTree(-i->k), i->t));
        }
    if (!t)
        t = new OpTree(0.);
#ifdef DEBUG_SIMPLIFY
    cout << "simplify_terms() [>] " << t->str() << endl;
#endif
    return t;
}

////////////////////////////////////////////////////////////////////////////


/// simplify exportable formula, i.e. mathematical function f(x),
/// without variables other than x
string simplify_formula(string const &formula, const char* num_fmt)
{
    Lexer lex(formula.c_str());
    ExpressionParser ep(NULL);
    // if the formula has not-expanded-functions, like Voigt(2,3,4,5),
    // it throws SyntaxError
    try {
        ep.parse_expr(lex, -1, NULL, NULL, ExpressionParser::kAstMode);
    }
    catch (SyntaxError&) {
        return formula;
    }
    // derivatives are calculated only as a side effect
    vector<OpTree*> trees = prepare_ast_with_der(ep.vm(), 1);
    vector<string> vars(1, "x");
    OpTreeFormat fmt = { num_fmt, &vars };
    string simplified = trees.back()->str(fmt);
    purge_all_elements(trees);
    return simplified;
}

/// debug utility, shows symbolic derivatives of given formula
void get_derivatives_str(const char* formula, string& result)
{
    Lexer lex(formula);
    ExpressionParser ep(NULL);
    vector<string> vars;
    ep.parse_expr(lex, -1, NULL, &vars);
    vector<OpTree*> trees = prepare_ast_with_der(ep.vm(), vars.size());

    OpTreeFormat fmt = { "%g", &vars };
    result += "f(" + join_vector(vars,", ") + ") = " + trees.back()->str(fmt);
    for (size_t i = 0; i != vars.size(); ++i)
        result += "\ndf / d " + vars[i] + " = " + trees[i]->str(fmt);
    purge_all_elements(trees);
}

/// returns array of trees,
/// first n=vars.size() derivatives and the last tree for value
static
vector<OpTree*> calculate_deriv(vector<int>::const_iterator &i, int len,
                                const VMData& vm)
{
  vector<OpTree*> results(len + 1, (OpTree*) NULL);
  --i;
  assert(i >= vm.code().begin());

  if (*i < 0) {
      --i;
      assert(i >= vm.code().begin());
      assert(VMData::has_idx(*i));
  }

  switch (*i) {

    case OP_NUMBER: {
        for (int k = 0; k < len; ++k)
            results[k] = new OpTree(0.);
        int negative_idx = *(i+1);
        int idx = -1 - negative_idx;
        assert(is_index(idx, vm.numbers()));
        realt value = vm.numbers()[idx];
        results[len] = new OpTree(value);
        break;
    }

    case OP_SYMBOL: {
        int negative_idx = *(i+1);
        int idx = -1 - negative_idx;
        assert(idx <= len);
        for (int k = 0; k < len; ++k) {
            realt der_value = (k == idx ? 1. : 0.);
            results[k] = new OpTree(der_value);
        }
        results[len] = new OpTree(NULL, idx);
        break;
    }

    case OP_X: {
        int n = len - 1;
        // the rest is the same as in OP_SYMBOL
        for (int k = 0; k < len; ++k) {
            realt der_value = (k == n ? 1. : 0.);
            results[k] = new OpTree(der_value);
        }
        results[len] = new OpTree(NULL, n);
        break;
    }


    case OP_SQRT:
    case OP_EXP:
    case OP_ERFC:
    case OP_ERF:
    case OP_LOG10:
    case OP_LN:
    case OP_SINH:
    case OP_COSH:
    case OP_TANH:
    case OP_SIN:
    case OP_COS:
    case OP_TAN:
    case OP_ASIN:
    case OP_ACOS:
    case OP_ATAN:
    case OP_LGAMMA:
    case OP_ABS:
    {
        int op = *i;
        vector<OpTree*> arg = calculate_deriv(i, len, vm);
        OpTree* (* do_op)(OpTree *) = NULL;
        OpTree* der = 0;
        OpTree* larg = arg.back()->clone();
        if (op == OP_SQRT) {
            der = do_divide(new OpTree(0.5), do_sqrt(larg));
            do_op = do_sqrt;
        }
        else if (op == OP_EXP) {
            der = do_exp(larg);
            do_op = do_exp;
        }
        else if (op == OP_ERFC) {
            // d/dz erfc(z) = -2/sqrt(pi) * exp(-z^2)
            der = do_multiply(do_exp(do_neg(do_sqr(larg))),
                              new OpTree(-2/sqrt(M_PI)));
            do_op = do_erfc;
        }
        else if (op == OP_ERF) {
            // d/dz erf(z) =  2/sqrt(pi) * exp(-z^2)
            der = do_multiply(do_exp(do_neg(do_sqr(larg))),
                              new OpTree(2/sqrt(M_PI)));
            do_op = do_erf;
        }
        else if (op == OP_LOG10) {
            OpTree *ln_10 = do_ln(new OpTree(10.));
            der = do_oneover(do_multiply(larg, ln_10));
            do_op = do_log10;
        }
        else if (op == OP_LN) {
            der = do_oneover(larg);
            do_op = do_ln;
        }
        else if (op == OP_SINH) {
            der = do_cosh(larg);
            do_op = do_sinh;
        }
        else if (op == OP_COSH) {
            der = do_sinh(larg);
            do_op = do_cosh;
        }
        else if (op == OP_TANH) {
            der = do_oneover(do_sqr(do_cosh(larg)));
            do_op = do_tanh;
        }
        else if (op == OP_SIN) {
            der = do_cos(larg);
            do_op = do_sin;
        }
        else if (op == OP_COS) {
            der = do_neg(do_sin(larg));
            do_op = do_cos;
        }
        else if (op == OP_TAN) {
            der = do_oneover(do_sqr(do_cos(larg)));
            do_op = do_tan;
        }
        else if (op == OP_ASIN) {
            OpTree *root_arg = do_sub(new OpTree(1.), do_sqr(larg));
            der = do_oneover(do_sqrt(root_arg));
            do_op = do_asin;
        }
        else if (op == OP_ACOS) {
            OpTree *root_arg = do_sub(new OpTree(1.), do_sqr(larg));
            der = do_divide(new OpTree(-1.), do_sqrt(root_arg));
            do_op = do_acos;
        }
        else if (op == OP_ATAN) {
            der = do_oneover(do_add(new OpTree(1.), do_sqr(larg)));
            do_op = do_atan;
        }
        else if (op == OP_LGAMMA) {
            der = do_digamma(larg);
            do_op = do_lgamma;
        }
        else if (op == OP_ABS) {
            der = do_divide(do_abs(larg), larg->clone());
            do_op = do_abs;
        }
        else
            assert(0);
        for (int k = 0; k < len; ++k)
            results[k] = do_multiply(der->clone(), arg[k]);
        delete der;
        results[len] = (*do_op)(arg[len]);
        break;
    }

    case OP_VOIGT:
    {
        int op = *i;
        vector<OpTree*> right = calculate_deriv(i, len, vm);
        vector<OpTree*> left = calculate_deriv(i, len, vm);
        assert (op == OP_VOIGT);
        OpTree *d1 = do_dvoigt_dx(left[len]->clone(), right[len]->clone());
        OpTree *d2 = do_dvoigt_dy(left[len]->clone(), right[len]->clone());
        for (int k = 0; k < len; ++k) {
            results[k] = do_add(do_multiply(d1->clone(), left[k]),
                                do_multiply(d2->clone(), right[k]));
        }
        results[len] = do_voigt(left[len], right[len]);
        delete d1;
        delete d2;
        break;
    }

    case OP_POW: {
        vector<OpTree*> right = calculate_deriv(i, len, vm);
        vector<OpTree*> left = calculate_deriv(i, len, vm);
        // this special case is needed, because in cases like -2^4
        // there was a problem with logarithm in formula below
        if (left[len]->op == 0 && right[len]->op == 0) {
            for (int k = 0; k < len; ++k) {
                assert(left[k]->op == 0 && right[k]->op == 0);
                delete left[k];
                delete right[k];
                results[k] = new OpTree(0.);
            }
            results[len] = do_pow(left[len], right[len]);
        }
        //another special cases like a(x)^n are not handeled separately.
        //Should they?
        else {
            for (int k = 0; k < len; ++k) {
                OpTree *a = left[len],
                       *b = right[len],
                       *ap = left[k],
                       *bp = right[k];
                //        b(x)            b(x)  b(x) a'(x)
                //    (a(x)   )'    = a(x)     (---------- + ln(a(x)) b'(x))
                //                                 a(x)
                OpTree *pow_a_b = do_pow(a->clone(), b->clone());
                OpTree *term1 = do_divide(do_multiply(b->clone(), ap),
                                          a->clone());
                OpTree *term2 = do_multiply(do_ln(a->clone()), bp);
                results[k] = do_multiply(pow_a_b, do_add(term1, term2));
            }
            results[len] = do_pow(left[len], right[len]);
        }
        break;
    }

    case OP_NEG: {
        vector<OpTree*> arg = calculate_deriv(i, len, vm);
        for (int k = 0; k < len+1; ++k)
            results[k] = do_neg(arg[k]);
        break;
    }

    case OP_MUL: {
        vector<OpTree*> right = calculate_deriv(i, len, vm);
        vector<OpTree*> left = calculate_deriv(i, len, vm);
        for (int k = 0; k < len; ++k) {
            // a*b' + a'*b
            OpTree *a = left[len],
                   *b = right[len],
                   *ap = left[k],
                   *bp = right[k];
            results[k] = do_add(do_multiply(a->clone(), bp),
                                do_multiply(ap, b->clone()));
        }
        results[len] = do_multiply(left[len], right[len]);
        break;
    }

    case OP_DIV: {
        vector<OpTree*> right = calculate_deriv(i, len, vm);
        vector<OpTree*> left = calculate_deriv(i, len, vm);
        for (int k = 0; k < len; ++k) {
            // (a'*b - b'*a) / (b*b)
            OpTree *a = left[len],
                   *b = right[len],
                   *ap = left[k],
                   *bp = right[k];
            OpTree *upper = do_sub(do_multiply(ap, b->clone()),
                                   do_multiply(bp, a->clone()));
            results[k] = do_divide(upper, do_sqr(b->clone()));
        }
        results[len] = do_divide(left[len], right[len]);
        break;
    }

    case OP_ADD: {
        vector<OpTree*> right = calculate_deriv(i, len, vm);
        vector<OpTree*> left = calculate_deriv(i, len, vm);
        for (int k = 0; k < len+1; ++k)
            results[k] = do_add(left[k], right[k]);
        break;
    }

    case OP_SUB: {
        vector<OpTree*> right = calculate_deriv(i, len, vm);
        vector<OpTree*> left = calculate_deriv(i, len, vm);
        for (int k = 0; k < len+1; ++k)
            results[k] = do_sub(left[k], right[k]);
        break;
    }

    /* to be added:
    case OP_GAMMA:
    case OP_ROUND:
    case OP_MOD:
    case OP_MIN2:
    case OP_MAX2:
    case OP_GT:
    case OP_GE:
    case OP_LT:
    case OP_LE:
    case OP_EQ:
    case OP_NEQ:
    case OP_NOT:
    case OP_OR:
    case OP_AFTER_OR:
    case OP_AND:
    case OP_AFTER_AND:
    case OP_TERNARY:
    case OP_TERNARY_MID:
    case OP_AFTER_TERNARY:
    */

    default:
        purge_all_elements(results);
        throw ExecuteError("`" + op2str(*i) + "' is not allowed for "
                           "variables/functions");
    }
  assert(results[0] != NULL);
  assert(results[len] != NULL);

  for (int k = 0; k < len+1; ++k)
      results[k] = simplify_terms(results[k]);

  //printf("%s\n", results[len]->str().c_str());
  return results;
}

vector<OpTree*> prepare_ast_with_der(const VMData& vm, int len)
{
    assert(!vm.code().empty());
    const_cast<VMData&>(vm).flip_indices();
    vector<int>::const_iterator iter = vm.code().end();
    vector<OpTree*> r = calculate_deriv(iter, len, vm);
    assert (iter == vm.code().begin());
    const_cast<VMData&>(vm).flip_indices();
    return r;
}

void add_bytecode_from_tree(const OpTree* tree, const vector<int> &symbol_map,
                            VMData& vm)
{
    int op = tree->op;
    if (op < 0) {
        size_t n = -op-1;
        if (n == symbol_map.size()) {
            vm.append_code(OP_X);
        }
        else {
            assert(is_index(n, symbol_map));
            vm.append_code(OP_SYMBOL);
            vm.append_code(symbol_map[n]);
        }
    }
    else if (op == 0) {
        vm.append_number(tree->val);
    }
    else if (op >= OP_ONE_ARG && op < OP_TWO_ARG) { //one argument
        add_bytecode_from_tree(tree->c1, symbol_map, vm);
        vm.append_code(op);
    }
    else if (op >= OP_TWO_ARG) { //two arguments
        add_bytecode_from_tree(tree->c1, symbol_map, vm);
        add_bytecode_from_tree(tree->c2, symbol_map, vm);
        vm.append_code(op);
    }
}

} // namespace fityk

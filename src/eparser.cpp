// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
// $Id: $

/// This parser is not used yet.
/// In the future it will replace the current parser (datatrans* files)
/// Data expression parser.

#include "eparser.h"

#include <iostream>
#include <cstring>
#include <cmath>
#include <boost/math/special_functions/gamma.hpp>

#include "lexer.h"
#include "common.h"
#include "datatrans.h"
#include "numfuncs.h"
#include "voigt.h"

#include "logic.h"
#include "var.h" // $v
#include "fit.h" // $var.error
#include "func.h" // %f(...)
#include "model.h" // F(...)

using namespace std;

using namespace dataVM;

/// debuging utility
#define OP_(x) \
    if (op == OP_##x) return #x;
string dt_op(int op)
{
    OP_(NEG)   OP_(EXP)
    OP_(SIN)   OP_(COS)  OP_(TAN)  OP_(SINH) OP_(COSH)  OP_(TANH)
    OP_(ABS)  OP_(ROUND)
    OP_(ATAN) OP_(ASIN) OP_(ACOS)
    OP_(LOG10) OP_(LN)  OP_(SQRT)  OP_(POW)
    OP_(GAMMA) OP_(LGAMMA) OP_(VOIGT) OP_(XINDEX)
    OP_(ADD)   OP_(SUB)   OP_(MUL)   OP_(DIV)  OP_(MOD)
    OP_(MIN2)   OP_(MAX2) OP_(RANDNORM) OP_(RANDU)
    OP_(VAR_X) OP_(VAR_Y) OP_(VAR_S) OP_(VAR_A)
    OP_(VAR_x) OP_(VAR_y) OP_(VAR_s) OP_(VAR_a)
    OP_(VAR_n) OP_(VAR_M) OP_(NUMBER)
    OP_(OR) OP_(AFTER_OR) OP_(AND) OP_(AFTER_AND) OP_(NOT)
    OP_(TERNARY) OP_(TERNARY_MID) OP_(AFTER_TERNARY)
    OP_(GT) OP_(GE) OP_(LT) OP_(LE) OP_(EQ) OP_(NEQ)
    OP_(INDEX)
    OP_(ASSIGN_X) OP_(ASSIGN_Y) OP_(ASSIGN_S) OP_(ASSIGN_A)
    OP_(DO_ONCE) OP_(RESIZE) OP_(BEGIN) OP_(END)
    OP_(END_AGGREGATE) OP_(AGCONDITION)
    OP_(AGSUM) OP_(AGMIN) OP_(AGMAX) OP_(AGAREA) OP_(AGAVG) OP_(AGSTDDEV)
    OP_(FUNC) OP_(SUM_F) OP_(SUM_Z) OP_(NUMAREA) OP_(FINDX) OP_(FIND_EXTR)
    return S(op);
};
#undef OP_

string get_code_as_text(vector<int> const& code, vector<fp> const& numbers)
{
    string txt;
    for (vector<int>::const_iterator i = code.begin(); i != code.end(); ++i) {
        txt += " " + dt_op(*i);
        if (*i == OP_NUMBER && i+1 != code.end()) {
            ++i;
            txt += "(" + S(numbers[*i]) + ")";
        }
    }
    return txt;
}

bool is_data_dependent_code(const vector<int>& code)
{
    for (vector<int>::const_iterator i = code.begin(); i != code.end(); ++i)
        if ((*i >= OP_VAR_FIRST_OP && *i <= OP_VAR_LAST_OP)
                || *i == OP_END_AGGREGATE)
            return true;
    return false;
}

int get_op_priority(int op)
{
    switch (op) {
        case OP_POW: return 9;
        case OP_NEG: return 8;
        case OP_MUL: return 7;
        case OP_DIV: return 7;
        case OP_ADD: return 6;
        case OP_SUB: return 6;
        case OP_GT: return 5;
        case OP_GE: return 5;
        case OP_LT: return 5;
        case OP_LE: return 5;
        case OP_EQ: return 5;
        case OP_NEQ: return 5;
        case OP_NOT: return 4;
        case OP_AFTER_AND: return 3;
        case OP_AFTER_OR: return 2;
        case OP_TERNARY_MID: return 1;
        case OP_AFTER_TERNARY: return 1;
        default: return 0;
    }
}

string function_name(int op)
{
    switch (op) {
        // 1-arg functions
        case OP_SQRT: return "sqrt";
        case OP_GAMMA: return "gamma";
        case OP_LGAMMA: return "lgamma";
        case OP_ERFC: return "erfc";
        case OP_ERF: return "erf";
        case OP_EXP: return "exp";
        case OP_LOG10: return "log10";
        case OP_LN: return "ln";
        case OP_SINH: return "sinh";
        case OP_COSH: return "cosh";
        case OP_TANH: return "tanh";
        case OP_SIN: return "sin";
        case OP_COS: return "cos";
        case OP_TAN: return "tan";
        case OP_ATAN: return "atan";
        case OP_ASIN: return "asin";
        case OP_ACOS: return "acos";
        case OP_ABS: return "abs";
        case OP_ROUND: return "round";
        case OP_XINDEX: return "index";
        // 2-args functions
        case OP_MOD: return "mod";
        case OP_MIN2: return "min2";
        case OP_MAX2: return "max2";
        case OP_VOIGT: return "voigt";
        case OP_RANDNORM: return "randnormal";
        case OP_RANDU: return "randuniform";
        // Ftk functions
        case OP_FUNC: return "%function";
        case OP_SUM_F: return "F";
        case OP_SUM_Z: return "Z";
        default: return "";
    }
}


int get_function_narg(int op)
{
    switch (op) {
        // 1-arg functions
        case OP_SQRT:
        case OP_GAMMA:
        case OP_LGAMMA:
        case OP_ERFC:
        case OP_ERF:
        case OP_EXP:
        case OP_LOG10:
        case OP_LN:
        case OP_SINH:
        case OP_COSH:
        case OP_TANH:
        case OP_SIN:
        case OP_COS:
        case OP_TAN:
        case OP_ATAN:
        case OP_ASIN:
        case OP_ACOS:
        case OP_ABS:
        case OP_ROUND:
        case OP_XINDEX:
            return 1;
        // 2-args functions
        case OP_MOD:
        case OP_MIN2:
        case OP_MAX2:
        case OP_VOIGT:
        case OP_RANDNORM:
        case OP_RANDU:
            return 2;
        // Ftk functions
        case OP_FUNC:
        case OP_SUM_F:
        case OP_SUM_Z:
            return 1;
        // "methods" of %f/F/Z
        case OP_NUMAREA:
        case OP_FINDX:
            return 3;
        case OP_FIND_EXTR:
            return 2;
        default:
            return 0;
    }
}

bool is_function(int op)
{
    return (bool) get_function_narg(op);
}

bool is_array_var(int op)
{
    switch (op) {
        case OP_VAR_x:
        case OP_VAR_y:
        case OP_VAR_s:
        case OP_VAR_a:
        case OP_VAR_X:
        case OP_VAR_Y:
        case OP_VAR_S:
        case OP_VAR_A:
            return true;
        default:
            return false;
    }
}

void ExpressionParser::pop_onto_que()
{
    int op = opstack_.back();
    opstack_.pop_back();
    code_.push_back(op);
}

void ExpressionParser::put_number(double value)
{
    if (expected_ == kOperator) {
        finished_ = true;
        return;
    }
    //cout << "put_number() " << value << endl;
    code_.push_back(OP_NUMBER);
    int number_pos = numbers_.size();
    code_.push_back(number_pos);
    numbers_.push_back(value);
    expected_ = kOperator;
}

void ExpressionParser::put_unary_op(VMOp op)
{
    if (expected_ == kOperator) {
        finished_ = true;
        return;
    }
    opstack_.push_back(op);
    expected_ = kValue;
}

void ExpressionParser::put_binary_op(VMOp op)
{
    if (expected_ != kOperator) {
        finished_ = true;
        return;
    }
    //cout << "put_binary_op() " << op << endl;
    int pri = get_op_priority(op);
    while (!opstack_.empty() && get_op_priority(opstack_.back()) >= pri)
        pop_onto_que();
    opstack_.push_back(op);
    expected_ = kValue;
}

void ExpressionParser::put_function(VMOp op)
{
    //cout << "put_function() " << op << endl;
    arg_cnt_.push_back(0); // start new counter
    opstack_.push_back(op);
    expected_ = kValue;
}

class AggregFunc
{
public:
    AggregFunc() : counter_(0), v_(0.) {}
    virtual ~AggregFunc() {}
    void put(double x, int n) { ++counter_; op(x, n); }
    virtual double value() const { return v_; }

protected:
    int counter_;
    double v_;

    virtual void op(double x, int n) = 0;
};

class AggregSum : public AggregFunc
{
protected:
    virtual void op(double x, int)
    {
        v_ += x;
    }
};

class AggregMin : public AggregFunc
{
protected:
    virtual void op(double x, int)
    {
        if (counter_ == 1 || x < v_)
            v_ = x;
    }
};

class AggregMax : public AggregFunc
{
protected:
    virtual void op(double x, int)
    {
        if (counter_ == 1 || x > v_)
            v_ = x;
    }
};

class AggregDArea : public AggregFunc
{
public:
    AggregDArea(const vector<Point>& points) : points_(points) {}
protected:
    const vector<Point>& points_;
    virtual void op(double x, int n)
    {
        int M = points_.size();
        double dx = (points_[min(n+1, M-1)].x - points_[max(n-1, 0)].x) / 2.;
        v_ += x * dx;
    }
};

class AggregAvg : public AggregFunc
{
protected:
    virtual void op(double x, int)
    {
        v_ += (x - v_) / counter_;
    }
};

class AggregStdDev : public AggregFunc
{
public:
    AggregStdDev() : mean_(0.) {}
protected:
    double mean_;

    virtual void op(double x, int)
    {
        // see: http://en.wikipedia.org/wiki/Algorithms_for_calculating_variance
        double delta = x - mean_;
        mean_ += delta / counter_;
        v_ += delta * (x - mean_);
    }

    virtual double value() const { return sqrt(v_ / (counter_ - 1)); }
};

void ExpressionParser::put_ag_function(Lexer& lex, int ds, AggregFunc& ag)
{
    //cout << "put_ag_function() " << op << endl;
    lex.get_expected_token(kTokenOpen); // discard '('
    ExpressionParser ep(F_);
    ep.parse2vm(lex, ds);
    const vector<Point>& points = F_->get_data(ds)->points();
    Token t = lex.get_expected_token(kTokenClose, "if");
    if (t.type == kTokenClose) {
        for (size_t n = 0; n != points.size(); ++n) {
            double x = ep.calculate(n, points);
            ag.put(x, n);
        }
    }
    else { // "if"
        ExpressionParser cond_p(F_);
        cond_p.parse2vm(lex, ds);
        lex.get_expected_token(kTokenClose); // discard ')'
        for (size_t n = 0; n != points.size(); ++n) {
            double c = cond_p.calculate(n, points);
            if (fabs(c) >= 0.5) {
                double x = ep.calculate(n, points);
                ag.put(x, n);
            }
        }
    }
    put_number(ag.value());
}

void ExpressionParser::put_array_var(bool has_index, VMOp op)
{
    if (has_index) {
        opstack_.push_back(op);
        expected_ = kValue;
    }
    else {
        code_.push_back(OP_VAR_n);
        code_.push_back(op);
        expected_ = kOperator;
    }
}

void ExpressionParser::put_variable_sth(Lexer& lex, const string& name)
{
    if (F_ == NULL)
        lex.throw_syntax_error("$variables can not be used here");
    const Variable *v = F_->find_variable(name);
    if (lex.peek_token().type == kTokenDot) {
        lex.get_token(); // discard '.'
        lex.get_expected_token("error"); // discard "error"
        double e = F_->get_fit_container()->get_standard_error(v);
        if (e == -1.)
            lex.throw_syntax_error("unknown error of " + v->xname
                                  + "; it is not simple variable");
        put_number(e);
    }
    else
        put_number(v->get_value());
}

void ExpressionParser::put_func_sth(Lexer& lex, const string& name)
{
    if (F_ == NULL)
        lex.throw_syntax_error("%functions can not be used here");
    if (lex.peek_token().type == kTokenOpen) {
        int n = F_->find_function_nr(name);
        if (n == -1)
            throw ExecuteError("undefined function: %" + name);
        // we will put n into code_ when handling ')'
        opstack_.push_back(n);
        put_function(OP_FUNC);
    }
    else if (lex.peek_token().type == kTokenDot) {
        lex.get_token(); // discard '.'
        string word = lex.get_expected_token(kTokenLname).as_string();
        if (lex.peek_token().type == kTokenOpen) { // method of %function
            int n = F_->find_function_nr(name);
            if (n == -1)
                throw ExecuteError("undefined function: %" + name);
            // we will put ds into code_ when handling ')'
            opstack_.push_back(n);
            opstack_.push_back(OP_FUNC);
            if (word == "numarea")
                put_function(OP_NUMAREA);
            else if (word == "findx")
                put_function(OP_FINDX);
            else if (word == "extremum")
                put_function(OP_FIND_EXTR);
            else
                lex.throw_syntax_error("unknown method of F/Z");
        }
        else { // property of %function (= $variable)
            const Function *f = F_->find_function(name);
            string v = f->get_var_name(f->get_param_nr(word));
            put_variable_sth(lex, v);
        }
    }
    else
        lex.throw_syntax_error("expected '.' or '(' after %function");
}

void ExpressionParser::put_fz_sth(Lexer& lex, char fz, int ds)
{
    if (F_ == NULL || ds < 0)
        lex.throw_syntax_error("F/Z can not be used here");
    if (lex.peek_token().type == kTokenLSquare) {
        lex.get_token(); // discard '['
        ExpressionParser ep(F_);
        ep.parse2vm(lex, ds);
        lex.get_expected_token(kTokenRSquare); // discard ']'
        int idx = iround(ep.calculate());
        const string& name = F_->get_model(ds)->get_func_name(fz, idx);
        put_func_sth(lex, name);
    }
    else if (lex.peek_token().type == kTokenOpen) {
        opstack_.push_back(ds); // we will put ds into code_ when handling ')'
        put_function(fz == 'F' ? OP_SUM_F : OP_SUM_Z);
    }
    else if (lex.peek_token().type == kTokenDot) {
        lex.get_token(); // discard '.'
        string word = lex.get_expected_token(kTokenLname).as_string();
        if (lex.peek_token().type != kTokenOpen)
            lex.throw_syntax_error("F/Z has no .properties, only .methods()");
        // we will put ds into code_ when handling ')'
        opstack_.push_back(ds);
        opstack_.push_back(fz == 'F' ? OP_SUM_F : OP_SUM_Z);
        if (word == "numarea")
            put_function(OP_NUMAREA);
        else if (word == "findx")
            put_function(OP_FINDX);
        else if (word == "extremum")
            put_function(OP_FIND_EXTR);
        else
            lex.throw_syntax_error("unknown method of F/Z");
    }
    else {
        lex.throw_syntax_error("unexpected token after F/Z");
    }
}

void ExpressionParser::put_var(VMOp op)
{
    code_.push_back(op);
    expected_ = kOperator;
}

void ExpressionParser::pop_until_bracket()
{
    while (!opstack_.empty()) {
        int op = opstack_.back();
        if (op == OP_OPEN_ROUND || op == OP_OPEN_SQUARE || op == OP_TERNARY_MID)
            break;
        opstack_.pop_back();
        code_.push_back(op);
    }
}

void ExpressionParser::clear_vm()
{
    code_.clear();
    numbers_.clear();
}

// implementation of the shunting-yard algorithm
void ExpressionParser::parse2vm(Lexer& lex, int default_ds)
{
    opstack_.clear();
    arg_cnt_.clear();
    finished_ = false;
    expected_ = kValue;
    if (F_ != NULL && default_ds >= F_->get_dm_count())
        lex.throw_syntax_error("wrong dataset index");
    while (!finished_) {
        const Token token = lex.get_token();
        //cout << "> " << token2str(token) << endl;
        switch (token.type) {
            case kTokenNumber:
                put_number(token.value.d);
                break;
            case kTokenLname: {
                string word = token.as_string();
                // "not", "and", "or" can be followed by '(' or not.
                if (word == "not")
                    put_unary_op(OP_NOT);
                else if (word == "and") {
                    put_binary_op(OP_AFTER_AND);
                    code_.push_back(OP_AND);
                }
                else if (word == "or") {
                    put_binary_op(OP_AFTER_OR);
                    code_.push_back(OP_OR);
                }
                else if (word == "if") {
                    pop_until_bracket();
                    if (expected_ == kOperator && opstack_.empty())
                        finished_ = true;
                    else
                        lex.throw_syntax_error("unexpected `if'");
                }
                else if (lex.peek_token().type == kTokenOpen) {
                    if (expected_ == kOperator) {
                        finished_ = true;
                        break;
                    }
                    // 1-arg functions
                    if (word == "sqrt")
                        put_function(OP_SQRT);
                    else if (word == "gamma")
                        put_function(OP_GAMMA);
                    else if (word == "lgamma")
                        put_function(OP_LGAMMA);
                    else if (word == "erfc")
                        put_function(OP_ERFC);
                    else if (word == "erf")
                        put_function(OP_ERF);
                    else if (word == "exp")
                        put_function(OP_EXP);
                    else if (word == "log10")
                        put_function(OP_LOG10);
                    else if (word == "ln")
                        put_function(OP_LN);
                    else if (word == "sinh")
                        put_function(OP_SINH);
                    else if (word == "cosh")
                        put_function(OP_COSH);
                    else if (word == "tanh")
                        put_function(OP_TANH);
                    else if (word == "sin")
                        put_function(OP_SIN);
                    else if (word == "cos")
                        put_function(OP_COS);
                    else if (word == "tan")
                        put_function(OP_TAN);
                    else if (word == "atan")
                        put_function(OP_ATAN);
                    else if (word == "asin")
                        put_function(OP_ASIN);
                    else if (word == "acos")
                        put_function(OP_ACOS);
                    else if (word == "abs")
                        put_function(OP_ABS);
                    else if (word == "round")
                        put_function(OP_ROUND);
                    else if (word == "index")
                        put_function(OP_XINDEX);
                    // 2-args functions
                    else if (word == "mod")
                        put_function(OP_MOD);
                    else if (word == "min2")
                        put_function(OP_MIN2);
                    else if (word == "max2")
                        put_function(OP_MAX2);
                    else if (word == "voigt")
                        put_function(OP_VOIGT);
                    else if (word == "randnormal")
                        put_function(OP_RANDNORM);
                    else if (word == "randuniform")
                        put_function(OP_RANDU);
                    // aggregate functions
                    else if (word == "sum") {
                        AggregSum ag;
                        put_ag_function(lex, default_ds, ag);
                    }
                    else if (word == "min") {
                        AggregMin ag;
                        put_ag_function(lex, default_ds, ag);
                    }
                    else if (word == "max") {
                        AggregMax ag;
                        put_ag_function(lex, default_ds, ag);
                    }
                    else if (word == "avg") {
                        AggregAvg ag;
                        put_ag_function(lex, default_ds, ag);
                    }
                    else if (word == "stddev") {
                        AggregStdDev ag;
                        put_ag_function(lex, default_ds, ag);
                    }
                    else if (word == "darea") {
                        if (F_ == NULL)
                            lex.throw_syntax_error("darea: unknown @dataset");
                        AggregDArea ag(F_->get_data(default_ds)->points());
                        put_ag_function(lex, default_ds, ag);
                    }
                    else
                        lex.throw_syntax_error("unknown function: " + word);
                }
                else {
                    if (expected_ == kOperator) {
                        finished_ = true;
                        break;
                    }
                    bool has_index = (lex.peek_token().type == kTokenLSquare);
                    if (word == "x")
                        put_array_var(has_index, OP_VAR_x);
                    else if (word == "y")
                        put_array_var(has_index, OP_VAR_y);
                    else if (word == "s")
                        put_array_var(has_index, OP_VAR_s);
                    else if (word == "a")
                        put_array_var(has_index, OP_VAR_a);
                    else if (word == "n")
                        put_var(OP_VAR_n);
                    else if (word == "pi")
                        put_number(M_PI);
                    else if (word == "true")
                        put_number(1.);
                    else if (word == "false")
                        put_number(0.);
                    else
                        lex.throw_syntax_error("unknown name: " + word);
                }
                break;
            }
            case kTokenUletter: {
                if (expected_ == kOperator) {
                    finished_ = true;
                    break;
                }
                bool has_index = (lex.peek_token().type == kTokenLSquare);
                if (*token.str == 'X')
                    put_array_var(has_index, OP_VAR_X);
                else if (*token.str == 'Y')
                    put_array_var(has_index, OP_VAR_Y);
                else if (*token.str == 'S')
                    put_array_var(has_index, OP_VAR_S);
                else if (*token.str == 'A')
                    put_array_var(has_index, OP_VAR_A);
                else if (*token.str == 'M')
                    put_var(OP_VAR_M);
                else if (*token.str == 'F' || *token.str == 'Z') {
                    put_fz_sth(lex, *token.str, default_ds);
                }
                else
                    lex.throw_syntax_error("unknown name: "+ token.as_string());
                break;
            }
            case kTokenDataset: {
                if (expected_ == kOperator) {
                    finished_ = true;
                    break;
                }
                lex.get_expected_token(kTokenDot); // discard '.'
                Token t = lex.get_expected_token(kTokenUletter);
                if (*t.str == 'F' || *t.str == 'Z') {
                    put_fz_sth(lex, *t.str, token.value.i);
                }
                else
                    lex.throw_syntax_error("unknown name: "+ token.as_string());
                break;
            }
            case kTokenOpen:
                if (expected_ == kOperator) {
                    finished_ = true;
                    break;
                }
                opstack_.push_back(OP_OPEN_ROUND);
                expected_ = kValue;
                break;
            case kTokenLSquare:
                if (expected_ == kOperator) {
                    finished_ = true;
                    break;
                }
                opstack_.push_back(OP_OPEN_SQUARE);
                expected_ = kValue;
                break;

            case kTokenClose:
                pop_until_bracket();
                if (opstack_.empty()) {
                    finished_ = true;
                    break;
                }
                else if (opstack_.back() == OP_OPEN_SQUARE)
                    lex.throw_syntax_error("mismatching '[' and ')'");
                else if (opstack_.back() == OP_TERNARY_MID)
                    lex.throw_syntax_error("mismatching '?' and ')'");
                // if we are here, opstack_.back() == OP_OPEN_ROUND
                opstack_.pop_back();

                // check if this is closing bracket of func()
                if (!opstack_.empty()) {
                    int top = opstack_.back();
                    if (is_function(top)) {
                        pop_onto_que();
                        int n = arg_cnt_.back() + 1;
                        int expected_n = get_function_narg(top);
                        if (n != expected_n)
                            lex.throw_syntax_error(
                                 "function " + function_name(top) + "expects "
                                 + S(expected_n) + " arguments, not " + S(n));
                        arg_cnt_.pop_back();
                        if (top==OP_FUNC || top==OP_SUM_F || top==OP_SUM_Z)
                            pop_onto_que(); // pop function index
                        else if (top==OP_NUMAREA || top==OP_FINDX ||
                                 top==OP_FIND_EXTR) {
                            pop_onto_que(); // pop OP_FUNC/OP_SUM_F/Z
                            pop_onto_que(); // pop function index
                        }
                    }
                    else if (top == OP_END_AGGREGATE) {
                        pop_onto_que();
                    }
                }

                expected_ = kOperator;
                break;

            case kTokenComma:
                pop_until_bracket();
                if (opstack_.empty()) {
                    finished_ = true;
                    break;
                }
                else if (opstack_.back() == OP_OPEN_SQUARE)
                    lex.throw_syntax_error("unexpected ',' after '['");
                else if (opstack_.back() == OP_TERNARY_MID)
                    lex.throw_syntax_error("unexpected ',' after '?'");
                // if we are here, opstack_.back() == OP_OPEN_ROUND
                else if (opstack_.size() < 2 ||
                         !is_function(*(opstack_.end() - 2)))
                    lex.throw_syntax_error("',' outside of function");
                else
                    // don't pop OP_OPEN_ROUND from the stack
                    ++ arg_cnt_.back();
                expected_ = kValue;
                break;

            case kTokenRSquare:
                pop_until_bracket();
                if (opstack_.empty()) {
                    finished_ = true;
                    break;
                }
                else if (opstack_.back() == OP_OPEN_ROUND)
                    lex.throw_syntax_error("mismatching '(' and ']'");
                else if (opstack_.back() == OP_TERNARY_MID)
                    lex.throw_syntax_error("mismatching '?' and ']'");
                // if we are here, opstack_.back() == OP_OPEN_SQUARE
                opstack_.pop_back();
                if (opstack_.empty() || !is_array_var(opstack_.back()))
                    lex.throw_syntax_error("[index] can be used only after "
                                           "x, y, s, a, X, Y, S or A.");
                pop_onto_que();
                expected_ = kOperator;
                break;

            case kTokenNop:
                finished_ = true;
                break;
            case kTokenPower:
                put_binary_op(OP_POW);
                break;
            case kTokenMult:
                put_binary_op(OP_MUL);
                break;
            case kTokenDiv:
                put_binary_op(OP_DIV);
                break;
            case kTokenPlus:
                if (expected_ == kOperator)
                    put_binary_op(OP_ADD);
                else
                    {} // do nothing for unary +
                break;
            case kTokenMinus:
                if (expected_ == kOperator)
                    put_binary_op(OP_SUB);
                else
                    put_unary_op(OP_NEG);
                break;
            case kTokenGT:
                put_binary_op(OP_GT);
                break;
            case kTokenGE:
                put_binary_op(OP_GE);
                break;
            case kTokenLT:
                put_binary_op(OP_LT);
                break;
            case kTokenLE:
                put_binary_op(OP_LE);
                break;
            case kTokenEQ:
                put_binary_op(OP_EQ);
                break;
            case kTokenNE:
                put_binary_op(OP_NEQ);
                break;
            case kTokenQMark:
                put_binary_op(OP_TERNARY_MID);
                code_.push_back(OP_TERNARY);
                break;
            case kTokenColon:
                for (;;) {
                    if (opstack_.empty()) {
                        finished_ = true;
                        break;
                    }
                    // pop OP_TERNARY_MID from the stack onto the que
                    int op = opstack_.back();
                    opstack_.pop_back();
                    code_.push_back(op);
                    if (op == OP_TERNARY_MID)
                        break;
                }
                if (!finished_)
                    put_binary_op(OP_AFTER_TERNARY);
                break;
            case kTokenVarname:
                put_variable_sth(lex, Lexer::get_string(token));
                break;
            case kTokenFuncname:
                put_func_sth(lex, Lexer::get_string(token));
                break;

            case kTokenString:
            case kTokenCname:
            case kTokenBang:
            case kTokenAppend:
            case kTokenAddAssign:
            case kTokenSubAssign:
            case kTokenDots:
            case kTokenPlusMinus:
            case kTokenLCurly:
            case kTokenRCurly:
            case kTokenAssign:
            case kTokenSemicolon:
            case kTokenDot:
            case kTokenTilde:
                finished_ = true;
                break;

            // these are never return by get_token()
            case kTokenFilename:
            case kTokenExpr:
            case kTokenRest:
                assert(0);
                break;
        }

        if (finished_ && token.type != kTokenNop)
                lex.go_back(token);
    }
    // the expression should not end with operator
    if (expected_ != kOperator)
        lex.throw_syntax_error("unexpected token or end of expression");

    // no more tokens to read
    pop_until_bracket(); // there should be no bracket
    if (!opstack_.empty())
        lex.throw_syntax_error("mismatching '" + S(opstack_.back()) + "'");
}

void ExpressionParser::push_assign_lhs(const Token& t)
{
    VMOp op;
    switch (toupper(*t.str)) {
        case 'X': op = OP_ASSIGN_X; break;
        case 'Y': op = OP_ASSIGN_Y; break;
        case 'S': op = OP_ASSIGN_S; break;
        case 'A': op = OP_ASSIGN_A; break;
        default: assert(0);
    }
    code_.push_back(op);
}

/*
// defined in datatrans.cpp
namespace datatrans {
bool execute_code(int n, int &M, vector<fp>& stack,
                  vector<Point> const& old_points, vector<Point>& new_points,
                  vector<int> const& code);
extern vector<fp> numbers;  //  VM data (numeric values)
}

double ExpressionParser::calculate_expression_value() const
{
    static vector<fp> stack(128);
    if (is_data_dependent_code(code_))
        throw ExecuteError("Expression depends on dataset.");
    vector<Point> dummy;
    // n==M => one-time op.
    int M = 0;
    datatrans::numbers = numbers;
    bool t = datatrans::execute_code(0, M, stack, dummy, dummy, code_);
    assert(!t);
    return stack.front();
}
*/


namespace {

vector<int>::const_iterator
skip_code(vector<int>::const_iterator i, int start_op, int finish_op)
{
    int counter = 1;
    while (counter) {
        ++i;
        if (*i == finish_op) counter--;
        else if (*i == start_op)  counter++;
    }
    return i;
}

template<typename T>
fp get_var_with_idx(fp idx, vector<Point> const& points, T Point::*t)
{
    if (points.empty())
        return 0.;
    else if (idx <= 0)
        return points[0].*t;
    else if (idx >= points.size() - 1)
        return points.back().*t;
    else if (is_eq(idx, iround(idx)))
        return points[iround(idx)].*t;
    else {
        int flo = int(floor(idx));
        fp fra = idx - flo;
        return (1-fra) * fp(points[flo].*t) + fra * fp(points[flo+1].*t);
    }
}

/// returns floating-point "index" of x in sorted vec of points
fp find_idx_in_sorted(vector<Point> const& pp, fp x)
{
    if (pp.empty())
        return 0;
    else if (x <= pp.front().x)
        return 0;
    else if (x >= pp.back().x)
        return pp.size() - 1;
    vector<Point>::const_iterator i = lower_bound(pp.begin(), pp.end(),
                                                  Point(x, 0));
    assert (i > pp.begin() && i < pp.end());
    if (is_eq(x, i->x))
            return i - pp.begin();
    else
        return i - pp.begin() - (i->x - x) / (i->x - (i-1)->x);
}

}

#define STACK_OFFSET_CHANGE(ch) stackPtr+=(ch)

inline void DataVM::run_const_op(vector<int>::const_iterator& i,
                                 double*& stackPtr,
                                 const int n,
                                 const vector<Point>& old_points,
                                 const vector<Point>& new_points) const
{
    switch (*i) {
        //unary-operators
        case OP_NEG:
            *stackPtr = - *stackPtr;
            break;
        case OP_SQRT:
            *stackPtr = sqrt(*stackPtr);
            break;
        case OP_GAMMA:
            *stackPtr = boost::math::tgamma(*stackPtr);
            break;
        case OP_LGAMMA:
            *stackPtr = boost::math::lgamma(*stackPtr);
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
        case OP_SIN:
            *stackPtr = sin(*stackPtr);
            break;
        case OP_COS:
            *stackPtr = cos(*stackPtr);
            break;
        case OP_TAN:
            *stackPtr = tan(*stackPtr);
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
        case OP_ATAN:
            *stackPtr = atan(*stackPtr);
            break;
        case OP_ASIN:
            *stackPtr = asin(*stackPtr);
            break;
        case OP_ACOS:
            *stackPtr = acos(*stackPtr);
            break;
        case OP_ABS:
            *stackPtr = fabs(*stackPtr);
            break;
        case OP_ROUND:
            *stackPtr = floor(*stackPtr + 0.5);
            break;

        case OP_XINDEX:
            *stackPtr = find_idx_in_sorted(old_points, *stackPtr);
            break;

#ifndef STANDALONE_DATATRANS
        case OP_FUNC:
            i++;
            *stackPtr = F_->get_function(*i)->calculate_value(*stackPtr);
            break;
        case OP_SUM_F:
            i++;
            *stackPtr = F_->get_model(*i)->value(*stackPtr);
            break;
        case OP_SUM_Z:
            i++;
            *stackPtr = F_->get_model(*i)->zero_shift(*stackPtr);
            break;
        case OP_NUMAREA:
            i += 2;
            STACK_OFFSET_CHANGE(-2);
            if (*(i-1) == OP_FUNC) {
                *stackPtr = F_->get_function(*i)->numarea(*stackPtr,
                                    *(stackPtr+1), iround(*(stackPtr+2)));
            }
            else if (*(i-1) == OP_SUM_F) {
                *stackPtr = F_->get_model(*i)->numarea(*stackPtr,
                                    *(stackPtr+1), iround(*(stackPtr+2)));
            }
            else // OP_SUM_Z
                throw ExecuteError("numarea(Z,...) is not implemented."
                                   "Does anyone need it?");
            break;

        case OP_FINDX:
            i += 2;
            STACK_OFFSET_CHANGE(-2);
            if (*(i-1) == OP_FUNC) {
                *stackPtr = F_->get_function(*i)->find_x_with_value(
                                  *stackPtr, *(stackPtr+1), *(stackPtr+2));
            }
            else if (*(i-1) == OP_SUM_F) {
                throw ExecuteError("findx(F,...) is not implemented. "
                                   "Does anyone need it?");
            }
            else // OP_SUM_Z
                throw ExecuteError("findx(Z,...) is not implemented. "
                                   "Does anyone need it?");
            break;

        case OP_FIND_EXTR:
            i += 2;
            STACK_OFFSET_CHANGE(-1);
            if (*(i-1) == OP_FUNC) {
                *stackPtr = F_->get_function(*i)->find_extremum(*stackPtr,
                                                            *(stackPtr+1));
            }
            else if (*(i-1) == OP_SUM_F) {
                throw ExecuteError("extremum(F,...) is not implemented. "
                                   "Does anyone need it?");
            }
            else // OP_SUM_Z
                throw ExecuteError("extremum(Z,...) is not implemented. "
                                   "Does anyone need it?");
            break;
#endif //not STANDALONE_DATATRANS

        //binary-operators
        case OP_MIN2:
            STACK_OFFSET_CHANGE(-1);
            *stackPtr = min(*stackPtr, *(stackPtr+1));
            break;
        case OP_MAX2:
            STACK_OFFSET_CHANGE(-1);
            *stackPtr = max(*stackPtr, *(stackPtr+1));
            break;
        case OP_RANDU:
            STACK_OFFSET_CHANGE(-1);
            *stackPtr = rand_uniform(*stackPtr, *(stackPtr+1));
            break;
        case OP_RANDNORM:
            STACK_OFFSET_CHANGE(-1);
            *stackPtr += rand_gauss() * *(stackPtr+1);
            break;
        case OP_ADD:
            STACK_OFFSET_CHANGE(-1);
            *stackPtr += *(stackPtr+1);
            break;
        case OP_SUB:
            STACK_OFFSET_CHANGE(-1);
            *stackPtr -= *(stackPtr+1);
            break;
        case OP_MUL:
            STACK_OFFSET_CHANGE(-1);
            *stackPtr *= *(stackPtr+1);
            break;
        case OP_DIV:
            STACK_OFFSET_CHANGE(-1);
            *stackPtr /= *(stackPtr+1);
            break;
        case OP_MOD:
            STACK_OFFSET_CHANGE(-1);
            *stackPtr -= floor(*stackPtr / *(stackPtr+1)) * *(stackPtr+1);
            break;
        case OP_POW:
            STACK_OFFSET_CHANGE(-1);
            *stackPtr = pow(*stackPtr, *(stackPtr+1));
            break;
        case OP_VOIGT:
            STACK_OFFSET_CHANGE(-1);
            *stackPtr = humlik(*stackPtr, *(stackPtr+1))
                                                           / sqrt(M_PI);
            break;

        // comparisions
        case OP_LT:
            STACK_OFFSET_CHANGE(-1);
            *stackPtr = is_lt(*stackPtr, *(stackPtr+1));
            break;
        case OP_GT:
            STACK_OFFSET_CHANGE(-1);
            *stackPtr = is_gt(*stackPtr, *(stackPtr+1));
            break;
        case OP_LE:
            STACK_OFFSET_CHANGE(-1);
            *stackPtr = is_le(*stackPtr, *(stackPtr+1));
            break;
        case OP_GE:
            STACK_OFFSET_CHANGE(-1);
            *stackPtr = is_ge(*stackPtr, *(stackPtr+1));
            break;
        case OP_EQ:
            STACK_OFFSET_CHANGE(-1);
            *stackPtr = is_eq(*stackPtr, *(stackPtr+1));
            break;
        case OP_NEQ:
            STACK_OFFSET_CHANGE(-1);
            *stackPtr = is_neq(*stackPtr, *(stackPtr+1));
            break;

        // putting-number-to-stack-operators
        case OP_NUMBER:
            STACK_OFFSET_CHANGE(+1);
            i++;
            *stackPtr = numbers_[*i];
            break;
        case OP_VAR_n:
            STACK_OFFSET_CHANGE(+1);
            *stackPtr = static_cast<fp>(n);
            break;
        case OP_VAR_M:
            STACK_OFFSET_CHANGE(+1);
            *stackPtr = static_cast<fp>(old_points.size());
            break;

        case OP_VAR_x:
            *stackPtr = get_var_with_idx(*stackPtr, old_points, &Point::x);
            break;
        case OP_VAR_y:
            *stackPtr = get_var_with_idx(*stackPtr, old_points, &Point::y);
            break;
        case OP_VAR_s:
            *stackPtr = get_var_with_idx(*stackPtr, old_points,
                                         &Point::sigma);
            break;
        case OP_VAR_a:
            *stackPtr = bool(iround(get_var_with_idx(*stackPtr, old_points,
                                                     &Point::is_active)));
            break;
        case OP_VAR_X:
            *stackPtr = get_var_with_idx(*stackPtr, new_points, &Point::x);
            break;
        case OP_VAR_Y:
            *stackPtr = get_var_with_idx(*stackPtr, new_points, &Point::y);
            break;
        case OP_VAR_S:
            *stackPtr = get_var_with_idx(*stackPtr, new_points,
                                         &Point::sigma);
            break;
        case OP_VAR_A:
            *stackPtr = bool(iround(get_var_with_idx(*stackPtr, new_points,
                                                     &Point::is_active)));
            break;

        // logical; can skip part of VM code !
        case OP_NOT:
            *stackPtr = is_eq(*stackPtr, 0.);
            break;
        case OP_AND:
            if (is_neq(*stackPtr, 0))    //return second
                stackPtr--;
            else              // return first
                i = skip_code(i, OP_AND, OP_AFTER_AND);
            break;

        case OP_OR:
            if (is_neq(*stackPtr, 0))    //return first
                i = skip_code(i, OP_OR, OP_AFTER_OR);
            else              // return second
                stackPtr--;
            break;

        case OP_TERNARY:
            if (! *stackPtr)
                i = skip_code(i, OP_TERNARY, OP_TERNARY_MID);
            stackPtr--;
            break;
        case OP_TERNARY_MID:
            //if we are here, condition was true. Skip.
            i = skip_code(i, OP_TERNARY_MID, OP_AFTER_TERNARY);
            break;

        case OP_AFTER_AND: //do nothing
        case OP_AFTER_OR:
        case OP_AFTER_TERNARY:
            break;

        // obsolete
        case OP_BEGIN:
        case OP_END:
        case OP_DO_ONCE:
        case OP_RESIZE:
        case OP_INDEX:
        default:
            //cerr << "Unknown operator in VM code: " << *i << endl;
            assert(0);
    }
}

inline void DataVM::run_mutab_op(vector<int>::const_iterator& i,
                                 double*& stackPtr,
                                 const int n,
                                 const vector<Point>& old_points,
                                 vector<Point>& new_points) const
{
    //cerr << "op " << dt_op(*i) << endl;
    switch (*i) {
        //assignment-operators
        case OP_ASSIGN_X:
            new_points[n].x = *stackPtr;
            STACK_OFFSET_CHANGE(-1);
            break;
        case OP_ASSIGN_Y:
            new_points[n].y = *stackPtr;
            STACK_OFFSET_CHANGE(-1);
            break;
        case OP_ASSIGN_S:
            new_points[n].sigma = *stackPtr;
            STACK_OFFSET_CHANGE(-1);
            break;
        case OP_ASSIGN_A:
            new_points[n].is_active = is_neq(*stackPtr, 0.);
            STACK_OFFSET_CHANGE(-1);
            break;
        default:
            run_const_op(i, stackPtr, n, old_points, new_points);
    }
}

void DataVM::transform_data(vector<Point>& points)
{
    if (points.empty())
        return;

    double stack[16];
    double* stackPtr = stack - 1; // will be ++'ed first
    vector<Point> new_points = points;

    // do time-consuming checking only for the first point
    for (vector<int>::const_iterator i = code_.begin(); i != code_.end(); i++) {
        run_mutab_op(i, stackPtr, 0, points, new_points);
        if (stackPtr - stack >= 16)
            throw ExecuteError("stack overflow");
    }
    assert(stackPtr == stack - 1); // ASSIGN_ op must be at the end

    for (int n = 1; n != size(points); ++n)
        for (vector<int>::const_iterator i=code_.begin(); i != code_.end(); i++)
            run_mutab_op(i, stackPtr, n, points, new_points);
    new_points = points;
}

double DataVM::calculate() const
{
    static const vector<Point> empty_vec;
    return calculate(0, empty_vec);
}

double DataVM::calculate(int n, const vector<Point>& points) const
{
    double stack[16];
    double* stackPtr = stack - 1; // will be ++'ed first
    for (vector<int>::const_iterator i = code_.begin(); i != code_.end(); i++) {
        run_const_op(i, stackPtr, n, points, points);
        if (stackPtr - stack >= 16)
            throw ExecuteError("stack overflow");
    }
    //cerr << "stackPtr: " << stackPtr - stack << endl;
    assert(stackPtr == stack); // no ASSIGN_ at the end
    return stack[0];
}


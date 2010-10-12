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

#include "lexer.h"
#include "common.h"
#include "datatrans.h"

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
    code.push_back(op);
}

void ExpressionParser::put_number(double value)
{
    if (expected_ == kOperator) {
        finished_ = true;
        return;
    }
    //cout << "put_number() " << value << endl;
    code.push_back(OP_NUMBER);
    int number_pos = numbers.size();
    code.push_back(number_pos);
    numbers.push_back(value);
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

void ExpressionParser::put_ag_function(VMOp op)
{
    //cout << "put_ag_function() " << op << endl;
    opstack_.push_back(OP_END_AGGREGATE);
    code.push_back(op);
    expected_ = kValue;
}

void ExpressionParser::put_array_var(Lexer& lex, VMOp op)
{
    if (lex.peek_token().type == kTokenLSquare) {
        opstack_.push_back(op);
        expected_ = kValue;
    }
    else {
        code.push_back(OP_VAR_n);
        code.push_back(op);
        expected_ = kOperator;
    }
}

void ExpressionParser::put_var(VMOp op)
{
    code.push_back(op);
    expected_ = kOperator;
}

void ExpressionParser::pop_until_bracket()
{
    while (!opstack_.empty()) {
        int op = opstack_.back();
        if (op == OP_OPEN_ROUND || op == OP_OPEN_SQUARE || op == OP_TERNARY_MID)
            break;
        opstack_.pop_back();
        code.push_back(op);
    }
}

//TODO:
//    OP_ASSIGN_X, OP_ASSIGN_Y, OP_ASSIGN_S, OP_ASSIGN_A,
//    OP_INDEX, OP_DO_ONCE, OP_RESIZE, OP_BEGIN, OP_END,
//
//    OP_FUNC, OP_SUM_F, OP_SUM_Z,
//
//    OP_NUMAREA, OP_FINDX, OP_FIND_EXTR


// implementation of the shunting-yard algorithm
void ExpressionParser::parse(Lexer& lex)
{
    code.clear();
    numbers.clear();
    opstack_.clear();
    finished_ = false;
    while (!finished_) {
        const Token token = lex.get_token();
        //cout << "> " << token2str(token) << endl;
        switch (token.type) {
            case kTokenNumber:
                put_number(token.info.number);
                break;
            case kTokenName: {
                string word = Lexer::get_string(token);
                // "not", "and", "or" can be followed by '(' or not.
                if (word == "not")
                    put_unary_op(OP_NOT);
                else if (word == "and") {
                    put_binary_op(OP_AFTER_AND);
                    code.push_back(OP_AND);
                }
                else if (word == "or") {
                    put_binary_op(OP_AFTER_OR);
                    code.push_back(OP_OR);
                }
                else if (word == "if") {
                    pop_until_bracket();
                    if (opstack_.empty())
                        finished_ = false;
                    else if (opstack_.back() == OP_OPEN_SQUARE)
                        lex.throw_syntax_error("unexpected 'if' after '['");
                    else if (opstack_.back() == OP_TERNARY_MID)
                        lex.throw_syntax_error("unexpected 'if' after '?'");
                    // if we are here, opstack_.back() == OP_OPEN_ROUND
                    else if (opstack_.size() < 2 ||
                             *(opstack_.end() - 2) != OP_END_AGGREGATE)
                        lex.throw_syntax_error(
                                "'if' outside of aggregate function");
                    else
                        // don't pop OP_OPEN_ROUND from the stack
                        code.push_back(OP_AGCONDITION);

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
                    else if (word == "sum")
                        put_ag_function(OP_AGSUM);
                    else if (word == "min")
                        put_ag_function(OP_AGMIN);
                    else if (word == "max")
                        put_ag_function(OP_AGMAX);
                    else if (word == "avg")
                        put_ag_function(OP_AGAVG);
                    else if (word == "stddev")
                        put_ag_function(OP_AGSTDDEV);
                    else if (word == "darea")
                        put_ag_function(OP_AGAREA);
                    else
                        lex.throw_syntax_error("unknown function: " + word);
                }
                else {
                    if (expected_ == kOperator) {
                        finished_ = true;
                        break;
                    }
                    if (word == "x")
                        put_array_var(lex, OP_VAR_x);
                    else if (word == "y")
                        put_array_var(lex, OP_VAR_y);
                    else if (word == "s")
                        put_array_var(lex, OP_VAR_s);
                    else if (word == "a")
                        put_array_var(lex, OP_VAR_a);
                    else if (word == "X")
                        put_array_var(lex, OP_VAR_X);
                    else if (word == "Y")
                        put_array_var(lex, OP_VAR_Y);
                    else if (word == "S")
                        put_array_var(lex, OP_VAR_S);
                    else if (word == "A")
                        put_array_var(lex, OP_VAR_A);
                    else if (word == "n")
                        put_var(OP_VAR_n);
                    else if (word == "M")
                        put_var(OP_VAR_M);
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
                if (opstack_.empty())
                    lex.throw_syntax_error("mismatching ')'");
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
                    }
                    else if (top == OP_END_AGGREGATE) {
                        pop_onto_que();
                    }
                }

                expected_ = kOperator;
                break;

            case kTokenComma:
                pop_until_bracket();
                if (opstack_.empty())
                    finished_ = true;
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
                if (opstack_.empty())
                    lex.throw_syntax_error("mismatching ']'");
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
                code.push_back(OP_TERNARY);
                break;
            case kTokenColon:
                for (;;) {
                    if (opstack_.empty())
                        lex.throw_syntax_error("':' doesn't match '?'");
                    // pop OP_TERNARY_MID from the stack onto the que
                    int op = opstack_.back();
                    opstack_.pop_back();
                    code.push_back(op);
                    if (op == OP_TERNARY_MID)
                        break;
                }
                put_binary_op(OP_AFTER_TERNARY);
                break;

            case kTokenString:
            case kTokenDataset:
            case kTokenVarname:
            case kTokenFuncname:
            case kTokenShell:
            case kTokenAppend:
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
    if (is_data_dependent_code(code))
        throw ExecuteError("Expression depends on dataset.");
    vector<Point> dummy;
    // n==M => one-time op.
    int M = 0;
    datatrans::numbers = numbers;
    bool t = datatrans::execute_code(0, M, stack, dummy, dummy, code);
    assert(!t);
    return stack.front();
}


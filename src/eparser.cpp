// data expression parser
//

#include "lexer.h"
#include "fityk.h" //SyntaxError
#include "datatrans.h"
//#include "datatrans2.h"

#include <iostream>
#include <cstring>

using namespace std;

namespace datatrans {
string get_code_as_text(vector<int> const& code, vector<fp> const& numbers);
}

class ExpressionParser
{
public:
    vector<int> code;    //  VM code
    vector<fp> numbers;  //  VM data (numeric values)
    void parse(Lexer& lex);
    std::string get_code_as_text() {
        return datatrans::get_code_as_text(code, numbers);
    }

private:
    vector<int> opstack; // operator stack

    void put_number(double value);
    void put_unary_op(int op);
    void put_binary_op(int op);
    void put_function(int op);
    void put_array_var(Lexer& lex, int op);
    void put_var(int op);
    void pop_onto_que();
};

namespace {

//vector<datatrans::ParameterizedFunction*> parameterized; // also used by VM

/// operators used in VM code
enum DataTransformVMOperator
{
    OP_NEG=-200, OP_EXP, OP_ERFC, OP_ERF, OP_SIN, OP_COS,  OP_TAN,
    OP_SINH, OP_COSH, OP_TANH, OP_ABS,  OP_ROUND,
    OP_ATAN, OP_ASIN, OP_ACOS, OP_LOG10, OP_LN,  OP_SQRT,  OP_POW,
    OP_GAMMA, OP_LGAMMA, OP_VOIGT,
    OP_ADD,   OP_SUB,   OP_MUL,   OP_DIV,  OP_MOD,
    OP_MIN2,   OP_MAX2, OP_RANDNORM, OP_RANDU,
    OP_VAR_X, OP_VAR_FIRST_OP=OP_VAR_X, OP_VAR_Y, OP_VAR_S, OP_VAR_A,
    OP_VAR_x, OP_VAR_y, OP_VAR_s, OP_VAR_a,
    OP_VAR_n, OP_VAR_M, OP_VAR_LAST_OP=OP_VAR_M,
    OP_NUMBER,
    OP_OR, OP_AFTER_OR, OP_AND, OP_AFTER_AND, OP_NOT,
    OP_TERNARY, OP_TERNARY_MID, OP_AFTER_TERNARY, OP_DELETE_COND,
    OP_GT, OP_GE, OP_LT, OP_LE, OP_EQ, OP_NEQ, OP_NCMP_HACK,
    OP_RANGE, OP_INDEX, OP_x_IDX,
    OP_ASSIGN_X, OP_ASSIGN_Y, OP_ASSIGN_S, OP_ASSIGN_A,
    OP_DO_ONCE, OP_RESIZE, OP_ORDER, OP_DELETE, OP_BEGIN, OP_END,
    OP_END_AGGREGATE, OP_AGCONDITION,
    OP_AGSUM, OP_AGMIN, OP_AGMAX, OP_AGAREA, OP_AGAVG, OP_AGSTDDEV,
    OP_PARAMETERIZED, OP_PLIST_BEGIN, OP_PLIST_SEP, OP_PLIST_END,
    OP_FUNC, OP_SUM_F, OP_SUM_Z, OP_NUMAREA, OP_FINDX, OP_FIND_EXTR
};

/// parametrized functions
enum {
    PF_INTERPOLATE, PF_SPLINE
};

}

int get_op_priority(int op)
{
    switch (op) {
        case OP_NOT: return 10;
        case OP_POW: return 9;
        case OP_NEG: return 8;
        case OP_MUL: return 7;
        case OP_DIV: return 7;
        case OP_MOD: return 7;
        case OP_ADD: return 6;
        case OP_SUB: return 6;
        case OP_GT: return 3;
        case OP_GE: return 3;
        case OP_LT: return 3;
        case OP_LE: return 3;
        case OP_EQ: return 3;
        case OP_NEQ: return 3;
        case OP_AFTER_AND: return 2;
        case OP_AFTER_OR: return 1;
        default: return 0;
    }
}



bool is_function(int op)
{
    switch (op) {
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
            return true;
        default:
            return false;
    }
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
    int op = opstack.back();
    opstack.pop_back();
    code.push_back(op);
}

void ExpressionParser::put_number(double value)
{
    //cout << "put_number() " << value << endl;
    code.push_back(OP_NUMBER);
    int number_pos = numbers.size();
    code.push_back(number_pos);
    numbers.push_back(value);
}

void ExpressionParser::put_unary_op(int op)
{
    opstack.push_back(op);
}

void ExpressionParser::put_binary_op(int op)
{
    //cout << "put_binary_op() " << op << endl;
    int pri = get_op_priority(op);
    while (!opstack.empty()) {
        int op2 = opstack.back();
        int pri2 = get_op_priority(op2);
        if (pri2 < pri)
            break;
        opstack.pop_back();
        code.push_back(op2);
    }
    opstack.push_back(op);
}

void ExpressionParser::put_function(int op)
{
    //cout << "put_function() " << op << endl;
    opstack.push_back(op);
}

void ExpressionParser::put_array_var(Lexer& lex, int op)
{
    if (lex.peek_token().type != kTokenLSquare) {
        code.push_back(OP_VAR_n);
        code.push_back(op);
    }
    else
        opstack.push_back(op);
}

void ExpressionParser::put_var(int op)
{
    code.push_back(op);
}

//TODO:
//    OP_NEG,
//    OP_VOIGT,
//    OP_MOD,
//    OP_MIN2, OP_MAX2, OP_RANDNORM, OP_RANDU,
//    OP_TERNARY, OP_TERNARY_MID, OP_AFTER_TERNARY, OP_DELETE_COND,
//    OP_NCMP_HACK,
//    OP_RANGE, OP_INDEX, OP_x_IDX,
//    OP_ASSIGN_X, OP_ASSIGN_Y, OP_ASSIGN_S, OP_ASSIGN_A,
//    OP_DO_ONCE, OP_RESIZE, OP_ORDER, OP_DELETE, OP_BEGIN, OP_END,
//    OP_END_AGGREGATE, OP_AGCONDITION,
//    OP_AGSUM, OP_AGMIN, OP_AGMAX, OP_AGAREA, OP_AGAVG, OP_AGSTDDEV,
//    OP_PARAMETERIZED, OP_PLIST_BEGIN, OP_PLIST_SEP, OP_PLIST_END,
//    OP_FUNC, OP_SUM_F, OP_SUM_Z, OP_NUMAREA, OP_FINDX, OP_FIND_EXTR


// implementation of the shunting-yard algorithm
void ExpressionParser::parse(Lexer& lex)
{
    code.clear();
    numbers.clear();
    opstack.clear();
    bool finished = false;
    while (!finished) {
        Token token = lex.get_token();
        switch (token.type) {
            case kTokenNumber:
                put_number(token.info.number);
                break;
            case kTokenName: {
                string word = Lexer::get_string(token);
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
                else if (word == "not")
                    put_unary_op(OP_NOT);
                else if (word == "and") {
                    put_binary_op(OP_AFTER_AND);
                    code.push_back(OP_AND);
                }
                else if (word == "or") {
                    put_binary_op(OP_AFTER_OR);
                    code.push_back(OP_OR);
                }
                else if (word == "sqrt")
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
                else {
                    if (lex.peek_token().type == kTokenOpen)
                        lex.throw_syntax_error("unknown function: " + word);
                    else
                        lex.throw_syntax_error("unknown name: " + word);
                }
                break;
            }
            case kTokenOpen:
                opstack.push_back('(');
                break;
            case kTokenLSquare:
                opstack.push_back('[');
                break;
            case kTokenClose:
                for (;;) {
                    if (opstack.empty())
                        lex.throw_syntax_error("mismatching ')'");
                    int op = opstack.back();
                    opstack.pop_back();
                    if (op == '(')
                        break;
                    code.push_back(op);
                }
                if (!opstack.empty() && is_function(opstack.back())) {
                    pop_onto_que();
                }
                break;
            case kTokenRSquare:
                for (;;) {
                    if (opstack.empty())
                        lex.throw_syntax_error("mismatching ']'");
                    int op = opstack.back();
                    opstack.pop_back();
                    if (op == '[')
                        break;
                    code.push_back(op);
                }
                if (opstack.empty() || !is_array_var(opstack.back()))
                    lex.throw_syntax_error("[index] can be used only after "
                                           "x, y, s, a, X, Y, S or A.");
                pop_onto_que();
                break;
            case kTokenEOL:
                finished = true;
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
                put_binary_op(OP_ADD);
                break;
            case kTokenMinus:
                put_binary_op(OP_SUB);
                // TODO
                // put_operator(OP_NEG);
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
            case kTokenComma:
            case kTokenSemicolon:
            case kTokenDot:
            case kTokenColon:
            case kTokenTilde:
            case kTokenQMark:
                lex.throw_syntax_error("unexpected token");
                break;
        }
    }
    // no more tokens to read
    while (!opstack.empty()) {
        int op = opstack.back();
        opstack.pop_back();
        if (op == '(')
            lex.throw_syntax_error("mismatching '('");
        if (op == '[')
            lex.throw_syntax_error("mismatching '['");
        code.push_back(op);
    }
}


int main(int argc, char **argv)
{
    if (argc != 2)
        return 1;
    string old_text = get_trans_repr(argv[1]);
    cout << "old: " << old_text << endl;
    cout << "new: ";
    Lexer lex(argv[1]);
    try {
        ExpressionParser parser;
        parser.parse(lex);
        string new_text = parser.get_code_as_text();
        cout << new_text << endl;
        cout << (old_text == new_text ? "same" : "NOT same") << endl;
    }
    catch (fityk::SyntaxError& e) {
        cout << "ERROR: " << e.what() << endl;
    }
    return 0;
}


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
    enum ExpectedType
    {
        kValue,
        kOperator,
    };

    vector<int> code;    //  VM code
    vector<fp> numbers;  //  VM data (numeric values)

    ExpressionParser()
        : expected_(kValue) {}

    void parse(Lexer& lex);
    std::string get_code_as_text() {
        return datatrans::get_code_as_text(code, numbers);
    }

private:
    vector<int> opstack_; // operator stack
    ExpectedType expected_; // used to give more meaningful error messages

    void put_number(double value);
    void put_unary_op(int op);
    void put_binary_op(int op);
    void put_function(int op);
    void put_ag_function(int op);
    void put_array_var(Lexer& lex, int op);
    void put_var(int op);
    void pop_onto_que();
};

namespace {

/// operators used in VM code
enum DataTransformVMOperator
{
    OP_NEG=-200, OP_EXP, OP_ERFC, OP_ERF, OP_SIN, OP_COS,  OP_TAN,
    OP_SINH, OP_COSH, OP_TANH, OP_ABS,  OP_ROUND,
    OP_ATAN, OP_ASIN, OP_ACOS, OP_LOG10, OP_LN,  OP_SQRT,  OP_POW,
    OP_GAMMA, OP_LGAMMA, OP_VOIGT, OP_XINDEX,
    OP_ADD,   OP_SUB,   OP_MUL,   OP_DIV,  OP_MOD,
    OP_MIN2,   OP_MAX2, OP_RANDNORM, OP_RANDU,
    OP_VAR_X, OP_VAR_FIRST_OP=OP_VAR_X, OP_VAR_Y, OP_VAR_S, OP_VAR_A,
    OP_VAR_x, OP_VAR_y, OP_VAR_s, OP_VAR_a,
    OP_VAR_n, OP_VAR_M, OP_VAR_LAST_OP=OP_VAR_M,
    OP_NUMBER,
    OP_OR, OP_AFTER_OR, OP_AND, OP_AFTER_AND, OP_NOT,
    OP_TERNARY, OP_TERNARY_MID, OP_AFTER_TERNARY,
    OP_GT, OP_GE, OP_LT, OP_LE, OP_EQ, OP_NEQ,
    OP_INDEX,
    OP_ASSIGN_X, OP_ASSIGN_Y, OP_ASSIGN_S, OP_ASSIGN_A,
    OP_DO_ONCE, OP_RESIZE, OP_BEGIN, OP_END,
    OP_END_AGGREGATE, OP_AGCONDITION,
    OP_AGSUM, OP_AGMIN, OP_AGMAX, OP_AGAREA, OP_AGAVG, OP_AGSTDDEV,
    OP_FUNC, OP_SUM_F, OP_SUM_Z, OP_NUMAREA, OP_FINDX, OP_FIND_EXTR
};

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
    //cout << "put_number() " << value << endl;
    code.push_back(OP_NUMBER);
    int number_pos = numbers.size();
    code.push_back(number_pos);
    numbers.push_back(value);
    expected_ = kOperator;
}

void ExpressionParser::put_unary_op(int op)
{
    opstack_.push_back(op);
    expected_ = kValue;
}

void ExpressionParser::put_binary_op(int op)
{
    //cout << "put_binary_op() " << op << endl;
    int pri = get_op_priority(op);
    while (!opstack_.empty() && get_op_priority(opstack_.back()) >= pri)
        pop_onto_que();
    opstack_.push_back(op);
    expected_ = kValue;
}

void ExpressionParser::put_function(int op)
{
    //cout << "put_function() " << op << endl;
    opstack_.push_back(0); // argument counter
    opstack_.push_back(op);
    expected_ = kValue;
}

void ExpressionParser::put_ag_function(int op)
{
    //cout << "put_ag_function() " << op << endl;
    opstack_.push_back(OP_END_AGGREGATE);
    code.push_back(op);
    expected_ = kValue;
}

void ExpressionParser::put_array_var(Lexer& lex, int op)
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

void ExpressionParser::put_var(int op)
{
    code.push_back(op);
    expected_ = kOperator;
}

//TODO:
//    OP_INDEX, OP_XINDEX,
//    OP_ASSIGN_X, OP_ASSIGN_Y, OP_ASSIGN_S, OP_ASSIGN_A,
//    OP_DO_ONCE, OP_RESIZE, OP_BEGIN, OP_END,
//
//    OP_FUNC, OP_SUM_F, OP_SUM_Z, OP_NUMAREA, OP_FINDX, OP_FIND_EXTR


// implementation of the shunting-yard algorithm
void ExpressionParser::parse(Lexer& lex)
{
    code.clear();
    numbers.clear();
    opstack_.clear();
    bool finished = false;
    while (!finished) {
        Token token = lex.get_token();
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
                    for (;;) {
                        if (opstack_.empty())
                            lex.throw_syntax_error("unexpected 'if'");
                        // don't pop '(' from the stack
                        int op = opstack_.back();
                        if (op == '(')
                            break;
                        else if (op == '[')
                            lex.throw_syntax_error("unexpected 'if' after '['");
                        else if (op == OP_TERNARY_MID)
                            lex.throw_syntax_error("unexpected 'if' after '?'");
                        opstack_.pop_back();
                        code.push_back(op);
                    }
                    if (opstack_.size() < 2 ||
                            *(opstack_.end() - 2) != OP_END_AGGREGATE)
                        lex.throw_syntax_error(
                                "'if' outside of aggregate function");
                    code.push_back(OP_AGCONDITION);
                }
                else if (lex.peek_token().type == kTokenOpen) {
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
                opstack_.push_back('(');
                expected_ = kValue;
                break;
            case kTokenLSquare:
                opstack_.push_back('[');
                expected_ = kValue;
                break;
            case kTokenClose:
                for (;;) {
                    if (opstack_.empty())
                        lex.throw_syntax_error("mismatching ')'");
                    int op = opstack_.back();
                    opstack_.pop_back();
                    if (op == '(')
                        break;
                    else if (op == '[')
                        lex.throw_syntax_error("mismatching '[' or ')'");
                    else if (op == OP_TERNARY_MID)
                        lex.throw_syntax_error("mismatching '?' or ')'");
                    code.push_back(op);
                }

                if (!opstack_.empty()) {
                    int top = opstack_.back();
                    if (is_function(top)) {
                        pop_onto_que();
                        int narg = opstack_.back() + 1;
                        int expected_narg = get_function_narg(top);
                        if (narg != expected_narg)
                            lex.throw_syntax_error( "function "
                                    + function_name(top) + "expects "
                                    + S(expected_narg) + " arguments, not "
                                    + S(narg));
                        opstack_.pop_back(); // remove arg count
                    }
                    else if (top == OP_END_AGGREGATE) {
                        pop_onto_que();
                    }
                }
                expected_ = kOperator;
                break;
            case kTokenComma:
                for (;;) {
                    if (opstack_.empty())
                        lex.throw_syntax_error("unexpected ','");
                    // unlike in kTokenClose, don't pop '(' from the stack
                    int op = opstack_.back();
                    if (op == '(')
                        break;
                    else if (op == '[')
                        lex.throw_syntax_error("unexpected ',' after '['");
                    else if (op == OP_TERNARY_MID)
                        lex.throw_syntax_error("unexpected ',' after '?'");
                    opstack_.pop_back();
                    code.push_back(op);
                }
                if (opstack_.size() < 3 || !is_function(*(opstack_.end() - 2)))
                    lex.throw_syntax_error("',' outside of function");
                ++ *(opstack_.end() - 3);
                break;
            case kTokenRSquare:
                for (;;) {
                    if (opstack_.empty())
                        lex.throw_syntax_error("mismatching ']'");
                    int op = opstack_.back();
                    opstack_.pop_back();
                    if (op == '[')
                        break;
                    else if (op == '(')
                        lex.throw_syntax_error("mismatching '('");
                    else if (op == OP_TERNARY_MID)
                        lex.throw_syntax_error("mismatching '?'");
                    code.push_back(op);
                }
                if (opstack_.empty() || !is_array_var(opstack_.back()))
                    lex.throw_syntax_error("[index] can be used only after "
                                           "x, y, s, a, X, Y, S or A.");
                pop_onto_que();
                expected_ = kOperator;
                break;
            case kTokenEOL:
                if (expected_ != kOperator)
                    lex.throw_syntax_error("unexpected end of expression");
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
                lex.throw_syntax_error("unexpected token");
                break;
        }
    }
    // no more tokens to read
    while (!opstack_.empty()) {
        int op = opstack_.back();
        opstack_.pop_back();
        if (op == '(')
            lex.throw_syntax_error("mismatching '('");
        else if (op == '[')
            lex.throw_syntax_error("mismatching '['");
        else if (op == OP_TERNARY_MID)
            lex.throw_syntax_error("mismatching '?'");
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
    lex.set_mode(Lexer::kMath);
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


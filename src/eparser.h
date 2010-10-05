// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
// $Id: $

// data expression parser

#ifndef FITYK_EPARSER_H_
#define FITYK_EPARSER_H_

#include <vector>
#include <string>

class Lexer;

namespace dataVM {

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

} // namespace

bool is_data_dependent_code(const std::vector<int>& code);


class ExpressionParser
{
public:
    enum ExpectedType
    {
        kValue,
        kOperator,
    };

    std::vector<int> code;    //  VM code
    std::vector<double> numbers;  //  VM data (numeric values)

    // contains variables (including expressions like @0.F[1].height.error)
    // and names of custom functions (e.g. %foo or F or @0.F[3]).
    // In resolve_names() the variables are replaced by numbers,
    // and function names by indices.
    std::vector<std::string> names;

    ExpressionParser() : expected_(kValue), finished_(false) {}

    void parse(Lexer& lex);
    //void resolve_names(...);

private:
    std::vector<int> opstack_; // operator stack for the shunting-yard algorithm
    ExpectedType expected_;
    bool finished_;

    void put_number(double value);
    void put_unary_op(int op);
    void put_binary_op(int op);
    void put_function(int op);
    void put_ag_function(int op);
    void put_array_var(Lexer& lex, int op);
    void put_var(int op);
    void pop_onto_que();
};

#endif // FITYK_EPARSER_H_


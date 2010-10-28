// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
// $Id: $

/// This parser is not used yet.
/// In the future it will replace the current parser (datatrans* files)
/// Data expression parser.

#ifndef FITYK_EPARSER_H_
#define FITYK_EPARSER_H_

#include <vector>
#include <string>

#include "fityk.h" // struct Point
using fityk::Point;

class Lexer;
class Ftk;

namespace dataVM {

/// operators used in VM code
enum VMOp
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
    OP_FUNC, OP_SUM_F, OP_SUM_Z, OP_NUMAREA, OP_FINDX, OP_FIND_EXTR,

    // these two are not VM operators, but are handy to have here
    // and are used in implementation of shunting yard algorithm
    OP_OPEN_ROUND, OP_OPEN_SQUARE
};

} // namespace

std::string dt_op(int op);

std::string get_code_as_text(std::vector<int> const& code,
                             std::vector<double> const& numbers);

bool is_data_dependent_code(const std::vector<int>& code);


class DataVM
{
public:
    DataVM(const Ftk* F) : F_(F) {}

    /// calculate value of expression that does not depend on dataset
    //double calculate_expression_value() const;

    /// calculate value of expression that does not depend on dataset
    double calculate() const;
    /// calculate value of expression that may depend on dataset
    double calculate(int n, const std::vector<Point>& points) const;
    /// transform data (X=..., Y=..., S=..., A=...)
    void transform_data(std::vector<Point>& points);

    std::string list_ops() const { return get_code_as_text(code_, numbers_); }

protected:
    const Ftk* F_;
    std::vector<int> code_;    //  VM code
    std::vector<double> numbers_;  //  VM data (numeric values)

private:
    inline
    void run_mutab_op(std::vector<int>::const_iterator& i, double*& stackPtr,
                      const int n, const std::vector<Point>& old_points,
                      std::vector<Point>& new_points) const;
    inline
    void run_const_op(std::vector<int>::const_iterator& i, double*& stackPtr,
                      const int n, const std::vector<Point>& old_points,
                      const std::vector<Point>& new_points) const;
};

class ExpressionParser : public DataVM
{
public:
    enum ExpectedType
    {
        kValue,
        kOperator,
    };

    // if F is NULL, $variables, %functions, etc. are not handled
    ExpressionParser(const Ftk* F) : DataVM(F), expected_(kValue),
                                     finished_(false) {}

    /// parse expression
    void parse2vm(Lexer& lex, int default_ds);

private:
    // operator stack for the shunting-yard algorithm
    std::vector<dataVM::VMOp> opstack_;
    // argument counters for functions
    std::vector<int> arg_cnt_;
    // expected type of the next token (basic shunting-yard algorithm parses
    // e.g. two numbers in sequence, like "1 2", ignores the the first one
    // and doesn't signal error. The expected_ flag is used to stop parsing
    // such input just after getting token from lexer)
    ExpectedType expected_;
    // We set this to true when the next token is not part of the expression.
    // Exception is not thrown, because ExpressionParser can be used as
    // sub-parser from Parser.
    bool finished_;

    void put_number(double value);
    void put_unary_op(dataVM::VMOp op);
    void put_binary_op(dataVM::VMOp op);
    void put_function(dataVM::VMOp op);
    void put_ag_function(dataVM::VMOp op);
    void put_array_var(Lexer& lex, dataVM::VMOp op);
    void put_var(dataVM::VMOp op);
    void pop_onto_que();
    void pop_until_bracket();
};

#endif // FITYK_EPARSER_H_


// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
// $Id: $

/// Data expression parser.

#ifndef FITYK_EPARSER_H_
#define FITYK_EPARSER_H_

#include <vector>
#include <string>

#include "fityk.h" // struct Point
using fityk::Point;

struct Token;
class Lexer;
class Ftk;

namespace dataVM {

/// operators used in VM code
// obsolete: OP_INDEX, OP_DO_ONCE, OP_RESIZE, OP_BEGIN, OP_END
enum Op
{
    OP_NEG=-200, OP_EXP, OP_ERFC, OP_ERF, OP_SIN, OP_COS,  OP_TAN,
    OP_SINH, OP_COSH, OP_TANH, OP_ABS,  OP_ROUND,
    OP_ATAN, OP_ASIN, OP_ACOS, OP_LOG10, OP_LN,  OP_SQRT,  OP_POW,
    OP_GAMMA, OP_LGAMMA, OP_VOIGT, OP_XINDEX,
    OP_ADD,   OP_SUB,   OP_MUL,   OP_DIV,  OP_MOD,
    OP_MIN2,   OP_MAX2, OP_RANDNORM, OP_RANDU,
    OP_VAR_X, OP_VAR_Y, OP_VAR_S, OP_VAR_A,
    OP_VAR_x, OP_VAR_y, OP_VAR_s, OP_VAR_a,
    OP_VAR_n, OP_VAR_M,
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
    OP_CUSTOM,

    // these two are not VM operators, but are handy to have here
    // and are used in implementation of shunting yard algorithm
    OP_OPEN_ROUND, OP_OPEN_SQUARE
};

} // namespace dataVM

/// base class used to implement aggregate functions (sum, min, avg, etc.)
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


/// handles VM data and provides low-level access to it
class VirtualMachineData
{
public:
    const std::vector<int>& code() const { return code_; }
    const std::vector<double>& numbers() const { return numbers_; }

    void append_code(int op) { code_.push_back(op); }
    void append_number(double d);
    void clear_data() { code_.clear(); numbers_.clear(); }
private:
    std::vector<int> code_;    //  VM code
    std::vector<double> numbers_;  //  VM data (numeric values)
};


class ExprCalculator
{
public:
    ExprCalculator(const Ftk* F) : F_(F) {}

    /// calculate value of expression that may depend on dataset
    double calculate(int n, const std::vector<Point>& points) const;

    /// calculate value of expression that does not depend on dataset
    double calculate() const { return calculate(0, std::vector<Point>()); }

    /// calculate value of expression that was parsed with custom_vars set,
    /// the values in custom_val should correspond to names in custom_vars
    double calculate_custom(const std::vector<double>& custom_val) const;

    /// transform data (X=..., Y=..., S=..., A=...)
    void transform_data(std::vector<Point>& points);

protected:
    const Ftk* F_;
    VirtualMachineData vm_;

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

/// Expression parser.
/// Derived from ExpressionParser only because it is handy to keep both
/// in one class. 
class ExpressionParser : public ExprCalculator
{
public:
    enum ExpectedType
    {
        kOperator,
        kValue,
        kIndex,
    };

    // if F is NULL, $variables, %functions, etc. are not handled
    ExpressionParser(const Ftk* F) : ExprCalculator(F), expected_(kValue),
                                     finished_(false) {}

    /// reset state
    void clear_vm() { vm_.clear_data(); }

    /// parse expression
    void parse_expr(Lexer& lex, int default_ds,
                    const std::vector<std::string> *custom_vars=NULL,
                    std::vector<std::string> *new_vars=NULL);

    // does not throw; returns true if all string is parsed
    bool parse_full(Lexer& lex, int default_ds,
                    const std::vector<std::string> *custom_vars=NULL);

    /// adds OP_ASSIGN_? to the code
    void push_assign_lhs(const Token& t);

    // debugging utility
    std::string list_ops() const;

private:
    // operator stack for the shunting-yard algorithm
    std::vector<int> opstack_;

    // argument counters for functions
    // TODO: use opstack_ for arg_cnt_
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
    void put_unary_op(dataVM::Op op);
    void put_binary_op(dataVM::Op op);
    void put_function(dataVM::Op op);
    void put_ag_function(Lexer& lex, int ds, AggregFunc& ag);
    void put_value_from_curly(Lexer& lex, int ds);
    void put_array_var(bool has_index, dataVM::Op op);
    void put_var(dataVM::Op op);
    void put_name(Lexer& lex, const std::string& word,
                  const std::vector<std::string>* custom_vars,
                  std::vector<std::string>* new_vars);
    void put_variable_sth(Lexer& lex, const std::string& name);
    void put_func_sth(Lexer& lex, const std::string& name);
    void put_fz_sth(Lexer& lex, char fz, int ds);

    void pop_onto_que();
    void pop_until_bracket();
};

#endif // FITYK_EPARSER_H_


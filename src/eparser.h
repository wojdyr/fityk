// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

/// Data expression parser.

#ifndef FITYK_EPARSER_H_
#define FITYK_EPARSER_H_

#include <vector>
#include <string>

#include "vm.h"

struct Token;
class Lexer;

/// base class used to implement aggregate functions (sum, min, avg, etc.)
class AggregFunc
{
public:
    AggregFunc() : counter_(0), v_(0.) {}
    virtual ~AggregFunc() {}
    // x - expression value, n - index of the point in Data::p_
    void put(double x, int n) { ++counter_; op(x, n); }
    virtual double value() const { return v_; }

protected:
    int counter_;
    double v_;

    virtual void op(double x, int n) = 0;
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
                    std::vector<std::string> *new_vars=NULL,
                    bool ast_mode=false);

    // does not throw; returns true if all string is parsed
    bool parse_full(Lexer& lex, int default_ds,
                    const std::vector<std::string> *custom_vars=NULL);

    /// adds OP_ASSIGN_? to the code
    void push_assign_lhs(const Token& t);

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
    void put_unary_op(Op op);
    void put_binary_op(Op op);
    void put_function(Op op);
    void put_ag_function(Lexer& lex, int ds, AggregFunc& ag);
    void put_value_from_curly(Lexer& lex, int ds);
    void put_array_var(bool has_index, Op op);
    void put_name(Lexer& lex, const std::string& word,
                  const std::vector<std::string>* custom_vars,
                  std::vector<std::string>* new_vars,
                  bool ast_mode);
    void put_variable_sth(Lexer& lex, const std::string& name, bool ast_mode);
    void put_func_sth(Lexer& lex, const std::string& name, bool ast_mode);
    void put_fz_sth(Lexer& lex, char fz, int ds, bool ast_mode);

    void pop_onto_que();
    void pop_until_bracket();
};

#endif // FITYK_EPARSER_H_


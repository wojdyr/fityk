// This file is part of fityk program. Copyright 2001-2013 Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

/// Data expression parser.

#define BUILDING_LIBFITYK
#include "eparser.h"

#include <cstring>
#include <cmath>

#include "lexer.h"
#include "common.h"

#include "logic.h"
#include "data.h"
#include "fit.h" // $var.error
#include "var.h" // $v
#include "func.h" // %f(...)
#include "model.h" // F(...)

using std::string;
using std::vector;
using std::min;
using std::max;

namespace {

using namespace fityk;

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

const char* function_name(int op)
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
        case OP_DT_SUM_SAME_X: return "sum_same_x";
        case OP_DT_AVG_SAME_X: return "avg_same_x";
        case OP_DT_SHIRLEY_BG: return "shirley_bg";
        // 2-args functions
        case OP_MOD: return "mod";
        case OP_MIN2: return "min2";
        case OP_MAX2: return "max2";
        case OP_VOIGT: return "voigt";
        case OP_DVOIGT_DX: return "dvoigt_dx";
        case OP_DVOIGT_DY: return "dvoigt_dy";
        case OP_RANDNORM: return "randnormal";
        case OP_RANDU: return "randuniform";
        // Fityk functions
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
        case OP_DT_SUM_SAME_X:
        case OP_DT_AVG_SAME_X:
        case OP_DT_SHIRLEY_BG:
            return 1;
        // 2-args functions
        case OP_MOD:
        case OP_MIN2:
        case OP_MAX2:
        case OP_VOIGT:
        case OP_DVOIGT_DX:
        case OP_DVOIGT_DY:
        case OP_RANDNORM:
        case OP_RANDU:
            return 2;
        // Fityk functions
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
    return get_function_narg(op) != 0;
}

bool is_array_var(int op)
{
    switch (op) {
        case OP_Px:
        case OP_Py:
        case OP_Ps:
        case OP_Pa:
        case OP_PX:
        case OP_PY:
        case OP_PS:
        case OP_PA:
            return true;
        default:
            return false;
    }
}


class AggregSum : public AggregFunc
{
protected:
    virtual void op(double x, int)
    {
        v_ += x;
    }
};

class AggregCount : public AggregFunc
{
protected:
    virtual void op(double x, int)
    {
        if (fabs(x) >= 0.5)
            v_ += 1;
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

class AggregArgMin : public AggregFunc
{
public:
    AggregArgMin(const vector<Point>& points) : points_(points) {}
protected:
    virtual void op(double x, int n)
    {
        if (counter_ == 1 || x < min_) {
            min_ = x;
            v_ = points_[n].x;
        }
    }
private:
    double min_;
    const vector<Point>& points_;
};

class AggregArgMax : public AggregFunc
{
public:
    AggregArgMax(const vector<Point>& points) : points_(points) {}
protected:
    virtual void op(double x, int n)
    {
        if (counter_ == 1 || x > max_) {
            max_ = x;
            v_ = points_[n].x;
        }
    }
private:
    double max_;
    const vector<Point>& points_;
};

class AggregDArea : public AggregFunc
{
public:
    AggregDArea(const vector<Point>& points) : points_(points) {}
protected:
    virtual void op(double x, int n)
    {
        int M = points_.size();
        double dx = (points_[min(n+1, M-1)].x - points_[max(n-1, 0)].x) / 2.;
        v_ += x * dx;
    }
private:
    const vector<Point>& points_;
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

} // anonymous namespace

namespace fityk {

void ExpressionParser::pop_onto_que()
{
    int op = opstack_.back();
    opstack_.pop_back();
    vm_.append_code(op);
}

void ExpressionParser::put_number(double value)
{
    if (expected_ == kOperator) {
        finished_ = true;
        return;
    }
    //cout << "put_number() " << value << endl;
    vm_.append_number(value);
    expected_ = kOperator;
}

void ExpressionParser::put_unary_op(Op op)
{
    if (expected_ == kOperator) {
        finished_ = true;
        return;
    }
    opstack_.push_back(op);
    expected_ = kValue;
}

void ExpressionParser::put_binary_op(Op op)
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

void ExpressionParser::put_function(Op op)
{
    //cout << "put_function() " << op << endl;
    opstack_.push_back(0); // argument counter
    opstack_.push_back(op);
    expected_ = kValue;
}

void ExpressionParser::put_ag_function(Lexer& lex, int ds, AggregFunc& ag)
{
    //cout << "put_ag_function() " << op << endl;
    lex.get_expected_token(kTokenOpen); // discard '('
    ExpressionParser ep(F_);
    ep.parse_expr(lex, ds);
    const vector<Point>& points = F_->dk.data(ds)->points();
    Token t = lex.get_expected_token(kTokenClose, "if");
    if (t.type == kTokenClose) {
        for (size_t n = 0; n != points.size(); ++n) {
            double x = ep.calculate(n, points);
            ag.put(x, n);
        }
    }
    else { // "if"
        ExpressionParser cond_p(F_);
        cond_p.parse_expr(lex, ds);
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

void ExpressionParser::put_value_from_curly(Lexer& lex, int ds)
{
    ExpressionParser ep(F_);
    ep.parse_expr(lex, ds);
    lex.get_expected_token(kTokenRCurly); // discard '}'
    double x = ep.calculate(0, F_->dk.data(ds)->points());
    put_number(x);
}

void ExpressionParser::put_array_var(bool has_index, Op op)
{
    if (has_index) {
        opstack_.push_back(op);
        expected_ = kIndex;
    }
    else {
        vm_.append_code(OP_Pn);
        vm_.append_code(op);
        expected_ = kOperator;
    }
}

void ExpressionParser::put_variable_sth(Lexer& lex, const string& name,
                                        bool ast_mode)
{
    if (F_ == NULL)
        lex.throw_syntax_error("$variables can not be used here");
    const Variable *v = F_->mgr.find_variable(name);
    if (lex.peek_token().type == kTokenDot) {
        lex.get_token(); // discard '.'
        lex.get_expected_token("error"); // discard "error"
        double e = F_->fit_manager()->get_standard_error(v);
        if (e == -1.)
            lex.throw_syntax_error("unknown error of $" + v->name
                                  + "; it is not simple variable");
        put_number(e);
    }
    else {
        if (ast_mode) {
            int n = F_->mgr.find_variable_nr(name);
            vm_.append_code(OP_SYMBOL);
            vm_.append_code(n);
            expected_ = kOperator;
        }
        else
            put_number(v->value());
    }
}

void ExpressionParser::put_func_sth(Lexer& lex, const string& name,
                                    bool ast_mode)
{
    if (F_ == NULL)
        lex.throw_syntax_error("%functions can not be used here");
    if (lex.peek_token().type == kTokenOpen) {
        int n = F_->mgr.find_function_nr(name);
        if (n == -1)
            throw ExecuteError("undefined function: %" + name);
        // we will put n into code when handling ')'
        opstack_.push_back(n);
        put_function(OP_FUNC);
    }
    else if (lex.peek_token().type == kTokenDot) {
        lex.get_token(); // discard '.'
        Token arg = lex.get_expected_token(kTokenLname, kTokenCname);
        string word = arg.as_string();
        if (arg.type == kTokenCname) {
            const Function *f = F_->mgr.find_function(name);
            double val = f->get_param_value(word);
            put_number(val);
        }
        else if (lex.peek_token().type == kTokenOpen) { // method of %function
            int n = F_->mgr.find_function_nr(name);
            if (n == -1)
                throw ExecuteError("undefined function: %" + name);
            // we will put ds into code when handling ')'
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
            const Function *f = F_->mgr.find_function(name);
            string v = f->used_vars().get_name(f->get_param_nr(word));
            put_variable_sth(lex, v, ast_mode);
        }
    }
    else
        lex.throw_syntax_error("expected '.' or '(' after %function");
}

void ExpressionParser::put_fz_sth(Lexer& lex, char fz, int ds, bool ast_mode)
{
    if (F_ == NULL || ds < 0)
        lex.throw_syntax_error("F/Z can not be used here");
    if (lex.peek_token().type == kTokenLSquare) {
        lex.get_token(); // discard '['
        ExpressionParser ep(F_);
        ep.parse_expr(lex, ds);
        lex.get_expected_token(kTokenRSquare); // discard ']'
        int idx = iround(ep.calculate());
        const string& name = F_->dk.get_model(ds)->get_func_name(fz, idx);
        put_func_sth(lex, name, ast_mode);
    }
    else if (lex.peek_token().type == kTokenOpen) {
        opstack_.push_back(ds); // we will put ds into code when handling ')'
        put_function(fz == 'F' ? OP_SUM_F : OP_SUM_Z);
    }
    else if (lex.peek_token().type == kTokenDot) {
        lex.get_token(); // discard '.'
        string word = lex.get_expected_token(kTokenLname).as_string();
        if (lex.peek_token().type != kTokenOpen)
            lex.throw_syntax_error("F/Z has no .properties, only .methods()");
        // we will put ds into code when handling ')'
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

void ExpressionParser::put_name(Lexer& lex,
                                const string& word,
                                const vector<string>* custom_vars,
                                vector<string>* new_vars,
                                bool ast_mode)
{
    if (word == "pi") {
        put_number(M_PI);
        return;
    }
    if (word == "true") {
        put_number(1.);
        return;
    }
    if (word == "false") {
        put_number(0.);
        return;
    }

    if (ast_mode && word == "x") {
        vm_.append_code(OP_X);
        expected_ = kOperator;
        return;
    }

    if (custom_vars != NULL) {
        int idx = index_of_element(*custom_vars, word);
        if (idx != -1) {
            vm_.append_code(OP_SYMBOL);
            vm_.append_code(idx);
            expected_ = kOperator;
            return;
        }
    }

    if (new_vars != NULL) {
        int idx = index_of_element(*new_vars, word);
        if (idx == -1) {
            idx = new_vars->size();
            new_vars->push_back(word);
        }
        vm_.append_code(OP_SYMBOL);
        // new_vars is to be appended to custom_vars later
        int cv_len = custom_vars != NULL ? (int) custom_vars->size() : 0;
        vm_.append_code(cv_len + idx);
        expected_ = kOperator;
        return;
    }


    if (custom_vars == NULL && new_vars == NULL && !ast_mode) { // data points
        bool has_index = (lex.peek_token().type == kTokenLSquare);
        if (word.size() == 1 && (word[0] == 'x' || word[0] == 'y' ||
                        word[0] == 's' || word[0] == 'a' || word[0] == 'n')) {
            if (word[0] == 'x')
                put_array_var(has_index, OP_Px);
            else if (word[0] == 'y')
                put_array_var(has_index, OP_Py);
            else if (word[0] == 's')
                put_array_var(has_index, OP_Ps);
            else if (word[0] == 'a')
                put_array_var(has_index, OP_Pa);
            else if (word[0] == 'n') {
                vm_.append_code(OP_Pn);
                expected_ = kOperator;
            }
            return;
        }
    }

    lex.throw_syntax_error("unknown name: " + word);
}

void ExpressionParser::pop_until_bracket()
{
    while (!opstack_.empty()) {
        int op = opstack_.back();
        if (op == OP_OPEN_ROUND || op == OP_OPEN_SQUARE || op == OP_TERNARY_MID)
            break;
        opstack_.pop_back();
        vm_.append_code(op);
    }
}

bool ExpressionParser::parse_full(Lexer& lex, int default_ds,
                                  const vector<string> *custom_vars)
{
    try {
        parse_expr(lex, default_ds, custom_vars);
    }
    catch (...) {
        return false;
    }
    return lex.peek_token().type == kTokenNop;
}

// implementation of the shunting-yard algorithm
void ExpressionParser::parse_expr(Lexer& lex, int default_ds,
                                  const vector<string> *custom_vars,
                                  vector<string> *new_vars,
                                  ParseMode mode)
{
    opstack_.clear();
    finished_ = false;
    expected_ = kValue;
    if (F_ != NULL && default_ds >= F_->dk.count())
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
                    vm_.append_code(OP_AND);
                }
                else if (word == "or") {
                    put_binary_op(OP_AFTER_OR);
                    vm_.append_code(OP_OR);
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
                    else if (word == "count") {
                        AggregCount ag;
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
                    else if (word == "argmin") {
                        AggregArgMin ag(F_->dk.data(default_ds)->points());
                        put_ag_function(lex, default_ds, ag);
                    }
                    else if (word == "argmax") {
                        AggregArgMax ag(F_->dk.data(default_ds)->points());
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
                        AggregDArea ag(F_->dk.data(default_ds)->points());
                        put_ag_function(lex, default_ds, ag);
                    }

                    // dataset functions
                    else if (mode == kDatasetTrMode && word == "sum_same_x")
                        put_function(OP_DT_SUM_SAME_X);
                    else if (mode == kDatasetTrMode && word == "avg_same_x")
                        put_function(OP_DT_AVG_SAME_X);
                    else if (mode == kDatasetTrMode && word == "shirley_bg")
                        put_function(OP_DT_SHIRLEY_BG);

                    else
                        lex.throw_syntax_error("unknown function: " + word);
                }
                else {
                    if (expected_ == kOperator) {
                        finished_ = true;
                        break;
                    }
                    put_name(lex, word, custom_vars, new_vars, mode==kAstMode);
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
                    put_array_var(has_index, OP_PX);
                else if (*token.str == 'Y')
                    put_array_var(has_index, OP_PY);
                else if (*token.str == 'S')
                    put_array_var(has_index, OP_PS);
                else if (*token.str == 'A')
                    put_array_var(has_index, OP_PA);
                else if (*token.str == 'M') {
                    vm_.append_code(OP_PM);
                    expected_ = kOperator;
                }
                else if (*token.str == 'F' || *token.str == 'Z') {
                    put_fz_sth(lex, *token.str, default_ds, mode==kAstMode);
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
                if (lex.peek_token().type == kTokenDot) {
                    lex.get_token(); // discard '.'
                    Token t = lex.get_expected_token(kTokenUletter);
                    if (*t.str == 'F' || *t.str == 'Z') {
                        put_fz_sth(lex, *t.str, token.value.i, mode==kAstMode);
                    }
                    else
                        lex.throw_syntax_error("unknown name: " +
                                               token.as_string());
                }
                else {
                    if (mode != kDatasetTrMode)
                        lex.get_expected_token(kTokenDot);
                    int n = token.value.i;
                    if (n == Lexer::kAll || n == Lexer::kNew)
                        lex.throw_syntax_error("@*/@+ not allowed at RHS");
                    vm_.append_code(OP_DATASET);
                    vm_.append_code(n);
                    expected_ = kOperator;
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
                if (expected_ != kIndex) {
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
                        int n = opstack_.back() + 1;
                        opstack_.pop_back();
                        int expected_n = get_function_narg(top);
                        if (n != expected_n)
                            lex.throw_syntax_error(
                               S("function ") + function_name(top) + " expects "
                               + S(expected_n) + " arguments, not " + S(n));
                        if (top==OP_FUNC || top==OP_SUM_F || top==OP_SUM_Z)
                            pop_onto_que(); // pop function index
                        else if (top==OP_NUMAREA || top==OP_FINDX ||
                                 top==OP_FIND_EXTR) {
                            pop_onto_que(); // pop OP_FUNC/OP_SUM_F/Z
                            pop_onto_que(); // pop function index
                        }
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
                else if (opstack_.size() < 3 ||
                         !is_function(*(opstack_.end() - 2)))
                    lex.throw_syntax_error("',' outside of function");
                else
                    // don't pop OP_OPEN_ROUND from the stack
                    ++ *(opstack_.end() - 3);
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
                else if (lex.peek_token().type == kTokenNumber) {
                    // In '-3', '-3+5', '-3*5', 5*-3', etc. '-3' is parsed
                    // as a number. The exception is '-3^5', where '-3'
                    // is parsed as OP_NEG and number, because '^' has higher
                    // precedence than '-'
                    Token num = lex.get_token();
                    if (lex.peek_token().type != kTokenPower) {
                        put_number(-num.value.d);
                    }
                    else {
                        put_unary_op(OP_NEG);
                        put_number(num.value.d);
                    }
                }
                else
                    put_unary_op(OP_NEG);
                break;
            case kTokenGT:
                // This token can be outside of the expression.
                // We handle one special case: '>' followed by string,
                // to allow "print x, y > 'filename'".
                if (lex.peek_token().type == kTokenString)
                    finished_ = true;
                else
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
                // special case, for handling SplitFunction that has
                // ... expr ":" TypeName ...
                if (lex.peek_token().type == kTokenCname) {
                    finished_ = true;
                    break;
                }
                put_binary_op(OP_TERNARY_MID);
                vm_.append_code(OP_TERNARY);
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
                    vm_.append_code(op);
                    if (op == OP_TERNARY_MID)
                        break;
                }
                if (!finished_)
                    put_binary_op(OP_AFTER_TERNARY);
                break;
            case kTokenVarname:
                put_variable_sth(lex, Lexer::get_string(token), mode==kAstMode);
                break;
            case kTokenFuncname:
                put_func_sth(lex, Lexer::get_string(token), mode==kAstMode);
                break;

            case kTokenTilde:
                if (expected_ == kOperator)
                    lex.throw_syntax_error("unexpected `~'");
                vm_.append_code(OP_TILDE);
                break;

            case kTokenLCurly:
                put_value_from_curly(lex, default_ds);
                break;

            case kTokenString:
            case kTokenCname:
            case kTokenBang:
            case kTokenAppend:
            case kTokenAddAssign:
            case kTokenSubAssign:
            case kTokenDots:
            case kTokenPlusMinus:
            case kTokenRCurly:
            case kTokenAssign:
            case kTokenSemicolon:
            case kTokenDot:
                finished_ = true;
                break;

            // these are never return by get_token()
            case kTokenFilename:
            case kTokenExpr:
            case kTokenEVar:
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
        lex.throw_syntax_error("mismatching bracket");
}

void ExpressionParser::push_assign_lhs(const Token& t)
{
    Op op;
    switch (toupper(*t.str)) {
        case 'X': op = OP_ASSIGN_X; break;
        case 'Y': op = OP_ASSIGN_Y; break;
        case 'S': op = OP_ASSIGN_S; break;
        case 'A': op = OP_ASSIGN_A; break;
        default: assert(0);
    }
    vm_.append_code(op);
}

} // namespace fityk

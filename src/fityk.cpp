// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

/// implementation of libfityk public API

#include <cassert>
#include <cctype>
#include "fityk.h"
#include "common.h"
#include "ui.h"
#include "logic.h"
#include "data.h"
#include "model.h"
#include "fit.h"
#include "func.h"
#include "info.h"

using namespace std;

#define CATCH_EXECUTE_ERROR \
    catch (ExecuteError& e) { \
        last_error_ = string("ExecuteError: ") + e.what(); \
        if (throws_) \
            throw; \
    }

#define CATCH_SYNTAX_ERROR \
    catch (SyntaxError& e) { \
        last_error_ = string("SyntaxError: ") + e.what(); \
        if (throws_) \
            throw; \
    }

namespace {

fityk::t_show_message *simple_message_handler = 0;

FILE* message_sink = 0;

// a bridge between fityk::set_show_message()
// and UserInterface::set_show_message()
void message_handler(UserInterface::Style style, string const& s)
{
    if (simple_message_handler && style != UserInterface::kInput)
        (*simple_message_handler)(s);
}

void message_redir(UserInterface::Style style, string const& s)
{
    if (message_sink && style != UserInterface::kInput)
        fprintf(message_sink, "%s\n", s.c_str());
}


realt get_wssr_or_ssr(Ftk const* ftk, int dataset, bool weigthed)
{
    if (dataset == fityk::all_datasets) {
        realt result = 0;
        for (int i = 0; i < ftk->get_dm_count(); ++i)
            result += Fit::compute_wssr_for_data(ftk->get_dm(i), weigthed);
        return result;
    }
    else {
        return Fit::compute_wssr_for_data(ftk->get_dm(dataset), weigthed);
    }
}


vector<DataAndModel*> get_datasets_(Ftk* ftk, int dataset)
{
    vector<DataAndModel*> dd;
    if (dataset == fityk::all_datasets) {
        for (int i = 0; i < ftk->get_dm_count(); ++i)
            dd.push_back(ftk->get_dm(i));
    }
    else {
        dd.push_back(ftk->get_dm(dataset));
    }
    return dd;
}

} //anonymous namespace

namespace fityk
{

Point::Point() : x(0), y(0), sigma(1), is_active(true) {}
Point::Point(realt x_, realt y_) : x(x_), y(y_), sigma(1), is_active(true) {}
Point::Point(realt x_, realt y_, realt sigma_) : x(x_), y(y_), sigma(sigma_),
                                                is_active(true) {}
string Point::str() const { return "(" + S(x) + "; " + S(y) + "; " +
                                 S(sigma) + (is_active ? ")*" : ") "); }


Fityk::Fityk()
    : throws_(true), owns_(true)
{
    ftk_ = new Ftk;
}

Fityk::Fityk(Ftk* F)
    : throws_(true), owns_(false)
{
    ftk_ = F;
}

Fityk::~Fityk()
{
    if (owns_)
        delete ftk_;
}

void Fityk::execute(string const& s)  throw(SyntaxError, ExecuteError,
                                            ExitRequestedException)
{
    // TODO: execute_line() doesn't throw, wrap it differently
    try {
        ftk_->get_ui()->raw_execute_line(s);
    }
    CATCH_SYNTAX_ERROR
    CATCH_EXECUTE_ERROR
}

string Fityk::get_info(string const& s, int dataset)  throw(SyntaxError,
                                                            ExecuteError)
{
    try {
        Lexer lex(s.c_str());
        Parser parser(ftk_);
        vector<Token> args;
        parser.parse_info_args(lex, args);
        if (lex.peek_token().type != kTokenNop)
            lex.throw_syntax_error("unexpected argument");
        string result;
        eval_info_args(ftk_, dataset, args, args.size(), result);
        return result;
    }
    CATCH_SYNTAX_ERROR
    CATCH_EXECUTE_ERROR
    return "";
}

realt Fityk::calculate_expr(string const& s, int dataset)  throw(SyntaxError,
                                                                  ExecuteError)
{
    try {
        Lexer lex(s.c_str());
        ExpressionParser ep(ftk_);
        ep.parse_expr(lex, dataset);
        return ep.calculate(0, ftk_->get_data(dataset)->points());
    }
    CATCH_SYNTAX_ERROR
    CATCH_EXECUTE_ERROR
    return 0.;
}

int Fityk::get_dataset_count()
{
    return ftk_->get_dm_count();
}

realt Fityk::get_model_value(realt x, int dataset)  throw(ExecuteError)
{
    try {
        return ftk_->get_model(dataset)->value(x);
    }
    CATCH_EXECUTE_ERROR
    return 0.;
}

vector<realt> Fityk::get_model_vector(vector<realt> const& x, int dataset)
                                                          throw(ExecuteError)
{
    vector<realt> xx(x);
    vector<realt> yy(x.size(), 0.);
    try {
        ftk_->get_model(dataset)->compute_model(xx, yy);
    }
    CATCH_EXECUTE_ERROR
    return yy;
}

int Fityk::get_variable_nr(string const& name)  throw(ExecuteError)
{
    try {
        if (name.empty())
            throw ExecuteError("get_variable_nr() called with empty name");
        string vname;
        if (name[0] == '$')
            vname = string(name, 1);
        else if (name[0] == '%' && name.find('.') < name.size() - 1) {
            string::size_type pos = name.find('.');
            Function const* f = ftk_->find_function(string(1, pos-1));
            string pname = name.substr(pos+1);
            vname = f->get_var_name(f->get_param_nr(pname));
        }
        else
            vname = name;
        return ftk_->find_variable(vname)->get_nr();
    }
    CATCH_EXECUTE_ERROR
    return 0;
}

void Fityk::load_data(int dataset,
                      vector<realt> const& x,
                      vector<realt> const& y,
                      vector<realt> const& sigma,
                      string const& title)     throw(ExecuteError)
{
    try {
        ftk_->get_data(dataset)->load_arrays(x, y, sigma, title);
    }
    CATCH_EXECUTE_ERROR
}

void Fityk::add_point(realt x, realt y, realt sigma, int dataset)
                                                          throw(ExecuteError)
{
    try {
        ftk_->get_data(dataset)->add_one_point(x, y, sigma);
    }
    CATCH_EXECUTE_ERROR
}

vector<Point> const& Fityk::get_data(int dataset)  throw(ExecuteError)
{
    static const vector<Point> empty;
    try {
        return ftk_->get_data(dataset)->points();
    }
    CATCH_EXECUTE_ERROR
    return empty;
}


void Fityk::set_show_message(t_show_message *func)
{
    simple_message_handler = func;
    ftk_->get_ui()->set_show_message(message_handler);
}

void Fityk::redir_messages(FILE *stream)
{
    message_sink = stream;
    ftk_->get_ui()->set_show_message(message_redir);
}

void Fityk::out(string const& s) const
{
    ftk_->get_ui()->output_message(UserInterface::kNormal, s);
}

realt Fityk::get_wssr(int dataset)  throw(ExecuteError)
{
    try {
        return get_wssr_or_ssr(ftk_, dataset, true);
    }
    CATCH_EXECUTE_ERROR
    return 0.;
}

realt Fityk::get_ssr(int dataset)  throw(ExecuteError)
{
    try {
        return get_wssr_or_ssr(ftk_, dataset, false);
    }
    CATCH_EXECUTE_ERROR
    return 0.;
}

realt Fityk::get_rsquared(int dataset)  throw(ExecuteError)
{
    try {
        if (dataset == fityk::all_datasets) {
            realt result = 0;
            for (int i = 0; i < ftk_->get_dm_count(); ++i)
                result += Fit::compute_r_squared_for_data(ftk_->get_dm(i),
                                                          NULL, NULL);
            return result;
        }
        else {
            return Fit::compute_r_squared_for_data(ftk_->get_dm(dataset),
                                                   NULL, NULL);
        }
    }
    CATCH_EXECUTE_ERROR
    return 0.;
}

int Fityk::get_dof(int dataset)  throw(ExecuteError)
{
    try {
        return ftk_->get_fit()->get_dof(get_datasets_(ftk_, dataset));
    }
    CATCH_EXECUTE_ERROR
    return 0;
}

vector<vector<realt> > Fityk::get_covariance_matrix(int dataset)
                                                           throw(ExecuteError)
{
    try {
        vector<DataAndModel*> dss = get_datasets_(ftk_, dataset);
        vector<realt> c = ftk_->get_fit()->get_covariance_matrix(dss);
        //reshape
        size_t na = ftk_->parameters().size();
        assert(c.size() == na * na);
        vector<vector<realt> > r(na);
        for (size_t i = 0; i != na; ++i)
            r[i] = vector<realt>(c.begin() + i*na, c.begin() + i*(na+1));
        return r;
    }
    CATCH_EXECUTE_ERROR
    return vector<vector<realt> >();
}

} //namespace fityk


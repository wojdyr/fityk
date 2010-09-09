// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
// $Id$

/// implementation of public API of libfityk, declared in fityk.h

#include <cassert>
#include <cctype>
#include "fityk.h"
#include "common.h"
#include "ui.h"
#include "logic.h"
#include "data.h"
#include "model.h"
#include "fit.h"
#include "cmd.h"
#include "func.h"
#include "info.h"

using namespace std;

#define CATCH_EXECUTE_ERROR \
    catch (ExecuteError& e) { \
        last_error_ = string("ExecuteError: ") + e.what(); \
        if (throws_) \
            throw; \
    }

namespace {

fityk::t_show_message *simple_message_handler = 0;

FILE* message_sink = 0;

// a bridge between fityk::set_show_message()
// and UserInterface::set_show_message()
void message_handler(UserInterface::Style style, std::string const& s)
{
    if (simple_message_handler && style != UserInterface::kInput)
        (*simple_message_handler)(s);
}

void message_redir(UserInterface::Style style, std::string const& s)
{
    if (message_sink && style != UserInterface::kInput)
        fprintf(message_sink, "%s\n", s.c_str());
}


double get_wssr_or_ssr(Ftk const* ftk, int dataset, bool weigthed)
{
    if (dataset == fityk::all_datasets) {
        double result = 0;
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
Point::Point(double x_, double y_) : x(x_), y(y_), sigma(1), is_active(true) {}
Point::Point(double x_, double y_, fp sigma_) : x(x_), y(y_), sigma(sigma_),
                                                is_active(true) {}
std::string Point::str() const { return "(" + S(x) + "; " + S(y) + "; " +
                                 S(sigma) + (is_active ? ")*" : ") "); }


Fityk::Fityk()
    : throws_(true)
{
    if (AL != 0)
        throw ExecuteError("Program is not thread-safe yet, "
                            "so you can only have one Fityk instance.");
    ftk_ = new Ftk;
    AL = ftk_;
}

Fityk::~Fityk()
{
    delete ftk_;
    AL = 0;
}

void Fityk::execute(string const& s)  throw(SyntaxError, ExecuteError,
                                            ExitRequestedException)
{
    try {
        bool r = parse_and_execute_e(s);
        if (!r) {
            last_error_ = "SyntaxError";
            if (throws_)
                throw SyntaxError();
        }
    }
    CATCH_EXECUTE_ERROR
}

bool Fityk::safe_execute(string const& s)  throw(ExitRequestedException)
{
    return ftk_->exec(s) == Commands::status_ok;
}

string Fityk::get_info(string const& s, bool full)
                                             throw(SyntaxError, ExecuteError)
{
    try {
        string result;
        size_t end = get_info_string(ftk_, s, full, result);
        if (end < s.size()) // not all the string was parsed
            throw SyntaxError();
        return result;
    } catch (ExecuteError& e) {
        if (startswith(e.what(), "Syntax error")) {
            last_error_ = "SyntaxError";
            if (throws_)
                throw SyntaxError();
        }
        else {
            last_error_ = string("ExecuteError: ") + e.what();
            if (throws_)
                throw;
        }
        return "";
    }
}

int Fityk::get_dataset_count()
{
    return ftk_->get_dm_count();
}

double Fityk::get_model_value(double x, int dataset)  throw(ExecuteError)
{
    try {
        return ftk_->get_model(dataset)->value(x);
    }
    CATCH_EXECUTE_ERROR
    return 0.;
}

vector<double> Fityk::get_model_vector(vector<double> const& x, int dataset)
                                                          throw(ExecuteError)
{
    vector<double> xx(x);
    vector<double> yy(x.size(), 0.);
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

double Fityk::get_variable_value(string const& name)  throw(ExecuteError)
{
    try {
        if (name.empty())
            throw ExecuteError("get_variable_value() called with empty name");
        if (name[0] == '$')
            return ftk_->find_variable(string(name, 1))->get_value();
        else if (name[0] == '%' && name.find('.') < name.size() - 1) {
            string::size_type pos = name.find('.');
            Function const* f = ftk_->find_function(string(name, 1, pos-1));
            return f->get_param_value(string(name, pos+1));
        }
        else
            return ftk_->find_variable(name)->get_value();
    }
    CATCH_EXECUTE_ERROR
    return 0.;
}

void Fityk::load_data(int dataset,
                      std::vector<double> const& x,
                      std::vector<double> const& y,
                      std::vector<double> const& sigma,
                      std::string const& title)     throw(ExecuteError)
{
    try {
        ftk_->get_data(dataset)->load_arrays(x, y, sigma, title);
    }
    CATCH_EXECUTE_ERROR
}

void Fityk::add_point(double x, double y, double sigma, int dataset)
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

void Fityk::redir_messages(std::FILE *stream)
{
    message_sink = stream;
    ftk_->get_ui()->set_show_message(message_redir);
}

double Fityk::get_wssr(int dataset)  throw(ExecuteError)
{
    try {
        return get_wssr_or_ssr(ftk_, dataset, true);
    }
    CATCH_EXECUTE_ERROR
    return 0.;
}

double Fityk::get_ssr(int dataset)  throw(ExecuteError)
{
    try {
        return get_wssr_or_ssr(ftk_, dataset, false);
    }
    CATCH_EXECUTE_ERROR
    return 0.;
}

double Fityk::get_rsquared(int dataset)  throw(ExecuteError)
{
    try {
        if (dataset == fityk::all_datasets) {
            double result = 0;
            for (int i = 0; i < ftk_->get_dm_count(); ++i)
                result += Fit::compute_r_squared_for_data(ftk_->get_dm(i));
            return result;
        }
        else {
            return Fit::compute_r_squared_for_data(ftk_->get_dm(dataset));
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

vector<vector<double> > Fityk::get_covariance_matrix(int dataset)
                                                           throw(ExecuteError)
{
    try {
        vector<DataAndModel*> dss = get_datasets_(ftk_, dataset);
        vector<double> c = ftk_->get_fit()->get_covariance_matrix(dss);
        //reshape
        size_t na = ftk_->get_parameters().size();
        assert(c.size() == na * na);
        vector<vector<double> > r(na);
        for (size_t i = 0; i != na; ++i)
            r[i] = vector<double>(c.begin() + i*na, c.begin() + i*(na+1));
        return r;
    }
    CATCH_EXECUTE_ERROR
    return vector<vector<double> >();
}

} //namespace fityk


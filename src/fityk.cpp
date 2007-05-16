// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License version 2
// $Id$

/// implementation of public API of libfityk, declared in fityk.h

#include <cassert>
#include <cctype>
#include <locale.h>
#include "fityk.h"
#include "common.h"
#include "ui.h"
#include "logic.h"
#include "data.h"
#include "sum.h"
#include "fit.h"
#include "cmd.h"
#include "func.h"

using namespace std;

namespace {

fityk::t_show_message *simple_message_handler = 0;

FILE* message_sink = 0;

void message_handler(OutputStyle style, std::string const& s)
{
    if (simple_message_handler && style != os_input)
        (*simple_message_handler)(s);
}

void message_redir(OutputStyle style, std::string const& s)
{
    if (message_sink && style != os_input)
        fprintf(message_sink, "%s\n", s.c_str());
}


void check_valid_dataset(int /*dataset*/)
{
    // do nothing, let throwing exception
    //assert(dataset >= 0);
    //assert(dataset < AL->get_ds_count());
}

double get_wssr_or_ssr(int dataset, bool weigthed)
{
    if (dataset == fityk::all_ds) {
        double result = 0;
        for (int i = 0; i < AL->get_ds_count(); ++i)
            result += Fit::compute_wssr_for_data(AL->get_ds(i), weigthed);
        return result;
    }
    else {
        check_valid_dataset(dataset);
        return Fit::compute_wssr_for_data(AL->get_ds(dataset), weigthed);
    }
}


vector<DataWithSum*> get_datasets_(int dataset)
{
    vector<DataWithSum*> dd;
    if (dataset == fityk::all_ds) {
        for (int i = 0; i < AL->get_ds_count(); ++i)
            dd.push_back(AL->get_ds(i));
    }
    else {
        check_valid_dataset(dataset);
        dd.push_back(AL->get_ds(dataset));
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
std::string Point::str() { return "(" + S(x) + "; " + S(y) + "; " + S(sigma) 
                               + (is_active ? ")*" : ") "); }


void initialize()
{
    setlocale(LC_NUMERIC, "C");
    if (AL != 0) 
        delete AL;
    AL = new Fityk; 
}


void execute(string const& s)  throw(SyntaxError, ExecuteError, 
                                     ExitRequestedException)
{
    bool r = parse_and_execute_e(s);
    if (!r)
        throw SyntaxError();
}

bool safe_execute(string const& s)  throw(ExitRequestedException)
{
    return AL->exec(s) == Commands::status_ok; 
}

string get_info(string const& s, bool full)  throw(SyntaxError, ExecuteError)
{
    try {
        return get_info_string(s, full);
    } catch (ExecuteError& e) {
        if (startswith(e.what(), "Syntax error"))
            throw SyntaxError();
        else
            throw;
    }
}

int get_dataset_count()
{
    return AL->get_ds_count();
}

double get_sum_value(double x, int dataset)  throw(ExecuteError)
{
    check_valid_dataset(dataset);
    return AL->get_sum(dataset)->value(x);
}

vector<double> get_sum_vector(vector<double> const& x, int dataset)  
                                                          throw(ExecuteError)
{
    check_valid_dataset(dataset);
    vector<double> xx(x);
    vector<double> yy(x.size(), 0.);
    AL->get_sum(dataset)->calculate_sum_value(xx, yy);
    return yy;
}

int get_variable_nr(string const& name)  throw(ExecuteError) 
{
    if (name.empty())
        throw ExecuteError("get_variable_nr() called with empty name");
    string vname;
    if (name[0] == '$')
        vname = string(name, 1);
    else if (name[0] == '%' && name.find('.') < name.size() - 1) {
        string::size_type pos = name.find('.');
        Function const* f = AL->find_function(string(1, pos-1));
        vname = f->get_param_varname(string(name, pos+1));
    }
    else
        vname = name;
    return AL->find_variable(vname)->get_nr();
}

double get_variable_value(string const& name)  throw(ExecuteError)
{
    if (name.empty())
        throw ExecuteError("get_variable_value() called with empty name");
    if (name[0] == '$')
        return AL->find_variable(string(name, 1))->get_value();
    else if (name[0] == '%' && name.find('.') < name.size() - 1) {
        string::size_type pos = name.find('.');
        Function const* f = AL->find_function(string(name, 1, pos-1));
        return f->get_param_value(string(name, pos+1));
    }
    else
        return AL->find_variable(name)->get_value();
}

void load_data(int dataset, 
               std::vector<double> const& x, 
               std::vector<double> const& y, 
               std::vector<double> const& sigma, 
               std::string const& title)  throw(ExecuteError)
{
    check_valid_dataset(dataset);
    AL->get_data(dataset)->load_arrays(x, y, sigma, title);
}

void add_point(double x, double y, double sigma, int dataset)  
                                                          throw(ExecuteError)
{
    check_valid_dataset(dataset);
    AL->get_data(dataset)->add_one_point(x, y, sigma);
}

vector<Point> const& get_data(int dataset)  throw(ExecuteError)
{
    check_valid_dataset(dataset);
    return AL->get_data(dataset)->points();
}


void set_show_message(t_show_message *func)
{ 
    simple_message_handler = func;
    AL->get_ui()->set_show_message(message_handler); 
}

void redir_messages(std::FILE *stream)
{
    message_sink = stream;
    AL->get_ui()->set_show_message(message_redir); 
}

double get_wssr(int dataset)  throw(ExecuteError)
{
    return get_wssr_or_ssr(dataset, true);
}

double get_ssr(int dataset)  throw(ExecuteError)
{
    return get_wssr_or_ssr(dataset, false);
}

double get_rsquared(int dataset)  throw(ExecuteError)
{
    if (dataset == fityk::all_ds) {
        double result = 0;
        for (int i = 0; i < AL->get_ds_count(); ++i)
            result += Fit::compute_r_squared_for_data(AL->get_ds(i));
        return result;
    }
    else {
        check_valid_dataset(dataset);
        return Fit::compute_r_squared_for_data(AL->get_ds(dataset));
    }
}

int get_dof(int dataset)  throw(ExecuteError)
{
    return AL->get_fit()->get_dof(get_datasets_(dataset));
}

vector<vector<double> > get_covariance_matrix(int dataset) throw(ExecuteError)
{
    vector<double> c = AL->get_fit()->get_covariance_matrix(get_datasets_(dataset));
    //reshape
    size_t na = AL->get_parameters().size(); 
    assert(c.size() == na * na);
    vector<vector<double> > r(na);
    for (size_t i = 0; i != na; ++i)
        r[i] = vector<double>(c.begin() + i*na, c.begin() + i*(na+1));
    return r;
}

} //namespace fityk


// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License version 2 or (at your option) 3
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
std::string Point::str() { return "(" + S(x) + "; " + S(y) + "; " + S(sigma) 
                               + (is_active ? ")*" : ") "); }


Fityk::Fityk()
{
    if (AL != 0) 
        throw ExecuteError("Program is not thread-safe yet, "
                            "so you can only have one Fityk instance.");
    ftk = new Ftk;
    AL = ftk;
}

Fityk::~Fityk()
{
    delete ftk;
    AL = 0;
}

void Fityk::execute(string const& s)  throw(SyntaxError, ExecuteError, 
                                            ExitRequestedException)
{
    bool r = parse_and_execute_e(s);
    if (!r)
        throw SyntaxError();
}

bool Fityk::safe_execute(string const& s)  throw(ExitRequestedException)
{
    return ftk->exec(s) == Commands::status_ok; 
}

string Fityk::get_info(string const& s, bool full)  
                                             throw(SyntaxError, ExecuteError)
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

int Fityk::get_dataset_count()
{
    return ftk->get_dm_count();
}

double Fityk::get_model_value(double x, int dataset)  throw(ExecuteError)
{
    return ftk->get_model(dataset)->value(x);
}

vector<double> Fityk::get_model_vector(vector<double> const& x, int dataset)  
                                                          throw(ExecuteError)
{
    vector<double> xx(x);
    vector<double> yy(x.size(), 0.);
    ftk->get_model(dataset)->compute_model(xx, yy);
    return yy;
}

int Fityk::get_variable_nr(string const& name)  throw(ExecuteError) 
{
    if (name.empty())
        throw ExecuteError("get_variable_nr() called with empty name");
    string vname;
    if (name[0] == '$')
        vname = string(name, 1);
    else if (name[0] == '%' && name.find('.') < name.size() - 1) {
        string::size_type pos = name.find('.');
        Function const* f = ftk->find_function(string(1, pos-1));
        vname = f->get_param_varname(string(name, pos+1));
    }
    else
        vname = name;
    return ftk->find_variable(vname)->get_nr();
}

double Fityk::get_variable_value(string const& name)  throw(ExecuteError)
{
    if (name.empty())
        throw ExecuteError("get_variable_value() called with empty name");
    if (name[0] == '$')
        return ftk->find_variable(string(name, 1))->get_value();
    else if (name[0] == '%' && name.find('.') < name.size() - 1) {
        string::size_type pos = name.find('.');
        Function const* f = ftk->find_function(string(name, 1, pos-1));
        return f->get_param_value(string(name, pos+1));
    }
    else
        return ftk->find_variable(name)->get_value();
}

void Fityk::load_data(int dataset, 
                      std::vector<double> const& x, 
                      std::vector<double> const& y, 
                      std::vector<double> const& sigma, 
                      std::string const& title)     throw(ExecuteError)
{
    ftk->get_data(dataset)->load_arrays(x, y, sigma, title);
}

void Fityk::add_point(double x, double y, double sigma, int dataset)  
                                                          throw(ExecuteError)
{
    ftk->get_data(dataset)->add_one_point(x, y, sigma);
}

vector<Point> const& Fityk::get_data(int dataset)  throw(ExecuteError)
{
    return ftk->get_data(dataset)->points();
}


void Fityk::set_show_message(t_show_message *func)
{ 
    simple_message_handler = func;
    ftk->get_ui()->set_show_message(message_handler); 
}

void Fityk::redir_messages(std::FILE *stream)
{
    message_sink = stream;
    ftk->get_ui()->set_show_message(message_redir); 
}

double Fityk::get_wssr(int dataset)  throw(ExecuteError)
{
    return get_wssr_or_ssr(ftk, dataset, true);
}

double Fityk::get_ssr(int dataset)  throw(ExecuteError)
{
    return get_wssr_or_ssr(ftk, dataset, false);
}

double Fityk::get_rsquared(int dataset)  throw(ExecuteError)
{
    if (dataset == fityk::all_datasets) {
        double result = 0;
        for (int i = 0; i < ftk->get_dm_count(); ++i)
            result += Fit::compute_r_squared_for_data(ftk->get_dm(i));
        return result;
    }
    else {
        return Fit::compute_r_squared_for_data(ftk->get_dm(dataset));
    }
}

int Fityk::get_dof(int dataset)  throw(ExecuteError)
{
    return ftk->get_fit()->get_dof(get_datasets_(ftk, dataset));
}

vector<vector<double> > Fityk::get_covariance_matrix(int dataset) 
                                                           throw(ExecuteError)
{
    vector<double> c 
        = ftk->get_fit()->get_covariance_matrix(get_datasets_(ftk, dataset));
    //reshape
    size_t na = ftk->get_parameters().size(); 
    assert(c.size() == na * na);
    vector<vector<double> > r(na);
    for (size_t i = 0; i != na; ++i)
        r[i] = vector<double>(c.begin() + i*na, c.begin() + i*(na+1));
    return r;
}

} //namespace fityk


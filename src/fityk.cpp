// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License version 2
// $Id$

/// implementation of public API of libfityk, declared in fityk.h

#include <cassert>
#include "fityk.h"
#include "common.h"
#include "ui.h"
#include "logic.h"
#include "data.h"
#include "sum.h"

using namespace std;

namespace {

fityk::t_show_message *simple_message_handler = 0;

void message_handler(OutputStyle style, std::string const& s)
{
    if (simple_message_handler && style != os_input)
        (*simple_message_handler)(s);
}

void check_valid_dataset(int dataset)
{
    assert(dataset >= 0);
    assert(dataset < AL->get_ds_count());
}

Commands::Status last_exec_status = Commands::status_ok;

} //anonymous namespace

namespace fityk
{

bool execute(std::string const& s)
{
    last_exec_status = exec_command(s); 
    return last_exec_status == Commands::status_ok;
}

string get_execute_error()
{
    if (last_exec_status == Commands::status_ok)
        return "";
    else if (last_exec_status == Commands::status_syntax_error)
        return "Syntax error";
    else if (last_exec_status == Commands::status_execute_error)
        return ExecuteError::last_error;
    else
        assert(0);
}

string get_info(string const& s, bool full)
{
    //TODO
}

int get_dataset_count()
{
    return AL->get_ds_count();
}

double get_sum_value(double x, int dataset)
{
    //TODO
    return 0;
}

vector<double> get_sum_vector(vector<double> const& x, int dataset)
{
}

double get_parameter(std::string const& name, double *error)
{
    //TODO
    return 0;
}

void load_data(int dataset, 
               std::vector<double> const& x, 
               std::vector<double> const& y, 
               std::vector<double> const& sigma, 
               std::string const& title)
{
    //TODO
}

void add_point(double x, double y, double sigma, int dataset)
{
}

vector<Point> const& get_data(int dataset)
{
    check_valid_dataset(dataset);
    return AL->get_data(dataset)->points();
}


void set_show_message(t_show_message *func)
{ 
    simple_message_handler = func;
    getUI()->set_show_message(message_handler); 
}

void redir_messages(FILE *stream)
{
    //TODO
}

double get_wssr(int dataset)
{
}

double get_ssr(int dataset)
{
}

double get_rsquared(int dataset)
{
}

int get_dof(int dataset)
{
}


} //namespace fityk


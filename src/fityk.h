// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License version 2
// $Id$

#ifndef FITYK__API__H__
#define FITYK__API__H__

#ifndef __cplusplus
#error "This library does not have C API."
#endif

#include <string>
#include <vector>

namespace fityk
{

/// data point
struct Point 
{
    double x, y, sigma;
    bool is_active;

    Point();
    Point(double x_, double y_);
    Point(double x_, double y_, double sigma_);
    std::string str();
};


/// execute command; returns false on error
bool execute(std::string const& s);

/// return output of "info ..." or "info+ ..." command
std::string get_info(std::string const& s, bool full=false);

/// returns number of datasets n, always n >= 1
int get_dataset_count();

/// returns the value of the model (i.e. sum of function) for a given dataset
/// at x
double get_sum_value(double x, int dataset=0);

/// returns the value of a parameter (given as "$foo" or "%func.height")
/// optionally standard error can be returned. If the parameter is
/// a compound-parameter, error is set to -1
double get_parameter(std::string const& name, double *error=0);

/// load data
void load_data(int dataset, 
               std::vector<double> const& x, 
               std::vector<double> const& y, 
               std::vector<double> const& sigma, 
               std::string const& title="");

/// get data points
std::vector<Point> const& get_data(int dataset);

// handling output: general approach - callback
typedef void t_show_message(std::string const& s);
/// set show message callback
void set_show_message(t_show_message *func);

/// handling output: redirect it to ... (e.g. stdout or stderr)
void redir_messages(FILE *stream);


} // namespace

#endif


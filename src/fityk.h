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
#include <cstdio>
#include <stdexcept>
class Ftk;

/// \par
/// Public C++ API of libfityk is defined in namespace fityk, in file fityk.h
/// \par 
/// If you use it in your program, you may contact fityk developers 
/// (see the webpage for contact info) to let them know about it,
/// or to request more functions in the API.
/// \par
/// See description of Fityk class to see the list of methods it supports.
/// \par
/// Only one instance of Fityk class can be used.
/// \par
/// \b TODO 
/// - find out how to make bindings to set_show_message() and redir_messages()
///   in SWIG


namespace fityk
{

/// exception thrown at run-time (when executing parsed command)
struct ExecuteError : public std::runtime_error 
{
    ExecuteError(const std::string& msg) : runtime_error(msg) {}
};

/// syntax error exception, used only in public API
struct SyntaxError : public std::exception {};


/// exception thrown to finish the program (on command "quit")
struct ExitRequestedException : std::exception {};

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
inline bool operator< (Point const& p, Point const& q) { return p.x < q.x; }

/// type of function passed to Fityk::set_show_message()
typedef void t_show_message(std::string const& s);

/// used to get statistics for all datasets together, e.g. in Fityk::get_wssr()
const int all_ds=-1; 
    

/// the public API to libfityk 
class Fityk
{
public:

    Fityk();
    ~Fityk();
    
    /// @name execute fityk commands or change data
    // @{
    
    /// execute command; throws exception on error 
    void execute(std::string const& s) throw(SyntaxError, ExecuteError, 
                                             ExitRequestedException);
    
    /// execute command; returns false on error
    bool safe_execute(std::string const& s) throw(ExitRequestedException);
    
    /// load data
    void load_data(int dataset, 
                   std::vector<double> const& x, 
                   std::vector<double> const& y, 
                   std::vector<double> const& sigma, 
                   std::string const& title="")  throw(ExecuteError);
    
    /// add one data point to dataset
    void add_point(double x, double y, double sigma, int dataset=0)
                                                     throw(ExecuteError);
    
    // @}
    
    /// @name handling text output 
    // @{
    
    /// general approach: set show message callback; cancels redir_messages()
    void set_show_message(t_show_message *func);
    
    /// redirect output to ...(e.g. stdout/stderr); cancels set_show_message()
    void redir_messages(std::FILE *stream);
    
    // @}
    
    
    /// @name get data and informations 
    // @{
    
    /// return output of "info ..." or "info+ ..." command
    std::string get_info(std::string const& s, bool full=false) 
                                              throw(SyntaxError, ExecuteError);
    
    /// returns number of datasets n, always n >= 1
    int get_dataset_count();
    
    /// get data points
    std::vector<Point> const& get_data(int dataset=0)  throw(ExecuteError);
    
    /// \brief returns the value of the model (i.e. sum of function) 
    /// for a given dataset at x
    double get_sum_value(double x, int dataset=0)  throw(ExecuteError);
    
    /// multiple point version of the get_sum_value() 
    std::vector<double> 
    get_sum_vector(std::vector<double> const& x, int dataset=0)
                                                        throw(ExecuteError);
    
    /// returns the value of a variable (given as "$foo" or "%func.height")
    double get_variable_value(std::string const& name)  throw(ExecuteError);
    
    /// \brief returns the number of parameter hold by the variable
    /// (-1 for a compound-variable). Useful with get_covariance_matrix()
    int get_variable_nr(std::string const& name)  throw(ExecuteError);
    
    // @}
    
    /// @name get fit statistics    
    // @{
    
    /// get WSSR for given dataset or for all datasets
    double get_wssr(int dataset=all_ds)  throw(ExecuteError);
    
    /// get SSR for given dataset or for all datasets
    double get_ssr(int dataset=all_ds)  throw(ExecuteError);
    
    /// get R-squared for given dataset or for all datasets
    double get_rsquared(int dataset=all_ds)  throw(ExecuteError);
    
    /// get number of degrees-of-freedom for given dataset or for all datasets
    int get_dof(int dataset=all_ds)  throw(ExecuteError);
    
    /// \brief get covariance matrix (for given dataset or for all datasets)
    /// get_variable_nr() can be used to connect variables with parameter 
    /// positions
    std::vector<std::vector<double> > get_covariance_matrix(int dataset=all_ds)
                                                           throw(ExecuteError);
    // @}

private:
    Ftk *ftk;
};

} // namespace

#endif


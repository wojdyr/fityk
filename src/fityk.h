/* This file is part of fityk program. Copyright (C) Marcin Wojdyr
 * Licence: GNU General Public License ver. 2+
 */

#ifndef FITYK_FITYK_H_
#define FITYK_FITYK_H_

/* set precision used for storing data and fitting functions */
#define USE_LONG_DOUBLE 0
#if USE_LONG_DOUBLE
  typedef long double realt;
# define REALT_LENGTH_MOD "L"
#else
  typedef double realt;
# define REALT_LENGTH_MOD ""
#endif

#ifdef __cplusplus

#ifdef _MSC_VER
// C++ exception specifications are used by SWIG bindings
#pragma warning( disable : 4290 ) // C++ exception specification ignored...
#endif

#include <string>
#include <vector>
#include <cstdio>
#include <stdexcept>
class Ftk;


/// Public C++ API of libfityk: class Fityk and helpers.
///
/// Minimal examples of using libfityk in C++, Python, Lua and Perl are in
/// samples/hello.* files.
namespace fityk
{

class UiApi;
struct FitykInternalData;

/// exception thrown at run-time (when executing parsed command)
struct ExecuteError : public std::runtime_error
{
    ExecuteError(const std::string& msg) : runtime_error(msg) {}
};

/// syntax error exception
struct SyntaxError : public std::invalid_argument
{
    SyntaxError(const std::string& msg="") : invalid_argument(msg) {}
};


/// exception thrown to finish the program (on command "quit")
struct ExitRequestedException : std::exception
{
};

/// used to get statistics for all datasets together, e.g. in Fityk::get_wssr()
const int all_datasets=-1;

/// data point
struct Point
{
    realt x, y, sigma;
    bool is_active;
    Point();
    Point(realt x_, realt y_);
    Point(realt x_, realt y_, realt sigma_);
    std::string str() const;
    bool operator< (Point const& q) const { return x < q.x; }
};



/// the public API to libfityk
class Fityk
{
public:

    Fityk();
    Fityk(Ftk* F);
    ~Fityk();

    /// @name execute fityk commands or change data
    // @{

    /// execute command; throws exception on error
    void execute(std::string const& s) throw(SyntaxError, ExecuteError,
                                             ExitRequestedException);

    /// load data
    void load_data(int dataset,
                   std::vector<realt> const& x,
                   std::vector<realt> const& y,
                   std::vector<realt> const& sigma,
                   std::string const& title="")  throw(ExecuteError);

    /// add one data point to dataset
    void add_point(realt x, realt y, realt sigma, int dataset=0)
                                                     throw(ExecuteError);

    // @}

    /// @name (alternative to exceptions) handling of program errors
    // @{

    /// If set to false, does not throw exceptions. Default: true.
    void set_throws(bool state) { throws_ = state; }

    /// Return error handling mode: true if exceptions are thrown.
    bool get_throws() const { return throws_; }

    ///\brief Returns a string with last error or an empty string.
    /// Useful after calling set_throws(false). See also: clear_last_error().
    std::string const& last_error() const { return last_error_; }

    /// Clear last error message. See also: last_error().
    void clear_last_error() { last_error_.clear(); }

    // @}

    /// @name handling text output
    // @{

    /// redirect output to file or stdout/stderr; called with NULL reverts
    /// previous call(s).
    /// Don't use with more than one Fityk instance at the same time.
    /// Internally uses UiApi::set_show_message().
    void redir_messages(std::FILE *stream);

    /// print string in the output of GUI/CLI (useful for embedded Lua)
    void out(std::string const& s) const;

    // @}


    /// @name get data and informations
    // @{

    /// return output of the info command
    std::string get_info(std::string const& s, int dataset=0)
                                            throw(SyntaxError, ExecuteError);

    /// return expression value, similarly to the print command
    realt calculate_expr(std::string const& s, int dataset=0)
                                            throw(SyntaxError, ExecuteError);

    /// returns number of datasets n, always n >= 1
    int get_dataset_count() const;

    /// returns dataset set by "use @n" command
    int get_default_dataset() const;

    /// returns number of simple-variables (parameters that can be fitted)
    int get_parameter_count() const;

    /// get data points
    std::vector<Point> const& get_data(int dataset=0)  throw(ExecuteError);

    /// returns the value of the model for a given dataset at x
    realt get_model_value(realt x, int dataset=0)  throw(ExecuteError);

    /// multiple point version of the get_model_value()
    std::vector<realt>
    get_model_vector(std::vector<realt> const& x, int dataset=0)
                                                        throw(ExecuteError);

    /// \brief returns the index of parameter hold by the variable
    /// (the same index as in get_covariance_matrix(),
    /// -1 for a compound-variable)
    int get_variable_nr(std::string const& name)  throw(ExecuteError);

    /// get coordinates of rectangle set by the plot command
    /// side is one of L(eft), R(right), T(op), B(ottom)
    double get_view_boundary(char side);

    // @}

    /// @name get fit statistics
    // @{

    /// get WSSR for given dataset or for all datasets
    realt get_wssr(int dataset=all_datasets)  throw(ExecuteError);

    /// get SSR for given dataset or for all datasets
    realt get_ssr(int dataset=all_datasets)  throw(ExecuteError);

    /// get R-squared for given dataset or for all datasets
    realt get_rsquared(int dataset=all_datasets)  throw(ExecuteError);

    /// get number of degrees-of-freedom for given dataset or for all datasets
    int get_dof(int dataset=all_datasets)  throw(ExecuteError);

    /// \brief get covariance matrix (for given dataset or for all datasets)
    /// get_variable_nr() can be used to connect variables with parameter
    /// positions
    std::vector<std::vector<realt> >
    get_covariance_matrix(int dataset=all_datasets)  throw(ExecuteError);
    // @}

    /// UiApi contains functions used by CLI and may be used to implement
    /// another user interface.
    UiApi* get_ui_api();

    // implementation details (for internal use)
    Ftk *get_ftk() { return ftk_; } // access to underlying data
    realt* get_covariance_matrix_as_array(int dataset);

private:
    Ftk *ftk_;
    bool throws_;
    std::string last_error_;
    FitykInternalData *p_;
};

} // namespace

#else /* !__cplusplus */
/* C API.
 * Functions below correspond to member functions of class Fityk.
 * To check for errors use fityk_last_error().
 * bool and Point here should be ABI-compatible with C++ bool and fityk::Point.
 */

#define bool _Bool

typedef struct Fityk_ Fityk;

typedef struct
{
    realt x, y, sigma;
    bool is_active;
} Point;


Fityk* fityk_create();
void fityk_delete(Fityk *f);
/* returns false on ExitRequestedException */
bool fityk_execute(Fityk *f, const char* command);
void fityk_load_data(Fityk *f, int dataset,
                     realt *x, realt *y, realt *sigma, int num,
                     const char* title);
/* returns NULL if no error happened since fityk_clear_last_error() */
const char* fityk_last_error(const Fityk *f);
void fityk_clear_last_error(Fityk *f);
/* caller is responsible to free() returned string */
char* fityk_get_info(Fityk *f, const char *s, int dataset);
realt fityk_calculate_expr(Fityk *f, const char* s, int dataset);
int fityk_get_dataset_count(const Fityk *f);
int fityk_get_parameter_count(const Fityk* f);
/* get data point, returns NULL if index is out of range */
const Point* fityk_get_data_point(Fityk *f, int dataset, int index);
realt fityk_get_model_value(Fityk *f, realt x, int dataset);
int fityk_get_variable_nr(Fityk *f, const char* name);
realt fityk_get_wssr(Fityk *f, int dataset);
realt fityk_get_ssr(Fityk *f, int dataset);
realt fityk_get_rsquared(Fityk *f, int dataset);
int fityk_get_dof(Fityk *f, int dataset);
/* returns matrix in array, which caller is responsible to free(); */
/* length of the array is parameter_count^2                        */
realt* fityk_get_covariance_matrix(Fityk *f, int dataset);

#endif /* __cplusplus */

#endif /* FITYK_FITYK_H_ */


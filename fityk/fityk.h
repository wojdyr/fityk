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

/* FITYK_API marks classes and functions visible in Windows DLL
 */
#if defined(_WIN32) && (defined(LIBFITYK_DLL) || defined(DLL_EXPORT))
# if defined(BUILDING_LIBFITYK)
#  define FITYK_API  __declspec(dllexport)
# else
#  define FITYK_API  __declspec(dllimport)
# endif
#else
# if __GNUC__-0 >= 4
#  define FITYK_API __attribute__ ((visibility ("default")))
# else
#  define FITYK_API
# endif
#endif

#ifdef __cplusplus

#include <cstdio>
#include <cmath>
#include <string>
#include <vector>
#include <stdexcept>

namespace fityk {

#ifdef _MSC_VER
// C++ exception specifications are used by SWIG bindings
#pragma warning( disable : 4290 ) // C++ exception specification ignored...
#endif

/// Public C++ API of libfityk: class Fityk and helpers.
///
/// Minimal examples of using libfityk in C++, Python, Lua and Perl are in
/// samples/hello.* files.

class Ftk;
class UiApi;
struct FitykInternalData;

/// exception thrown at run-time (when executing parsed command)
struct FITYK_API ExecuteError : public std::runtime_error
{
    ExecuteError(const std::string& msg) : runtime_error(msg) {}
};

/// syntax error exception
struct FITYK_API SyntaxError : public std::invalid_argument
{
    SyntaxError(const std::string& msg="") : invalid_argument(msg) {}
};

/// exception thrown to finish the program (on command "quit")
struct FITYK_API ExitRequestedException : std::exception
{
};

/// used for variable domain and for plot borders
struct FITYK_API RealRange
{
    double lo, hi;

    RealRange() : lo(-HUGE_VAL), hi(HUGE_VAL) {}
    RealRange(double low, double high) : lo(low), hi(high) {}
    bool lo_inf() const { return lo == -HUGE_VAL; }
    bool hi_inf() const { return hi == HUGE_VAL; }
    std::string str() const;
};

/// represents $variable
/// (public API has only a subset of members)
class FITYK_API Var
{
public:
    const std::string name;
    RealRange domain;

    realt value() const { return value_; };
    int gpos() const { return gpos_; };
    bool is_simple() const { return gpos_ != -1; }

protected:
    Var(const std::string &name_, int gpos) : name(name_), gpos_(gpos) {}
    int gpos_; /// see description of this class in var.h
    realt value_;
};

/// represents %function
/// (public API has only a subset of members)
class FITYK_API Func
{
public:
    const std::string name;
    virtual ~Func() {}

    virtual const std::string& get_template_name() const = 0;
    virtual std::string get_param(int n) const = 0;
    virtual const std::string& var_name(const std::string& param) const = 0;
    virtual realt get_param_value(const std::string& param) const = 0;
    virtual realt value_at(realt x) const = 0;
protected:
    Func(const std::string name_) : name(name_) {}
};

/// special dataset magic numbers used only in this API
enum {
    /// all datasets, used to get statistics for all datasets together
    ALL_DATASETS=-1,
    /// default dataset (as set by the 'use' command)
    DEFAULT_DATASET=-2
};

/// data point
struct FITYK_API Point
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
class FITYK_API Fityk
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
    void add_point(realt x, realt y, realt sigma, int dataset=DEFAULT_DATASET)
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

    /// @name settings
    // @{
    void set_option_as_string(const std::string& opt, const std::string& val)
                                                         throw(ExecuteError);
    void set_option_as_number(const std::string& opt, double val)
                                                         throw(ExecuteError);
    std::string get_option_as_string(const std::string& opt) const
                                                         throw(ExecuteError);
    double get_option_as_number(const std::string& opt) const
                                                         throw(ExecuteError);
    // @}

    /// @name input/output
    // @{

    /// redirect output to file or stdout/stderr; called with NULL reverts
    /// previous call(s).
    /// Internally uses UiApi::set_show_message().
    /// Bugs: can't be used with more than one Fityk instance at the same time.
    void redir_messages(std::FILE *stream);

    /// print string in the output of GUI/CLI (useful for embedded Lua)
    void out(std::string const& s) const;

    /// query user (useful for embedded Lua)
    /// If the prompt contains string "[y/n]" the GUI shows Yes/No buttons.
    std::string input(std::string const& prompt);

    // @}


    /// @name get data and informations
    // @{

    /// return output of the info command
    std::string get_info(std::string const& s, int dataset=DEFAULT_DATASET)
                                            throw(SyntaxError, ExecuteError);

    /// return expression value, similarly to the print command
    realt calculate_expr(std::string const& s, int dataset=DEFAULT_DATASET)
                                            throw(SyntaxError, ExecuteError);

    /// returns number of datasets n, always n >= 1
    int get_dataset_count() const;

    /// returns dataset set by "use @n" command
    int get_default_dataset() const;

    /// get data points
    std::vector<Point> const& get_data(int dataset=DEFAULT_DATASET)
                                                         throw(ExecuteError);

    /// returns number of simple-variables (parameters that can be fitted)
    int get_parameter_count() const;

    /// returns global array of parameters (values of simple-variables)
    const std::vector<realt>& all_parameters() const;

    /// returns all $variables
    std::vector<Var*> all_variables() const;

    /// returns variable $name
    const Var* get_variable(std::string const& name)  throw(ExecuteError);

    /// returns all %functions
    std::vector<Func*> all_functions() const;

    /// returns function with given name ("%" in the name is optional)
    const Func* get_function(const std::string& name) const;

    /// returns %functions used in dataset
    std::vector<Func*> get_components(int dataset, char fz='F');

    /// returns the value of the model for a given dataset at x
    realt get_model_value(realt x, int dataset=DEFAULT_DATASET)
                                                         throw(ExecuteError);

    /// multiple point version of the get_model_value()
    std::vector<realt>
    get_model_vector(std::vector<realt> const& x, int dataset=DEFAULT_DATASET)
                                                         throw(ExecuteError);

    /// get coordinates of rectangle set by the plot command
    /// side is one of L(eft), R(ight), T(op), B(ottom)
    double get_view_boundary(char side);

    // @}

    /// @name get fit statistics
    // @{

    /// get WSSR for given dataset or for all datasets
    realt get_wssr(int dataset=ALL_DATASETS)  throw(ExecuteError);

    /// get SSR for given dataset or for all datasets
    realt get_ssr(int dataset=ALL_DATASETS)  throw(ExecuteError);

    /// get R-squared for given dataset or for all datasets
    realt get_rsquared(int dataset=ALL_DATASETS)  throw(ExecuteError);

    /// get number of degrees-of-freedom for given dataset or for all datasets
    int get_dof(int dataset=ALL_DATASETS)  throw(ExecuteError);

    /// get covariance matrix (for given dataset or for all datasets)
    std::vector<std::vector<realt> >
    get_covariance_matrix(int dataset=ALL_DATASETS)  throw(ExecuteError);
    // @}

    /// UiApi contains functions used by CLI and may be used to implement
    /// another user interface.
    UiApi* get_ui_api();
    void process_cmd_line_arg(const std::string& arg);

    // implementation details (for internal use)
    Ftk *get_ftk() { return ftk_; } // access to underlying data
    realt* get_covariance_matrix_as_array(int dataset);

private:
    Ftk *ftk_;
    bool throws_;
    std::string last_error_;
    FitykInternalData *p_;
    // disallow copy and assign
    Fityk(const Fityk&);
    void operator=(const Fityk&);
};

} // namespace fityk

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


FITYK_API Fityk* fityk_create();
FITYK_API void fityk_delete(Fityk *f);
/* returns false on ExitRequestedException */
FITYK_API bool fityk_execute(Fityk *f, const char* command);
FITYK_API void fityk_load_data(Fityk *f, int dataset,
                               double *x, double *y, double *sigma, int num,
                               const char* title);
/* returns NULL if no error happened since fityk_clear_last_error() */
FITYK_API const char* fityk_last_error(const Fityk *f);
FITYK_API void fityk_clear_last_error(Fityk *f);
/* caller is responsible to free() returned string */
FITYK_API char* fityk_get_info(Fityk *f, const char *s, int dataset);
FITYK_API realt fityk_calculate_expr(Fityk *f, const char* s, int dataset);
FITYK_API int fityk_get_dataset_count(const Fityk *f);
FITYK_API int fityk_get_parameter_count(const Fityk* f);
/* get data point, returns NULL if index is out of range */
FITYK_API const Point* fityk_get_data_point(Fityk *f, int dataset, int index);
FITYK_API realt fityk_get_model_value(Fityk *f, realt x, int dataset);
FITYK_API realt fityk_get_wssr(Fityk *f, int dataset);
FITYK_API realt fityk_get_ssr(Fityk *f, int dataset);
FITYK_API realt fityk_get_rsquared(Fityk *f, int dataset);
FITYK_API int fityk_get_dof(Fityk *f, int dataset);
/* returns matrix in array, which caller is responsible to free(); */
/* length of the array is parameter_count^2                        */
FITYK_API realt* fityk_get_covariance_matrix(Fityk *f, int dataset);

#endif /* __cplusplus */

#endif /* FITYK_FITYK_H_ */


/* This file is part of fityk program. Copyright 2001-2013 Marcin Wojdyr
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

// C++ exception specifications are used by SWIG bindings.
// They are deprecated (in this form) in C++-11.
#if defined(_MSC_VER)
#pragma warning( disable : 4290 ) // C++ exception specification ignored...
#elif defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"
#endif

/// Public C++ API of libfityk: class Fityk and helpers.
///
/// Minimal examples of using libfityk in C++, Python, Lua and Perl are in
/// samples/hello.* files.

class Full;
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

    realt value() const { return value_; }
    int gpos() const { return gpos_; }
    bool is_simple() const { return gpos_ != -1; }

protected:
    Var(const std::string &name_, int gpos) : name(name_), gpos_(gpos) {}
    ~Var() {}
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

/// the only use of this struct is as an argument to Fityk::load()
struct FITYK_API LoadSpec
{
    enum { NN = -10000 }; // not given, default value
    std::string path;  // utf8 (ascii is valid utf8)
    std::vector<int> blocks;
    int x_col;
    int y_col;
    int sig_col;
    std::string format;
    std::string options;
    LoadSpec() : x_col(NN), y_col(NN), sig_col(NN) {}
    explicit LoadSpec(std::string const& p)
        : path(p), x_col(NN), y_col(NN), sig_col(NN) {}
};


/// the public API to libfityk
class FITYK_API Fityk
{
public:

    Fityk();
    Fityk(Full* F);
    ~Fityk();

    /// @name execute fityk commands or change data
    // @{

    /// execute command; throws exception on error
    void execute(std::string const& s);


    /// load data from file (path should be ascii or utf8, col=0 is index)
    void load(LoadSpec const& spec, int dataset=DEFAULT_DATASET);
    void load(std::string const& path, int dataset=DEFAULT_DATASET)
      { load(LoadSpec(path), dataset); }

    /// load data from arrays
    void load_data(int dataset,
                   std::vector<realt> const& x,
                   std::vector<realt> const& y,
                   std::vector<realt> const& sigma,
                   std::string const& title="");

    /// add one data point to dataset
    void add_point(realt x, realt y, realt sigma, int dataset=DEFAULT_DATASET);

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
    void set_option_as_string(const std::string& opt, const std::string& val);
    void set_option_as_number(const std::string& opt, double val);
    std::string get_option_as_string(const std::string& opt) const;
    double get_option_as_number(const std::string& opt) const;
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
    std::string get_info(std::string const& s, int dataset=DEFAULT_DATASET);

    /// return expression value, similarly to the print command
    realt calculate_expr(std::string const& s, int dataset=DEFAULT_DATASET);

    //(planned)
    /// returns dataset titles
    //std::vector<std::string> all_datasets() const;
    //or returns a new class, public API to Data (like Func and Var)
    //std::vector<Dataset*> all_datasets() const;

    /// returns number of datasets n, always n >= 1
    int get_dataset_count() const;

    /// returns dataset set by the "use" command
    int get_default_dataset() const;

    /// get data points
    std::vector<Point> const& get_data(int dataset=DEFAULT_DATASET);

    /// returns number of simple-variables (parameters that can be fitted)
    int get_parameter_count() const;

    /// returns global array of parameters (values of simple-variables)
    const std::vector<realt>& all_parameters() const;

    /// returns all $variables
    std::vector<Var*> all_variables() const;

    /// returns variable $name
    const Var* get_variable(std::string const& name) const;

    /// returns all %functions
    std::vector<Func*> all_functions() const;

    /// returns function with given name ("%" in the name is optional)
    const Func* get_function(const std::string& name) const;

    /// returns %functions used in dataset
    std::vector<Func*> get_components(int dataset=DEFAULT_DATASET, char fz='F');

    /// returns the value of the model for a given dataset at x
    realt get_model_value(realt x, int dataset=DEFAULT_DATASET) const;

    /// multiple point version of the get_model_value()
    std::vector<realt>
    get_model_vector(std::vector<realt> const& x, int dataset=DEFAULT_DATASET);

    /// get coordinates of rectangle set by the plot command
    /// side is one of L(eft), R(ight), T(op), B(ottom)
    double get_view_boundary(char side);

    // @}

    /// @name get fit statistics
    // @{

    /// get WSSR for given dataset or for all datasets
    realt get_wssr(int dataset=ALL_DATASETS);

    /// get SSR for given dataset or for all datasets
    realt get_ssr(int dataset=ALL_DATASETS);

    /// get R-squared for given dataset or for all datasets
    realt get_rsquared(int dataset=ALL_DATASETS);

    /// get number of degrees-of-freedom for given dataset or for all datasets
    int get_dof(int dataset=ALL_DATASETS);

    /// get covariance matrix (for given dataset or for all datasets)
    std::vector<std::vector<realt> >
    get_covariance_matrix(int dataset=ALL_DATASETS);
    // @}

    /// UiApi contains functions used by CLI and may be used to implement
    /// another user interface.
    UiApi* get_ui_api();
    void process_cmd_line_arg(const std::string& arg);

    // implementation details (for internal use)
    Full* priv() { return priv_; } // access to private API
    realt* get_covariance_matrix_as_array(int dataset);

private:
    Full *priv_;
    bool throws_;
    mutable std::string last_error_;
    FitykInternalData *p_; // members hidden for the sake of API stability
    // disallow copy and assign
    Fityk(const Fityk&);
    void operator=(const Fityk&);
};

} // namespace fityk

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#else /* !__cplusplus */
/* C API.
 * Functions below correspond to member functions of class Fityk.
 * To check for errors use fityk_last_error().
 * Point here should be ABI-compatible with C++ bool and fityk::Point.
 */

typedef struct Fityk_ Fityk;
typedef struct Func_ Func;
typedef struct Var_ Var;

typedef struct
{
    realt x, y, sigma;
#if __STDC_VERSION__-0 >= 199901L
    _Bool is_active;
#else
    unsigned char is_active; // best guess
#endif
} Point;

#endif /* __cplusplus */

#if !defined(__cplusplus) || defined(FITYK_DECLARE_C_API)
#ifdef __cplusplus
extern "C" {
using fityk::Point;
using fityk::Fityk;
using fityk::Func;
using fityk::Var;
#endif

FITYK_API Fityk* fityk_create();
FITYK_API void fityk_delete(Fityk *f);
/* returns 0 on ExitRequestedException */
FITYK_API int fityk_execute(Fityk *f, const char* command);
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
FITYK_API const Var* fityk_get_variable(const Fityk* f, const char* name);
FITYK_API const Func* fityk_get_function(const Fityk* f, const char* name);
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

FITYK_API realt fityk_var_value(const Var *var);
FITYK_API const char* fityk_var_name(const Func *func, const char *param);
FITYK_API realt fityk_value_at(const Func *func, realt x);

#ifdef __cplusplus
} // extern "C"
#endif

#endif

#endif /* FITYK_FITYK_H_ */


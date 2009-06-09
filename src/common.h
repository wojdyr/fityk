// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
// $Id$

/*
 *  various headers and definitions. Included by all files.
 */
#ifndef FITYK__COMMON__H__
#define FITYK__COMMON__H__

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#ifndef VERSION
#   define VERSION "unknown"
#endif

#include <string>
#include <sstream>
#include <iomanip>
#include <vector>
#include <map>
#include <math.h>
#include <assert.h>

#include "fityk.h" //ExecuteError
using fityk::ExecuteError;
using fityk::ExitRequestedException;

//--------------------------  N U M E R I C  --------------------------------

/// favourite floating point type
#ifdef FP_IS_LDOUBLE
typedef long double fp;
#else
typedef double fp;
#endif

/// epsilon is used for comparision of real numbers
/// defined in settings.cpp; it can be changed in Settings
extern fp epsilon;

inline bool is_eq(fp a, fp b) { return fabs(a-b) <= epsilon; }
inline bool is_neq(fp a, fp b) { return fabs(a-b) > epsilon; }
inline bool is_lt(fp a, fp b) { return a < b - epsilon; }
inline bool is_gt(fp a, fp b) { return a > b + epsilon; }
inline bool is_le(fp a, fp b) { return a <= b + epsilon; }
inline bool is_ge(fp a, fp b) { return a >= b - epsilon; }
inline bool is_zero(fp a) { return fabs(a) <= epsilon; }

inline bool is_finite(fp a)
#if HAVE_FINITE
    { return finite(a); }
#else
    { return a == a; }
#endif


#ifndef M_PI
# define M_PI    3.1415926535897932384626433832795029  // pi
#endif
#ifndef M_LN2
# define M_LN2   0.6931471805599453094172321214581766  // log_e 2
#endif
#ifndef M_SQRT2
# define M_SQRT2 1.4142135623730950488016887242096981  // sqrt(2)
#endif

/** idea of exp_() is taken from gnuplot:
 *  some machines have trouble with exp(-x) for large x
 *  if MINEXP is defined at compile time, use exp_(x) instead,
 *  which returns 0 for exp_(x) with x < MINEXP
 */
#ifdef MINEXP
  inline fp exp_(fp x) { return (x < (MINEXP)) ? 0.0 : exp(x); }
#else
#  define exp_(x) exp(x)
#endif

/// Round real to integer.
inline int iround(fp d) { return static_cast<int>(floor(d+0.5)); }

#ifndef __GNUC__
        // These functions are missing in some compilers. If your compiler
        // has these function, please modify the #if above.
#warning "Functions erf and erfc in fityk are working only with GCC."
#warning "Please try to fix this in common.h header. "
        // fake definitions
	inline float erfc(float f) { return 0.5f; }
	inline float erf(float f) { return 0.5f; }

        // this one is correct
	inline float trunc(float f) { return (float) (int) f; }
#endif	// !__GNUC__


//---------------------------  S T R I N G  --------------------------------

/// S() converts to string
template <typename T>
inline std::string S(T k) {
    return static_cast<std::ostringstream&>(std::ostringstream() << k).str();
}

inline std::string S(bool b) { return b ? "true" : "false"; }
inline std::string S(char const *k) { return std::string(k); }
inline std::string S(char *k) { return std::string(k); }
inline std::string S(char const k) { return std::string(1, k); }
inline std::string S(std::string const &k) { return k; }
inline std::string S() { return std::string(); }
// double -> string, with given precision
inline std::string S(double k, int /*prec*/) {
    return static_cast<std::ostringstream&>(std::ostringstream()
                                /* << std::setprecision(prec) */<< k).str();
}


/// True if the string contains only a real number
bool is_double (std::string const& s);

/// True if the string contains only an integer number
bool is_int (std::string const& s);

/// replace all occurences of old in string s with new_
void replace_all(std::string &s, std::string const &old,
                                 std::string const &new_);

void replace_words(std::string &t, std::string const &old_word,
                                   std::string const &new_word);

/// splits string into tokens, separated by one-character delimitors
template<typename T>
std::vector<std::string> split_string(std::string const &s, T delim) {
    std::vector<std::string> v;
    std::string::size_type start_pos = 0, pos=0;
    while (pos != std::string::npos) {
        pos = s.find_first_of(delim, start_pos);
        v.push_back(std::string(s, start_pos, pos-start_pos));
        start_pos = pos+1;
    }
    return v;
}

/// similar to Python string.strip() method
inline std::string strip_string(std::string const &s) {
    char const *blank = " \r\n\t";
    std::string::size_type first = s.find_first_not_of(blank);
    if (first == std::string::npos)
        return std::string();
    return std::string(s, first, s.find_last_not_of(blank)-first+1);
}

/// similar to Python string.startswith() method
inline bool startswith(std::string const& s, std::string const& p) {
    return p.size() <= s.size() && std::string(s, 0, p.size()) == p;
}

std::string::size_type find_matching_bracket(std::string const& formula,
                                             std::string::size_type left_pos);

template<typename T, typename T2>
bool contains_element(std::basic_string<T> const& str, T2 const& t)
{
    return (str.find(t) != std::basic_string<T>::npos);
}
//---------------------------  V E C T O R  --------------------------------

/// Makes 1-element vector
template <typename T>
inline std::vector<T> vector1 (T a) { return std::vector<T>(1, a); }

/// Makes 2-element vector
template <typename T>
inline std::vector<T> vector2 (T a, T b)
    { std::vector<T> v = std::vector<T>(2, a); v[1] = b; return v;}

/// Make 3-element vector
template <typename T>
inline std::vector<T> vector3 (T a, T b, T c)
    { std::vector<T> v = std::vector<T>(3, a); v[1] = b; v[2] = c; return v;}

/// Make 4-element vector
template <typename T>
inline std::vector<T> vector4 (T a, T b, T c, T d) {
    std::vector<T> v = std::vector<T>(4, a); v[1] = b; v[2] = c; v[3] = d;
    return v;
}

/// Make (u-l)-element vector, filled by numbers: l, l+1, ..., u-1.
std::vector<int> range_vector(int l, int u);

/// Expression like "i<v.size()", where i is int and v is a std::vector gives:
/// "warning: comparison between signed and unsigned integer expressions"
/// implicit cast IMHO makes code less clear than "i<size(v)":
template <typename T>
inline int size(std::vector<T> const& v) { return static_cast<int>(v.size()); }

/// Return 0 <= n < a.size()
template <typename T>
inline bool is_index (int idx, std::vector<T> const& v)
{
    return idx >= 0 && idx < static_cast<int>(v.size());
}


template <typename T>
inline std::string join_vector(std::vector<T> const& v, std::string const& sep)
{
    if (v.empty())
        return "";
    std::string s = S(v[0]);
    for (typename std::vector<T>::const_iterator i = v.begin() + 1;
            i != v.end(); i++)
        s += sep + S(*i);
    return s;
}

template <typename T1, typename T2>
inline std::vector<std::string> concat_pairs(std::vector<T1> const& v1,
                                             std::vector<T2> const& v2,
                                             std::string const& sep="")
{
    std::vector<std::string> result(std::min(v1.size(), v2.size()));
    for (size_t i = 0; i < result.size(); ++i)
        result.push_back(S(v1[i]) + sep + S(v2[i]));
    return result;
}

template <typename T>
inline std::vector<std::string> concat_pairs(std::string const& s1,
                                             std::vector<T> const& v2)
{
    std::vector<std::string> result(v2.size());
    for (size_t i = 0; i != v2.size(); ++i)
        result[i] = s1 + S(v2[i]);
    return result;
}

template <typename T>
inline std::vector<std::string> concat_pairs(std::vector<T> const& v1,
                                             std::string const& s2)
{
    std::vector<std::string> result(v1.size());
    for (size_t i = 0; i != v1.size(); ++i)
        result[i] = S(v1[i]) + s2;
    return result;
}

/// for vector<T*> - delete object and erase pointer
template<typename T>
void purge_element(std::vector<T*> &vec, int n)
{
    assert(n >= 0 && n < size(vec));
    delete vec[n];
    vec.erase(vec.begin() + n);
}

/// delete all objects handled by pointers and clear vector
template<typename T>
void purge_all_elements(std::vector<T*> &vec)
{
    for (typename std::vector<T*>::iterator i=vec.begin(); i!=vec.end(); ++i)
        delete *i;
    vec.clear();
}

/// check if vector (first arg) contains given element (second arg)
template<typename T, typename T2>
bool contains_element(std::vector<T> const& vec, T2 const& t)
{
    return (find(vec.begin(), vec.end(), t) != vec.end());
}

/// return first index of value, or -1 if not found
template<typename T, typename T2>
int index_of_element(std::vector<T> const& vec, T2 const& t)
{
    typename std::vector<T>::const_iterator p = find(vec.begin(), vec.end(), t);
    if (p != vec.end())
        return p - vec.begin();
    else
        return -1;
}

//---------------------------  M A P  --------------------------------
template<typename T1, typename T2>
std::vector<T2> get_map_keys(std::map<T1,T2> const& m)
{
    std::vector<T2> result;
    for (typename std::map<T1,T2>::const_iterator i=m.begin(); i!=m.end(); ++i)
        result.push_back(i->first);
    return result;
}

template<typename T1, typename T2>
std::vector<T2> get_map_values(std::map<T1,T2> const& m)
{
    std::vector<T2> result;
    for (typename std::map<T1,T2>::const_iterator i=m.begin(); i!=m.end(); ++i)
        result.push_back(i->second);
    return result;
}

//----------------  filename utils  -------------------------------------
#if defined(_WIN32) || defined(WIN32) || defined(__NT__) || defined(__WIN32__) || defined(__OS2__)
#define PATH_COMPONENT_SEP '\\'
#elif defined(__MAC__) || defined(__APPLE__) || defined(macintosh)
//Mac OS <= 9
#define PATH_COMPONENT_SEP ':'
#else
#define PATH_COMPONENT_SEP '/'
#endif

inline std::string get_directory(std::string const& filename)
{
    std::string::size_type i = filename.rfind(PATH_COMPONENT_SEP);
    return i==std::string::npos ? std::string() : std::string(filename, 0, i+1);
}


//-------------------- M I S C E L A N O U S ------------------------------

// A macro to disallow the copy constructor and operator= functions.
// This should be used in the private: declarations for a class.
#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName&);               \
  void operator=(const TypeName&)

extern const char* fityk_version_line; /// it is used to put version to script

/// flag that is set to interrupt fitting (it is checked after each iteration)
extern volatile bool user_interrupt;

extern const std::string help_filename;


/// Get current date and time as formatted string
std::string time_now ();

enum OutputStyle  { os_normal, os_warn, os_quot, os_input };

#endif


// This file is part of fityk program. Copyright 2001-2013 Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

///  Various headers and definitions. Included by almost all files.

#ifndef FITYK__COMMON__H__
#define FITYK__COMMON__H__

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#ifndef VERSION
#   define VERSION "unknown"
#endif

#include <string>
#include <vector>
#include <cmath>

#include "fityk.h" //ExecuteError

// MS VC++ has no erf, erfc, trunc, snprintf functions
#ifdef _MSC_VER
#include <boost/math/special_functions/erf.hpp>
using boost::math::erf;
using boost::math::erfc;
inline double trunc(double a) { return a >= 0 ? floor(a) : ceil(a); }
#define snprintf sprintf_s
// disable warning about unsafe sprintf
#pragma warning( disable : 4996 )
#endif

#ifdef NDEBUG
#define soft_assert(expr) (void) 0
#else
#define soft_assert(expr) \
  if (!(expr)) \
      fprintf(stderr, "WARNING: failed assertion `%s' in %s:%d\n", \
                      #expr, __FILE__, __LINE__)
#endif

//---------------------------------------------------------------------------
// inline helper functions that we keep outside of namespace

/// Round real to integer.
inline int iround(double d) { return static_cast<int>(floor(d+0.5)); }

template <typename T, int N>
std::string format1(const char* fmt, T t)
{
    char buffer[N];
    snprintf(buffer, N, fmt, t);
    buffer[N-1] = '\0';
    return std::string(buffer);
}

/// S() converts to string

// generic version - disabled to prevent bugs such as printing pointer address
//template <typename T> inline std::string S(T k)
// { return static_cast<std::ostringstream&>(std::ostringstream() << k).str(); }
inline std::string S(bool b) { return b ? "true" : "false"; }
inline std::string S(const char *k) { return std::string(k); }
inline std::string S(char k) { return std::string(1, k); }
inline std::string S(const std::string& k) { return k; }
inline std::string S() { return std::string(); }
inline std::string S(int n) { return format1<int, 16>("%d", n); }
inline std::string S(long n) { return format1<long, 21>("%ld", n); }
inline std::string S(size_t n)
    { return format1<size_t, 21>("%lu", (unsigned long) n); }
inline std::string S(double d) { return format1<double, 16>("%g", d); }
// more exact version of S(); convert double number with 12 significant digits
inline std::string eS(double d) { return format1<double, 24>("%.12g", d); }
inline std::string S(long double d) { return S((double) d); }

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

/// check if vector (first arg) contains given element (second arg)
template<typename T, typename T2>
bool contains_element(T const& vec, T2 const& t)
{
    return (find(vec.begin(), vec.end(), t) != vec.end());
}

/// check if string (first arg) contains given substring (second arg)
template<typename T, typename T2>
bool contains_element(std::basic_string<T> const& str, T2 const& t)
{
    return (str.find(t) != std::basic_string<T>::npos);
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

/// similar to Python string.startswith() method
inline bool startswith(std::string const& s, std::string const& p) {
    return p.size() <= s.size() && std::string(s, 0, p.size()) == p;
}
/// similar to Python string.endswith() method
inline bool endswith(std::string const& s, std::string const& p) {
    return p.size() <= s.size() && std::string(s, s.size() - p.size()) == p;
}

/// similar to Python string.strip() method
inline std::string strip_string(std::string const &s) {
    char const *blank = " \r\n\t";
    std::string::size_type first = s.find_first_not_of(blank);
    if (first == std::string::npos)
        return std::string();
    std::string::size_type last = s.find_last_not_of(blank);
    if (first == 0 && last == s.size() - 1)
        return s;
    else
        return std::string(s, first, last-first+1);
}

//---------------------------  V E C T O R  --------------------------------

// boost/foreach.hpp includes quite a lot of code. Since only one version
// is needed here, let's keep it simple
#define v_foreach(type, iter, vec) \
for (vector<type>::const_iterator iter = vec.begin(); iter != vec.end(); ++iter)

#define vm_foreach(type, iter, vec) \
for (vector<type>::iterator iter = vec.begin(); iter != vec.end(); ++iter)

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
    { std::vector<T> v = std::vector<T>(3); v[0]=a; v[1]=b; v[2]=c; return v;}

/// Make 4-element vector
template <typename T>
inline std::vector<T> vector4 (T a, T b, T c, T d) {
    std::vector<T> v = std::vector<T>(4); v[0]=a; v[1]=b; v[2]=c; v[3]=d;
    return v;
}

/// Make n-element vector, e.g.: vector_of<int>(1)(2)(3)(4)(5)
//template <typename T>
//struct vector_of: public std::vector<T>
//{
//    vector_of(const T& t) { (*this)(t); }
//    vector_of& operator()(const T& t) { this->push_back(t); return *this; }
//};

/// Return 0 <= n < a.size()
template <typename T>
inline bool is_index (int idx, std::vector<T> const& v)
{
    return idx >= 0 && idx < static_cast<int>(v.size());
}


template <typename RandomAccessIterator>
inline std::string join(RandomAccessIterator first, RandomAccessIterator last,
                        std::string const& sep)
{
    if (last - first <= 0)
        return "";
    std::string s = S(*first);
    for (RandomAccessIterator i = first + 1; i != last; ++i)
        s += sep + S(*i);
    return s;
}

template <typename T>
inline std::string join_vector(std::vector<T> const& v, std::string const& sep)
{
    return join(v.begin(), v.end(), sep);
}

/// delete all objects handled by pointers and clear vector
template<typename T>
void purge_all_elements(std::vector<T*> &vec)
{
    for (typename std::vector<T*>::iterator i=vec.begin(); i!=vec.end(); ++i)
        delete *i;
    vec.clear();
}


namespace fityk {

//--------------------------  N U M E R I C  --------------------------------

/// epsilon is used for comparision of real numbers
/// defined in settings.cpp; it can be changed in Settings
extern FITYK_API double epsilon;

inline bool is_eq(double a, double b) { return fabs(a-b) <= epsilon; }
inline bool is_neq(double a, double b) { return fabs(a-b) > epsilon; }
inline bool is_lt(double a, double b) { return a < b - epsilon; }
inline bool is_gt(double a, double b) { return a > b + epsilon; }
inline bool is_le(double a, double b) { return a <= b + epsilon; }
inline bool is_ge(double a, double b) { return a >= b - epsilon; }
inline bool is_zero(double a) { return fabs(a) <= epsilon; }

inline bool is_finite(double a)
// to avoid "deprecated finite()" warning on OSX 10.9 we try first isfinite()
#if defined(HAVE_ISFINITE)
    { return isfinite(a); }
#elif defined(HAVE_FINITE)
    { return finite(a); }
#else
    { return a == a; } // this checks only for NaN (better than nothing)
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

//---------------------------  S T R I N G  --------------------------------

/// True if the string contains only a real number
bool is_double (std::string const& s);

/// True if the string contains only an integer number
bool is_int (std::string const& s);

/// replace all occurences of old in string s with new_
FITYK_API void replace_all(std::string &s, std::string const &old,
                                           std::string const &new_);

void replace_words(std::string &t, std::string const &old_word,
                                   std::string const &new_word);

std::string::size_type find_matching_bracket(std::string const& formula,
                                             std::string::size_type left_pos);

/// matches name against pattern containing '*' (wildcard)
bool match_glob(const char* name, const char* pattern);


//                           v e c t o r

/// Make (u-l)-element vector, filled by numbers: l, l+1, ..., u-1.
FITYK_API std::vector<int> range_vector(int l, int u);

/// Expression like "i<v.size()", where i is int and v is a std::vector gives:
/// "warning: comparison between signed and unsigned integer expressions"
/// implicit cast IMHO makes code less clear than "i<size(v)":
template <typename T>
inline int size(std::vector<T> const& v) { return static_cast<int>(v.size()); }

//----------------  filename utils  -------------------------------------
#if defined(_WIN32) || defined(WIN32) || defined(__NT__) || defined(__WIN32__) || defined(__OS2__)
#define PATH_COMPONENT_SEP '\\'
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

/// used to put version to script
extern FITYK_API const char* fityk_version_line;

extern const std::string help_filename;


/// Get current date and time as formatted string
std::string time_now();

} // namespace fityk
#endif


// This file is part of fityk program. Copyright (C) Marcin Wojdyr 
// $Id$

/*
 *  various headers and definitions. Included by all files.
 */
#ifndef COMMON__H__
#define COMMON__H__

#if HAVE_CONFIG_H   
#  include <config.h>  
#endif 

#ifndef VERSION
#   define VERSION "unknown"
#endif

#define USE_XTAL 1

#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <math.h>
#include <assert.h>

// favourite floating point type 
#ifdef FP_IS_LDOUBLE
typedef long double fp ;  
#else
typedef double fp ;  
#endif

extern const fp INF;


#ifndef M_PI
# define M_PI    3.1415926535897932384626433832795029  /* pi */
#endif
#ifndef M_LN2
# define M_LN2   0.6931471805599453094172321214581766  /* log_e 2 */
#endif

extern const std::vector<fp> fp_v0; //just empty vector
extern const std::vector<int> int_v0; //just empty vector

/* idea taken from gnuplot:
 * some machines have trouble with exp(-x) for large x
 * if MINEXP is defined at compile time, use exp_(x) instead,
 * which returns 0 for exp_(x) with x < MINEXP
 */
#ifdef MINEXP
  inline fp exp_(fp x) { return (x < (MINEXP)) ? 0.0 : exp(x); }
#else
#  define exp_(x) exp(x)
#endif

//this is only for RCS ident
//static const char *RCSid (const char *s); //for RCSID
//#define RCSID(x)  static const char *RCSid(const char *s) { return RCSid(x); }
#define RCSID(x)  static const char *RCSid = x; static const char *RCSid_() { return RCSid ? RCSid : RCSid_(); }

extern char verbosity;

//S() converts to string
template <typename T>
inline std::string S(T k) {
    return static_cast<std::ostringstream&>(std::ostringstream() << k).str();
}

inline std::string S(const char *k) { return std::string(k); }
inline std::string S(char *k) { return std::string(k); }
inline std::string S(const char k) { return std::string(1, k); }
inline std::string S() { return std::string(); }

//makes 1-element vector
template <typename T>
inline std::vector<T> vector1 (T a) { return std::vector<T>(1, a); }

//makes 2-element vector
template <typename T>
inline std::vector<T> vector2 (T a, T b) 
    { std::vector<T> v = std::vector<T>(2, a); v[1] = b; return v;}

//makes 3-element vector
template <typename T>
inline std::vector<T> vector3 (T a, T b, T c) 
    { std::vector<T> v = std::vector<T>(3, a); v[1] = b; v[2] = c; return v;}

//makes 4-element vector
template <typename T>
inline std::vector<T> vector4 (T a, T b, T c, T d) { 
    std::vector<T> v = std::vector<T>(4, a); v[1] = b; v[2] = c; v[3] = d;
    return v; 
}

std::vector<int> range_vector (int l, int u);

//usually i'm using `int', not `unsigned int', and expression like "i<v.size()"
//where v is a std::vector gives: 
//"warning: comparison between signed and unsigned integer expressions"
//implicit cast is too long and IMHO makes code less clear than this:
template <typename T>
inline int size(const std::vector<T>& v) { return static_cast<int>(v.size()); }

template <typename T>
inline bool is_index (int idx, const std::vector<T>& v) 
{ 
    return idx >= 0 && idx < static_cast<int>(v.size()); 
}

inline int iround(fp d) { return static_cast<int>(floor(d+0.5)); }

extern bool exit_on_error;
extern char auto_plot;
extern int smooth_limit;
extern volatile bool user_interrupt;
extern const std::string help_filename;

void gmessage (const std::string &str);

int warn (const std::string &s);

inline void mesg (const std::string &s) { if (verbosity >= 3) gmessage (s); }

inline void verbose (const std::string &s) { if (verbosity >= 4) gmessage (s); }

//in version below x is evalueted only when verbosity >= 4
#define verbose_lazy(x) \
    if(verbosity >= 4) { \
        gmessage((x)); \
    } 

inline void very_verbose (const std::string &s) 
    { if (verbosity >= 5) gmessage (s); }

int my_sleep (int seconds);
std::string time_now ();
bool is_double (std::string s);


class Sum;
class Data;
class V_f;
class V_z;
class V_g;
class LMfit;
class GAfit;
class NMfit;
class Crystal;
class GnuPlot;

#endif


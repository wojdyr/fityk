#ifndef XYLIB__COMMON__H__
#define XYLIB__COMMON__H__


// switches
//////////////////////////////////////////////////////////////////////////
/// favourite floating point type 
#ifdef FP_IS_LDOUBLE
typedef long double fp;  
#else
typedef double fp;  
#endif

#include <string>
#include <vector>


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


template <typename T>
inline int size(std::vector<T> const& v) { return static_cast<int>(v.size()); }

#endif

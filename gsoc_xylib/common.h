#ifndef XYLIB__COMMON__H__
#define XYLIB__COMMON__H__


#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <boost/detail/endian.hpp>
#include <boost/cstdint.hpp>


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

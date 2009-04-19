// private helper functions (namespace xylib::util)
// Licence: Lesser GNU Public License 2.1 (LGPL) 
// $Id$

#ifndef XYLIB_UTIL_H_
#define XYLIB_UTIL_H_

#include <string>
#include <vector>
#include <stdexcept>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <cassert>
#include <cmath>

#include "xylib.h"

namespace xylib { namespace util {

void le_to_host(void *ptr, int size);

unsigned int read_uint32_le(std::istream &f);
unsigned int read_uint16_le(std::istream &f);
int read_int16_le(std::istream &f);
float read_flt_le(std::istream &f);
double read_dbl_le(std::istream &f);

char read_char(std::istream &f);
std::string read_string(std::istream &f, unsigned len);

std::string str_trim(std::string const& str);
void str_split(std::string const& line, std::string const& sep, 
               std::string &key, std::string &val);
bool str_startwith(const std::string &str_src, const std::string &ss);
std::string str_tolower(const std::string &str);

std::string read_line(std::istream &is);
bool get_valid_line(std::istream &is, std::string &line, char comment_char);

void skip_whitespace(std::istream &f);
Column* read_start_step_end_line(std::istream& f);
Block* read_ssel_and_data(std::istream &f, int max_headers=0);

long my_strtol(const std::string &str);
double my_strtod(const std::string &str);

inline bool is_numeric(int c) 
    { return isdigit(c) || c=='+' ||  c=='-' || c=='.'; }

/// Round real to integer.
inline int iround(double d) { return static_cast<int>(floor(d+0.5)); }

// vector "constructors" 

inline std::vector<std::string> vector_string(std::string const& s1) 
  { return std::vector<std::string>(1, s1); }

inline std::vector<std::string> vector_string(std::string const& s1,
                                              std::string const& s2) 
  { std::vector<std::string> r(2); r[0] = s1; r[1] = s2; return r; }

inline std::vector<std::string> vector_string(std::string const& s1,
                                              std::string const& s2, 
                                              std::string const& s3) 
  { std::vector<std::string> r(3); r[0] = s1; r[1] = s2; r[2] = s3; return r; }

inline std::vector<std::string> vector_string(std::string const& s1,
                                              std::string const& s2, 
                                              std::string const& s3, 
                                              std::string const& s4) 
  { std::vector<std::string> r(4); r[0] = s1; r[1] = s2; r[2] = s3; r[3] = s4; 
    return r; }

/// S() converts any type to string
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


// column uses vector<double> to represent the data 
class VecColumn : public Column
{
public:
    VecColumn() : Column(0.) {}
    
    // implementation of the base interface 
    int get_point_count() const { return data.size(); }
    double get_value (int n) const
    {
        if (n < 0 || n >= get_point_count())
            throw RunTimeError("index out of range in VecColumn");
        return data[n];
    }

    void add_val(double val) { data.push_back(val); }
    void add_values_from_str(std::string const& str, char sep=' '); 
    double get_min() const;
    double get_max(int point_count=0) const;
    
protected:
    std::vector<double> data; 
    mutable double min_val, max_val;

    void calculate_min_max() const;
};


// column of fixed-step data 
class StepColumn : public Column
{
public:
    double start;
    int count; // -1 means unlimited...

    StepColumn(double start_, double step_, int count_ = -1) 
        : Column(step_), start(start_), count(count_) 
    {}

    int get_point_count() const { return count; }
    double get_value(int n) const
    {
        if (count != -1 && (n < 0 || n >= count))
            throw RunTimeError("point index out of range");
        return start + step * n;
    }
    double get_min() const { return start; }
    double get_max(int point_count=0) const
    {
        assert (point_count != 0 || count != -1);
        int n = (count == -1 ? point_count : count);
        return get_value(n-1);
    }
};

} // namespace util
} // namespace xylib

#endif // XYLIB_UTIL_H_

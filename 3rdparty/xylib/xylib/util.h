// Internal-used helper functions in namespace xylib::util 
// Licence: Lesser GNU Public License 2.1 (LGPL) 
// $Id: util.h $

#ifndef XYLIB__UTIL__H__
#define XYLIB__UTIL__H__

#include <string>
#include <vector>
#include <stdexcept>
#include <sstream>
#include <fstream>
#include <iomanip>

#include "xylib.h"

namespace xylib
{

// sub namespace that holds the utility functions
namespace util 
{
    unsigned read_uint32_le(std::istream &f);
    unsigned read_uint16_le(std::istream &f);
    int read_int16_le(std::istream &f);
    float read_flt_le(std::istream &f);
    double read_dbl_le(std::istream &f);
    char read_char(std::istream &f);
    std::string read_string(std::istream &f, unsigned len);

    void le_to_host(void *ptr, int size);

    std::string str_trim(std::string const& str);
    void str_split(std::string const& line, std::string const& sep, 
                   std::string &key, std::string &val);
    bool str_startwith(const std::string &str_src, const std::string &ss);
    std::string str_tolower(const std::string &str);

    void skip_lines(std::istream &f, int count);
    std::string read_line(std::istream &is);
    bool get_valid_line(std::istream &is, std::string &line, char comment_char);

    long my_strtol(const std::string &str);
    double my_strtod(const std::string &str);
    void my_read(std::istream &f, char *buf, int len);

    void my_assert(bool condition, const std::string &msg);

    inline bool is_numeric(int c) 
        { return isdigit(c) || c=='+' ||  c=='-' || c=='.'; }


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
        


// column uses vector<double> to represent the data 
class VecColumn : public Column
{
public:
    VecColumn() : Column(0.) {}
    
    // implementation of the base interface 
    int get_pt_cnt() const { return dat.size(); }
    double get_value (int n) const
    {
        my_assert(n >= 0 && n < get_pt_cnt(),"index out of range in VecColumn");
        return dat[n];
    }

    void add_val(double val) { dat.push_back(val); }
    void add_values_from_str(std::string const& str, char sep=' '); 
    
protected:
    std::vector<double> dat; 
};


//////////////////////////////////////////////////////////////////////////
// column of fixed-step data 
class StepColumn : public Column
{
public:
    double start;
    int count; // -1 means unlimited...

    StepColumn(double start_, double step_, int count_ = -1) 
        : Column(step_), start(start_), count(count_) 
    {}

    int get_pt_cnt() const { return count; }
    double get_value(int n) const
    {
        my_assert(count == -1 || (n>=0 && n<count), "point index out of range");
        return start + step * n;
    }
};


} // end of namespace util
} // end of namespace xylib


#endif //ifndef XYLIB__UTIL__H__

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

    void le_to_host_2(void *ptr);
    void le_to_host_4(void *ptr);
    void le_to_host_8(void *ptr);

    std::string str_trim(std::string const& str);
    void str_split(std::string const& line, std::string const& sep, 
                   std::string &key, std::string &val);
    bool str_startwith(const std::string &str_src, const std::string &ss);
    std::string str_tolower(const std::string &str);

    void get_all_numbers(std::string &line, std::vector<double>& result_numbers);
    int read_line_and_get_all_numbers(std::istream &is, 
        std::vector<double>& result_numbers);

    inline std::string my_getline(std::istream &f)
    {
        std::string line;
        if (!std::getline(f, line))
            throw XY_Error("unexpected end of file");
        return str_trim(line);
    }


    void skip_lines(std::istream &f, const int count);
    int read_line_int(std::istream &is);
    double read_line_double(std::istream &is);
    std::string read_line(std::istream &is);
    bool get_valid_line(std::istream &is, std::string &line, char comment_char);

    long my_strtol(const std::string &str);
    double my_strtod(const std::string &str);
    void my_read(std::istream &f, char *buf, int len);

    // find the index of @find_str in @array
    int get_array_idx(const std::string *array, 
        unsigned size,
        const std::string &find_str);

    void my_assert(bool condition, const std::string &msg);

    inline bool is_numeric(int c) 
        { return isdigit(c) || c=='+' ||  c=='-' || c=='.'; }


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
        
} // end of namespace util

} // end of namespace xylib


#endif //ifndef XYLIB__UTIL__H__

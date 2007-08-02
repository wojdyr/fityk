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

namespace xylib
{

// sub namespace to hold the utility functions
// move the original XY_Lib static member functions here
namespace util 
{
    unsigned read_uint32_le(std::ifstream &f, unsigned offset);
    unsigned read_uint16_le(std::ifstream &f, unsigned offset);
    unsigned read_int16_le(std::ifstream &f, unsigned offset);
    float read_flt_le(std::ifstream &f, unsigned offset);
    double read_dbl_le(std::ifstream &f, unsigned offset);
    std::string read_string(std::ifstream &f, unsigned offset, unsigned len);
    void le_to_host(void *p, unsigned len);

    // convert a float number to string. if 2nd param is true, return "undefined"
    std::string my_flt_to_string(float num, float undef);

    void rm_space(std::string &str);
    std::string str_trim(const std::string &str, std::string ws = " \r\n\t");
    void parse_line(const std::string &line, const std::string &sep, 
        std::string &key, std::string &val);
    bool str_startwith(const std::string &str_src, const std::string &ss);
    std::string str_tolower(const std::string &str);

    int read_line_and_get_all_numbers(std::istream &is, 
        std::vector<double>& result_numbers);

    bool peek_line(std::ifstream &f, std::string &line, bool throw_eof = true);
    bool my_getline(std::ifstream &f, std::string &line, bool throw_eof = true);
    void skip_lines(std::ifstream &f, const int count);
    int read_line_int(std::ifstream& is);
    double read_line_double(std::ifstream& is);
    std::string read_line(std::ifstream& is);

    // find the index of @find_str in @array
    int get_array_idx(const std::string *array, 
        unsigned size,
        const std::string &find_str);

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

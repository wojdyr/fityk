// Many internal-used helper functions are placed in namespace xylib::util  
// Licence: Lesser GNU Public License 2.1 (LGPL) 
// $Id: util.cpp $

#include "util.h"
#include "xylib.h"

#include <cmath>
#include <climits>
#include <algorithm>
#include <boost/detail/endian.hpp>
#include <boost/cstdint.hpp>

#if !defined(BOOST_LITTLE_ENDIAN) && !defined(BOOST_BIG_ENDIAN)
#error "Unknown endianess"
#endif

using namespace std;
using namespace xylib::util;
using namespace boost;

namespace xylib {

//////////////////////////////////////////////////////////////////////////
// the functions in xylib::util, which are only used internally
namespace util{
    // read a 32-bit, little-endian form int from f,
    // return equivalent int in host endianess
    unsigned read_uint32_le(istream &f)
    {
        uint32_t val;
        my_read(f, reinterpret_cast<char*>(&val), sizeof(val));
        le_to_host_4(&val);
        return val;
    }

    unsigned read_uint16_le(istream &f)
    {
        uint16_t val;
        my_read(f, reinterpret_cast<char*>(&val), sizeof(val));
        
        le_to_host_2(&val);
        return val;
    }

    int read_int16_le(istream &f)
    {
        int16_t val;
        my_read(f, reinterpret_cast<char*>(&val), sizeof(val));
        
        le_to_host_2(&val);
        return val;
    }

    // the same as above, but read a float
    float read_flt_le(istream &f)
    {
        float val;
        my_read(f, reinterpret_cast<char*>(&val), sizeof(val));
        
        le_to_host_4(&val);
        return val;
    }

    // the same as above, but read a float
    double read_dbl_le(istream &f)
    {
        double val;
        my_read(f, reinterpret_cast<char*>(&val), sizeof(val));
        
        le_to_host_8(&val);
        return val;
    }

    char read_char(istream &f) {
        char ret;
        f.read(&ret, 1);
        if (static_cast<unsigned>(f.gcount()) < 1) {
            throw XY_Error("unexpected eof in read_string()");
        }
        return ret;
    }

    // read a string from f
    string read_string(istream &f, unsigned len) 
    {
        static char buf[65536];
        if (len >= sizeof(buf)) {
            throw XY_Error("buffer overflow");
        }
        
        f.read(buf, len);
        if (static_cast<unsigned>(f.gcount()) < len) {
            throw XY_Error("unexpected eof in read_string()");
        }
        
        buf[len] = '\0';
        return string(buf);
    }
    
/////////////////////////

    // trim the string 
    string str_trim(string const& str)
    {
        std::string ws = " \r\n\t";
        string::size_type first, last;
        first = str.find_first_not_of(ws);
        last = str.find_last_not_of(ws);
        if (first == string::npos) 
            return "";
        return str.substr(first, last - first + 1);
    }


    // skip whitespace, get key and val that are separated by `sep'
    void str_split(string const& line, string const& sep, 
                   string &key, string &val)
    {
        string::size_type p = line.find_first_of(sep);
        if (p == string::npos) {
            key = line;
            val = "";
        }
        else {
            key = str_trim(line.substr(0, p));
            val = str_trim(line.substr(p + sep.size()));
        }
    }

    
    // true if str starts with ss
    bool str_startwith(const string &str, const string &ss)
    {
        return str.size() >= ss.size() && ss == str.substr(0, ss.size());
    }

    // change all letters in a string to lower case
    std::string str_tolower(const std::string &str)
    {
        string ret;
        for(string::const_iterator it = str.begin(); it != str.end(); ++it) {
            char ch = tolower(*it);
            ret.append(S(ch));
        }
        return ret;
    }


    // change the byte-order from "little endian" to host endian
    // for more info, see item "endian" in wikipedia (www.wikipedia.org)
    // le_to_host_x are versions for x-byte atomic element
    // ptr: pointer to the data
   
#if defined(BOOST_BIG_ENDIAN)
    void le_to_host_2(void *ptr)
    {
        char *p = ptr;
        swap(p[0], p[1]);
    }


    void le_to_host_4(void *ptr)
    {
        char *p = ptr;
        swap(p[0], p[3]);
        swap(p[1], p[2]);
    }


    void le_to_host_8(void *ptr)
    {
        char *p = ptr;
        swap(p[0], p[7]);
        swap(p[1], p[6]);
        swap(p[2], p[5]);
        swap(p[3], p[4]);
    }
#else
    void le_to_host_2(void *) {}
    void le_to_host_4(void *) {}
    void le_to_host_8(void *) {}
#endif


    // get all numbers in the first legal line
    void get_all_numbers(std::string &line, std::vector<double>& result_numbers)
    {
        for (string::iterator i = line.begin(); i != line.end(); i++) {
            if (*i == ',' || *i == ';' || *i == ':') {
                *i = ' ';
            }
        }

        string::size_type val_spos(0), val_epos(0);
        while (true) {
            val_spos = line.find_first_not_of(" \t,", val_epos);
            if (string::npos == val_spos) {
                break;
            }

            val_epos = line.find_first_of(" \t,", val_spos);

            string val = line.substr(val_spos, val_epos - val_spos);
            result_numbers.push_back(my_strtod(val));
        }
    }

    // skip meaningless lines and get all numbers in the first legal line
    int read_line_and_get_all_numbers(istream &is, 
        vector<double>& result_numbers)
    {
        // returns number of numbers in line
        result_numbers.clear();
        string s;
        while (getline(is, s) && 
            (s.empty() || s[0] == '#' 
            || s.find_first_not_of(" \t\r\n") == string::npos))
            ;   // ignore lines with '#' at first column or empty lines
        for (string::iterator i = s.begin(); i != s.end(); i++) {
            if (*i == ',' || *i == ';' || *i == ':') {
                *i = ' ';
            }
        }

        istringstream q(s);
        double d;
        while (q >> d) {
            result_numbers.push_back(d);
        }
        return !is.eof();
    }


    // skip "count" lines in f
    void skip_lines(istream &f, const int count)
    {
        string line;
        for (int i = 0; i < count; ++i) {
            if (!getline(f, line)) {
                throw XY_Error("unexpected end of file");
            }
        }
    }

    int read_line_int(istream& is)
    {
        string str;
        if (!getline(is, str)) {
            throw xylib::XY_Error("unexpected end of file");
        }
        return my_strtol(str);
    }

    double read_line_double(istream& is)
    {
        string str;
        if (!getline(is, str)) {
            throw xylib::XY_Error("unexpected end of file");
        }
        return my_strtod(str);
    }

    // read a line and return it as a string
    string read_line(istream& is) 
    {
        string line;
        if (!getline(is, line)) {
            throw xylib::XY_Error("unexpected end of file");
        }
        return str_trim(line);
    }

    // get a line that is not empty and not a comment
    bool get_valid_line(std::istream &is, std::string &line, char comment_char)
    {
        // read until get a valid line
        while (getline (is, line)) {
            line = str_trim(line);
            if (!line.empty() && line[0] != comment_char) 
                break;
        }

        // trim the "single-line" comments at the tail of a valid line, if any
        string::size_type pos = line.find_first_of(comment_char);
        if (pos != string::npos) {
            line = str_trim(line.substr(0, pos));
        }

        return !is.eof();
    }


    // get the index of find_str in array. return -1 if not exists
    int get_array_idx(const string *array, 
        unsigned size,
        const string &find_str)
    {
        const string *pos = find(array, array + size, find_str);
        if (array + size == pos) {
            return -1;
        } else {
            return (pos - array);
        }
    }


    long my_strtol(const std::string &str) 
    {
        string ss = str_trim(str);
        const char *startptr = ss.c_str();
        char *endptr = NULL;
        long val = strtol(startptr, &endptr, 10);

        if ((LONG_MAX == val) || (LONG_MIN == val)) {
            throw XY_Error("overflow when reading long");
        } else if ((0 == val) && (startptr == endptr)) {
            throw XY_Error("not an integer as expected");
        }

        return val;
    }


    double my_strtod(const std::string &str) 
    {
        const char *startptr = str.c_str();
        char *endptr = NULL;
        double val = strtod(startptr, &endptr);

        if ((HUGE_VAL == val) || (-HUGE_VAL == val)) {
            throw XY_Error("overflow when reading double");
        } else if (startptr == endptr) {
            throw XY_Error("not a double as expected");
        }

        return val;
    }


    void my_read(istream &f, char *buf, int len)
    {
        f.read(buf, len);
        if (f.gcount() < len) {
            throw XY_Error("unexpected eof");
        }
    }


    void my_assert(bool condition, const string &msg)
    {
        if (!condition) {
            throw XY_Error(msg);
        }
    }


} // end of namespace util
} // end of namespace xylib


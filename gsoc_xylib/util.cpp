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
        
        le_to_host_8(&val);
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


    // read a string from f
    string read_string(istream &f, unsigned len) 
    {
        static char buf[65536];
        if (len > sizeof(buf)) {
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
    string str_trim(const string &str, string ws /* = " \r\n\t" */)
    {
        string::size_type first, last;
        first = str.find_first_not_of(ws);
        last = str.find_last_not_of(ws);
        if (string::npos == first) {
            return "";
        }
        return str.substr(first, last - first + 1);
    }


    // parse a line. First skip the space chars, then get the separated key and val
    // e.g. after calling parse_line("a =  2.6", "=", key, val), key=='a' and val=='2.6'
    void parse_line(const string &line, const string &sep, 
        string &key, string &val)
    {
        string line2(line);
        string::size_type len1 = line2.find_first_of(sep);
        if (string::npos == len1) {
            key = line;
            val = "";
            return;
        }
        
        key = line2.substr(0, len1);
        key = str_trim(key);

        string::size_type off2 = len1 + sep.size();
        val = line2.substr(off2);
        val = str_trim(val);
    }

    
    // decide whether str_src starts with ss
    bool str_startwith(const string &str_src, const string &ss)
    {
        string sub = str_src.substr(0, ss.size());
        return (sub == ss);
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
    // p: pointer to the data
   
    void le_to_host_2(void *p)
    {
        if (!p) {
            return;
        }
        
#if defined(BOOST_LITTLE_ENDIAN)
        return;
#elif defined(BOOST_BIG_ENDIAN)
        uint8_t bytes[2];
        memcpy(bytes, p, sizeof(bytes));
        (reinterpret_cast<uint8_t*>(p))[0] = bytes[1];
        (reinterpret_cast<uint8_t*>(p))[1] = bytes[0];
#else
        throw XY_ERROR("system uses unsupported endianess");
#endif
    }


    void le_to_host_4(void *p)
    {
        if (!p) {
            return;
        }
        
#if defined(BOOST_LITTLE_ENDIAN)
        return;
#elif defined(BOOST_BIG_ENDIAN)
        uint8_t bytes[4];
        memcpy(bytes, p, sizeof(bytes));
        (reinterpret_cast<uint8_t*>(p))[0] = bytes[3];
        (reinterpret_cast<uint8_t*>(p))[1] = bytes[2];
        (reinterpret_cast<uint8_t*>(p))[2] = bytes[1];
        (reinterpret_cast<uint8_t*>(p))[3] = bytes[0];
#else
        throw XY_ERROR("system uses unsupported endianess");
#endif
    }


    void le_to_host_8(void *p)
    {
        if (!p) {
            return;
        }
        
#if defined(BOOST_LITTLE_ENDIAN)
        return;
#elif defined(BOOST_BIG_ENDIAN)
        uint8_t bytes[8];
        memcpy(bytes, p, sizeof(bytes));
        (reinterpret_cast<uint8_t*>(p))[0] = bytes[7];
        (reinterpret_cast<uint8_t*>(p))[1] = bytes[6];
        (reinterpret_cast<uint8_t*>(p))[2] = bytes[5];
        (reinterpret_cast<uint8_t*>(p))[3] = bytes[4];
        (reinterpret_cast<uint8_t*>(p))[4] = bytes[3];
        (reinterpret_cast<uint8_t*>(p))[5] = bytes[2];
        (reinterpret_cast<uint8_t*>(p))[6] = bytes[1];
        (reinterpret_cast<uint8_t*>(p))[7] = bytes[0];
#else
        throw XY_ERROR("system uses unsupported endianess");
#endif
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

    // get a valid line from "is"
    // line: out param, will take the line back
    // cmt_start: a string consists of all possible "comment start" chars
    bool get_valid_line(std::istream &is, std::string &line, std::string cmt_start)
    {
        // read until get a valid line
        while (getline (is, line)) {
            line = str_trim(line);
            if (!line.empty() && cmt_start.find_first_of(line[0]) == string::npos) {
                break;
            }
        }

        // trim the "single-line" comments at the tail of a valid line, if any
        string::size_type pos = line.find_first_of(cmt_start);
        if (string::npos != pos) {
            line = str_trim(line.substr(0, pos));
        }

        return !is.eof();
    }

/* 
    // remove all space chars in str, not used now
    void rm_spaces(string &str)
    {
        string ss(str);
        str.clear();
        do {
            string::size_type first_pos = ss.find_first_not_of(" \t\r\n");
            if (string::npos == first_pos) {
                break;
            } else {
                ss = ss.substr(first_pos);
                str.append(&ss[0],1);
                ss = ss.substr(1);
            }
        } while(ss.size() > 0);
    }
*/

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

    // preview the next line in "f"
    // return: true if not eof, false otherwise
    bool peek_line(std::istream &f, std::string &line, 
        bool throw_eof /* = true */ )
    {
        int pos = f.tellg();
        string ll;
        bool ret;
        
        getline(f, ll);
        ret = !f.eof();
        if (throw_eof && !ret) {
            throw XY_Error("unexpected end of file");
        }
        f.seekg(pos);

        line = str_trim(ll);
        return ret;
    }


    bool my_getline(std::istream &f, std::string &line,
        bool throw_eof /* = true */ )
    {
        getline(f, line);
        bool ret = !f.eof();
        if (throw_eof && !ret) {
            throw XY_Error("unexpected end of file");
        }

        line = str_trim(line);
        return ret;
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
        string ss = str_trim(str);
        const char *startptr = ss.c_str();
        char *endptr = NULL;
        double val = strtod(startptr, &endptr);

        if ((HUGE_VAL == val) || (-HUGE_VAL == val)) {
            throw XY_Error("overflow when reading double");
        } else if ((0 == val) && (startptr == endptr)) {
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


    void my_assert(int condition, const string &msg)
    {
        if (!condition) {
            throw XY_Error(msg);
        }
    }
} // end of namespace util
} // end of namespace xylib


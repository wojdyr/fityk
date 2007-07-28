// Many internal-used helper functions are placed in namespace xylib::util  
// Licence: Lesser GNU Public License 2.1 (LGPL) 
// $Id: util.cpp $

#include "util.h"
#include "xylib.h"

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
    unsigned read_uint32_le(ifstream &f, unsigned offset)
    {
        uint32_t val;
        f.seekg(offset);
        f.read(reinterpret_cast<char*>(&val), sizeof(val));
        le_to_host(&val, sizeof(val));
        return val;
    }

    unsigned read_uint16_le(ifstream &f, unsigned offset)
    {
        uint16_t val;
        f.seekg(offset);
        f.read(reinterpret_cast<char*>(&val), sizeof(val));
        le_to_host(&val, sizeof(val));
        return val;
    }

    // the same as above, but read a float
    float read_flt_le(ifstream &f, unsigned offset)
    {
        float val;
        f.seekg(offset);
        f.read(reinterpret_cast<char*>(&val), sizeof(val));
        le_to_host(&val, sizeof(val));
        return val;
    }


    // the same as above, but read a float
    string read_string(ifstream &f, unsigned offset, unsigned len) 
    {
        static char buf[65536];
        f.seekg(offset);
        f.read(buf, len);
        buf[offset + len] = '\0';
        return string(buf);
    }

    // remove all space chars in str, not used now
    void rm_spaces(string &str)
    {
        string ll(str);
        str.clear();
        do {
            string::size_type first_pos = ll.find_first_not_of(" \t\r\n");
            if (string::npos == first_pos) {
                break;
            } else {
                ll = ll.substr(first_pos);
                str.append(&ll[0],1);
                ll = ll.substr(1);
            }
        } while(ll.size() > 0);
    }


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
    // p: pointer to the data
    // len: length of the data type, should be 1, 2, 4
    void le_to_host(void *p, unsigned len)
    {
        /*
        for more info, see "endian" and "pdp-11" items in wikipedia
        www.wikipedia.org
        */
        if ((len != 1) && (len != 2) && (len != 4)) {
            throw XY_Error("len should be 1, 2, 4");
        }

# if defined(BOOST_LITTLE_ENDIAN)
        // no change
        return;
# else  // BIG_ENDIAN or PDP_ENDIAN
        // store the old values
        uint8_t byte0(0), byte1(0), byte2(0), byte3(0);
        switch (len) {
        case 1:
            break;
        case 2:
            byte0 = (reinterpret_cast<uint8_t*>(p))[0];
            byte1 = (reinterpret_cast<uint8_t*>(p))[1];
        case 4:
            byte2 = (reinterpret_cast<uint8_t*>(p))[2];
            byte3 = (reinterpret_cast<uint8_t*>(p))[3];
            break;
        default:
            break;
        }
#  if defined(BOOST_BIG_ENDIAN)
        switch (len) {
        case 1:
            break;
        case 2:
            (reinterpret_cast<uint8_t*>(p))[0] = byte1;
            (reinterpret_cast<uint8_t*>(p))[1] = byte0;
            break;
        case 4:
            (reinterpret_cast<uint8_t*>(p))[0] = byte3;
            (reinterpret_cast<uint8_t*>(p))[1] = byte2;
            (reinterpret_cast<uint8_t*>(p))[2] = byte1;
            (reinterpret_cast<uint8_t*>(p))[3] = byte0;
        }
#  elif defined(BOOST_PDP_ENDIAN)  // PDP_ENDIAN
        // 16-bit word are stored in little_endian
        // 32-bit word are stored in middle_endian: the most significant "half" (16-bits) followed by 
        // the less significant half (as if big-endian) but with each half stored in little-endian format
        switch (len) {
        case 1:
        case 2:
            break;
        case 4:
            (reinterpret_cast<uint8_t*>(p))[0] = byte2;
            (reinterpret_cast<uint8_t*>(p))[1] = byte3;
            (reinterpret_cast<uint8_t*>(p))[2] = byte0;
            (reinterpret_cast<uint8_t*>(p))[3] = byte1;
        }
#  endif    //#  if defined(BOOST_BIG_ENDIAN)
# endif     //# if defined(BOOST_LITTLE_ENDIAN)       
    }

    // convert num to string; if (num == undef), return "undefined"
    string my_flt_to_string(float num, float undef)
    {
        return ((undef == num) ? "undefined" : S(num));
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
    void skip_lines(ifstream &f, const int count)
    {
        string line;
        for (int i = 0; i < count; ++i) {
            if (!getline(f, line)) {
                throw XY_Error("unexpected end of file");
            }
        }
    }

    int read_line_int(ifstream& is)
    {
        string str;
        if (!getline(is, str)) {
            throw xylib::XY_Error("unexpected end of file");
        }
        return strtol(str.c_str(), NULL, 10);
    }

    double read_line_double(ifstream& is)
    {
        string str;
        if (!getline(is, str)) {
            throw xylib::XY_Error("unexpected end of file");
        }
        return strtod(str.c_str(), NULL);
    }

    // read a line and return it as a string
    string read_line(ifstream& is) {
        string line;
        if (!getline(is, line)) {
            throw xylib::XY_Error("unexpected end of file");
        }
        return line;
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

    // preview the next line in "f"
    // return: true if not eof, false otherwise
    bool peek_line(std::ifstream &f, std::string &line, 
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


    bool my_getline(std::ifstream &f, std::string &line,
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


} // end of namespace util
} // end of namespace xylib


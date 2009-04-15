// private helper functions (namespace xylib::util)
// Licence: Lesser GNU Public License 2.1 (LGPL) 
// $Id$

#include "util.h"
#include "xylib.h"

#include <cmath>
#include <climits>
#include <algorithm>
#include <cassert>
#include <cerrno>
#include <boost/detail/endian.hpp>
#include <boost/cstdint.hpp>

#if !defined(BOOST_LITTLE_ENDIAN) && !defined(BOOST_BIG_ENDIAN)
#error "Unknown endianess"
#endif

using namespace std;
using namespace xylib::util;
using namespace boost;

namespace xylib { namespace util {

// -------   standard library functions with added error checking    --------

long my_strtol(const std::string &str) 
{
    string ss = str_trim(str);
    const char *startptr = ss.c_str();
    char *endptr = NULL;
    long val = strtol(startptr, &endptr, 10);

    if (LONG_MAX == val || LONG_MIN == val) {
        throw FormatError("overflow when reading long");
    } else if (startptr == endptr) {
        throw FormatError("not an integer as expected");
    }

    return val;
}

double my_strtod(const std::string &str) 
{
    const char *startptr = str.c_str();
    char *endptr = NULL;
    double val = strtod(startptr, &endptr);

    if (HUGE_VAL == val || -HUGE_VAL == val) {
        throw FormatError("overflow when reading double");
    } else if (startptr == endptr) {
        throw FormatError("not a double as expected");
    }

    return val;
}


// ----------   istream::read()- & endiannes-related utilities   ----------

namespace {
void my_read(istream &f, char *buf, int len)
{
    f.read(buf, len);
    if (f.gcount() < len) {
        throw FormatError("unexpected eof");
    }
}
} // anonymous namespace

// change the byte-order from "little endian" to host endian
// ptr: pointer to the data, size - size in bytes
#if defined(BOOST_BIG_ENDIAN)
void le_to_host(void *ptr, int size)
{
    char *p = (char*) ptr;
    for (int i = 0; i < size/2; ++i)
        swap(p[i], p[size-i-1]);
}
#else
void le_to_host(void *, int) {}
#endif

// read a 32-bit, little-endian form int from f,
// return equivalent int in host endianess
unsigned int read_uint32_le(istream &f)
{
    uint32_t val;
    my_read(f, reinterpret_cast<char*>(&val), sizeof(val));
    le_to_host(&val, sizeof(val));
    return val;
}

unsigned int read_uint16_le(istream &f)
{
    uint16_t val;
    my_read(f, reinterpret_cast<char*>(&val), sizeof(val));
    le_to_host(&val, sizeof(val));
    return val;
}

int read_int16_le(istream &f)
{
    int16_t val;
    my_read(f, reinterpret_cast<char*>(&val), sizeof(val));
    le_to_host(&val, sizeof(val));
    return val;
}

// the same as above, but read a float
float read_flt_le(istream &f)
{
    float val;
    my_read(f, reinterpret_cast<char*>(&val), sizeof(val));
    le_to_host(&val, sizeof(val));
    return val;
}

// the same as above, but read a float
double read_dbl_le(istream &f)
{
    double val;
    my_read(f, reinterpret_cast<char*>(&val), sizeof(val));
    le_to_host(&val, sizeof(val));
    return val;
}

char read_char(istream &f) 
{
    char val;
    my_read(f, &val, sizeof(val));
    return val;
}

// read a string from f
string read_string(istream &f, unsigned len) 
{
    static char buf[256];
    assert(len < sizeof(buf));
    my_read(f, buf, len);
    buf[len] = '\0';
    return string(buf);
}

//    ----------       string utilities       -----------

// Return a copy of the string str with leading and trailing whitespace removed
string str_trim(string const& str)
{
    std::string ws = " \r\n\t";
    string::size_type first = str.find_first_not_of(ws);
    if (first == string::npos) 
        return "";
    string::size_type last = str.find_last_not_of(ws);
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
    return ss == str.substr(0, ss.size());
}

// change all letters in a string to lower case
std::string str_tolower(const std::string &str)
{
    string r(str);
    for (size_t i = 0; i != str.size(); ++i)
        r[i] = tolower(str[i]);
    return r;
}


//      --------   line-oriented file reading functions   --------

// read a line and return it as a string
string read_line(istream& is) 
{
    string line;
    if (!getline(is, line)) 
        throw xylib::FormatError("unexpected end of file");
    return line;
}


// get all numbers in the first legal line
// sep is _optional_ separator that can be used in addition to white space
void VecColumn::add_values_from_str(string const& str, char sep) 
{
    const char* p = str.c_str();
    while (isspace(*p) || *p == sep)
        ++p;
    while (*p != 0) {
        char *endptr = NULL;
        errno = 0; // To distinguish success/failure after call 
        double val = strtod(p, &endptr);
        if (p == endptr)
            throw(xylib::FormatError("Number not found in line:\n" + str));
        if (errno != 0)
            throw(xylib::FormatError("Numeric overflow or underflow in line:\n"
                                     + str));
        add_val(val);
        p = endptr;
        while (isspace(*p) || *p == sep)
            ++p;
    }
}

double VecColumn::get_min() const
{
    calculate_min_max();
    return min_val;
}

double VecColumn::get_max(int /*point_count*/) const
{
    calculate_min_max();
    return max_val;
}

void VecColumn::calculate_min_max() const
{
    static bool has_min_max = false;
    static size_t previous_length = 0;
    // public api of VecColumn don't allow changing data, only appending
    if (has_min_max && data.size() == previous_length)
        return;
    if (data.empty()) {
        min_val = max_val = 0.;
        return;
    }
    min_val = max_val = data[0];
    for (vector<double>::const_iterator i = data.begin() + 1; i != data.end(); 
                                                                         ++i) {
        if (*i < min_val)
            min_val = *i;
        if (*i > max_val)
            max_val = *i;
    }
}


// get a trimmed line that is not empty and not a comment
bool get_valid_line(std::istream &is, std::string &line, char comment_char)
{
    size_t start = 0;
    while (1) {
        if (!getline(is, line))
            return false;
        start = 0;
        while (isspace(line[start])) 
            ++start;
        if (line[start] && line[start] != comment_char)
            break;
    }
    size_t stop = start + 1;
    while (line[stop] && line[stop] != comment_char) 
        ++stop;
    while (isspace(line[stop-1]))
        --stop;
    if (start != 0 || stop != line.size())
        line = line.substr(start, stop - start);
    return true;
}


void skip_whitespace(istream &f)
{
    while (isspace(f.peek()))
        f.ignore();
}

// read line (popular e.g. in powder data ascii file types) in free format:
// start step count
// example:
//   15.000   0.020 110.000
// returns NULL on error
Column* read_start_step_end_line(istream& f)
{
    char line[256];
    f.getline(line, 255); 
    // the first line should contain start, step and stop
    char *endptr;
    const char *startptr = line;
    double start = strtod(startptr, &endptr);
    if (startptr == endptr)
        return NULL;

    startptr = endptr;
    double step = strtod(startptr, &endptr);
    if (startptr == endptr || step == 0.)
        return NULL;

    startptr = endptr;
    double stop = strtod(endptr, &endptr);
    if (startptr == endptr)
        return NULL;

    double dcount = (stop - start) / step + 1;
    int count = iround(dcount);
    if (count < 4 || fabs(count - dcount) > 1e-2)
        return NULL;

    return new StepColumn(start, step, count);
}

Block* read_ssel_and_data(istream &f, int max_headers)
{
    // we are looking for the first line with start-step-end numeric triple,
    // it should be one of the first (max_headers+1) lines
    Column *xcol = read_start_step_end_line(f);
    for (int i = 0; i < max_headers && xcol == NULL; ++i)
        xcol = read_start_step_end_line(f);

    if (!xcol) 
        return NULL;

    Block* blk = new Block;
    blk->add_column(xcol);
    
    VecColumn *ycol = new VecColumn;
    string s;
    // in PSI_DMC there is a text following the data, so we read only as many
    // data lines as necessary
    while (getline(f, s) && ycol->get_point_count() < xcol->get_point_count()) 
        ycol->add_values_from_str(s);
    blk->add_column(ycol);

    // both xcol and ycol should have known and same number of points
    if (xcol->get_point_count() != ycol->get_point_count()) {
        delete blk;
        return NULL;
    }
    return blk;
}

} } // namespace xylib::util


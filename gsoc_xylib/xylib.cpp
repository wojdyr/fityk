// XYlib library is a xy data reading library, aiming to read variaty of xy 
// data formats.
// Licence: GNU General Public License version 2
// $Id: xylib.h $

#include "xylib.h"
#include "common.h"
#include "ds_brucker_raw_v1.h"
#include "ds_brucker_raw_v23.h"
#include "ds_rigaku_dat.h"
#include "ds_text.h"
#include "ds_uxd.h"
#include "ds_vamas.h"

#include <algorithm>
#include <boost/detail/endian.hpp>
#include <boost/cstdint.hpp>

using namespace std;
using namespace xylib;
using namespace util;

using namespace boost;

namespace xylib {

const string g_ftype[] = {
    "",
    "text",
    "uxd",
    "rigaku_dat",
    "diffracat_v1_raw",
    "diffracat_v2v3_raw",
    "vamas_iso14976",
};

const string g_desc[] = {
    "",
    "the ascii plain text format",
    "Siemens/Bruker Diffrac-AT UXD File",
    "Rigaku dat File",
    "Siemens/Bruker Diffrac-AT Raw File v1",
    "Siemens/Bruker Diffrac-AT Raw File v2/v3",
};
    
//////////////////////////////////////////////////////////////////////////
// member functions of Class Range

void Range::check_idx(unsigned n, const string &name) const
{
    if (n >= get_pt_count()) {
        throw XY_Error("no " + name + " in this range with such index");
    }
}

fp Range::get_x(unsigned n) const
{
    check_idx(n, "point_x");
    return x[n];
}

fp Range::get_y(unsigned n) const
{
    check_idx(n, "point_y");
    return y[n];
}


bool Range::has_y_stddev(unsigned n) const
{ 
    if (n >= get_pt_count()) {
        return false;
    } else {
        return y_has_stddev[n]; 
    }    
} 

fp Range::get_y_stddev(unsigned n) const
{
    check_idx(n, "point_y_stddev");
    return y_stddev[n];
}

void Range::add_pt(fp x_, fp y_, fp stddev_)
{
    x.push_back(x_);
    y.push_back(y_);
    y_stddev.push_back(stddev_);
    y_has_stddev.push_back(true);
}

void Range::add_pt(fp x_, fp y_)
{
    x.push_back(x_);
    y.push_back(y_);
    y_stddev.push_back(1);  // meaningless data
    y_has_stddev.push_back(false);
}

void Range::export_xy_file(const string &fname) const
{
    ofstream of(fname.c_str());
    if(!of) {
        throw XY_Error("can't create file " + fname + " to output");
    }

    export_xy_file(of);
}


void Range::export_xy_file(ofstream &of) const
{
    int n = get_pt_count();
    of << ";total count:" << n << endl << endl;
    of << ";x\t\ty\t\t y_stddev" << endl;
    for(int i = 0; i < n; ++i) {
        of << setfill(' ') << setiosflags(ios::fixed) << setprecision(5) << setw(7) << 
            get_x(i) << "\t" << setprecision(8) << setw(10) << get_y(i) << "\t";
        if(has_y_stddev(i)) {
            of << get_y_stddev(i);
        }
        of << endl;
    }
}

void Range::add_meta(const string &key, const string &val)
{
    pair<map<string, string>::iterator, bool> ret = meta_map.insert(make_pair(key, val));
    if (!ret.second) {
        // the meta-info with this key already exists
        throw XY_Error("meta-info with key " + key + "already exists");
    }
}


// return a string vector containing all of the meta-info keys
vector<string> Range::get_all_meta_keys() const
{
    vector<string> keys;

    map<string, string>::const_iterator it = meta_map.begin();
    for (it = meta_map.begin(); it != meta_map.end(); ++it) {
        keys.push_back(it->first);
    }

    return keys;
}

bool Range::has_meta_key(const string &key) const
{
    map<string, string>::const_iterator it = meta_map.find(key);
    return (it != meta_map.end());
}

const string& Range::get_meta(string const& key) const
{
    map<string, string>::const_iterator it;
    it = meta_map.find(key);
    if (meta_map.end() == it) {
        // not found
        throw XY_Error("no such key in meta-info found");
    } else {
        return it->second;
    }
}

//////////////////////////////////////////////////////////////////////////
// member functions of Class FixedStepRange

void FixedStepRange::add_y(fp y_)
{
    y.push_back(y_);
    y_stddev.push_back(1);
    y_has_stddev.push_back(false);
}


void FixedStepRange::add_y(fp y_, fp stddev_)
{
    y.push_back(y_);
    y_stddev.push_back(stddev_);
    y_has_stddev.push_back(true);
}

//////////////////////////////////////////////////////////////////////////
// member functions of Class DataSet

const Range& DataSet::get_range(unsigned i) const
{
    if (i >= ranges.size()) {
        throw XY_Error("no range in this file with such index");
    }

    return *(ranges[i]);
}


void DataSet::export_xy_file(const string &fname) const
{
    unsigned range_num = get_range_cnt();
    ofstream of(fname.c_str());
    if (!of) {
        throw XY_Error("can't create file" + fname);
    }
    for(unsigned i = 0; i < range_num; ++i) {
        of << "; * range " << i << endl;
        get_range(i).export_xy_file(of);
    }
}

void DataSet::add_meta(const string &key, const string &val)
{
    pair<map<string, string>::iterator, bool> ret = meta_map.insert(make_pair(key, val));
    if (!ret.second) {
        // the meta-info with this key already exists
        throw XY_Error("meta-info with key " + key + "already exists");
    }
}


// return a string vector containing all of the meta-info keys
vector<string> DataSet::get_all_meta_keys() const
{
    vector<string> keys;

    map<string, string>::const_iterator it = meta_map.begin();
    for (it = meta_map.begin(); it != meta_map.end(); ++it) {
        keys.push_back(it->first);
    }

    return keys;
}

bool DataSet::has_meta_key(const string &key) const
{
    map<string, string>::const_iterator it = meta_map.find(key);
    return (it != meta_map.end());
}

const string& DataSet::get_meta(string const& key) const
{
    map<string, string>::const_iterator it;
    it = meta_map.find(key);
    if (meta_map.end() == it) {
        // not found
        throw XY_Error("no such key in meta-info found");
    } else {
        return it->second;
    }
}

DataSet::~DataSet()
{
    vector<Range*>::iterator it;
    for (it = ranges.begin(); it != ranges.end(); ++it) {      
        delete *it;
    }
    if (p_ifs) {
        delete p_ifs;
    }
}

void DataSet::init()
{
    // open given file
    p_ifs = new ifstream(filename.c_str(), ios::in | ios::binary);
    if (!p_ifs) {
        throw XY_Error("Can't open file: " + filename);
    }

    // check whether legal
    if (!is_filetype()) {
        throw XY_Error("file is not the expected " + get_filetype() + " format");
    }

    p_ifs->seekg(0);    // reset the ifstream, as if no lines have been read
}

#if 0
void DataSet::parse_fixedstep_file(
    const string &first_rg_key,
    const string &last_rg_key,
    const string &x_start_key,
    const string &x_step_key,
    const string &meta_sep /* = "=:" */,
    const string &data_sep /* = ",; \t\r\n" */,
    const string &cmt_start /* = ";#" */,
    const string &data_start_tag /* = "" */)
{
    /* format example: (words in square brackets are my notes)
    ; comments          [";" is specified by cmt_start]
    global_key1 = val1  ["=" is specified by meta_sep]
    global_key2 = val2  [white spaces of key and val will be trimmed]
    ...
    [following is range1]
    first_rg_key= val3  
    range_key2  = val4
    ...
    x_start     = start
    x_step      = step
    ... 
    last_rg_key = val
    6234      6185      5969 ... [separated by ANY char in data_sep]
    ...
    [more ranges are optional]
    head_range_key  = val5 
    ...
    */

    init();
    ifstream &f = *p_ifs;

    string line;
    // handle file-level meta-info and 1st range 
    while(getline(f, line)) {
        line = str_trim(line);
        if (line.empty()) {
            continue;
        } else if (string::npos != cmt_start.find(line[0])) {
            // comment line
            continue;
        } else if (string::npos != line.find_first_of(meta_sep)) {
            parse_line(line, meta_sep, key, val);
            if (first_rg_key == key) {
                // indicate a new range starts here
                FixedStepRange *p_rg = new FixedStepRange;
                p_rg->add_meta(key, val);
                parse_range(f, p_rg, x_start_key, x_step_key, 
                    data_start_tag, last_rg_key);
                continue;       // go on to handle other ranges
            } else {
                // file-level meta key-value line
                add_meta(key, val);
            }
        } else {
            continue;           // unknown line
        }
    }

}

// first consider the data_start_tag, then last_rg_key to determine the data start
void DataSet::parse_range(ifstream &f, 
    FixedStepRange *p_rg,
    const string &x_start_key,
    const string &x_step_key,
    const string &data_start_tag,
    const string &last_rg_key)
{
    fp x_start, x_step = 0;
    string line, key, val;
    unsigned line_cnt = lines.size();
    
    unsigned i = 0;

    // read in the range-scope meta-info
    while(getline(f, line)) {
        line = str_trim(line);

        line = str_trim(line);
        if (line.empty()) {
            continue;
        } else if (string::npos != cmt_start.find(line[0])) {
            // comment line
            continue;
        } else if (("" != data_start_tag) && 
                     str_startwith(line , data_start_tag)) {
            break;      // go out to read the x-y data
        } else if (string::npos != line.find_first_of(meta_sep)) {
            parse_line(line, meta_sep, key, val);
            p_rg->add_meta(key, val);
            if (x_start_key == key) {
                x_start = string_to_fp(val);
                p_rg->set_x_start(x_start);
            } else if (x_step_key == key) {
                x_start = string_to_fp(val);
                p_rg->set_x_step(x_step);
            }

            if (last_rg_key == key) {
                break;  // go out to read the x-y data, 
                        // but with a lower priority than data_start_tag
            }
        } else {
            continue;   // unknown line
        }
    }

    // read in xy data
    while (getline(f, line)) {
        line = str_trim(line);
        char ch = line[0];      // decide whether line consisits of a digit
        if (line.empty()) {
            continue;
        } else if (string::npos != cmt_start.find(line[0])) {
            // comment line
            continue;
        } else if ((0 == p_rg.size()) && 
                     string::npos != line.find_first_of(meta_sep)) {
            // pre-data "key-val" lines
            do {
                parse_line(line, meta_sep, key, val);
                p_rg->add_meta(key, val);
            } while (getline(f, line));
            break;
    
        fp val;
        unsigned j = 0;
        istringstream iss(line);
        while (iss >> val) {
            p_rg->add_y(val);
            ++j;
        }

        if (0 != p_rg.size()) {
            break;
        }
    }
}

#endif

/*
// get all lines of a range. stop reading once new "key-val" or "unknown" encountered
void get_rg_lines(ifstream &f, vector<string> rg_lines)
{
    string line;
    // handle file-level meta-info and 1st range 
    while(getline(f, line)) {
        line = str_trim(line);
        char ch = line[0];      // decide whether line consisits of a digit
        if (line.empty()) {
            continue;
        } else if (string::npos != cmt_start.find(line[0])) {
            // comment line
            continue;
        } else if (string::npos != line.find_first_of(meta_sep)) {
            // "key-val" line encountered
            break;
        } else if (('0' <= ch && ch <= '9') ||
                    ("." == ch) ||
                    ("+" == ch) || ("-" == ch)
                    ) {
            rg_lines.push_back(line);
        }
}
*/

DataSet* getNewDataSet(const string &filename, xy_ftype filetype /* = FT_UNKNOWN */)
{
    DataSet *pd = NULL;
    xy_ftype ft = (FT_UNKNOWN == filetype) ? guess_file_type(filename) : filetype;

    if (FT_BR_RAW1 == ft) {
        pd = new BruckerV1RawDataSet(filename); 
    } else if (FT_BR_RAW23 == ft) {
        pd = new BruckerV23RawDataSet(filename);
    } else if (FT_UXD == ft) {
        pd = new UxdDataSet(filename);
    } else if (FT_TEXT == ft) {
        pd = new TextDataSet(filename);
    } else if (FT_RIGAKU == ft) {
        pd = new RigakuDataSet(filename);
    } else if (FT_VAMAS == ft) {
        pd = new VamasDataSet(filename);
    } else {
        pd = NULL;
        throw XY_Error("unkown or unsupported file type");
    }

    if (NULL != pd) {
        pd->load_data(); 
    }

    return pd;
}


xy_ftype guess_file_type(const string &fname)
{
    string::size_type pos = fname.find_last_of('.');

    if(string::npos == pos) {
        return FT_UNKNOWN;
    }

    string ext = fname.substr(pos + 1);
    for(string::iterator it = ext.begin();it != ext.end();++it) {
        *it = tolower(*it);
    }


    if("txt" == ext) {
        return FT_TEXT;
    } else if("uxd" == ext) {
        return FT_UXD;
    } else if("raw" == ext) {
        //TODO: need to detect format by their content
        return FT_BR_RAW1;
    } else if("vms" == ext) {
        return FT_VAMAS;
    } else {
        return FT_UNKNOWN;
    }
}

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


    // convert a string to fp
    fp string_to_fp(const string &str)
    {
        fp ret;
        istringstream is(str);
        is >> ret;
        return ret;
    }
    

    // convert a string to int
    int string_to_int(const string &str)
    {
        int ret;
        istringstream is(str);
        is >> ret;
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
        if ((len != 1) && (len != 2) && (len != 4))
        {
            throw XY_Error("len should be 1, 2, 4");
        }

# if defined(BOOST_LITTLE_ENDIAN)
        // nothing need to do
        return;
# else   // BIG_ENDIAN or PDP_ENDIAN
        // store the old values
        uint8_t byte0, byte1, byte2, byte3;
        switch (len)
        {
        case 1:
            break;
        case 2:
            byte0 = (reinterpretcast<uint8_t*>(p))[0];
            byte1 = (reinterpretcast<uint8_t*>(p))[1];
        case 4:
            byte2 = (reinterpretcast<uint8_t*>(p))[2];
            byte3 = (reinterpretcast<uint8_t*>(p))[3];
            break;
        default:
            break;
        }
#  if defined(BOOST_BIG_ENDIAN)
        switch (len)
        {
        case 1:
            break;
        case 2:
            (reinterpretcast<uint8_t*>(p))[0] = byte1;
            (reinterpretcast<uint8_t*>(p))[1] = byte0;
            break;
        case 4:
            (reinterpretcast<uint8_t*>(p))[0] = byte3;
            (reinterpretcast<uint8_t*>(p))[1] = byte2;
            (reinterpretcast<uint8_t*>(p))[2] = byte1;
            (reinterpretcast<uint8_t*>(p))[3] = byte0;
        }
#  elif defined(BOOST_PDP_ENDIAN)  // PDP_ENDIAN
        // 16-bit word are stored in little_endian
        // 32-bit word are stored in middle_endian: the most significant "half" (16-bits) followed by 
        // the less significant half (as if big-endian) but with each half stored in little-endian format
        switch (len)
        {
        case 1:
        case 2:
            break;
        case 4:
            (reinterpretcast<uint8_t*>(p))[0] = byte2;
            (reinterpretcast<uint8_t*>(p))[1] = byte3;
            (reinterpretcast<uint8_t*>(p))[2] = byte0;
            (reinterpretcast<uint8_t*>(p))[3] = byte1;
        }
#  endif
# endif
    }

    // convert num to string; if (num == undef), return "undefined"
    string my_flt_to_string(float num, float undef)
    {
        return ((undef == num) ? "undefined" : S(num));
    }

    // skip meaningless lines and get all numbers in the first legal line
    int read_line_and_get_all_numbers(istream &is, 
        vector<fp>& result_numbers)
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
        fp d;
        while (q >> d) {
            result_numbers.push_back(d);
        }
        return !is.eof();
    }


    // skip "count" lines in f
    void skip_lines(ifstream &f, const int count)
    {
        string line;
        for (int i = 0; i < count; ++i)
        {
            if (!getline(f, line))
            {
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
        return string_to_int(str);
    }

    fp read_line_fp(ifstream& is)
    {
        string str;
        if (!getline(is, str)) {
            throw xylib::XY_Error("unexpected end of file");
        }
        return string_to_fp(str);
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
            return (pos - array + 1);
        }
    }

} // end of namespace util
} // end of namespace xylib


// XYlib library is a xy data reading library, aiming to read variaty of xy 
// data formats. This file includes the basic classes
// Licence: GNU General Public License version 2
// $Id: xylib.h $

#ifndef XYLIB__API__H__
#define XYLIB__API__H__


#ifndef __cplusplus
#error "This library does not have C API."
#endif


#ifdef FP_IS_LDOUBLE
typedef long doule fp;  
#else
typedef double fp;  
#endif

#include "common.h"
#include <string>
#include <vector>
#include <map>
#include <cstdio>
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <iomanip>


namespace xylib
{

/*
   supported file types are:
*/
enum xy_ftype {
    FT_UNKNOWN,
    FT_TEXT,
    FT_UXD,
    FT_RIGAKU,
    FT_BR_RAW1,
    FT_BR_RAW23,
    FT_VAMAS,
    FT_NUM,     // always at bottom to get the number of the types
};

extern const std::string g_ftype[FT_NUM], g_desc[FT_NUM];

//////////////////////////////////////////////////////////////////////////
// The class to describe the errors/exceptions in xylib
class XY_Error :
    public std::runtime_error
{
public:
    XY_Error(const std::string& msg) : std::runtime_error(msg) {};
};


//////////////////////////////////////////////////////////////////////////
// The class for holding a range/block of x-y data
class Range
{
public:
    Range(bool fixed_step_ = false) : fixed_step(fixed_step_) {} 
    virtual ~Range() {};

    ////////////////////////////////////////////////////////////////////////////////// 
    // reading functions

    // use y to get the size, because in the fixed-step case x is not used
    unsigned get_pt_count() const { return y.size(); }

    // n must be a valid index, zero-based
    virtual fp get_x(unsigned n) const;
    fp get_y(unsigned n) const; 

    // whether this range of data is in "fixed step"
    bool has_fixed_step() { return fixed_step; }

    // std. dev. is optional
    virtual bool has_y_stddev(unsigned n) const;
    virtual fp get_y_stddev(unsigned n) const;

    // basic support for range-level meta-data
    bool has_meta_key(const std::string &key) const;
    bool has_meta() const { return (0 != meta_map.size()); }
    std::vector<std::string> get_all_meta_keys() const;
    const std::string& get_meta(std::string const& key) const; 
    
    // add a <key, val> pair of meta-data to DataSet
    void add_meta(const std::string &key, const std::string &val);

    void export_xy_file(const std::string &fname) const;
    void export_xy_file(std::ofstream &of) const;

    //////////////////////////////////////////////////////////////////////////////////
    // writing functions    
    virtual void add_pt(fp x_, fp py_, fp stddev); 
    virtual void add_pt(fp x_, fp py_); 

protected:
    bool fixed_step;
    std::vector<fp> x;
    std::vector<fp> y;
    std::vector<fp> y_stddev;
    std::vector<bool> y_has_stddev;
    std::map<std::string, std::string> meta_map;

    void check_idx(unsigned n, const std::string &name) const;
};


class FixedStepRange : public Range 
{
public:
    FixedStepRange(fp x_start_ = 0, fp x_step_ = 0) 
        : Range(true), x_start(x_start_), x_step(x_step_)
    {}
    virtual ~FixedStepRange() {}

    fp get_x_start() const { return x_start; }
    fp get_x_step() const { return x_step; }
    fp get_x(unsigned n) const { check_idx(n, "point_x"); return x_start + n * x_step; }

    void add_y(fp y_); 
    void add_y(fp y_, fp stddev_); 
    
    void set_x_step(fp x_step_) { x_step = x_step_; }
    void set_x_start(fp x_start_) { x_start = x_start_; }

protected:
    fp x_start, x_step;
};  // end of FixedStepDataSet


// abstract base class for x-y data in one file, may consist of one or more range(s) of x-y data
class DataSet
{
public:
    DataSet(const std::string &filename_, xy_ftype ftype_ = FT_UNKNOWN)
        : filename(filename_), p_ifs(NULL), ftype(ftype_) {}
    virtual ~DataSet();

    // access the ranges in this file
    unsigned get_range_cnt() const { return ranges.size(); }
    const Range& get_range(unsigned i) const;

    // getters of some attributes associate to files 
    const std::string& get_filetype() const { return g_ftype[ftype]; };
    const std::string& get_filename() const { return filename; }
    // return a detailed description of current file type
    const std::string& get_filetype_desc() const { return g_desc[ftype]; }; 

    // basic support for file-level meta-data, common variables shared in the ranges in the file
    bool has_meta_key(const std::string &key) const;
    bool has_meta() const { return (0 != meta_map.size()); }
    std::vector<std::string> get_all_meta_keys() const;
    const std::string& get_meta(std::string const& key) const; 

    // check file to see whether it is the expected format, according to the "magic number"
    virtual bool is_filetype() const = 0;

    // input/output data from/to file
    virtual void load_data() = 0;
    virtual void load_metainfo() {}
    virtual void export_xy_file(const std::string& fname) const; 

    // add a <key, val> pair of meta-data to DataSet
    void add_meta(const std::string &key, const std::string &val);

protected:
    xy_ftype ftype;
    std::string filename;
    // use "Range*" to support polymorphism, but the memory management will be more complicated 
    std::vector<Range*> ranges; 
    // use a std::map to store the key/val pair
    std::map<std::string, std::string> meta_map;
    std::ifstream *p_ifs;

    // used by load_data to perform some common operations
    virtual void init(); 
#if 0
    // a generic function which can parse UXD-like text files
    void DataSet::parse_fixedstep_file(
        const std::string &first_rg_key,
        const std::string &last_rg_key,
        const std::string &meta_sep = "=:",
        const std::string &data_sep = ",; \t\r\n",
        const std::string &cmt_start = ";#",
        const std::string &data_start_tag = "");
    
    void parse_range(
        std::vector<std::string> &lines, 
        FixedStepRange *p_rg,
        const std::string &data_start_tag);
#endif
}; // end of DataSet

#include "ds_brucker_raw_v1.h"
#include "ds_brucker_raw_v23.h"
#include "ds_rigaku_dat.h"
#include "ds_text.h"
#include "ds_uxd.h"
#include "ds_vamas.h"

//////////////////////////////////////////////////////////////////////////
// namespace scope "global functions" 

// get a non-abstract DataSet ptr according to the given "filetype"
// if "filetype" is not given, auto-detection is used to decide the file type
DataSet* getNewDataSet(const std::string &filename, xy_ftype filetype = FT_UNKNOWN);
xy_ftype guess_file_type(const std::string &filename);


// sub namespace to hold the utility functions
// move the original XY_Lib static member functions (such as string_to_int etc.) here
namespace util 
{
/*
    std::string guess_file_type(const std::string &filename);
    int string_to_int(const string &str);
*/
    boost::uint32_t read_uint32_le(std::ifstream &f, unsigned offset);
    boost::uint16_t read_uint16_le(std::ifstream &f, unsigned offset);
    float read_flt_le(std::ifstream &f, unsigned offset);
    std::string read_string(std::ifstream &f, unsigned offset, unsigned len);
    void le_to_host(void *p, unsigned len);

    // convert a float number to string. if 2nd param is true, return "undefined"
    std::string my_flt_to_string(float num, float undef);

    void rm_space(std::string &str);
    std::string str_trim(const std::string &str, std::string ws = " \r\n\t");
    void parse_line(const std::string &line, const std::string &sep, 
        std::string &key, std::string &val);
    bool str_startwith(const std::string &str_src, const std::string &ss);
    fp string_to_fp(const std::string &str);
    int string_to_int(const std::string &str);

    int read_line_and_get_all_numbers(std::istream &is, 
        std::vector<fp>& result_numbers);

    void skip_lines(std::ifstream &f, const int count);
    int read_line_int(std::ifstream& is);
    fp read_line_fp(std::ifstream& is);

    int get_array_idx(const std::string *array, 
        unsigned size,
        const std::string &find_str);
        
    
} // end of namespace util

} // end of namespace xylib


#endif //ifndef XYLIB__API__H__

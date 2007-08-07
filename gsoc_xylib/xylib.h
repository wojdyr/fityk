// XYlib library is a xy data reading library, aiming to read variaty 
// of XY data formats
// data formats. This file includes the basic base classes
// Licence: Lesser GNU Public License 2.1 (LGPL) 
// $Id: xylib.h $

#ifndef XYLIB__API__H__
#define XYLIB__API__H__


#ifndef __cplusplus
#error "This library does not have C API."
#endif


#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <fstream>
#include <iomanip>


namespace xylib
{

/*
   so far, supported file formats types are:
*/
enum xy_ftype {
    FT_UNKNOWN,
    FT_TEXT,
    FT_UXD,
    FT_RIGAKU,
    FT_BR_RAW1,
    FT_BR_RAW23,
    FT_VAMAS,
    FT_UDF,
    FT_SPE,
    FT_NUM,     // always at bottom to get the number of the types
};

//////////////////////////////////////////////////////////////////////////
// The class to store the format related info
struct FormatInfo
{
    xy_ftype ftype;
    std::string name;    // short name, can be used in filter of open-file dialog
    std::string desc;    // full format name
    std::vector<std::string> exts;
    bool binary;
    bool multi_range;

    FormatInfo(const xy_ftype &ftype_, const std::string &name_, const std::string &desc_, 
        const std::vector<std::string> &exts_, bool binary_, bool multi_range_)
        : ftype(ftype_), name(name_), desc(desc_), exts(exts_), binary(binary_), 
          multi_range(multi_range_) {}

    bool has_extension(const std::string &ext); // case insensitive
};

extern const FormatInfo *g_fi[FT_NUM];

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
    virtual double get_x(unsigned n) const;
    double get_y(unsigned n) const; 

    // whether this range of data is in "fixed step"
    bool has_fixed_step() { return fixed_step; }

    // std. dev. is optional
    virtual bool has_y_stddev(unsigned n) const;
    virtual double get_y_stddev(unsigned n) const;

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
    virtual void add_pt(double x_, double py_, double stddev); 
    virtual void add_pt(double x_, double py_); 

protected:
    bool fixed_step;
    std::vector<double> x;
    std::vector<double> y;
    std::vector<double> y_stddev;
    std::vector<bool> y_has_stddev;
    std::map<std::string, std::string> meta_map;

    void check_idx(unsigned n, const std::string &name) const;
};


class FixedStepRange : public Range 
{
public:
    FixedStepRange(double x_start_ = 0, double x_step_ = 0) 
        : Range(true), x_start(x_start_), x_step(x_step_)
    {}
    virtual ~FixedStepRange() {}

    double get_x_start() const { return x_start; }
    double get_x_step() const { return x_step; }
    double get_x(unsigned n) const { check_idx(n, "point_x"); return x_start + n * x_step; }

    void add_y(double y_); 
    void add_y(double y_, double stddev_); 
    
    void set_x_step(double x_step_) { x_step = x_step_; }
    void set_x_start(double x_start_) { x_start = x_start_; }

protected:
    double x_start, x_step;
};  // end of FixedStepDataSet


// abstract base class for x-y data in one file, may consist of one or more range(s) of x-y data
class DataSet
{
public:
    DataSet(std::istream &is_, xy_ftype ftype_ = FT_UNKNOWN, const std::string &filename_ = "")
        : filename(filename_), f(is_)
    {
        ftype = ftype_;
    }
    
    virtual ~DataSet();

    // access the ranges in this file
    unsigned get_range_cnt() const { return ranges.size(); }
    const Range& get_range(unsigned i) const;

    // getters of some attributes associate to files 
    const std::string& get_filetype() const { return g_fi[ftype]->name; };
    const std::string& get_filename() const { return filename; }
    // return a detailed description of current file type
    const std::string& get_filetype_desc() const { return g_fi[ftype]->desc; }; 

    // basic support for file-level meta-data, shared with all ranges
    bool has_meta_key(const std::string &key) const;
    bool has_meta() const { return (0 != meta_map.size()); }
    std::vector<std::string> get_all_meta_keys() const;
    const std::string& get_meta(std::string const& key) const; 

    // input/output data from/to file
    virtual void load_data() = 0;
    virtual void export_xy_file(const std::string &fname, 
        bool with_meta = true, const std::string &cmt_str = ";") const; 

    // add a <key, val> pair of meta-data to DataSet
    void add_meta(const std::string &key, const std::string &val);

protected:
    xy_ftype ftype;
    std::string filename;
    std::vector<Range*> ranges;     // use "Range*" to support polymorphism
    std::map<std::string, std::string> meta_map;
    std::istream &f;
}; // end of DataSet


enum line_type { LT_COMMENT, LT_KEYVALUE, LT_EMPTY, LT_XYDATA, LT_UNKNOWN };

// the generic class to handle all UXD-like dataset
class UxdLikeDataSet : public DataSet
{
    public:
        UxdLikeDataSet(std::istream& is, xy_ftype filetype, const std::string &filename)
            :  DataSet(is, filetype, filename), meta_sep("=:"), 
            data_sep(", ;"), cmt_start(";#") {}

    protected:
        line_type get_line_type(const std::string &line);
        bool skip_invalid_lines(std::istream &f);

        // elements that only owned by UxdLikeDataSet
        std::string rg_start_tag;       // first tag in a range
        std::string x_start_key;
        std::string x_step_key;
        std::string meta_sep;            // separator chars between key & value
        std::string data_sep;            // separator chars between points-data
        std::string cmt_start;           // start chars of a comment line
};


//////////////////////////////////////////////////////////////////////////
// namespace-scope "global functions" 

// get a non-abstract DataSet ptr according to the given "filetype"
// if "filetype" is not given, uses auto-detection to decide its type
DataSet* getNewDataSet(std::istream &is, xy_ftype filetype = FT_UNKNOWN,
    const std::string &filename = "");

xy_ftype guess_file_type(const std::string &filename);
xy_ftype string_to_ftype(const std::string &ftype_name);


// output the meta-info to ostream os
// cmt_str: all meta info will be output as a comment line; cmt_str is the 
//          string at the biginning of the line to indicate it's a comment
template <typename T>
void output_meta(std::ostream &os, T *pds, const std::string &cmt_str)
{
    using namespace std;
	if(pds->has_meta()){
		os << cmt_str << "meta-key" << "\t" << "meta_val" << endl;
		vector<string> meta_keys = pds->get_all_meta_keys();
		vector<string>::iterator it = meta_keys.begin();
		for(; it != meta_keys.end(); ++it){
			os << cmt_str << *it << ":\t" << pds->get_meta(*it) << endl;
		}
	}
}


} // end of namespace xylib


#endif //ifndef XYLIB__API__H__


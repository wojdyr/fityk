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
#include <limits>

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
    FT_PDCIF,
    FT_PHILIPS_RD,
    FT_NUM,     // always at bottom to get the number of the types
};

//////////////////////////////////////////////////////////////////////////
// The class to store the format related info
struct FormatInfo
{
    xy_ftype ftype;
    std::string name;    // short name, can be used in dialog filter
    std::string desc;    // full format name
    std::vector<std::string> exts;
    bool binary;
    bool multi_range;

    FormatInfo(const xy_ftype &ftype_, const std::string &name_, 
        const std::string &desc_, const std::vector<std::string> &exts_, 
        bool binary_, bool multi_range_)
        : ftype(ftype_), name(name_), desc(desc_), exts(exts_), 
          binary(binary_), multi_range(multi_range_) {}

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
// abstract base class for a column
class Column
{
    // WinspecSpeDataSet need to set fixed_step;
    friend class WinspecSpeDataSet;
public:
    Column(bool fixed_step_ = false) : fixed_step(fixed_step_), stddev_exist(false) {}
    virtual ~Column() {}

    virtual unsigned get_pt_cnt() const = 0;
    virtual double get_val(unsigned n) const = 0; 
    bool is_fixed_step() const { return fixed_step; }
    
    std::string get_name() const { return name; }
    void set_name(std::string name_) { name = name_; }
    
protected:
    bool fixed_step;
    bool stddev_exist;
    std::string name;
};


//////////////////////////////////////////////////////////////////////////
// column uses vector<double> to represent the data 
class VecColumn : public Column
{
public:
    VecColumn(bool fixed_step_ = false) : Column(fixed_step_) {}
    
    // implementation of the base interface 
    unsigned get_pt_cnt() const { return dat.size(); }
    double get_val (unsigned n) const;

    void add_val(double val) { dat.push_back(val); }
    
protected:
    std::vector<double> dat; 
};


//////////////////////////////////////////////////////////////////////////
// column of fixed-step data 
class StepColumn : public Column
{
public:
    StepColumn(double start_ = 0, double step_ = 0, 
        unsigned count_ = std::numeric_limits<unsigned>::max()) 
        : Column(true), start(start_), step(step_), count(count_) 
    {}

    // implementation of the base interface
    unsigned get_pt_cnt() const { return (0 == step) ? 0 : count; }
    double get_val(unsigned n) const;
    
    double get_start() const { return start; } 
    double get_step() const { return step; }

    void set_start(double start_) { start = start_; }
    void set_step(double step_) { step = step_; }
    void set_count(unsigned count_) { count = count_; }
    
protected:
    double start, step;
    unsigned count;
};



//////////////////////////////////////////////////////////////////////////
// The class for holding a range/block of x-y data
class Range
{
public:
    enum col_type {CT_X, CT_Y, CT_STDDEV, CT_UNDEF};
    
    Range() : column_x(0), column_y(1), column_stddev(-1) {}
    virtual ~Range();

    ////////////////////////////////////////////////////////////////////////////////// 
    // reading functions

    // std. dev. is optional
    bool has_stddev() const { return (column_stddev != -1); }
    double get_stddev(unsigned n) const;
    double get_x(unsigned n) const;
    double get_y(unsigned n) const; 

    unsigned get_pt_cnt() const;          // rows of the columns
    unsigned get_column_cnt() const { return cols.size(); }
    const Column& get_column(unsigned n) const;
//    int get_col_idx(const std::string &name) const;

    int get_column_x() const { return column_x; }              // defaults to 0 
    int get_column_y() const { return column_y; }              // defaults to 1
    int get_column_stddev() const { return column_stddev; }   // defaults to -1
    
    void set_column_x(unsigned n);
    void set_column_y(unsigned n);
    void set_column_stddev(unsigned n);
    
    // basic support for range-level meta-data
    bool has_meta_key(const std::string &key) const;
    bool has_meta() const { return (0 != meta_map.size()); }
    std::vector<std::string> get_all_meta_keys() const;
    const std::string& get_meta(std::string const& key) const; 
    
    void export_xy_file(const std::string &fname) const;
    void export_xy_file(std::ofstream &of) const;

    //////////////////////////////////////////////////////////////////////////////////
    // writing functions, only called inside xylib

    // add a <key, val> pair of meta-data to DataSet
    bool add_meta(const std::string &key, const std::string &val);
    void add_column(Column *p_col, col_type type = CT_UNDEF);
    
protected:
    std::map<std::string, std::string> meta_map;
    std::vector<Column*> cols;
    int column_x, column_y, column_stddev;
};


// abstract base class for x-y data in one file, may consist of one or more range(s) of x-y data
class DataSet
{
public:
    DataSet(xy_ftype ftype_) : ftype(ftype_) {}
    virtual ~DataSet();

    // access the ranges in this file
    unsigned get_range_cnt() const { return ranges.size(); }
    const Range& get_range(unsigned i) const;

    // getters of some attributes associate to files 
    const std::string& get_filetype() const { return g_fi[ftype]->name; };
    // return a detailed description of current file type
    const std::string& get_filetype_desc() const { return g_fi[ftype]->desc; }; 

    // basic support for file-level meta-data, shared with all ranges
    bool has_meta_key(const std::string &key) const;
    bool has_meta() const { return (0 != meta_map.size()); }
    std::vector<std::string> get_all_meta_keys() const;
    const std::string& get_meta(std::string const& key) const; 

    // input/output data from/to file
    virtual void load_data(std::istream &f) = 0;
    void clear();
    void export_xy_file(const std::string &fname, 
        bool with_meta = true, const std::string &cmt_str = ";") const; 

    // add a <key, val> pair of meta-data to DataSet
    bool add_meta(const std::string &key, const std::string &val);

protected:
    xy_ftype ftype;
    std::vector<Range*> ranges;     // use "Range*" to support polymorphism
    std::map<std::string, std::string> meta_map;
}; // end of DataSet


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


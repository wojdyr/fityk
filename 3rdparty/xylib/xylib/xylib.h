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


// The list of supported file formats. 
// When changing order, change also the order of `formats' (TODO: change it)
// If the file format is not explicitely when reading a file, all formats that
// support the extension of the file will be tried, so more strict/specific
// formats should go first on this list.

enum xy_ftype {
    FT_UXD,
    FT_RIGAKU,
    FT_BR_RAW1,
    FT_BR_RAW2,
    FT_VAMAS,
    FT_UDF,
    FT_SPE,
    //FT_PDCIF,
    FT_PHILIPS_RAW,
    FT_GSAS,
    FT_TEXT,
    FT_UNKNOWN // FT_UNKNOWN doesn't have corresponding element in `formats'
};

//////////////////////////////////////////////////////////////////////////
// The class to store the format related info
struct FormatInfo
{
    typedef bool (*t_checker)(std::istream&);

    xy_ftype ftype;
    std::string name;    // short name, can be used in dialog filter
    std::string desc;    // full format name
    std::vector<std::string> exts;
    bool binary;
    bool multi_range;
    t_checker checker;

    FormatInfo(const xy_ftype &ftype_, 
               const std::string &name_, 
               const std::string &desc_, 
               const std::vector<std::string> &exts_, 
               bool binary_, 
               bool multi_range_,
               t_checker checker_=NULL)
        : ftype(ftype_), name(name_), desc(desc_), exts(exts_), 
          binary(binary_), multi_range(multi_range_), checker(checker_) {}

    bool has_extension(const std::string &ext) const; // case insensitive
    bool check(std::istream& f) const {return !checker || (*checker)(f);}
};

extern const FormatInfo *formats[];

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
public:
    Column(bool fixed_step_) 
        : fixed_step(fixed_step_), stddev(NULL) {}
    virtual ~Column() {}

    virtual unsigned get_pt_cnt() const = 0;
    virtual double get_val(unsigned n) const = 0; 
    bool has_fixed_step() const { return fixed_step; }
    
    std::string get_name() const { return name; }
    void set_name(std::string name_) { name = name_; }

    Column const* get_stddev() { }
    
protected:
    bool fixed_step;
    Column *stddev;
    std::string name;

    // WinspecSpeDataSet need to set fixed_step;
    friend class WinspecSpeDataSet;
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


class MetaData : public std::map<std::string, std::string>
{
public:
    bool has_key(const std::string &key) const { return find(key) != end(); }
    std::string const& get(std::string const& key) const; 
    bool set(std::string const& key, std::string const& val);
};

//////////////////////////////////////////////////////////////////////////
// The class for holding a block/range of X-Y data
class Block
{
public:
    enum col_type {CT_X, CT_Y, CT_STDDEV, CT_UNDEF};    // column type
    MetaData meta;
    
    Block();
    ~Block();

    ///////////////////////////////////////////////////////////////////////////
    // reading functions

    // std. dev. is optional
    bool has_stddev() const { return (column_stddev != -1); }
    double get_stddev(unsigned n) const;
    double get_x(unsigned n) const;
    double get_y(unsigned n) const; 

    unsigned get_pt_cnt() const;    // points count, also rows of the columns
    unsigned get_column_cnt() const { return cols.size(); }
    const Column& get_column(unsigned n) const;
    int get_col_idx(const std::string &name) const;

    int get_column_x() const { return column_x; }           
    int get_column_y() const { return column_y; }           
    int get_column_stddev() const { return column_stddev; } 
    std::string get_name() const { return name; }
    
    
    void export_xy_file(std::ostream &os) const;

    ///////////////////////////////////////////////////////////////////////////
    // writing functions, only called inside xylib

    // setters for some attributes
    void set_column_x(unsigned n);
    void set_column_y(unsigned n);
    void set_column_stddev(unsigned n);
    void set_name(std::string &name_) { name = name_; }

    void add_column(Column *p_col, col_type type = CT_UNDEF);
    void set_xy_columns(Column *x, Column *y);
    
protected:
    std::vector<Column*> cols;
    int column_x, column_y, column_stddev;  // which column is taken as x/y/stddev
                                            // x,y column idx defaut to 0,1
    std::string name;
    //int sub_blk_idx;    // idx of a sub-block in a block (e.g. some pdCIF format)

};


// abstract base class for X-Y data in *ONE FILE*, 
// may consist of one or more block(s) of X-Y data
class DataSet
{
public:
    DataSet(xy_ftype ftype_) : ftype(ftype_) {}
    virtual ~DataSet();

    // access the blocks in this file
    unsigned get_block_cnt() const { return blocks.size(); }
    const Block* get_block(unsigned i) const;

    // return a detailed description of current file type
    const std::string& get_filetype_desc() const 
                                            { return formats[ftype]->desc; }; 

    // input/output data from/to file
    virtual void load_data(std::istream &f) = 0;
    void clear();
    void export_xy_file(const std::string &fname, 
        bool with_meta = true, const std::string &cmt_str = ";") const; 


protected:
    xy_ftype ftype;
    std::vector<Block*> blocks;
    MetaData meta;

    const std::string& get_filetype() const { return formats[ftype]->name; };
}; 


//////////////////////////////////////////////////////////////////////////
// namespace-scope "global functions" 

// get a non-abstract DataSet ptr according to the given "filetype"
// if "filetype" is not given, uses auto-detection to decide its type
DataSet* getNewDataSet(std::istream &is, xy_ftype filetype = FT_UNKNOWN,
                       const std::string &filename = "");

xy_ftype guess_filetype(const std::string &path);
xy_ftype string_to_ftype(const std::string &ftype_name);

} // end of namespace xylib


#endif //ifndef XYLIB__API__H__


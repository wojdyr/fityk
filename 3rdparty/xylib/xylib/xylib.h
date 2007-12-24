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

//////////////////////////////////////////////////////////////////////////
// stores the format related info
struct FormatInfo
{
    typedef bool (*t_checker)(std::istream&);

    std::string name;    // short name, can be used in dialog filter
    std::string desc;    // full format name
    std::vector<std::string> exts;
    bool binary;
    bool multi_range;
    t_checker checker;

    FormatInfo(const std::string &name_, 
               const std::string &desc_, 
               const std::vector<std::string> &exts_, 
               bool binary_, 
               bool multi_range_,
               t_checker checker_=NULL)
        : name(name_), desc(desc_), exts(exts_), 
          binary(binary_), multi_range(multi_range_), checker(checker_) {}

    bool has_extension(const std::string &ext) const; // case insensitive
    bool check(std::istream& f) const {return !checker || (*checker)(f);}
};

extern const FormatInfo *formats[];

//////////////////////////////////////////////////////////////////////////
// The class to describe the errors/exceptions in xylib
class XY_Error : public std::runtime_error
{
public:
    XY_Error(const std::string& msg) : std::runtime_error(msg) {};
};


//////////////////////////////////////////////////////////////////////////
// abstract base class for a column
class Column
{
public:
    double step; // 0. means step is not fixed

    Column(double step_) 
        : step(step_), stddev(NULL) {}
    virtual ~Column() {}

    // return number of points or -1 for "unlimited" number of points
    virtual int get_pt_cnt() const = 0;
    virtual double get_val(int n) const = 0; 
    bool has_fixed_step() const { return step != 0.; }
    
    std::string get_name() const { return name; }
    void set_name(std::string name_) { name = name_; }

    //Column const* get_stddev() { }
    
protected:
    Column *stddev;
    std::string name;
};


//////////////////////////////////////////////////////////////////////////
// column uses vector<double> to represent the data 
class VecColumn : public Column
{
public:
    VecColumn() : Column(0.) {}
    
    // implementation of the base interface 
    int get_pt_cnt() const { return dat.size(); }
    double get_val (int n) const;

    void add_val(double val) { dat.push_back(val); }
    
protected:
    std::vector<double> dat; 
};


//////////////////////////////////////////////////////////////////////////
// column of fixed-step data 
class StepColumn : public Column
{
public:
    double start;
    int count; // -1 means unlimited...

    StepColumn(double start_, double step_, int count_ = -1) 
        : Column(step_), start(start_), count(count_) 
    {}

    int get_pt_cnt() const { return count; }
    double get_val(int n) const;
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
    MetaData meta;
    
    Block();
    ~Block();

    // std. dev. is optional
    bool has_stddev() const { return (column_stddev != -1); }
    double get_stddev(unsigned n) const;
    double get_x(unsigned n) const;
    double get_y(unsigned n) const; 

    int get_column_cnt() const { return cols.size(); }
    const Column& get_column(unsigned n) const;

    int get_column_x_idx() const { return column_x; }           
    int get_column_y_idx() const { return column_y; }           
    int get_column_stddev_idx() const { return column_stddev; } 
    Column const& get_column_x() const { return get_column(column_x); }
    Column const& get_column_y() const { return get_column(column_y); }
    Column const& get_column_stddev() const {return get_column(column_stddev);}
    std::string get_name() const { return name; }
    
    void export_xy_file(std::ostream &os) const;

    void set_column_stddev(unsigned n);
    void set_name(std::string &name_) { name = name_; }

    void set_xy_columns(Column *x, Column *y);
    void add_column(Column *c) { cols.push_back(c); }
    
protected:
    std::vector<Column*> cols;
    // which column is taken as x/y/stddev
    int column_x, column_y, column_stddev;  
    std::string name;
    //int sub_blk_idx;    // idx of a sub-block in a block (e.g. some pdCIF format)
};


// abstract base class for X-Y data in *ONE FILE*, 
// may consist of one or more block(s) of X-Y data
class DataSet
{
public:
    FormatInfo const* const fi;

    DataSet(FormatInfo const* fi_) : fi(fi_) {}
    virtual ~DataSet();

    // access the blocks in this file
    unsigned get_block_cnt() const { return blocks.size(); }
    const Block* get_block(unsigned i) const;

    // input/output data from/to file
    virtual void load_data(std::istream &f) = 0;
    void clear();
    void export_xy_file(const std::string &fname, 
        bool with_meta = true, const std::string &cmt_str = ";") const; 


protected:
    std::vector<Block*> blocks;
    MetaData meta;

    void format_assert(bool condition) 
    {
        if (!condition)
            throw XY_Error("Unexpected format for filetype: " + fi->name);
    }
}; 


// if format_name is not given, it is guessed
// return value: pointer to Dataset that contains all data read from file
DataSet* load_file(std::string const& path, std::string const& format_name="");

// return value: pointer to Dataset that contains all data read from file
DataSet* load_stream(std::istream &is, std::string const& format_name);

FormatInfo const* guess_filetype(const std::string &path);
FormatInfo const* string_to_format(const std::string &format_name);

} // namespace xylib


#endif //ifndef XYLIB__API__H__


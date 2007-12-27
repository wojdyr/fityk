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


namespace xylib
{

// stores format related info
struct FormatInfo
{
    typedef bool (*t_checker)(std::istream&);

    std::string name;    // short name, can be used in dialog filter
    std::string desc;    // full format name
    std::vector<std::string> exts; // possible extensions
    bool binary;
    bool multi_range;
    t_checker checker; // function used to check if a file has this format

    FormatInfo(const std::string &name_, 
               const std::string &desc_, 
               const std::vector<std::string> &exts_, 
               bool binary_, 
               bool multi_range_,
               t_checker checker_=NULL)
        : name(name_), desc(desc_), exts(exts_), 
          binary(binary_), multi_range(multi_range_), checker(checker_) {}

    // check if extension `ext' is in the list `exts'; case insensitive
    bool has_extension(const std::string &ext) const; 
    // check if file f can be of this format
    bool check(std::istream& f) const { return !checker || (*checker)(f); }
};

extern const FormatInfo *formats[];


// the only exception thrown by xylib
class XY_Error : public std::runtime_error
{
public:
    XY_Error(const std::string& msg) : std::runtime_error(msg) {};
};


// column abstract base class for a column
class Column
{
public:
    std::string name; // column can have a name
    double step; // 0. means step is not fixed

    Column(double step_) 
        : step(step_)/*, stddev(NULL)*/ {}
    virtual ~Column() {}

    // return number of points or -1 for "unlimited" number of points
    virtual int get_pt_cnt() const = 0;
    virtual double get_value(int n) const = 0; 
    bool has_fixed_step() const { return step != 0.; }
    
    //Column const* get_stddev() { }
    
protected:
    //Column *stddev;
};


// stores meta-data (additional data, that usually describe x-y data) 
// for block or dataset. For example: date of the experiment, wavelength, ...
class MetaData : public std::map<std::string, std::string>
{
public:
    bool has_key(const std::string &key) const { return find(key) != end(); }
    std::string const& get(std::string const& key) const; 
    bool set(std::string const& key, std::string const& val);
};


// The class for holding a block (range) of data
class Block
{
public:
    MetaData meta;
    std::string name; // block can have a name
    
    Block() {}
    ~Block();

    int get_column_cnt() const { return cols.size(); }
    const Column& get_column(unsigned n) const;

    void export_xy_file(std::ostream &os) const;

    void set_xy_columns(Column *x, Column *y);
    void add_column(Column *c) { cols.push_back(c); }
    
protected:
    std::vector<Column*> cols;
    //int sub_blk_idx; // idx of a sub-block in a block (e.g. some pdCIF format)
};


// abstract base class for X-Y data in *ONE FILE*, 
// may consist of one or more block(s) of X-Y data
class DataSet
{
public:
    FormatInfo const* const fi;

    DataSet(FormatInfo const* fi_) : fi(fi_) {}
    virtual ~DataSet();

    // number of blocks (usually 1)
    int get_block_cnt() const { return blocks.size(); }
    // access block number i
    const Block* get_block(int n) const;

    // read data from file
    virtual void load_data(std::istream &f) = 0;
    // delete all data stored in this class (use only if you want to 
    // use load_data() more than once)
    void clear();
    // export to text file
    void export_plain_text(const std::string &fname) const; 

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

// guess a format of the file
FormatInfo const* guess_filetype(const std::string &path);

// returns FormatInfo that has a name format_name
FormatInfo const* string_to_format(const std::string &format_name);

} // namespace xylib


#endif //ifndef XYLIB__API__H__


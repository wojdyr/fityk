// XYlib library is a xy data reading library, aiming to read variaty 
// of XY data formats
// data formats. This file includes the basic base classes
// Licence: Lesser GNU Public License 2.1 (LGPL) 
// $Id$

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

class DataSet;

/// stores format related info
struct FormatInfo
{
    typedef bool (*t_checker)(std::istream&);
    typedef DataSet* (*t_ctor)();

    std::string name;  /// short name, usually basename of .cpp/.h files  
    std::string desc;  /// full format name (reasonably short)
    std::vector<std::string> exts; // possible extensions
    bool binary; /// true if it's binary file
    bool multiblock; /// true if filetype supports multiple blocks

    t_ctor ctor; /// factory function
    t_checker checker; /// function used to check if a file has this format

    FormatInfo(std::string const& name_, 
               std::string const& desc_, 
               std::vector<std::string> const& exts_, 
               bool binary_, 
               bool multiblock_,
               t_ctor ctor_,
               t_checker checker_)
        : name(name_), desc(desc_), exts(exts_), 
          binary(binary_), multiblock(multiblock_), 
          ctor(ctor_), checker(checker_) {}

    /// check if extension `ext' is in the list `exts'; case insensitive
    bool has_extension(std::string const& ext) const; 
    /// check if file f can be of this format
    bool check(std::istream& f) const { return !checker || (*checker)(f); }
};

extern const FormatInfo *formats[];


/// unexpected format, unexpected EOF, etc
class FormatError : public std::runtime_error
{
public:
    FormatError(std::string const& msg) : std::runtime_error(msg) {};
};

/// all errors other than format error
class RunTimeError : public std::runtime_error
{
public:
    RunTimeError(std::string const& msg) : std::runtime_error(msg) {};
};


/// abstract base class for a column
class Column
{
public:
    std::string name; /// Column can have a name (but usually it doesn't have)
    double step; /// step, 0. means step is not fixed

    Column(double step_) : step(step_) {}
    virtual ~Column() {}

    /// return number of points or -1 for "unlimited" number of points
    virtual int get_point_count() const = 0;

    /// return value of n'th point (starting from 0-th)
    virtual double get_value(int n) const = 0; 

    /// get minimum value in column
    virtual double get_min() const = 0;
    /// get maximum value in column;
    /// point_count must be specified if column has "unlimited" length, it is
    /// ignored otherwise 
    virtual double get_max(int point_count=0) const = 0;
};


/// stores meta-data (additional data, that usually describe x-y data) 
/// for block or dataset. For example: date of the experiment, wavelength, ...
class MetaData : public std::map<std::string, std::string>
{
public:
    bool has_key(std::string const& key) const { return find(key) != end(); }
    std::string const& get(std::string const& key) const; 
    bool set(std::string const& key, std::string const& val);
};


/// a block of data
class Block
{
public:
    /// handy pseudo-column that returns index of point as value
    static Column* const index_column;

    MetaData meta; /// meta-data
    std::string name; /// block can have a name (but usually it doesn't have)
    
    Block() {}
    ~Block();

    /// number of real columns, not including 0-th pseudo-column
    int get_column_count() const { return cols.size(); }
    /// get column, 0-th column is index of point
    const Column& get_column(int n) const;

    /// return number of points or -1 for "unlimited" number of points
    /// each column should have the same number of points (or "unlimited"
    /// number if the column is a generator)
    int get_point_count() const;
    
    // add one column; for use in filetype implementations
    void add_column(Column *c, std::string const& title="", bool append=true); 

    // split block if it has columns with different sizes
    std::vector<Block*> split_on_column_lentgh();

protected:
    std::vector<Column*> cols;
};


/// DataSet represents data stored typically in one file. 
// may consist of one or more block(s) of X-Y data
class DataSet
{
public:
    // pointer to FormatInfo of a class derived from DataSet
    FormatInfo const* const fi;
    // if load_data() supports options, set it before it's called
    std::vector<std::string> options;

    // ctor is protected
    virtual ~DataSet();

    // number of blocks (usually 1)
    int get_block_count() const { return blocks.size(); }

    // access block number i
    Block const* get_block(int n) const;

    // read data from file
    virtual void load_data(std::istream &f) = 0;

    // delete all data stored in this class (use only if you want to 
    // call load_data() more than once)
    void clear();

    // export to text file
    void export_plain_text(std::string const& fname) const; 

protected:
    std::vector<Block*> blocks;
    MetaData meta;

    DataSet(FormatInfo const* fi_);

    void format_assert(bool condition, std::string const& comment = "") 
    {
        if (!condition)
            throw FormatError("Unexpected format for filetype: " + fi->name
                              + (comment.empty() ? comment : "; " + comment));
    }
}; 


// if format_name is not given, it is guessed
// return value: pointer to Dataset that contains all data read from file
DataSet* load_file(std::string const& path, std::string const& format_name="",
                   std::vector<std::string> const& options 
                                                = std::vector<std::string>());

// return value: pointer to Dataset that contains all data read from file
DataSet* load_stream(std::istream &is, FormatInfo const* fi,
                     std::vector<std::string> const& options);

// guess a format of the file
FormatInfo const* guess_filetype(std::string const& path);

// returns FormatInfo that has a name format_name
FormatInfo const* string_to_format(std::string const& format_name);

// return wildcard for file dialog in format:
// "All Files (*)|*|ASCII X Y Files (*)|*|Sietronics Sieray CPI (*.cpi)|*.cpi"
std::string get_wildcards_string(std::string const& all_files="*");

} // namespace xylib


// macro used in declarations of classes derived from DataSet
#define OBLIGATORY_DATASET_MEMBERS(class_name) \
    public: \
        class_name() : DataSet(&fmt_info) {} \
        void load_data(std::istream &f); \
        static bool check(std::istream &f); \
        static DataSet* ctor() { return new class_name; } \
        static const FormatInfo fmt_info; 

#endif //ifndef XYLIB__API__H__


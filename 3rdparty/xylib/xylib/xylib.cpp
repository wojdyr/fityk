// XYlib library is a xy data reading library, aiming to read variaty of xy 
// data formats.
// Licence: GNU General Public License version 2
// $Id: xylib.cpp $

#include <iostream>
#include <algorithm>
#include <boost/detail/endian.hpp>
#include <boost/cstdint.hpp>

#include "xylib.h"
#include "util.h"

#include "brucker_raw.h"
#include "rigaku_dat.h"
#include "text.h"
#include "uxd.h"
#include "vamas.h"
#include "philips_udf.h"
#include "winspec_spe.h"
//#include "pdcif.h"
#include "philips_raw.h"
#include "gsas.h"

using namespace std;
using namespace xylib::util;
using namespace boost;

namespace xylib {

const FormatInfo *formats[] = {
    &UxdDataSet::fmt_info,
    &RigakuDataSet::fmt_info,
    &BruckerRawDataSet::fmt_info,
    &VamasDataSet::fmt_info,
    &UdfDataSet::fmt_info,
    &WinspecSpeDataSet::fmt_info,
    //&PdCifDataSet::fmt_info,
    &PhilipsRawDataSet::fmt_info,
    &GsasDataSet::fmt_info,
    &TextDataSet::fmt_info,
    NULL // it must be a NULL-terminated array
};


bool FormatInfo::has_extension(const std::string &ext) const
{ 
    string lower_ext = str_tolower(ext);
    return (find(exts.begin(), exts.end(), lower_ext) != exts.end()); 
}

//////////////////////////////////////////////////////////////////////////

double VecColumn::get_val(int n) const 
{
    my_assert(n >= 0 && n < get_pt_cnt(), "index out of range in VecColumn");
    return dat[n];
}


//////////////////////////////////////////////////////////////////////////

double StepColumn::get_val(int n) const 
{
    my_assert(count == -1 || (n >= 0 && n < count), "point index out of range");
    return start + step * n;
}
 
//////////////////////////////////////////////////////////////////////////

string const& MetaData::get(string const& key) const
{
    const_iterator it = find(key);
    if (it == end()) 
        throw XY_Error("no such key in meta-info found");
    return it->second;
}

bool MetaData::set(string const& key, string const& val)
{
    return insert(make_pair(key, val)).second;
}

//////////////////////////////////////////////////////////////////////////

Block::Block() 
    : column_x(0), column_y(1), column_stddev(-1)
{
}

Block::~Block()
{
    vector<Column*>::iterator it;
    for (it = cols.begin(); it != cols.end(); ++it) {      
        delete *it;
    }
}

double Block::get_x(unsigned n) const
{
    my_assert(column_x < int(cols.size()), "no column_x");
    return cols[column_x]->get_val(n);
}

double Block::get_y(unsigned n) const
{
    my_assert(column_y < int(cols.size()), "no column_y");
    return cols[column_y]->get_val(n);
}


const Column& Block::get_column(unsigned n) const
{
    my_assert(n < cols.size(), "column index out of range");
    return *cols[n];
}


void Block::set_column_stddev(unsigned n)
{
    my_assert(n < cols.size(), "column index out of range");
    column_stddev = n;
}

double Block::get_stddev(unsigned n) const
{
    my_assert(has_stddev() && (column_stddev < int(cols.size())), "no column_stddev");
    return cols[column_stddev]->get_val(n);
}

void Block::export_xy_file(ostream &os) const
{
    int nx = get_column_x().get_pt_cnt();
    int ny = get_column_y().get_pt_cnt();
    if (nx == -1 && ny == -1)
        return;
    int n = (nx != -1 && nx < ny) ? nx : ny;

    for (int i = 0; i < n; ++i) {
        os << setfill(' ') << setiosflags(ios::fixed) 
           << setprecision(5) << setw(7) << get_x(i) << "\t" 
           << setprecision(8) << setw(10) << get_y(i) << "\t";
        if (has_stddev()) {
            os << get_stddev(i);
        }
        os << endl;
    }
}

void Block::set_xy_columns(Column *x, Column *y)
{
    my_assert(cols.empty(), "Internal error in set_xy_columns()");
    cols.push_back(x);
    cols.push_back(y);
    column_x = 0;
    column_y = 1;
}




//////////////////////////////////////////////////////////////////////////
// member functions of Class DataSet

DataSet::~DataSet()
{
    for (vector<Block*>::iterator i = blocks.begin(); i != blocks.end(); ++i)
        delete *i;
}

const Block* DataSet::get_block(unsigned i) const
{
    if (i >= blocks.size()) {
        throw XY_Error("no block in this file with such index");
    }

    return blocks[i];
}

void DataSet::export_xy_file(const string &fname, 
    bool with_meta /* = true */, const std::string &cmt_str /* = ";" */) const
{
    unsigned range_num = get_block_cnt();
    ofstream of(fname.c_str());
    my_assert(of != NULL, "can't create file " + fname);

    // output the file-level meta-info
    if (with_meta) {
        of << cmt_str << "exported by xylib from a " << fi->name << " file" << endl;
        of << cmt_str << "total blocks: " << blocks.size() << endl << endl;
        for (map<string,string>::const_iterator i = meta.begin();
                                                i != meta.end(); ++i) 
            of << cmt_str << i->first << ":\t" << i->second << endl;
    }
    
    for(unsigned i = 0; i < range_num; ++i) {
        const Block *blk = get_block(i);
        if (with_meta) {
            of << endl;
            for (int j = 0; j < 40; ++j) {
                of << cmt_str;
            }
            of << endl << cmt_str << "* block " << i << "  " << blk->get_name() << endl;

            for (map<string,string>::const_iterator j = blk->meta.begin();
                                                    j != blk->meta.end(); ++j) 
                of << cmt_str << j->first << ":\t" << j->second << endl;

            string x_label, y_label, stddev_label;
            try {
                x_label = blk->get_column_x().get_name();
                y_label = blk->get_column_y().get_name();
                if (blk->has_stddev())
                    stddev_label = blk->get_column_stddev().get_name();
            } catch (...) {
                // this must be a block without any data, move to next block
                continue;
            }
            of << endl << cmt_str << "x\ty\t y_stddev" << endl;
            of << cmt_str << x_label << "\t" << y_label << "\t" << stddev_label << endl;
        }
        blk->export_xy_file(of);
    }
}

// clear all the data of this dataset
void DataSet::clear()
{
    this->~DataSet();
    blocks.clear();
    meta.clear();
}


//////////////////////////////////////////////////////////////////////////
// namespace scope global functions

DataSet* dataset_factory(FormatInfo const* ft)
{
#define FACTORY_ITEM(classname) \
    if (ft == &classname::fmt_info) \
        return new classname(); 
    FACTORY_ITEM(BruckerRawDataSet)
    FACTORY_ITEM(UxdDataSet)
    FACTORY_ITEM(TextDataSet)
    FACTORY_ITEM(RigakuDataSet)
    FACTORY_ITEM(VamasDataSet)
    FACTORY_ITEM(UdfDataSet)
    FACTORY_ITEM(WinspecSpeDataSet)
    //FACTORY_ITEM(PdCifDataSet)
    FACTORY_ITEM(PhilipsRawDataSet)
    FACTORY_ITEM(GsasDataSet)
#undef FACTORY_ITEM
    throw XY_Error("unkown or unsupported file type");
    return 0; // to avoid warnings
}

DataSet* load_file(string const& path, string const& format_name)
{
    ifstream is(path.c_str(), ios::in | ios::binary);
    if (!is) 
        throw XY_Error("Error: can't open input file: " + path);
    string filetype;
    if (format_name.empty()) {
        FormatInfo const* fi = guess_filetype(path);
        my_assert(fi, "Format of file can not be guessed");
        filetype = fi->name;
    }
    else
        filetype = format_name;
    return load_stream(is, filetype);
}

DataSet* load_stream(istream &is, string const& format_name)
{
    FormatInfo const* fi = string_to_format(format_name);
    my_assert(fi != NULL, "when loading data: format of the file is not known");
    DataSet *pd = dataset_factory(fi);
    pd->load_data(is); 
    return pd;
}

// filename: path, filename or only extension with dot
vector<FormatInfo const*> get_possible_filetypes(string const& filename)
{
    vector<FormatInfo const*> results;

    // get extension
    string::size_type pos = filename.find_last_of('.');
    if (pos == string::npos) 
        return results;
    string ext = str_tolower(filename.substr(pos + 1));

    for (FormatInfo const **i = formats; *i != NULL; ++i) {
        if ((*i)->has_extension(ext))
            results.push_back(*i);
    }
    return results;
}

FormatInfo const* guess_filetype(const string &path)
{
    vector<FormatInfo const*> possible = get_possible_filetypes(path);
    if (possible.empty())
        return NULL;
    ifstream f(path.c_str(), ios::in | ios::binary);
    if (possible.size() == 1)
        return possible[0]->check(f) ? possible[0] : NULL;
    else {
        for (vector<FormatInfo const*>::const_iterator i = possible.begin(); 
                                                    i != possible.end(); ++i) {
            if ((*i)->check(f)) 
                return *i;
            f.seekg(0);
            f.clear();
        }

        return NULL;
    }
}


FormatInfo const* string_to_format(string const& format_name) 
{
    for (FormatInfo const **i = formats; *i != NULL; ++i) 
        if (format_name == (*i)->name) 
            return *i;
    return NULL;
}


} // end of namespace xylib



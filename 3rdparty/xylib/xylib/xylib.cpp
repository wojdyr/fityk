// XYlib library is a xy data reading library, aiming to read variaty of xy 
// data formats.
// Licence: GNU General Public License version 2
// $Id$

#include <iomanip>
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
#include "cpi.h"
#include "dbws.h"
#include "canberra_mca.h"

using namespace std;
using namespace xylib::util;
using namespace boost;

namespace xylib {

const FormatInfo *formats[] = {
    &CpiDataSet::fmt_info,
    &UxdDataSet::fmt_info,
    &RigakuDataSet::fmt_info,
    &BruckerRawDataSet::fmt_info,
    &VamasDataSet::fmt_info,
    &UdfDataSet::fmt_info,
    &WinspecSpeDataSet::fmt_info,
    //&PdCifDataSet::fmt_info,
    &PhilipsRawDataSet::fmt_info,
    //&GsasDataSet::fmt_info,
    &CanberraMcaDataSet::fmt_info,
    &DbwsDataSet::fmt_info,
    &TextDataSet::fmt_info,
    NULL // it must be a NULL-terminated array
};


bool FormatInfo::has_extension(const std::string &ext) const
{ 
    string lower_ext = str_tolower(ext);
    return exts.empty() 
           || find(exts.begin(), exts.end(), lower_ext) != exts.end();
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


Block::~Block()
{
    vector<Column*>::iterator it;
    for (it = cols.begin(); it != cols.end(); ++it) {      
        delete *it;
    }
}


const Column& Block::get_column(unsigned n) const
{
    my_assert(n < cols.size(), "column index out of range");
    return *cols[n];
}


void Block::export_xy_file(ostream &os) const
{
    int ncol = get_column_cnt();
    os << "# ";
    for (int i = 0; i < ncol; ++i) {
        string const& name = get_column(i).name;
        os << (name.empty() ? "column_"+S(i) : name) << "\t";
    }
    os << endl;
    int nrow = 0;
    for (int i = 0; i < ncol; ++i) {
        int c = get_column(i).get_pt_cnt();
        if (c > nrow)
            nrow = c;
    }
    for (int i = 0; i < nrow; ++i) {
        for (int j = 0; j < ncol; ++j) 
            os << setfill(' ') << setiosflags(ios::fixed) << setprecision(6) 
                << setw(8) << get_column(j).get_value(i) << "\t";
        os << endl;
    }
}

void Block::set_xy_columns(Column *x, Column *y)
{
    my_assert(cols.empty(), "Internal error in set_xy_columns()");
    cols.push_back(x);
    cols.push_back(y);
}


//////////////////////////////////////////////////////////////////////////

DataSet::DataSet(FormatInfo const* fi_) 
    : fi(fi_) 
{}

DataSet::~DataSet()
{
    for (vector<Block*>::iterator i = blocks.begin(); i != blocks.end(); ++i)
        delete *i;
}

const Block* DataSet::get_block(int n) const
{
    if (n < 0 || (size_t)n >= blocks.size()) 
        throw XY_Error("no block #" + S(n) + "in this file.");
    return blocks[n];
}

void DataSet::export_plain_text(const string &fname) const
{
    unsigned range_num = get_block_cnt();
    ofstream of(fname.c_str());
    my_assert(of != NULL, "can't create file " + fname);

    // output the file-level meta-info
    of << "# exported by xylib from a " << fi->name << " file" << endl;
    for (map<string,string>::const_iterator i = meta.begin();
                                            i != meta.end(); ++i) 
        of << "# " << i->first << ":\t" << i->second << endl;
    
    for (unsigned i = 0; i < range_num; ++i) {
        const Block *blk = get_block(i);
        if (range_num > 1 || !blk->name.empty())
            of << endl << "### block #" << i << " " << blk->name << endl;
        for (map<string,string>::const_iterator j = blk->meta.begin();
                                                j != blk->meta.end(); ++j) 
            of << "#" << j->first << ":\t" << j->second << endl;

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
    DataSet *pd = (*fi->ctor)();
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
    string ext = filename.substr(pos + 1);

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
    if (!f)
        throw XY_Error("can't open input file: " + path);
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



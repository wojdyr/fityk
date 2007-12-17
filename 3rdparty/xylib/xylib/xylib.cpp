// XYlib library is a xy data reading library, aiming to read variaty of xy 
// data formats.
// Licence: GNU General Public License version 2
// $Id: xylib.cpp $

#include "xylib.h"
#include "util.h"

#include "brucker_raw_v1.h"
#include "brucker_raw_v2.h"
#include "rigaku_dat.h"
#include "text.h"
#include "uxd.h"
#include "vamas.h"
#include "philips_udf.h"
#include "winspec_spe.h"
//#include "pdcif.h"
#include "philips_raw.h"
#include "gsas.h"

#include <iostream>
#include <algorithm>
#include <boost/detail/endian.hpp>
#include <boost/cstdint.hpp>

using namespace std;
using namespace xylib::util;
using namespace boost;

namespace xylib {

// elements in formats[] are ordered, in the same order as enum xy_ftype
const FormatInfo *formats[] = {
    &UxdDataSet::fmt_info,
    &RigakuDataSet::fmt_info,
    &BruckerV1RawDataSet::fmt_info,
    &BruckerV23RawDataSet::fmt_info,
    &VamasDataSet::fmt_info,
    &UdfDataSet::fmt_info,
    &WinspecSpeDataSet::fmt_info,
    //&PdCifDataSet::fmt_info,
    &PhilipsRawDataSet::fmt_info,
    &GsasDataSet::fmt_info,
    &TextDataSet::fmt_info,
    NULL,
};


bool FormatInfo::has_extension(const std::string &ext) const
{ 
    string lower_ext = str_tolower(ext);
    return (find(exts.begin(), exts.end(), lower_ext) != exts.end()); 
}

//////////////////////////////////////////////////////////////////////////
// member functions of Class VecColumn

double VecColumn::get_val(unsigned n) const 
{
    my_assert(n <= get_pt_cnt(), "index out of range in VecColumn");
    return dat[n];
}


//////////////////////////////////////////////////////////////////////////
// member functions of Class StepColumn

double StepColumn::get_val(unsigned n) const 
{
    my_assert(n <= get_pt_cnt(), "index out of range in VecColumn");
    my_assert(step != 0, "x_step not set, file is likely conrupt");
    return (start + step * n);
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
// member functions of Class Block

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

// return the pt_cnt in all columns (they are assumed to be the same)
unsigned Block::get_pt_cnt() const
{
    unsigned pt_cnt(0);

    if (cols.size() != 0) {
        for (unsigned i = 0; i < cols.size(); ++i) {
            pt_cnt = cols[i]->get_pt_cnt();
            if (pt_cnt != numeric_limits<unsigned>::max()) {
                break;
            }
        }
    }
    
    return pt_cnt;
}

const Column& Block::get_column(unsigned n) const
{
    my_assert(n < cols.size(), "column index out of range");
    return *cols[n];
}


// get the index of the column with a name of "name"
// return -1 if not found
int Block::get_col_idx(const string &name) const
{
    int ret_val(-1);
    
    for (unsigned i = 0; i < get_column_cnt(); ++i) {
        const Column &col = get_column(i);
        if (name == col.get_name()) {
            ret_val = i;
            break;
        }
    }
    return ret_val;
}

void Block::set_column_x(unsigned n)
{
    my_assert(n < cols.size(), "column index out of range");
    column_x = n;
}

void Block::set_column_y(unsigned n)
{
    my_assert(n < cols.size(), "column index out of range");
    column_y = n;
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
    int n = get_pt_cnt();

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

//TODO: remove it
void Block::add_column(Column *p_col, col_type type /* = CT_UNDEF */)
{
    cols.push_back(p_col);

    unsigned i = cols.size() - 1;
    switch (type) {
    case CT_X:
        set_column_x(i);
        break;
    case CT_Y:
        set_column_y(i);
        break; 
    case CT_STDDEV:
        set_column_stddev(i);
        break;
    default:
        break;
    }
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
        of << cmt_str << "exported by xylib from a " << get_filetype() << " file" << endl;
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
            of << endl << cmt_str << "* block " << i << endl;
            of << cmt_str << "name: " << blk->get_name() << endl;
            of << cmt_str << "total point count: " << blk->get_pt_cnt() << endl;
            of << endl;

            for (map<string,string>::const_iterator j = blk->meta.begin();
                                                    j != blk->meta.end(); ++j) 
                of << cmt_str << j->first << ":\t" << j->second << endl;

            string x_label, y_label, stddev_label;
            try {
                x_label = blk->get_column(blk->get_column_x()).get_name();
                y_label = blk->get_column(blk->get_column_y()).get_name();
                if (blk->has_stddev())
                    stddev_label = blk->get_column(blk->get_column_stddev()).get_name();
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

DataSet* dataset_factory(xy_ftype ft)
{
    if (FT_BR_RAW1 == ft) 
        return new BruckerV1RawDataSet(); 
    else if (FT_BR_RAW2 == ft) 
        return new BruckerV23RawDataSet();
    else if (FT_UXD == ft) 
        return new UxdDataSet();
    else if (FT_TEXT == ft) 
        return new TextDataSet();
    else if (FT_RIGAKU == ft) 
        return new RigakuDataSet();
    else if (FT_VAMAS == ft) 
        return new VamasDataSet();
    else if (FT_UDF == ft) 
        return new UdfDataSet();
    else if (FT_SPE == ft) 
        return new WinspecSpeDataSet();
    //else if (FT_PDCIF == ft) 
    //    return new PdCifDataSet();
    else if (FT_PHILIPS_RAW == ft) 
        return new PhilipsRawDataSet();
    else if (FT_GSAS== ft) 
        return new GsasDataSet();
    else {
        throw XY_Error("unkown or unsupported file type");
        return 0; // to avoid warnings
    }
}

DataSet* getNewDataSet(istream &is, xy_ftype filetype /* = FT_UNKNOWN */, 
    const string &filename /* = "" */)
{
    if (filetype == FT_UNKNOWN)
        filetype = guess_filetype(filename);
    DataSet *pd = dataset_factory(filetype);
    pd->load_data(is); 
    return pd;
}

// filename: path, filename or only extension *with dot*
vector<FormatInfo const*> get_possible_filetypes(string const& filename)
{
    vector<FormatInfo const*> results;

    // get extension
    string::size_type pos = filename.find_last_of('.');
    if (pos == string::npos) 
        return results;
    string ext = str_tolower(filename.substr(pos + 1));

    for (FormatInfo const **i = formats; *i != NULL; ++i) {
        vector<string> const& exts = (*i)->exts;
        if (find(exts.begin(), exts.end(), ext) != exts.end())
            results.push_back(*i);
    }
    return results;
}

xy_ftype guess_filetype(const string &path)
{
    vector<FormatInfo const*> possible = get_possible_filetypes(path);
    if (possible.empty())
        return FT_UNKNOWN;
    else if (possible.size() == 1)
        // don't check file's content
        return possible[0]->ftype;
    else {
        ifstream f(path.c_str(), ios::in | ios::binary);
        for (vector<FormatInfo const*>::const_iterator i = possible.begin(); 
                                                     i != possible.end(); ++i)
            if ((*i)->check(f)) 
                return (*i)->ftype;

        return FT_UNKNOWN;
    }
}


xy_ftype string_to_ftype(const std::string &ftype_name) 
{
    for (int i = 0; formats[i] != NULL; ++i) {
        if (ftype_name == formats[i]->name) {
            return static_cast<xy_ftype>(i);
        }
    }
    return FT_UNKNOWN;
}


} // end of namespace xylib



// Implementation of Public API of xylib library.
// Licence: Lesser GNU Public License 2.1 (LGPL) 
// $Id$

#include "xylib.h"

#include <cassert>
#include <iomanip>
#include <algorithm>

#include "util.h"
#include "brucker_raw.h"
#include "rigaku_dat.h"
#include "text.h"
#include "uxd.h"
#include "vamas.h"
#include "philips_udf.h"
#include "winspec_spe.h"
#include "pdcif.h"
#include "philips_raw.h"
//#include "gsas.h"
#include "cpi.h"
#include "dbws.h"
#include "canberra_mca.h"
#include "xfit_xdd.h"
#include "riet7.h"

using namespace std;
using namespace xylib::util;

namespace xylib {

const FormatInfo *formats[] = {
    &CpiDataSet::fmt_info,
    &UxdDataSet::fmt_info,
    &RigakuDataSet::fmt_info,
    &BruckerRawDataSet::fmt_info,
    &VamasDataSet::fmt_info,
    &UdfDataSet::fmt_info,
    &WinspecSpeDataSet::fmt_info,
    &PdCifDataSet::fmt_info,
    &PhilipsRawDataSet::fmt_info,
    //&GsasDataSet::fmt_info,
    &CanberraMcaDataSet::fmt_info,
    &XfitXddDataSet::fmt_info,
    &Riet7DataSet::fmt_info,
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

const FormatInfo* get_format(int n)
{
    if (n < 0 || (size_t) n > sizeof(formats) / sizeof(formats[0]))
        throw RunTimeError("Format index out of range: " + S(n));
    return formats[n];
}
//////////////////////////////////////////////////////////////////////////

string const& MetaData::get(string const& key) const
{
    const_iterator it = find(key);
    if (it == end()) 
        throw RunTimeError("no such key in meta-info found");
    return it->second;
}

bool MetaData::set(string const& key, string const& val)
{
    return insert(make_pair(key, val)).second;
}

//////////////////////////////////////////////////////////////////////////

Column* const Block::index_column = new StepColumn(0, 1);

Block::~Block()
{
    vector<Column*>::iterator it;
    for (it = cols.begin(); it != cols.end(); ++it) {      
        delete *it;
    }
}


const Column& Block::get_column(int n) const
{
    if (n == 0)
        return *index_column;
    int c = (n < 0 ? n + cols.size() : n - 1);
    if (c < 0 || c >= (int) cols.size()) 
        throw RunTimeError("column index out of range: " + S(n));
    return *cols[c];
}

void Block::add_column(Column *c, string const& title, bool append) 
{ 
    if (!title.empty())
        c->name = title;
    cols.insert((append ? cols.end() : cols.begin()), c); 
}

int Block::get_point_count() const
{
    int min_n = -1;
    for (vector<Column*>::const_iterator i=cols.begin(); i != cols.end(); ++i){
        int n = (*i)->get_point_count();
        if (min_n == -1 || (n != -1 && n < min_n))
            min_n = n;
    }
    return min_n;
}

vector<Block*> Block::split_on_column_lentgh()
{
    vector<Block*> result;
    if (cols.empty())
        return result;
    result.push_back(this);
    const int n1 = cols[0]->get_point_count();
    for (size_t i = 1; i < cols.size(); /*nothing*/) {
        const int n = cols[i]->get_point_count();
        if (n == n1)
            ++i;
        else {
            int new_b_idx = -1;
            for (size_t j = 1; j < result.size(); ++j) {
                if (result[j]->get_point_count() == n) {
                    new_b_idx = j;
                    break;
                }
            }
            if (new_b_idx == -1) {
                new_b_idx = result.size();
                Block* new_block = new Block;
                new_block->meta = meta;
                new_block->name = name + "_" + S(n);
                result.push_back(new_block);
            }
            result[new_b_idx]->add_column(cols[i]);
            cols.erase(cols.begin() + i);
        }
    }
    return result;
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
        throw RunTimeError("no block #" + S(n) + " in this file.");
    return blocks[n];
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

DataSet* load_file(string const& path, string const& format_name,
                   vector<string> const& options)
{
    ifstream is(path.c_str(), ios::in | ios::binary);
    if (!is) 
        throw RunTimeError("can't open input file: " + path);

    FormatInfo const* fi = NULL;
    if (format_name.empty()) {
        fi = guess_filetype(path);
        if (!fi)
            throw RunTimeError ("Format of the file can not be guessed");
    }
    else {
        fi = string_to_format(format_name);
        if (!fi)
            throw RunTimeError("Unsupported (misspelled?) data format: " 
                                + format_name);
    }

    return load_stream(is, fi, options);
}

DataSet* load_stream(istream &is, FormatInfo const* fi, 
                     vector<string> const& options)
{
    assert(fi != NULL);
    DataSet *pd = (*fi->ctor)();
    pd->options = options;
    pd->load_data(is); 
    return pd;
}

// filename: path, filename or only extension with dot
vector<FormatInfo const*> get_possible_filetypes(string const& filename)
{
    vector<FormatInfo const*> results;

    // get extension
    string::size_type pos = filename.find_last_of('.');
    string ext = (pos == string::npos) ? string() : filename.substr(pos + 1);

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
        throw RunTimeError("can't open input file: " + path);
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

string get_wildcards_string(string const& all_files)
{
    string r;
    for (FormatInfo const **i = formats; *i != NULL; ++i) {
        if (!r.empty())
            r += "|";
        string ext_list;
        if ((*i)->exts.empty())
            ext_list = all_files;
        else {
            for (size_t j = 0; j < (*i)->exts.size(); ++j) {
                if (j != 0)
                    ext_list += ";";
                ext_list += "*." + (*i)->exts[j];
            }
        }
        string up = ext_list;
        transform(up.begin(), up.end(), up.begin(), (int(*)(int)) toupper);
        r += (*i)->desc + " (" + ext_list + ")|" + ext_list; 
        if (up != ext_list) // if it contains only (*.*) it won't be appended 
            r += ";" + up;
    }
    return r;
}

/// see also XYLIB_VERSION
string get_version()
{
    int major = XYLIB_VERSION / 10000;
    int minor = XYLIB_VERSION / 100 % 100;
    int subminor = XYLIB_VERSION % 100;
    return S(major) + "." + S(minor) + "." + S(subminor);
}

} // end of namespace xylib



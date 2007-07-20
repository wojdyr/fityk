// XYlib library is a xy data reading library, aiming to read variaty of xy 
// data formats.
// Licence: GNU General Public License version 2
// $Id: xylib.h $

#include "xylib.h"
#include "util.h"
#include "common.h"
#include "ds_brucker_raw_v1.h"
#include "ds_brucker_raw_v23.h"
#include "ds_rigaku_dat.h"
#include "ds_text.h"
#include "ds_uxd.h"
#include "ds_vamas.h"
#include "ds_philips_udf.h"

#include <algorithm>
#include <boost/detail/endian.hpp>
#include <boost/cstdint.hpp>

using namespace std;
using namespace xylib::util;
using namespace boost;

namespace xylib {

const string g_ftype[] = {
    "",
    "text",
    "uxd",
    "rigaku_dat",
    "diffracat_v1_raw",
    "diffracat_v2v3_raw",
    "vamas_iso14976",
    "philips_udf",
};

const string g_desc[] = {
    "",
    "the ascii plain text Format",
    "Siemens/Bruker Diffrac-AT UXD Format",
    "Rigaku dat Format",
    "Siemens/Bruker Diffrac-AT Raw Format v1",
    "Siemens/Bruker Diffrac-AT Raw Format v2/v3",
    "ISO14976 VAMAS Surface Chemical Analysis Standard Data Transfer Format",
    "Philipse UDF Format",
};


 
//////////////////////////////////////////////////////////////////////////
// member functions of Class Range

void Range::check_idx(unsigned n, const string &name) const
{
    if (n >= get_pt_count()) {
        throw XY_Error("no " + name + " in this range with such index");
    }
}

double Range::get_x(unsigned n) const
{
    check_idx(n, "point_x");
    return x[n];
}

double Range::get_y(unsigned n) const
{
    check_idx(n, "point_y");
    return y[n];
}


bool Range::has_y_stddev(unsigned n) const
{ 
    if (n >= get_pt_count()) {
        return false;
    } else {
        return y_has_stddev[n]; 
    }    
} 

double Range::get_y_stddev(unsigned n) const
{
    check_idx(n, "point_y_stddev");
    return y_stddev[n];
}

void Range::add_pt(double x_, double y_, double stddev_)
{
    x.push_back(x_);
    y.push_back(y_);
    y_stddev.push_back(stddev_);
    y_has_stddev.push_back(true);
}

void Range::add_pt(double x_, double y_)
{
    x.push_back(x_);
    y.push_back(y_);
    y_stddev.push_back(1);  // meaningless data
    y_has_stddev.push_back(false);
}

void Range::export_xy_file(const string &fname) const
{
    ofstream of(fname.c_str());
    if(!of) {
        throw XY_Error("can't create file " + fname + " to output");
    }

    export_xy_file(of);
}


void Range::export_xy_file(ofstream &of) const
{
    int n = get_pt_count();
    of << ";total count:" << n << endl << endl;
    of << ";x\t\ty\t\t y_stddev" << endl;
    for(int i = 0; i < n; ++i) {
        of << setfill(' ') << setiosflags(ios::fixed) << setprecision(5) << setw(7) << 
            get_x(i) << "\t" << setprecision(8) << setw(10) << get_y(i) << "\t";
        if(has_y_stddev(i)) {
            of << get_y_stddev(i);
        }
        of << endl;
    }
}

void Range::add_meta(const string &key, const string &val)
{
    pair<map<string, string>::iterator, bool> ret = meta_map.insert(make_pair(key, val));
    if (!ret.second) {
        // the meta-info with this key already exists
        //throw XY_Error("meta-info with key " + key + "already exists");
    }
}


// return a string vector containing all of the meta-info keys
vector<string> Range::get_all_meta_keys() const
{
    vector<string> keys;

    map<string, string>::const_iterator it = meta_map.begin();
    for (it = meta_map.begin(); it != meta_map.end(); ++it) {
        keys.push_back(it->first);
    }

    return keys;
}

bool Range::has_meta_key(const string &key) const
{
    map<string, string>::const_iterator it = meta_map.find(key);
    return (it != meta_map.end());
}

const string& Range::get_meta(string const& key) const
{
    map<string, string>::const_iterator it;
    it = meta_map.find(key);
    if (meta_map.end() == it) {
        // not found
        throw XY_Error("no such key in meta-info found");
    } else {
        return it->second;
    }
}

//////////////////////////////////////////////////////////////////////////
// member functions of Class FixedStepRange

void FixedStepRange::add_y(double y_)
{
    y.push_back(y_);
    y_stddev.push_back(1);
    y_has_stddev.push_back(false);
}


void FixedStepRange::add_y(double y_, double stddev_)
{
    y.push_back(y_);
    y_stddev.push_back(stddev_);
    y_has_stddev.push_back(true);
}

//////////////////////////////////////////////////////////////////////////
// member functions of Class DataSet

const Range& DataSet::get_range(unsigned i) const
{
    if (i >= ranges.size()) {
        throw XY_Error("no range in this file with such index");
    }

    return *(ranges[i]);
}


void DataSet::export_xy_file(const string &fname, 
    bool with_meta /* = true */, const std::string &cmt_str /* = ";" */) const
{
    unsigned range_num = get_range_cnt();
    ofstream of(fname.c_str());
    if (!of) {
        throw XY_Error("can't create file" + fname);
    }

    of << cmt_str << "exported from " << filename << endl;

    // output the file-level meta-info
    if (with_meta) {
        output_meta(of, this, cmt_str);
    }
    
    for(unsigned i = 0; i < range_num; ++i) {
        const Range &rg = get_range(i);
        of << endl;
        for (int j = 0; j < 40; ++j) {
            of << cmt_str;
        }
        of << endl << cmt_str << "* range " << i << endl;
        if (with_meta) {
            output_meta(of, &rg, cmt_str);
        }
        rg.export_xy_file(of);
    }
}

void DataSet::add_meta(const string &key, const string &val)
{
    pair<map<string, string>::iterator, bool> ret = meta_map.insert(make_pair(key, val));
    if (!ret.second) {
        // the meta-info with this key already exists
        //throw XY_Error("meta-info with key " + key + "already exists");
    }
}


// return a string vector containing all of the meta-info keys
vector<string> DataSet::get_all_meta_keys() const
{
    vector<string> keys;

    map<string, string>::const_iterator it = meta_map.begin();
    for (it = meta_map.begin(); it != meta_map.end(); ++it) {
        keys.push_back(it->first);
    }

    return keys;
}

bool DataSet::has_meta_key(const string &key) const
{
    map<string, string>::const_iterator it = meta_map.find(key);
    return (it != meta_map.end());
}

const string& DataSet::get_meta(string const& key) const
{
    map<string, string>::const_iterator it;
    it = meta_map.find(key);
    if (meta_map.end() == it) {
        // not found
        throw XY_Error("no such key in meta-info found");
    } else {
        return it->second;
    }
}

DataSet::~DataSet()
{
    vector<Range*>::iterator it;
    for (it = ranges.begin(); it != ranges.end(); ++it) {      
        delete *it;
    }
    if (p_ifs) {
        delete p_ifs;
    }
}

void DataSet::init()
{
    // open given file
    p_ifs = new ifstream(filename.c_str(), ios::in | ios::binary);
    if (!p_ifs) {
        throw XY_Error("Can't open file: " + filename);
    }

    // check whether legal
    if (!is_filetype()) {
        throw XY_Error("file is not the expected " + get_filetype() + " format");
    }

    p_ifs->seekg(0);    // reset the ifstream, as if no lines have been read
}


//////////////////////////////////////////////////////////////////////////
// members of UxdLikeDataSet

// set default value of variables in UxdLikeDataSet
string UxdLikeDataSet::rg_start_tag("");
string UxdLikeDataSet::x_start_key("");
string UxdLikeDataSet::x_step_key("");
string UxdLikeDataSet::meta_sep("=:");
string UxdLikeDataSet::data_sep(", ;");
string UxdLikeDataSet::cmt_start(";#");


line_type UxdLikeDataSet::get_line_type(const string &line)
{
    string ll = str_trim(line);
   
    if (ll.empty()) {
        return LT_EMPTY;
    } else {
        char ch = ll[0];
        if (str_startwith(ll, cmt_start)) {
            return LT_COMMENT;
        } else if (string::npos != ll.find_first_of(meta_sep)) {
            return LT_KEYVALUE;
        } else if (string::npos != string("0123456789+-").find(ch)) {
            return LT_XYDATA;
        } else {
            return LT_UNKNOWN;
        }
    }    
}


// move "if" ptr to a line which has meaning to us
// return ture if not eof, false otherwise
bool UxdLikeDataSet::skip_invalid_lines(std::ifstream &f)
{
    string line;
    if (!peek_line(f, line, false)) {
        f.seekg(-1, ios_base::end); // set eof
        return false;
    }
    
    line_type type = get_line_type(line);
    while (LT_COMMENT == type || LT_EMPTY == type) {
        skip_lines(f, 1);
        if (!peek_line(f, line, false)) {
            f.seekg(-1, ios_base::end); // set eof
            return false;
        }
        type = get_line_type(line);
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
// namespace scope global functions

DataSet* getNewDataSet(const string &filename, xy_ftype filetype /* = FT_UNKNOWN */)
{
    DataSet *pd = NULL;
    xy_ftype ft = (FT_UNKNOWN == filetype) ? guess_file_type(filename) : filetype;

    if (FT_BR_RAW1 == ft) {
        pd = new BruckerV1RawDataSet(filename); 
    } else if (FT_BR_RAW23 == ft) {
        pd = new BruckerV23RawDataSet(filename);
    } else if (FT_UXD == ft) {
        pd = new UxdDataSet(filename);
    } else if (FT_TEXT == ft) {
        pd = new TextDataSet(filename);
    } else if (FT_RIGAKU == ft) {
        pd = new RigakuDataSet(filename);
    } else if (FT_VAMAS == ft) {
        pd = new VamasDataSet(filename);
    } else if (FT_UDF == ft) {
        pd = new UdfDataSet(filename);
    } else {
        pd = NULL;
        throw XY_Error("unkown or unsupported file type");
    }

    if (NULL != pd) {
        pd->load_data(); 
    }

    return pd;
}


xy_ftype guess_file_type(const string &fname)
{
    string::size_type pos = fname.find_last_of('.');

    if(string::npos == pos) {
        return FT_UNKNOWN;
    }

    string ext = fname.substr(pos + 1);
    for(string::iterator it = ext.begin();it != ext.end();++it) {
        *it = tolower(*it);
    }


    if("txt" == ext) {
        return FT_TEXT;
    } else if ("uxd" == ext) {
        return FT_UXD;
    } else if ("raw" == ext) {
        //TODO: need to detect format by their content
        return FT_BR_RAW1;
    } else if ("vms" == ext) {
        return FT_VAMAS;
    } else if ("udf" == ext) {
        return FT_UDF;
    } else {
        return FT_UNKNOWN;
    }
}


xy_ftype string_to_ftype(const std::string &ftype_name) {
    return (xy_ftype)get_array_idx(g_ftype, FT_NUM, ftype_name);
}


} // end of namespace xylib



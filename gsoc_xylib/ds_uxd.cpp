// Implementation of class UxdDataSet for reading meta-data and xy-data from
// Siemens/Bruker Diffrac-AT UXD Format
// Licence: Lesser GNU Public License 2.1 (LGPL) 
// $Id: ds_uxd.cpp $

/*

FORMAT DESCRIPTION:
====================

Siemens/Bruker Diffrac-AT UXD Format, data format used in Siemens/Brucker 
X-ray diffractors. It can be inter-convertable to RAW format by the official 
tool XCH.

///////////////////////////////////////////////////////////////////////////////
    * Name in progam:   uxd
    * Extension name:   uxd
    * Binary/Text:      text
    * Multi-ranged:     Y
    
///////////////////////////////////////////////////////////////////////////////
    * Format details: 
It has a header indicating the file-scope parameters in the form of 
"key=val" format. Followed the file header are range sections. Each section 
starts with "_DRIVER=XXX". In each section, first lines are range-scope meta-
info; X-Y data starts after "_COUNT".

///////////////////////////////////////////////////////////////////////////////
    * Sample Fragment: ("#xxx": comments added by me; ...: omitted lines)

# File header with some file-scope prarmeters.
_FILEVERSION=1
_SAMPLE='test'
_WL1=1.540600
...
# Data for Range 1
_DRIVE='COUPLED'
_STEPTIME=37.000000
_STEPSIZE=0.020000   # x_step
_STEPMODE='C'
_START=10.0000       # x_start
...
# Range 1 data starts
_COUNTS
     1048      1162      1108      1163      1071      1057      1055       973
     ...
# Repeat if there are more ranges

///////////////////////////////////////////////////////////////////////////////
    * Implementation Ref of xylib: based on the analysis of the sample files.
    
*/

#include "ds_uxd.h"

using namespace std;
using namespace xylib::util;

namespace xylib {

const FormatInfo UxdDataSet::fmt_info(
    FT_UXD,
    "uxd",
    "Siemens/Bruker Diffrac-AT UXD Format",
    vector<string>(1, "uxd"),
    false,                       // whether binary
    true                         // whether multi-ranged
);

bool UxdDataSet::is_filetype() const
{
    return true;
}

void UxdDataSet::load_data() 
{
    init();
    istream &f = *p_is;

    string line, key, val;
    line_type ln_type;

    // file-scope meta-info
    while (true) {
        skip_invalid_lines(f);
        int pos = f.tellg();
        my_getline(f, line);
        if (str_startwith(line, rg_start_tag)) {
            f.seekg(pos);
            break;
        }
        
        ln_type = get_line_type(line);

        if (LT_KEYVALUE == ln_type) {   // file-level meta key-value line
            parse_line(line, meta_sep, key, val);
            key = ('_' == key[0]) ? key.substr(1) : key;
            add_meta(key, val);
        } else {                        // unkonw line type
            continue;
        }
    }

    // handle ranges
    while (!f.eof()) {
        FixedStepRange *p_rg = new FixedStepRange;
        parse_range(p_rg);
        ranges.push_back(p_rg);
    } 
}


// parse a single range of the file
void UxdDataSet::parse_range(FixedStepRange *p_rg)
{
    istream &f = *p_is;

    string line;
    // get range-scope meta-info
    while (true) {
        skip_invalid_lines(f);
        int pos = f.tellg();
        my_getline(f, line);
        line_type ln_type = get_line_type(line);
        if (LT_XYDATA == ln_type) {
            f.seekg(pos);
            break;
        }

        if (LT_KEYVALUE == ln_type) {   // range-level meta key-value line
            string key, val;
            parse_line(line, meta_sep, key, val);
            if (key == x_start_key) {
                p_rg->set_x_start(strtod(val.c_str(), NULL));
            } else if (key == x_step_key) {
                p_rg->set_x_step(strtod(val.c_str(), NULL));
            }
            key = ('_' == key[0]) ? key.substr(1) : key;
            p_rg->add_meta(key, val);
        } else {                        // unkonw line type
            continue;
        }
    }

    // get all x-y data
    while (true) {
        if (!skip_invalid_lines(f)) {
            return;
        }  
        int pos = f.tellg();
        my_getline(f, line, false);
        line_type ln_type = get_line_type(line);
        if (str_startwith(line, rg_start_tag)) {
            f.seekg(pos);
            return;                     // new range
        } else if (LT_XYDATA != ln_type) {
            continue;
        }

        for (string::iterator i = line.begin(); i != line.end(); ++i) {
            if (string::npos != data_sep.find(*i)) {
                *i = ' ';
            }
        }
        
        istringstream q(line);
        double d;
        while (q >> d) {
            p_rg->add_y(d);
        }
    }
}

} // end of namespace xylib


// Implementation of class RigakuDataSet for reading meta-data and xy-data 
// from Rigaku ".dat" format files
// Licence: Lesser GNU Public License 2.1 (LGPL) 
// $Id: ds_rigaku_udf.cpp $

/*
FORMAT DESCRIPTION:
====================

Data format used in the Japanese X-ray instrument manufacturer Rigaku Inc.

///////////////////////////////////////////////////////////////////////////////
    * Name in progam:   rigaku_dat	
    * Extension name:   dat
    * Binary/Text:      text
    * Multi-ranged:     Y

///////////////////////////////////////////////////////////////////////////////
    * Format details: 
It has a file header indicating some file-scope parameters.
It may contain multiple groups/ranges of data, and each group has its own 
group header. Each group header contains some parameters ("*START", "*STOP" 
and "*STEP" included).The data body of one group begins after "*COUNT=XXX"). 

///////////////////////////////////////////////////////////////////////////////
    * Sample Fragment: ("#xxx": comments added by me; ...: omitted lines)

# file header
*TYPE           =  Raw
*CLASS          =  Standard measurement
...
*GROUP_COUNT    =  2
...
# group 0 header
*BEGIN
*GROUP          =  0
...
*START          =  10.0000    
*STOP           =  103.1200  
*STEP           =  0.0200 
...
*COUNT          =  4657
# data in group 0
 1048, 1162, 1108, 1163
 1071, 1057, 1055, 973
# group 0 end
*END
# repeat group segment if extra group(s) exist
...
# end of file
*EOF

///////////////////////////////////////////////////////////////////////////////
    Implementation Ref of xylib: based on the analysis of the sample files.
    
*/

#include "ds_rigaku_dat.h"
#include "util.h"

using namespace std;
using namespace xylib::util;

namespace xylib {

const FormatInfo RigakuDataSet::fmt_info(
    FT_RIGAKU,
    "rigaku_dat",
    "Rigaku dat Format",
    vector<string>(1, "dat"),
    false,                       // whether binary
    true                         // whether multi-ranged
);


// return true if is this type, false otherwise
bool RigakuDataSet::check(istream &f)
{
    f.clear();
    // the first 5 letters must be "*TYPE"
    string head = read_string(f, 5);
    if(f.rdstate() & ios::failbit) {
        throw XY_Error("error when reading file head");
    }
    
    f.seekg(0);
    return ("*TYPE" == head);
}


void RigakuDataSet::load_data() 
{
    if (!check(f)) {
        throw XY_Error("file is not the expected " + get_filetype() + " format");
    }

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
            key = ('*' == key[0]) ? key.substr(1) : key;
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


void RigakuDataSet::parse_range(FixedStepRange* p_rg)
{
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
                p_rg->set_x_start(my_strtod(val));
            } else if (key == x_step_key) {
                p_rg->set_x_step(my_strtod(val));
            }
            key = ('*' == key[0]) ? key.substr(1) : key;
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


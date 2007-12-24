// Implementation of class UxdDataSet for reading meta-info and xy-data from
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
    * Multi-blocks:     Y
    
///////////////////////////////////////////////////////////////////////////////
    * Format details: 
It has a header indicating the file-scope parameters in the form of 
"key=val" format. Followed the file header are block sections. Each section 
starts with "_DRIVER=XXX". In each section, first lines are block-scope meta-
info; X-Y data starts after "_COUNT".

///////////////////////////////////////////////////////////////////////////////
    * Sample Fragment: ("#xxx": comments added by me; ...: omitted lines)

# File header with some file-scope prarmeters.
_FILEVERSION=1
_SAMPLE='test'
_WL1=1.540600
...
# Data for Block 1
_DRIVE='COUPLED'
_STEPTIME=37.000000
_STEPSIZE=0.020000   # x_step
_STEPMODE='C'
_START=10.0000       # x_start
...
# Block 1 data starts
_COUNTS
     1048      1162      1108      1163      1071      1057      1055       973
     ...
# Repeat if there are more blocks/ranges

///////////////////////////////////////////////////////////////////////////////
    * Implementation Ref of xylib: based on the analysis of the sample files.
    
*/

#include "uxd.h"
#include "util.h"

using namespace std;
using namespace xylib::util;

namespace xylib {

const FormatInfo UxdDataSet::fmt_info(
    "uxd",
    "Siemens/Bruker Diffrac-AT UXD Format",
    vector<string>(1, "uxd"),
    false,                       // whether binary
    true                         // whether has multi-blocks
);

bool UxdDataSet::check(istream &f) 
{
    string line;
    while (getline(f, line)) {
        string::size_type p = line.find_first_not_of(" \t\r\n");
        if (p != string::npos && line[p] != ';')
            break;
    }
    return str_startwith(line, "_FILEVERSION");
}


void UxdDataSet::load_data(std::istream &f) 
{
    Block *p_blk = NULL;
    VecColumn *p_ycol = NULL;
    string line;
    double start=0., step=0.;

    while (get_valid_line(f, line, ';')) {
        if (str_startwith(line, "_DRIVE")) { // block starts
            p_blk = new Block;
        }
        else if (str_startwith(line, "_COUNT")) { // data starts
            StepColumn *p_xcol = new StepColumn(start, step);
            p_ycol = new VecColumn;
            p_blk->set_xy_columns(p_xcol, p_ycol);
            blocks.push_back(p_blk);
        } 
        else if (str_startwith(line, "_")) { // meta-data 
            // other meta key-value pair. 
            // NOTE the order, it must follow other "_XXX" branches
            string key, val;
            str_split(line.substr(1), "=", key, val);
            
            if (key == "START") 
                start = my_strtod(val);
            else if (key == "STEPSIZE") 
                step = my_strtod(val);
            else {
                if (p_blk)
                    p_blk->meta[key] = val;
                else
                    meta[key] = val;
            }
            
        } 
        else if (is_numeric(line[0])) {   
            vector<double> values;
            get_all_numbers(line, values);
            
            for (unsigned i = 0; i < values.size(); ++i) {
                p_ycol->add_val(values[i]);
            }
            
        } 
        else {                 
            // unknown type of line. it should not appear in a correct file
            // what should we do here? continue or throw an exception?
            continue;
        }
    }
    format_assert(p_blk);
}

} // end of namespace xylib


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
    FT_UXD,
    "uxd",
    "Siemens/Bruker Diffrac-AT UXD Format",
    vector<string>(1, "uxd"),
    false,                       // whether binary
    true                         // whether has multi-blocks
);

bool UxdDataSet::check(istream &f) {
    string line;
    do {
        my_getline(f, line);
    } while ("" == line || str_startwith(line, ";"));

    f.seekg(0);
    return str_startwith(line, "_FILEVERSION");
}

void UxdDataSet::load_data(std::istream &f) 
{
    if (!check(f)) {
        throw XY_Error("file is not the expected " + get_filetype() + " format");
    }
    clear();

    // indicate where we are
    enum {
        FILE_META,
        RANGE_META,
        RANGE_DATA,
    } pos_flg = FILE_META;

    Block *p_blk = NULL;
    StepColumn *p_xcol = NULL;
    VecColumn *p_ycol = NULL;
    string line;

    while (get_valid_line(f, line, ";")) {
        if (str_startwith(line, "_DRIVE")) {        // first key in a block, indicates block starts
            pos_flg = RANGE_META;
            if (p_blk != NULL) {
                // save the last unsaved block
                blocks.push_back(p_blk);
                p_blk = NULL;
            }
        
            p_xcol = new StepColumn;
            p_ycol = new VecColumn;
            p_blk = new Block;
            
            p_blk->add_column(p_xcol, Block::CT_X);
            p_blk->add_column(p_ycol, Block::CT_Y);
            
        } else if (str_startwith(line, "_")) {    // other meta key-value pair. NOTE the order, it must follow other "_XXX" branches
            string key, val;
            parse_line(line, key, val, "=");
            key = key.substr(1);    // rm the leading '_'
            
            if ("START" == key) {
                p_xcol->set_start(my_strtod(val));
            } else if ("STEPSIZE" == key) {
                p_xcol->set_step(my_strtod(val));
            }
            
            switch (pos_flg) {
            case FILE_META:
                add_meta(key, val);
                break;
            case RANGE_META:
                p_blk->add_meta(key, val);
                break;
            case RANGE_DATA:
                p_blk->add_meta(key, val);
                break;            
            default:
                break;
            }
            
        } else if (start_as_num(line)){            // should be a li  ne of values
            vector<double> values;
            get_all_numbers(line, values);
            
            for (unsigned i = 0; i < values.size(); ++i) {
                p_ycol->add_val(values[i]);
            }
            
            if (RANGE_META == pos_flg) {
                pos_flg = RANGE_DATA;
            }
        } else {                 // unknown type of line. it should not appear in a correct file
            // what should we do here? continue or throw an exception?
            continue;
        }
    }

    // add the last block
    if (p_blk != NULL) {
        // save the last unsaved block
        blocks.push_back(p_blk);
        p_blk = NULL;
    }
}

} // end of namespace xylib


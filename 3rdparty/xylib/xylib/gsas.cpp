// Implementation of class GsasDataSet for reading meta-info and xy-data from
// GSAS File Format
// Licence: Lesser GNU Public License 2.1 (LGPL) 
// $Id: ds_gsas.cpp $

/*

FORMAT DESCRIPTION:
====================

GSAS data format, used in the software called GSAS.

///////////////////////////////////////////////////////////////////////////////
    * Name in progam:   gsas
    * Extension name:   gss
    * Binary/Text:      text
    * Multi-blocks:     N
    
///////////////////////////////////////////////////////////////////////////////
    * Format details: 
This format is very simple, see the sample fragment section.

///////////////////////////////////////////////////////////////////////////////
    * Sample Fragment: ("#xxx": comments added by me; ...: omitted lines)

SampleIdent CPD RR S  
#              pt_cnt                   x_start*100 (x_step*100)
BANK    1       7251       726 CONST        500.00      2.00     0.000     0.000
# Y-data start. Note "1' is put as a separator
 1   153 1   161 1   141 1   141 1   148 1   163 1   139 1   139 1   129 1   132
 ...    # Y-data points
 
    
///////////////////////////////////////////////////////////////////////////////
    * Implementation Ref of xylib: based on the analysis of the sample files.
   
*/

#include "gsas.h"
#include "util.h"

using namespace std;
using namespace xylib::util;

namespace xylib {

const FormatInfo GsasDataSet::fmt_info(
    FT_GSAS,
    "gsas",
    "GSAS Instrumental File Format",
    vector<string>(1, "gss"),
    false,                       // whether binary
    false                         // whether has multi-blocks
);

bool GsasDataSet::check(istream &f) {
    string line;
    do {
        my_getline(f, line);
    } while ("" == line || str_startwith(line, ";"));

    f.seekg(0);
    return str_startwith(line, "SampleIdent");
}

void GsasDataSet::load_data(std::istream &f) 
{
    if (!check(f)) {
        throw XY_Error("file is not the expected " + get_filetype() + " format");
    }
    clear();

    // gsas format has only one block with fixed-step X, 
    // so create them here
    StepColumn *p_xcol = new StepColumn;
    VecColumn *p_ycol = new VecColumn;

    Block *p_blk = new Block;
    p_blk->set_xy_columns(p_xcol, p_ycol);

    string line;

    // read in line 1, parsing "sample ident" meta-info
    my_getline(f, line);
    string ident = line.substr(13);
    p_blk->meta["sample ident"] = ident;

    // read in line 2. get pt_cnt, x_start & x_step
    my_getline(f, line);
    string::size_type spos(0), epos(0);
    int pt_cnt(0);

    int cnt = 0;
    while (string::npos != spos) {
        epos = line.find_first_of(" \t", spos);
        string word = line.substr(spos, epos - spos);
        
        switch (cnt++) {
            case 2:
                pt_cnt = my_strtol(word);
                break;
            case 5:
                p_xcol->set_start(my_strtod(word) / 100.0);
                break; 
            case 6:
                p_xcol->set_step(my_strtod(word) / 100.0);
                break;
            default:
                break;
        }
        
        spos = line.find_first_not_of(" \t", epos);
    }

    // read in the X-Y data
    int idx = 0;
    bool flag = true;
    while (flag) {
        bool non_eof = get_valid_line(f, line, "");
        if(!non_eof) {
            flag = false;
        }

        string::size_type val_spos(0), val_epos(0);
        while (true) {
            val_spos = line.find_first_not_of(" \t", val_epos);
            if (string::npos == val_spos) {
                break;
            }

            val_epos = line.find_first_of(" \t", val_spos);
            string val = line.substr(val_spos, val_epos - val_spos);

            if (0 == (idx++ % 2)) {
                my_assert("1" == val, "format error: splitter '1' missing");      // "1" as splitter
            } else {
                p_ycol->add_val(my_strtod(val));
            }
        }
    }

    my_assert(p_ycol->get_pt_cnt() == pt_cnt, "file corrupt: points count is not as expected");
    
    blocks.push_back(p_blk);
}

} // end of namespace xylib


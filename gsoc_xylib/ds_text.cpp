// Implementation of class TextDataSet for reading meta-data and xy-data from 
// ascii plain text Format
// Licence: Lesser GNU Public License 2.1 (LGPL) 
// $Id: TextDataSet.h $

/*

FORMAT DESCRIPTION:
====================

X-Y plain text format.

///////////////////////////////////////////////////////////////////////////////
    * Name in progam:   ds_brucker_raw_v1
    * Extension name:   txt, dat, asc
    * Binary/Text:      text
    * Multi-ranged:     N
    
///////////////////////////////////////////////////////////////////////////////
    * Format details: 
In every valid date line, the format is 
x y [std_dev]\n x y [std_dev]\n x y [std_dev]\n ...
delimiters between X and Y may be white spaces or  , : ;
There may be some comment lines without any valid XY data

///////////////////////////////////////////////////////////////////////////////
    * Sample Fragment: ("#xxx": comments added by me; ...: omitted lines)

; Sample date: 2000/12/31 21:32        # comments
38.834110      361
38.872800  ,   318                     # delimiters may be "," etc.
38.911500      352.431
    
///////////////////////////////////////////////////////////////////////////////
    * Implementation Ref of xylib: based on the analysis of the sample files.

*/

#include "ds_text.h"
#include "ds_rigaku_dat.h"
#include "util.h"

using namespace std;
using namespace xylib::util;

namespace xylib {

string exts[3] = { "txt", "dat", "asc" };

const FormatInfo TextDataSet::fmt_info(
    FT_TEXT,
    "text",
    "the ascii plain text Format",
    vector<string>(exts, exts + 3),
    false,                       // whether binary
    false                        // whether multi-ranged
);

bool TextDataSet::check(istream &f) {
    if (RigakuDataSet::check(f)) {
        return false;
    }

    f.seekg(0);
    return true;
}

void TextDataSet::load_data() 
{
    if (!check(f)) {
        throw XY_Error("file is not the expected " + get_filetype() + " format");
    }

    Range* p_rg = new Range;

    vector<double> xy;
    while (read_line_and_get_all_numbers(f, xy)) {
        if (xy.empty()) {
            continue;
        }

        if (xy.size() == 1) {
            throw XY_Error("only one number in a line");
        }
        
        double x = xy[0];
        double y = xy[1];
        if (xy.size() == 2) {
            p_rg->add_pt(x, y);
        } else if (xy.size() == 3){
            double sig = xy[2];
            if (sig >= 0) {
                p_rg->add_pt(x, y, sig);
            }
        }
    }

    ranges.push_back(p_rg);
}

} // end of namespace xylib


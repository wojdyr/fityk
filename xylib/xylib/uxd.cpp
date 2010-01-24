// Siemens/Bruker Diffrac-AT UXD text format (for powder diffraction data)
// Licence: Lesser GNU Public License 2.1 (LGPL)
// $Id$

#include "uxd.h"
#include "util.h"

using namespace std;
using namespace xylib::util;

namespace xylib {

const FormatInfo UxdDataSet::fmt_info(
    "uxd",
    "Siemens/Bruker Diffrac-AT UXD",
    vector<string>(1, "uxd"),
    false,                       // whether binary
    true,                        // whether has multi-blocks
    &UxdDataSet::ctor,
    &UxdDataSet::check
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

/*
A header (with file-scope parameters) is followed by block sections.
Each section consists of:
 _DRIVE=...
 parameters - key-value pairs
 _COUNT keyword
 list of intensities
comments start with semicolon ';'

Format example:

; File header with some file-scope prarmeters.
_FILEVERSION=1
_SAMPLE='test'
_WL1=1.540600
...
; Data for Block 1
_DRIVE='COUPLED'
_STEPTIME=37.000000
_STEPSIZE=0.020000   # x_step
_STEPMODE='C'
_START=10.0000       # x_start
...
; Block 1 data starts
_COUNTS
     1048      1162      1108      1163      1071      1057      1055       973
     1000      1031      1068      1015       983      1028      1030      1019
     ...
; Repeat if there are more blocks/ranges

*/

void UxdDataSet::load_data(std::istream &f)
{
    Block *blk = NULL;
    VecColumn *ycol = NULL;
    string line;
    double start=0., step=0.;

    while (get_valid_line(f, line, ';')) {
        if (str_startwith(line, "_DRIVE")) { // block starts
            blk = new Block;
        }
        else if (str_startwith(line, "_COUNT")) { // data starts
            StepColumn *xcol = new StepColumn(start, step);
            blk->add_column(xcol);
            ycol = new VecColumn;
            blk->add_column(ycol);
            blocks.push_back(blk);
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
                if (blk)
                    blk->meta[key] = val;
                else
                    meta[key] = val;
            }
        }
        else { //data
            format_assert(is_numeric(line[0]), "line: "+line);
            ycol->add_values_from_str(line);
        }
    }
    format_assert(blk != NULL);
}

} // namespace xylib


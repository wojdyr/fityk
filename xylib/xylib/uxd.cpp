// Siemens/Bruker Diffrac-AT UXD text format (for powder diffraction data)
// Licence: Lesser GNU Public License 2.1 (LGPL)
// $Id$

#define BUILDING_XYLIB
#include "uxd.h"

#include <cerrno>

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

Later versions of this format are more complicated.
In particular, two column data, e.g. angle and counts, are supported.
*/

// get all numbers in the first legal line
// sep is _optional_ separator that can be used in addition to white space
static
void add_values_from_str(string const& str, char sep,
                         VecColumn** cols, int ncols)
{
    const char* p = str.c_str();
    while (isspace(*p) || *p == sep)
        ++p;
    int n = 0;
    while (*p != 0) {
        char *endptr = NULL;
        errno = 0; // To distinguish success/failure after call
        double val = strtod(p, &endptr);
        if (p == endptr)
            throw(xylib::FormatError("Number not found in line:\n" + str));
        if (errno != 0)
            throw(xylib::FormatError("Numeric overflow or underflow in line:\n"
                                     + str));
        cols[n]->add_val(val);
        ++n;
        if (n == ncols)
            n = 0;
        p = endptr;
        while (isspace(*p) || *p == sep)
            ++p;
    }
}

void UxdDataSet::load_data(std::istream &f)
{
    Block *blk = NULL;
    VecColumn* cols[2] = { NULL, NULL };
    int ncols = 0;
    string line;
    double start=0., step=0.;
    bool peak_list = false;

    while (get_valid_line(f, line, ';')) {
        if (str_startwith(line, "_DRIVE")) { // block starts
            blk = new Block;
        }
        else if (str_startwith(line, "_COUNT") ||
                 str_startwith(line, "_CPS")) {
            ncols = 1;
            StepColumn* xcol = new StepColumn(start, step);
            blk->add_column(xcol);
            VecColumn* ycol = new VecColumn;
            blk->add_column(ycol);
            cols[0] = ycol;
            ncols = 1;
            blocks.push_back(blk);
            peak_list = false;
        }
        else if (str_startwith(line, "_2THETACOUNTS") ||
                 str_startwith(line, "_2THETACPS") ||
                 str_startwith(line, "_2THETACOUNTSTIME")) { // data starts
            VecColumn* xcol = new VecColumn;
            blk->add_column(xcol);
            VecColumn* ycol = new VecColumn;
            blk->add_column(ycol);
            cols[0] = xcol;
            cols[1] = ycol;
            ncols = 2;
            blocks.push_back(blk);
            peak_list = false;
        }
        // these keywords specify peak list, which we are not interested in
        else if (str_startwith(line, "_D-I") ||
                 str_startwith(line, "_2THETA-I")) {
            peak_list = true;
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
        else if (!peak_list) { //data
            format_assert(is_numeric(line[0]), "line: "+line);
            format_assert(cols[0] != NULL,
                          "Data started without raw data keyword:\n" + line);
            add_values_from_str(line, ',', cols, ncols);
        }
    }
    format_assert(blk != NULL);
}

} // namespace xylib


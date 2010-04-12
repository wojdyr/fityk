// Sietronics Sieray CPI format
// Licence: Lesser GNU Public License 2.1 (LGPL)
// $Id$

#define BUILDING_XYLIB
#include "cpi.h"
#include "util.h"

using namespace std;
using namespace xylib::util;

namespace xylib {


const FormatInfo CpiDataSet::fmt_info(
    "cpi",
    "Sietronics Sieray CPI",
    vector_string("cpi"),
    false,                       // whether binary
    false,                       // whether has multi-blocks
    &CpiDataSet::ctor,
    &CpiDataSet::check
);

bool CpiDataSet::check(istream &f)
{
    string line;
    getline(f, line);
    return str_startwith(line, "SIETRONICS XRD SCAN");
}

void CpiDataSet::load_data(std::istream &f)
{
    /* format example:
        SIETRONICS XRD SCAN
        10.00
        155.00
        0.010
        Cu
        1.54056
        1-1-1900
        0.600
        HH117 CaO:Nb2O5 neutron batch .0
        SCANDATA
        8992
        9077
        9017
        9018
        9129
        9057
        ...
    */

    Block* blk = new Block;

    string s;
    getline (f, s); // first line
    getline (f, s);//xmin
    double xmin = my_strtod(s);
    getline (f, s); //xmax
    getline (f, s); //xstep
    double xstep = my_strtod(s);
    StepColumn *xcol = new StepColumn(xmin, xstep);
    blk->add_column(xcol);

    // ignore the rest of the header
    while (!str_startwith(s, "SCANDATA"))
        getline (f, s);

    // data
    VecColumn *ycol = new VecColumn();
    while (getline(f, s))
        ycol->add_val(my_strtod(s));

    blk->add_column(ycol);
    blocks.push_back(blk);
}

} // namespace xylib


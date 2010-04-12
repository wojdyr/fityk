// RIET7/LHPM/CSRIET DAT,  ILL_D1A5 and PSI_DMC formats
// Licence: Lesser GNU Public License 2.1 (LGPL)
// $Id$

#define BUILDING_XYLIB
#include "riet7.h"
#include "util.h"

using namespace std;
using namespace xylib::util;

namespace xylib {


const FormatInfo Riet7DataSet::fmt_info(
    "riet7",
    "RIET7/ILL_D1A5/PSI_DMC DAT",
    vector_string("dat"),
    false,                       // whether binary
    false,                       // whether has multi-blocks
    &Riet7DataSet::ctor,
    &Riet7DataSet::check
);


bool Riet7DataSet::check(istream &f)
{
    for (int i = 0; i < 6; ++i) {
        Column *c = read_start_step_end_line(f);
        if (c) {
            delete c;
            return true;
        }
    }
    return false;
}

void Riet7DataSet::load_data(std::istream &f)
{
    Block *block = read_ssel_and_data(f, 5);
    format_assert(block != NULL);
    blocks.push_back(block);
}

} // namespace xylib


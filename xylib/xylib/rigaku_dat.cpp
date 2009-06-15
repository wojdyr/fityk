// Rigaku .dat format - powder diffraction data from Rigaku diffractometers
// Licence: Lesser GNU Public License 2.1 (LGPL)
// $Id$

#include "rigaku_dat.h"
#include "util.h"

using namespace std;
using namespace xylib::util;

namespace xylib {

const FormatInfo RigakuDataSet::fmt_info(
    "rigaku_dat",
    "Rigaku dat Format",
    vector<string>(1, "dat"),
    false,                       // whether binary
    true,                        // whether has multi-blocks
    &RigakuDataSet::ctor,
    &RigakuDataSet::check
);


// return true if is this type, false otherwise
bool RigakuDataSet::check(istream &f)
{
    string head = read_string(f, 5);
    return head == "*TYPE";
}

/*
The format has a file header indicating some file-scope parameters.
It may contain multiple blocks/ranges/groups of data, and each block has its
own group header. Each group header contains some parameters ("*START", "*STOP"
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
*/
void RigakuDataSet::load_data(std::istream &f)
{
    Block *blk = NULL;
    VecColumn *ycol = NULL;
    int grp_cnt = 0;
    double start = 0., step = 0.;
    int count = 0;
    string line;

    while (get_valid_line(f, line, '#')) {
        if (line[0] == '*') {
            if (str_startwith(line, "*BEGIN")) {   // block starts
                ycol = new VecColumn;
                blk = new Block;
            }
            else if (str_startwith(line, "*END")) { // block ends
                format_assert(count == ycol->get_point_count(),
                              "count of x and y differ");
                StepColumn *xcol = new StepColumn(start, step, count);
                blk->add_column(xcol);
                blk->add_column(ycol);
                blocks.push_back(blk);
                blk = NULL;
                ycol = NULL;
            }
            else if (str_startwith(line, "*EOF")) { // file ends
                break;
            }
            else { // meta key-value pair
                string key, val;
                // parse "*KEY = VALUE"
                str_split(line.substr(1), "=", key, val);
                if (key == "START")
                    start = my_strtod(val);
                else if (key == "STEP")
                    step = my_strtod(val);
                else if (key == "COUNT")
                    count = my_strtol(val);
                else if (key == "GROUP_COUNT")
                    grp_cnt = my_strtol(val);

                if (blk)
                    blk->meta[key] = val;
                else
                    meta[key] = val;
            }
        }
        else { // should be a line of values
            format_assert(is_numeric(line[0]));
            ycol->add_values_from_str(line, ',');
        }
    }
    format_assert(grp_cnt != 0, "no GROUP_COUNT attribute given");
    format_assert(grp_cnt == (int) blocks.size(),
                  "block count different from expected");
}

} // namespace xylib


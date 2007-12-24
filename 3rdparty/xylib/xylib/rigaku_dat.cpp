// Implementation of class RigakuDataSet for reading meta-info and xy-data 
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
    * Multi-blocks:     Y

///////////////////////////////////////////////////////////////////////////////
    * Format details: 
It has a file header indicating some file-scope parameters.
It may contain multiple blocks/ranges/groups of data, and each block has its own 
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
    true                         // whether has multi-blocks
);


// return true if is this type, false otherwise
bool RigakuDataSet::check(istream &f)
{
    string head = read_string(f, 5);
    return head == "*TYPE";
}


void RigakuDataSet::load_data(std::istream &f) 
{
    Block *p_blk = NULL;
    VecColumn *p_ycol = NULL;
    int grp_cnt = 0;
    double start = 0., step = 0.;
    int count = 0;
    string line;

    while (get_valid_line(f, line, '#')) {
        if (line[0] == '*') {
            if (str_startwith(line, "*BEGIN")) {   // block starts
                p_ycol = new VecColumn;
                p_blk = new Block;
            } 
            else if (str_startwith(line, "*END")) { // block ends
                my_assert(count == p_ycol->get_pt_cnt(), 
                          "file corrupt: count of x and y differ");
                StepColumn *p_xcol = new StepColumn(start, step, count);
                p_blk->set_xy_columns(p_xcol, p_ycol);
                blocks.push_back(p_blk);
                p_blk = NULL;
                p_ycol = NULL;
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
                
                if (p_blk) 
                    p_blk->meta[key] = val;
                else
                    meta[key] = val;
            } 
        }
        else if (is_numeric(line[0])) {     // should be a line of values
            vector<double> values;
            get_all_numbers(line, values);
            
            for (unsigned i = 0; i < values.size(); ++i) {
                p_ycol->add_val(values[i]);
            }
        } 
        else 
            ; // unexpected line. ignore.
    }
    my_assert(grp_cnt != 0, "no GROUP_COUNT attribute given");
    my_assert(grp_cnt == (int) blocks.size(), 
              "file corrupt: actual block count differ from expected");
}

} // end of namespace xylib


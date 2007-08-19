// Implementation of class RigakuDataSet for reading meta-data and xy-data 
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
    * Multi-ranged:     Y

///////////////////////////////////////////////////////////////////////////////
    * Format details: 
It has a file header indicating some file-scope parameters.
It may contain multiple groups/ranges of data, and each group has its own 
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

#include "ds_rigaku_dat.h"
#include "util.h"

using namespace std;
using namespace xylib::util;

namespace xylib {

const FormatInfo RigakuDataSet::fmt_info(
    FT_RIGAKU,
    "rigaku_dat",
    "Rigaku dat Format",
    vector<string>(1, "dat"),
    false,                       // whether binary
    true                         // whether multi-ranged
);


// return true if is this type, false otherwise
bool RigakuDataSet::check(istream &f)
{
    f.clear();
    // the first 5 letters must be "*TYPE"
    string head = read_string(f, 5);
    if(f.rdstate() & ios::failbit) {
        return false;
    }
    
    f.seekg(0);
    return ("*TYPE" == head);
}


void RigakuDataSet::load_data(std::istream &f) 
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

    Range *p_rg = NULL;
    StepColumn *p_xcol = NULL;
    VecColumn *p_ycol = NULL;
    unsigned grp_cnt = 0;
    string line;

    while (get_valid_line(f, line, "#")) {
        if (str_startwith(line, "*BEGIN")) {        // range starts
            pos_flg = RANGE_META;
            if (p_rg != NULL) {
                // save the last unsaved range with sanity check
                my_assert(p_xcol->get_pt_cnt() == p_ycol->get_pt_cnt(), 
                    "file corrupt: count of x and y differ");
                
                ranges.push_back(p_rg);
                p_rg = NULL;
            }
        
            p_xcol = new StepColumn;
            p_ycol = new VecColumn;
            p_rg = new Range;
            
            p_rg->add_column(p_xcol, Range::CT_X);
            p_rg->add_column(p_ycol, Range::CT_Y);
            
        } else if (str_startwith(line, "*END")) { // range ends
            pos_flg = FILE_META;
            
        } else if (str_startwith(line, "*EOF")) { // file ends
            break;
        
        } else if (str_startwith(line, "*")) {    // other meta key-value pair. NOTE the order, it must follow other "*XXX" branches
            string key, val;
            parse_line(line, key, val, "=");
            key = key.substr(1);    // rm the leading '*'
            
            if ("START" == key) {
                p_xcol->set_start(my_strtod(val));
            } else if ("STEP" == key) {
                p_xcol->set_step(my_strtod(val));
            } else if ("COUNT" == key) {
                p_xcol->set_count(static_cast<unsigned>(my_strtol(val)));
            } else if ("GROUP_COUNT" == key) {
                grp_cnt = static_cast<unsigned>(my_strtol(val));
            }
            
            switch (pos_flg) {
            case FILE_META:
                add_meta(key, val);
                break;
            case RANGE_META:
                p_rg->add_meta(key, val);
                break;
            case RANGE_DATA:
                p_rg->add_meta(key, val);
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

    // add the last range
    if (p_rg != NULL) {
        // save the last unsaved range with sanity check 
        my_assert(p_xcol->get_pt_cnt() == p_ycol->get_pt_cnt(), 
            "file corrupt: count of x and y differ at last range");

        my_assert(grp_cnt != 0, "no GROUP_COUNT attribute given");
        my_assert(ranges.size() + 1 == grp_cnt, 
            "file corrupt: actual range count differ from expected");
        
        ranges.push_back(p_rg);
        p_rg = NULL;
    }
}

} // end of namespace xylib


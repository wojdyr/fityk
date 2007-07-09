// Implementation of class UxdDataSet for reading meta-data and xy-data from 
// Licence: GNU General Public License version 2
// $Id: UxdDataSet.h $

#include "xylib.h"
#include "common.h"

using namespace std;
using namespace boost;
using namespace xylib;
using namespace xylib::util;

namespace xylib {

bool UxdDataSet::is_filetype() const
{
    return true;
}

void UxdDataSet::load_data() 
{
    /* format example
    ; C:\httpd\HtDocs\course\content\week-1\sample5.raw - (Diffrac AT V1) File converted by XCH V1.0
    _FILEVERSION=1
    _SAMPLE='D2'.
    ...
    ; (Data for Range number 1)
    _DRIVE='COUPLED'
    _STEPTIME=39.000000
    _STEPSIZE=0.020000
    _STEPMODE='C'
    _START=4.990000
    _2THETA=4.990000
    _THETA=0.000000
    _KHI=0.000000
    _PHI=0.000000
    _COUNTS
    6234      6185      5969      6129      6199      5988      6046      5922
    ...
    */
    init();
    ifstream &f = *p_ifs;

    string line;

    // skip the comments in the head
    while (getline(f, line) && str_startwith(line, ";"))
        ;
    
    // the file-scope meta-info
    string key, val;
    while (getline(f, line)) {
        if (str_startwith(line , "; (Data for Range number")) {
            // indicate a new range starts
            break;
        } else if (str_startwith(line , ";")) {
            continue;
        } else {
            parse_line(line, "=", key, val);
            add_meta(key, val);
        }
    }

    vector<string> rg_lines;
    while (getline(f, line)) {
        if (!str_startwith(line, "; (Data for Range number")) {
            rg_lines.push_back(line);
        } else {
            // a new range starts. parse the previous range
            FixedStepRange *p_rg = new FixedStepRange;
            parse_range(rg_lines, p_rg);
            ranges.push_back(p_rg);
        }
    }

    // store the last range
    if (0 != rg_lines.size()) {
        FixedStepRange *p_rg = new FixedStepRange;
        parse_range(rg_lines, p_rg);
        ranges.push_back(p_rg);
    }
}


// parse a single range of the file
void UxdDataSet::parse_range(std::vector<string> &lines, FixedStepRange *p_rg)
{
    fp x_start, x_step = 0;
    string line, key, val;
    unsigned line_cnt = lines.size();
    
    unsigned i = 0;

    // read in the range-scope meta-info
    for (; i < line_cnt; ++i) {
        line = lines[i];

        if (str_startwith(line , ";")) {
            continue;
        } else if (str_startwith(line , "_COUNTS")) {
            break;
        } else {
            
            parse_line(line, "=", key, val);
            if ("_STEPSIZE" == key) {
                x_step = string_to_fp(val);
                p_rg->set_x_step(x_step);
            } else if ("_START" == key) {
                x_start = string_to_fp(val);
                p_rg->set_x_start(x_start);
            } else {
                p_rg->add_meta(key, val);
            }
        }
    }

    // read in xy data
    for (; i < line_cnt; ++i) {
        line = lines[i];
        fp val;
        unsigned j = 0;
        istringstream iss(line);
        while (iss >> val) {
            p_rg->add_y(val);
            ++j;
        }
    }
}

} // end of namespace xylib


// Implementation of class TextDataSet for reading meta-data and xy-data from 
// Licence: GNU General Public License version 2
// $Id: TextDataSet.h $

#include "ds_text.h"
#include "common.h"

using namespace std;
using namespace xylib::util;

namespace xylib {

bool TextDataSet::is_filetype() const
{
    // no detection performed
    return true;
}

void TextDataSet::load_data() 
{
    // text format has only one range
    /* format  x y \n x y \n x y \n ...
    *           38.834110      361
    *           38.872800  ,   318
    *           38.911500      352.431
    * delimiters: white spaces and  , : ;
    */

    init();
    ifstream &f = *p_ifs;

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


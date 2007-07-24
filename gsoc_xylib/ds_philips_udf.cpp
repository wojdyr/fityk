// Implementation of class RigakuDataSet for reading meta-data and xy-data from 
// Licence: GNU General Public License version 2
// $Id: RigakuDataSet.h $

#include "ds_philips_udf.h"
#include "common.h"

using namespace std;
using namespace xylib::util;

namespace xylib {

bool UdfDataSet::is_filetype() const
{
    ifstream &f = *p_ifs;

    f.clear();
    string head = read_string(f, 0, 11);
    if(f.rdstate() & ios::failbit) {
        throw XY_Error("error when reading file head");
    }

    return ("SampleIdent" == head);
}


void UdfDataSet::load_data() 
{
    init();
    ifstream &f = *p_ifs;

    string line, key, val;
    line_type ln_type;
    FixedStepRange *p_rg = new FixedStepRange;

    // file-scope meta-info
    while (true) {
        skip_invalid_lines(f);
        int pos = f.tellg();
        my_getline(f, line);
        if (str_startwith(line, rg_start_tag)) {
            f.seekg(pos);
            break;
        }
        
        ln_type = get_line_type(line);

        if (LT_KEYVALUE == ln_type) {   // file-level meta key-value line
            string tmp1, tmp2;
            parse_line(line, meta_sep, key, tmp1);
            // need split again to get val
            parse_line(tmp1, meta_sep, val, tmp2);
            add_meta(key, val);
            if (key == x_start_key) {
                p_rg->set_x_start(strtod(val.c_str(), NULL));
            } else if (key == x_step_key) {
                p_rg->set_x_step(strtod(val.c_str(), NULL));
            }
        } else {                        // unkonw line type
            continue;
        }
    }

    // UDF format has only 1 ranges
    if (!f.eof()) {
        parse_range(p_rg);
        ranges.push_back(p_rg);
    } 
}


void UdfDataSet::parse_range(FixedStepRange* p_rg)
{
    ifstream &f = *p_ifs;

    string line;

    // get all x-y data
    while (true) {
        if (!skip_invalid_lines(f)) {
            return;
        }
        my_getline(f, line, false);
        line_type ln_type = get_line_type(line);

        if (LT_COMMENT == ln_type) {
            continue;
        }

        for (string::iterator i = line.begin(); i != line.end(); ++i) {
            if (string::npos != data_sep.find(*i)) {
                *i = ' ';
            }
        }
        
        istringstream q(line);
        double d;
        while (q >> d) {
            p_rg->add_y(d);
        }
    }
}

} // end of namespace xylib


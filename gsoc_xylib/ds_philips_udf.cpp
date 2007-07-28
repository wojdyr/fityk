// Implementation of class UdfDataSet for reading meta-data and 
// xy-data from Philis UDF format
// Licence: Lesser GNU Public License 2.1 (LGPL) 
// $Id: ds_philips_udf.cpp $

/*
FORMAT DESCRIPTION:
====================

Data format used in Philips X-ray diffractors.

///////////////////////////////////////////////////////////////////////////////
    * Name in progam:   philipse_udf	
    * Extension name:   udf
    * Binary/Text:      text
    * Multi-ranged:     N
    
///////////////////////////////////////////////////////////////////////////////
    * Format details: 
It has a header indicating some file-scope parameters in the form of 
"key, val1, [val2 ...] ,/" pattern.It has only 1 group/range of data;
data body begins after "RawScan"). 
    
///////////////////////////////////////////////////////////////////////////////
    * Sample Fragment: ("#xxx": comments added by me; ...: omitted lines)

# meta-info
SampleIdent,Sample5 ,/
Title1,Dat2rit program ,/
Title2,Sample5 ,/
...
DataAngleRange,   5.0000, 120.0000,/    # x_start & x_end
ScanStepSize,    0.020,/                # x_step
...
RawScan
    6234,    6185,    5969,    6129,    6199,    5988,    6046,    5922
    6017,    5966,    5806,    5918,    5843,    5938,    5899,    5851
    ...
    
///////////////////////////////////////////////////////////////////////////////
    Implementation Ref of xylib: based on the analysis of the sample files.
*/

#include "ds_philips_udf.h"

using namespace std;
using namespace xylib::util;

namespace xylib {

const FormatInfo UdfDataSet::fmt_info(
    FT_UDF,
    "philips_udf",
    "Philipse UDF Format",
    vector<string>(1, "udf"),
    false,                       // whether binary
    false                        // whether multi-ranged
);

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


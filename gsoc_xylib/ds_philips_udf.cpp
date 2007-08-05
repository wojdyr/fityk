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
    442/                                # last data ends with a '/'
    
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
    istream &f = *p_is;

    f.clear();
    string head = read_string(f, 11);
    if(f.rdstate() & ios::failbit) {
        throw XY_Error("error when reading file head");
    }

    return ("SampleIdent" == head);
}


void UdfDataSet::load_data() 
{
    init();
    istream &f = *p_is;

    double x_start(0), x_step(0);
    string key, val;
    FixedStepRange *p_rg = new FixedStepRange;
    
    while(true) {
        get_key_val(f, key, val);
        if ("RawScan" == key) {
            break;      // indicates XY data start
        } else if ("DataAngleRange" == key) {
            string::size_type pos = val.find_first_of(",");
            x_start = strtod(val.substr(0, pos).c_str(), NULL);
            p_rg->set_x_start(x_start);
        } else if ("ScanStepSize" == key) {
            x_step = strtod(val.c_str(), NULL);
            p_rg->set_x_step(x_step);
        }

        p_rg->add_meta(key, val);
    }

    string line;
    bool end_flg(false);
    while (!end_flg && my_getline(f, line, false)) {
        if (string::npos != line.find_first_of("/")) {
            end_flg = true;
        }

        for (string::iterator i = line.begin(); i != line.end(); i++) {
            if (*i == ',') {
                *i = ' ';
            }
        }

        istringstream ss(line);
        double d;
        while (ss >> d) {
            p_rg->add_y(d);
        }
    }

    ranges.push_back(p_rg);
/*
   while (true) {
        int d;
        f >> d;
        if (!f) {
            throw XY_Error("format error: unexpected EOF");
        }
        p_rg->add_y(d);
        int c = f.get();  // c should be blank or ',' or '/'
        if (c == '/') {
            // end of data
            return;
        }
    }
*/
}


void UdfDataSet::get_key_val(istream &f, string &key, string &val)
{
    string line;
    my_getline(f, line, true);
    string::size_type pos1 = line.find(',');
    if (string::npos == pos1) {
        key = str_trim(line);
        val = "";
        return;
    } else {
        string::size_type pos2 = line.rfind(',');
        key = str_trim(line.substr(0, pos1));
        val = str_trim(line.substr(pos1 + 1, pos2 - pos1 - 1));
    }
}

} // end of namespace xylib


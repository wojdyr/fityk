// Implementation of class UdfDataSet for reading meta-info and 
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
    * Multi-blocks:     N
    
///////////////////////////////////////////////////////////////////////////////
    * Format details: 
It has a header indicating some file-scope parameters in the form of 
"key, val1, [val2 ...] ,/" pattern.It has only 1 blocks/ranges of data;
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

#include "philips_udf.h"
#include "util.h"

using namespace std;
using namespace xylib::util;

namespace xylib {

const FormatInfo UdfDataSet::fmt_info(
    "philips_udf",
    "Philips UDF",
    vector<string>(1, "udf"),
    false,                       // whether binary
    false                        // whether has multi-blocks
);


bool UdfDataSet::check (istream &f)
{
    string head = read_string(f, 11);
    return head == "SampleIdent";
}


void UdfDataSet::load_data(std::istream &f) 
{
    Block *p_blk = new Block;

    double x_start = 0;
    double x_step = 0;
    // read header
    while (true) {
        string line = str_trim(read_line(f));
        if (line == "RawScan") // indicates XY data start
            break;      

        string::size_type pos1 = line.find(',');
        string::size_type pos2 = line.rfind(',');
        // there should be at least two ',' in a key-value line
        format_assert(pos1 != pos2);
        string key = str_trim(line.substr(0, pos1));
        string val = str_trim(line.substr(pos1 + 1, pos2 - pos1 - 1));

        if (key == "DataAngleRange") {
            // both start and end value are given, separated with ','
            string::size_type pos = val.find_first_of(",");
            x_start = my_strtod(val.substr(0, pos));
        } 
        else if (key == "ScanStepSize") {
            x_step = my_strtod(val);
        } 
        else {
            p_blk->meta[key] = val;
        }
    }

    StepColumn *p_xcol = new StepColumn(x_start, x_step);
    p_xcol->name = "data angle";

    VecColumn *p_ycol = new VecColumn;
    p_ycol->name = "raw scan";

    // read data
    string line;
    while (getline(f, line)) {
        bool has_slash = false;
        for (string::iterator i = line.begin(); i != line.end(); i++) {
            if (*i == ',') 
                *i = ' ';
            else if (*i == '/')
                has_slash = true;
            // format checking: only space and digit allowed
            else if (!isdigit(*i) && !isspace(*i)) 
                throw XY_Error("unexpected char when reading data");
        }

        istringstream ss(line);
        double d;
        while (ss >> d) 
            p_ycol->add_val(d);
    
        if (has_slash)
            break;
    }

    p_blk->set_xy_columns(p_xcol, p_ycol);
    blocks.push_back(p_blk);
}

} // end of namespace xylib


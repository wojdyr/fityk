// Philips UDF format - powder diffraction data from Philips diffractometers
// Licence: Lesser GNU Public License 2.1 (LGPL) 
// $Id$

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
    false,                       // whether has multi-blocks
    &UdfDataSet::ctor,
    &UdfDataSet::check
);


bool UdfDataSet::check (istream &f)
{
    string head = read_string(f, 11);
    return head == "SampleIdent";
}

/*
Sample Fragment: ("#xxx": comments added by me; ...: omitted lines)

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
*/
void UdfDataSet::load_data(std::istream &f) 
{
    Block *blk = new Block;

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
            blk->meta[key] = val;
        }
    }

    StepColumn *xcol = new StepColumn(x_start, x_step);
    blk->add_column(xcol, "data angle");

    VecColumn *ycol = new VecColumn;
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
                throw FormatError("unexpected char when reading data");
        }

        istringstream ss(line);
        double d;
        while (ss >> d) 
            ycol->add_val(d);
    
        if (has_slash)
            break;
    }
    blk->add_column(ycol, "raw scan");
    blocks.push_back(blk);
}

} // namespace xylib


// Implementation of class BruckerV23RawDataSet for reading meta-data and 
// xy-data from Siemens/Bruker Diffrac-AT Raw Format v2/3
// Licence: Lesser GNU Public License 2.1 (LGPL) 
// ds_brucker_raw_v23.cpp $

/*
FORMAT DESCRIPTION:
====================

Siemens/Bruker Diffrac-AT Raw Format version 2/3, Data format used in 
Siemens/Brucker X-ray diffractors.

///////////////////////////////////////////////////////////////////////////////
    * Name in progam:   ds_brucker_raw_v23, 
    * Extension name:   raw
    * Binary/Text:      binary
    * Multi-ranged:     Y

///////////////////////////////////////////////////////////////////////////////
    * Format details:   
It is of the same type in the data organizaton with the v1 format, except that 
the fields have different meanings and offsets. See ds_brucker_raw_v1.cpp for 
reference.

///////////////////////////////////////////////////////////////////////////////
    * Implementation Ref: See ds_brucker_raw_v1.cpp
*/


#include "ds_brucker_raw_v23.h"
#include "util.h"

using namespace std;
using namespace xylib::util;

namespace xylib {

const FormatInfo BruckerV23RawDataSet::fmt_info(
    FT_BR_RAW23,
    "diffracat_v2v3_raw",
    "Siemens/Bruker Diffrac-AT Raw File v2/v3",
    vector<string>(1, "raw"),
    true,                       // whether binary
    true                        // whether multi-ranged
);


bool BruckerV23RawDataSet::check(istream &f)
{
    // the first 4 letters must be "RAW2"
    f.clear();
    string head = read_string(f, 4);
    if(f.rdstate() & ios::failbit) {
        return false;
    }

    f.seekg(0);
    return ("RAW2" == head);
}


void BruckerV23RawDataSet::load_data(std::istream &f) 
{
    if (!check(f)) {
        throw XY_Error("file is not the expected " + get_filetype() + " format");
    }
    clear();

    f.ignore(4);
    unsigned range_cnt = read_uint16_le(f);

    // add file-scope meta-info
    f.ignore(162);
    add_meta("DATE_TIME_MEASURE", read_string(f, 20));
    add_meta("CEMICAL SYMBOL FOR TUBE ANODE", read_string(f, 2));
    add_meta("LAMDA1", S(read_flt_le(f)));
    add_meta("LAMDA2", S(read_flt_le(f)));
    add_meta("INTENSITY_RATIO", S(read_flt_le(f)));
    f.ignore(8);
    add_meta("TOTAL_SAMPLE_RUNTIME_IN_SEC", S(read_flt_le(f)));

    f.ignore(42);   // move ptr to the start of 1st range
    for (unsigned cur_range = 0; cur_range < range_cnt; ++cur_range) {
        Range* p_rg = new Range;
        my_assert(p_rg != NULL, "no memory to allocate");

        // add the range-scope meta-info
        unsigned cur_header_len = read_uint16_le(f);
        my_assert (cur_header_len > 48, "range header too short");

        unsigned cur_range_steps = read_uint16_le(f);
        f.ignore(4);
        p_rg->add_meta("SEC_PER_STEP", S(read_flt_le(f)));
        
        float x_step = read_flt_le(f);
        float x_start = read_flt_le(f);
        StepColumn *p_xcol = new StepColumn(x_start, x_step);
        my_assert(p_xcol != NULL, "no memory to allocate");

        f.ignore(26);
        p_rg->add_meta("TEMP_IN_K", S(read_uint16_le(f)));

        f.ignore(cur_header_len - 48);  // move ptr to the data_start
        VecColumn *p_ycol = new VecColumn;
        my_assert(p_ycol != NULL, "no memory to allocate");
        for(unsigned i = 0; i < cur_range_steps; ++i) {
            float y = read_flt_le(f);
            p_ycol->add_val(y);
        }
        p_rg->add_column(p_xcol, Range::CT_X);
        p_rg->add_column(p_ycol, Range::CT_Y);
        
        ranges.push_back(p_rg);
    }
}

} // end of namespace xylib


// Implementation of class BruckerV1RawDataSet for reading meta-data and 
// xy-data from Siemens/Bruker Diffrac-AT Raw Format version 1 format files
// Licence: Lesser GNU Public License 2.1 (LGPL) 
// $Id: ds_brucker_raw_v1.cpp $

/*

FORMAT DESCRIPTION:
====================

Siemens/Bruker Diffrac-AT Raw Format version 1, data format used in 
Siemens/Brucker X-ray diffractors.

///////////////////////////////////////////////////////////////////////////////
    * Name in progam:   ds_brucker_raw_v1
    * Extension name:   raw
    * Binary/Text:      binary
    * Multi-ranged:     Y
    
///////////////////////////////////////////////////////////////////////////////
    * Format details: 
direct access, binary raw format without any record deliminators. 
All data are stored in records, and each record has a fixed length. Note that 
in one record, the data is stored as little-endian (see "endian" in wikipedia
www.wikipedia.org for details), so on the platform other than little-endian
(e.g. PDP and Sun SPARC), the byte-order needs be exchanged.

It may contain multiple groups/ranges in one file.
There is a common header in the head of the file, with fixed length fields 
indicating file-scope meta-info whose actrual meaning can be found in the 
format specification.Each range has its onw range-scope meta-info at the 
beginning, followed by the X-Y data.

///////////////////////////////////////////////////////////////////////////////
    * Implementation Ref of xylib: 
mainly based on the file format specification: some scanned pages from a book, 
sent to me by Marcin Wojdyr (wojdyr@gmail.com). 
The title of these pages are "Appendix B: DIFFRAC-AT Raw Data File Format". 
In these pages, there are some tables listing every field of the DIFFRAC-AT v1 
and v2/v3 formats; and at the end, a sample FORTRAN program is given.

*/

#include "ds_brucker_raw_v1.h"

using namespace std;
using namespace xylib::util;

namespace xylib {

const FormatInfo BruckerV1RawDataSet::fmt_info(
    FT_BR_RAW1,
    "diffracat_v1_raw",
    "Siemens/Bruker Diffrac-AT Raw Format v1",
    vector<string>(1, "raw"),
    true,                       // whether binary
    true                        // whether multi-ranged
);


bool BruckerV1RawDataSet::is_filetype() const
{
    return check(*p_ifs);
}


bool BruckerV1RawDataSet::check(ifstream &f)
{
    // the first 3 letters must be "RAW"
    f.clear();
    string head = read_string(f, 0, 3);
    if(f.rdstate() & ios::failbit) {
        throw XY_Error("error when reading file head");
    }

    return ("RAW" == head);
}


void BruckerV1RawDataSet::load_data() 
{
    // no file-scope meta-info in this format
    init();
    ifstream &f = *p_ifs;

    unsigned max_range_idx = read_uint32_le(f, 152);
    
    unsigned cur_range_offset = 0;
    unsigned cur_range_idx = 0;
    while (cur_range_idx <= max_range_idx) {
        unsigned cur_range_steps = read_uint32_le(f, cur_range_offset + 4);
        float x_start = read_flt_le(f, cur_range_offset + 24);
        float x_step = read_flt_le(f, cur_range_offset + 12);

        FixedStepRange* p_rg = new FixedStepRange(x_start, x_step);

        // add the range-scope meta-info
        p_rg->add_meta("MEASUREMENT_TIME_PER_STEP", S(read_flt_le(f, cur_range_offset + 8)));
        p_rg->add_meta("SCAN_MODE", S(read_uint32_le(f, cur_range_offset + 16)));
        p_rg->add_meta("THETA_START",
            my_flt_to_string(read_flt_le(f, cur_range_offset + 28), -1e6));
        p_rg->add_meta("KHI_START", 
            my_flt_to_string(read_flt_le(f, cur_range_offset + 32), -1e6));
        p_rg->add_meta("PHI_START", 
            my_flt_to_string(read_flt_le(f, cur_range_offset + 36), -1e6));
        p_rg->add_meta("SAMPLE_NAME", read_string(f, 40, 32));
        p_rg->add_meta("K_ALPHA1", S(read_flt_le(f, 72)));
        p_rg->add_meta("K_ALPHA2", S(read_flt_le(f, 76)));

        for(unsigned i = 0; i < cur_range_steps; ++i) {
            float y = read_flt_le(f, cur_range_offset + 156 + i * 4);
            p_rg->add_y(y);
        }
        ranges.push_back(p_rg);

        ++cur_range_idx;
        cur_range_offset += (39 + cur_range_steps) * 4;
    }
}

} // end of namespace xylib


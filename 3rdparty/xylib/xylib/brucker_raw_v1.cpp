// Implementation of class BruckerV1RawDataSet for reading meta-info and 
// xy-data from Siemens/Bruker Diffrac-AT Raw Format version 1 format files
// Licence: Lesser GNU Public License 2.1 (LGPL) 
// $Id: ds_brucker_raw_v1.cpp $

/*

FORMAT DESCRIPTION:
====================

Siemens/Bruker Diffrac-AT Raw Format version 1, data format used in 
Siemens/Brucker X-ray diffractors.

///////////////////////////////////////////////////////////////////////////////
    * Name in progam:   diffracat_v1_raw
    * Extension name:   raw
    * Binary/Text:      binary
    * Multi-blocks:     Y
    
///////////////////////////////////////////////////////////////////////////////
    * Format details: 
direct access, binary raw format without any record deliminators. 
All data are stored in records, and each record has a fixed length. Note that 
in one record, the data is stored as little-endian (see "endian" in wikipedia
www.wikipedia.org for details), so on the platform other than little-endian
(e.g. PDP and Sun SPARC), the byte-order needs be exchanged.

It may contain multiple blocks/ranges in one file.
There is a common header in the head of the file, with fixed length fields 
indicating file-scope meta-info whose actrual meaning can be found in the 
format specification.Each block has its onw block-scope meta-info at the 
beginning, followed by the Y data.

///////////////////////////////////////////////////////////////////////////////
    * Implementation Ref of xylib: 
mainly based on the file format specification: some scanned pages from a book, 
sent to me by Marcin Wojdyr (wojdyr@gmail.com). 
The title of these pages are "Appendix B: DIFFRAC-AT Raw Data File Format". 
In these pages, there are some tables listing every field of the DIFFRAC-AT v1 
and v2/v3 formats; and at the end, a sample FORTRAN program is given.

*/

#include "brucker_raw_v1.h"
#include "util.h"

using namespace std;
using namespace xylib::util;

namespace xylib {

const FormatInfo BruckerV1RawDataSet::fmt_info(
    "diffracat_v1_raw",
    "Siemens/Bruker Diffrac-AT Raw Format v1",
    vector<string>(1, "raw"),
    true,                       // whether binary
    true,                       // whether has multi-blocks
    &BruckerV1RawDataSet::check
);


bool BruckerV1RawDataSet::check(istream &f)
{
    string head = read_string(f, 4);
    return head == "RAW ";
}


void BruckerV1RawDataSet::load_data(std::istream &f) 
{
    f.ignore(4);
    unsigned following_range = 1;
    
    while (following_range > 0) {
        Block* p_blk = new Block;
    
        unsigned cur_range_steps = read_uint32_le(f);  
        // early DIFFRAC-AT raw data files didn't repeat the "RAW " 
        // on additional ranges
        // (and if it's the first block, 4 bytes from file were already read)
        if (!blocks.empty()) {
            char rawstr[4] = {'R', 'A', 'W', ' '};
            le_to_host_4(rawstr);
            if (cur_range_steps == *reinterpret_cast<uint32_t*>(rawstr))
                cur_range_steps = read_uint32_le(f);
        }
        
        p_blk->meta["MEASUREMENT_TIME_PER_STEP"] = S(read_flt_le(f));
        float x_step = read_flt_le(f); 
        p_blk->meta["SCAN_MODE"] = S(read_uint32_le(f));
        f.ignore(4); 
        float x_start = read_flt_le(f);

        StepColumn *p_xcol = new StepColumn(x_start, x_step);
        
        float t = read_flt_le(f);
        if (-1e6 != t)
            p_blk->meta["THETA_START"] = S(t);
            
        t = read_flt_le(f);
        if (-1e6 != t)
            p_blk->meta["KHI_START"] = S(t);
            
        t = read_flt_le(f);
        if (-1e6 != t)
            p_blk->meta["PHI_START"], S(t);

        p_blk->meta["SAMPLE_NAME"] = read_string(f, 32);
        p_blk->meta["K_ALPHA1"] = S(read_flt_le(f));
        p_blk->meta["K_ALPHA2"] = S(read_flt_le(f));

        f.ignore(72);   // unused fields
        following_range = read_uint32_le(f);
        
        VecColumn *p_ycol = new VecColumn();
        
        for(unsigned i = 0; i < cur_range_steps; ++i) {
            float y = read_flt_le(f);
            p_ycol->add_val(y);
        }
        p_blk->set_xy_columns(p_xcol, p_ycol);

        blocks.push_back(p_blk);
    } 
}

} // end of namespace xylib


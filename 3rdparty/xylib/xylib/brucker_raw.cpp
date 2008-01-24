// Siemens/Bruker Diffrac-AT Raw Format version 1/2/3
// Licence: Lesser GNU Public License 2.1 (LGPL) 
// $Id$

#include "brucker_raw.h"
#include "util.h"

using namespace std;
using namespace xylib::util;

namespace xylib {

const FormatInfo BruckerRawDataSet::fmt_info(
    "bruker_raw",
    "Siemens/Bruker RAW ver. 1/2/3",
    vector<string>(1, "raw"),
    true,                       // whether binary
    true,                       // whether has multi-blocks
    &BruckerRawDataSet::ctor,
    &BruckerRawDataSet::check
);


bool BruckerRawDataSet::check(istream &f)
{
    string head = read_string(f, 4);
    return head == "RAW " || head == "RAW2";
}


void BruckerRawDataSet::load_data(std::istream &f) 
{
    string head = read_string(f, 4);
    format_assert(head == "RAW " || head == "RAW2"); 
    if (head[3] == ' ')
        load_version1(f);
    else
        load_version2(f);
}

void BruckerRawDataSet::load_version1(std::istream &f) 
{
    unsigned following_range = 1;
    
    while (following_range > 0) {
        Block* blk = new Block;
    
        unsigned cur_range_steps = read_uint32_le(f);  
        // early DIFFRAC-AT raw data files didn't repeat the "RAW " 
        // on additional ranges
        // (and if it's the first block, 4 bytes from file were already read)
        if (!blocks.empty()) {
            istringstream raw_stream("RAW ");
            unsigned raw_int = read_uint32_le(raw_stream);
            if (cur_range_steps == raw_int)
                cur_range_steps = read_uint32_le(f);
        }
        
        blk->meta["MEASUREMENT_TIME_PER_STEP"] = S(read_flt_le(f));
        float x_step = read_flt_le(f); 
        blk->meta["SCAN_MODE"] = S(read_uint32_le(f));
        f.ignore(4); 
        float x_start = read_flt_le(f);

        StepColumn *xcol = new StepColumn(x_start, x_step);
        
        float t = read_flt_le(f);
        if (-1e6 != t)
            blk->meta["THETA_START"] = S(t);
            
        t = read_flt_le(f);
        if (-1e6 != t)
            blk->meta["KHI_START"] = S(t);
            
        t = read_flt_le(f);
        if (-1e6 != t)
            blk->meta["PHI_START"], S(t);

        blk->meta["SAMPLE_NAME"] = read_string(f, 32);
        blk->meta["K_ALPHA1"] = S(read_flt_le(f));
        blk->meta["K_ALPHA2"] = S(read_flt_le(f));

        f.ignore(72);   // unused fields
        following_range = read_uint32_le(f);
        
        VecColumn *ycol = new VecColumn();
        
        for(unsigned i = 0; i < cur_range_steps; ++i) {
            float y = read_flt_le(f);
            ycol->add_val(y);
        }
        blk->set_xy_columns(xcol, ycol);

        blocks.push_back(blk);
    } 
}

void BruckerRawDataSet::load_version2(std::istream &f) 
{
    unsigned range_cnt = read_uint16_le(f);

    // add file-scope meta-info
    f.ignore(162);
    meta["DATE_TIME_MEASURE"] = read_string(f, 20);
    meta["CEMICAL SYMBOL FOR TUBE ANODE"] = read_string(f, 2);
    meta["LAMDA1"] = S(read_flt_le(f));
    meta["LAMDA2"] = S(read_flt_le(f));
    meta["INTENSITY_RATIO"] = S(read_flt_le(f));
    f.ignore(8);
    meta["TOTAL_SAMPLE_RUNTIME_IN_SEC"] = S(read_flt_le(f));

    f.ignore(42);   // move ptr to the start of 1st block
    for (unsigned cur_range = 0; cur_range < range_cnt; ++cur_range) {
        Block* blk = new Block;

        // add the block-scope meta-info
        unsigned cur_header_len = read_uint16_le(f);
        format_assert (cur_header_len > 48);

        unsigned cur_range_steps = read_uint16_le(f);
        f.ignore(4);
        blk->meta["SEC_PER_STEP"] = S(read_flt_le(f));
        
        float x_step = read_flt_le(f);
        float x_start = read_flt_le(f);
        StepColumn *xcol = new StepColumn(x_start, x_step);

        f.ignore(26);
        blk->meta["TEMP_IN_K"] = S(read_uint16_le(f));

        f.ignore(cur_header_len - 48);  // move ptr to the data_start
        VecColumn *ycol = new VecColumn;
        
        for(unsigned i = 0; i < cur_range_steps; ++i) {
            float y = read_flt_le(f);
            ycol->add_val(y);
        }
        blk->set_xy_columns(xcol, ycol);
        blocks.push_back(blk);
    }
}

} // end of namespace xylib


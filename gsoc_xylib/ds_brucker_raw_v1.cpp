// Implementation of class BruckerV1RawDataSet for reading meta-data and xy-data from 
// Licence: GNU General Public License version 2
// $Id: __MY_FILE_ID__ $

#include "ds_brucker_raw_v1.h"
#include "common.h"

using namespace std;
using namespace boost;
using namespace xylib;
using namespace xylib::util;

namespace xylib {

bool BruckerV1RawDataSet::is_filetype() const
{
    // the first 3 letters must be "RAW"
    ifstream &f = *p_ifs;

    f.clear();
    string head = read_string(f, 0, 3);
    if(f.rdstate() & ios::failbit) {
        throw XY_Error("error when reading file head");
    }

    return ("RAW" == head) ? true : false;
}


// if user doesn't want the meta-info, they can simply ignore it to improve the performance
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


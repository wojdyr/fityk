// Implementation of class BruckerV23RawDataSet for reading meta-data and xy-data from 
// Licence: GNU General Public License version 2
// $Id: BruckerV23RawDataSet.h $

#include "xylib.h"
#include "common.h"

using namespace std;
using namespace xylib;
using namespace xylib::util;
using namespace boost;

namespace xylib {

bool BruckerV23RawDataSet::is_filetype() const
{
    // the first 4 letters must be "RAW2"
    ifstream &f = *p_ifs;

    f.clear();
    string head = read_string(f, 0, 4);
    if(f.rdstate() & ios::failbit) {
        throw XY_Error("error when reading file head");
    }

    return ("RAW2" == head) ? true : false;
}


void BruckerV23RawDataSet::load_data() 
{
    init();
    ifstream &f = *p_ifs;

    // add file-scope meta-info
    add_meta("DATE_TIME_MEASURE", read_string(f, 168, 20));
    add_meta("LAMDA1", S(read_flt_le(f, 190)));
    add_meta("LAMDA2", S(read_flt_le(f, 194)));
    add_meta("INTENSITY_RATIO", S(read_flt_le(f, 198)));
    add_meta("TOTAL_SAMPLE_RUNTIME_IN_SEC", S(read_flt_le(f, 210)));

    uint16_t range_cnt = read_uint16_le(f, 4);

    uint16_t cur_range_offset = 256;    // 1st range offset
    for (uint16_t cur_range = 0; cur_range < range_cnt; ++cur_range) {
        uint16_t cur_header_len = read_uint16_le(f, cur_range_offset);
        float x_start = read_flt_le(f, cur_range_offset + 16);
        float x_step = read_flt_le(f, cur_range_offset + 12);
        uint16_t cur_range_steps = read_uint16_le(f, cur_range_offset + 2);

        FixedStepRange* p_rg = new FixedStepRange(x_start, x_step);
        // add the range-scope meta-info here
        p_rg->add_meta("SEC_PER_STEP", S(read_flt_le(f, cur_range_offset + 8)));
        p_rg->add_meta("TEMP_IN_K", S(read_uint16_le(f, cur_range_offset + 46)));

        for(uint16_t i = 0; i < cur_range_steps; ++i) {
            float y = read_flt_le(f, cur_range_offset + cur_header_len + i * 4);
            p_rg->add_y(y);
        }
        ranges.push_back(p_rg);

        cur_range_offset += cur_header_len + cur_range_steps * 4;
    }
}

} // end of namespace xylib


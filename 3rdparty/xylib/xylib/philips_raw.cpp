// Implementation of class PhilipsRawDataSet for reading meta-info and 
// xy-data from Philips RD raw scan format V3/V5
// Licence: Lesser GNU Public License 2.1 (LGPL) 
// ds_philips_rd.cpp $

/*

FORMAT DESCRIPTION:
====================

Philips RD raw scan format V3/V5
".rd" files are V3, ".sd" files are V5.

///////////////////////////////////////////////////////////////////////////////
    * Name in progam:   philips_rd
    * Extension name:   rd, sd
    * Binary/Text:      binary
    * Multi-blocks:     N

///////////////////////////////////////////////////////////////////////////////
    * Format details: 
direct access, binary raw format without any record deliminators. 
All data are stored in records, and each record has a fixed offset and length. 
Note that in one record, data is stored as little-endian (see "endian" in 
wikipedia www.wikipedia.org for details), so on the platform other than 
little-endian (e.g. PDP and Sun SPARC), the byte-order needs be exchanged.

It contains ONLY 1 blocks/ranges in one file.

///////////////////////////////////////////////////////////////////////////////
    * Implementation Ref of xylib: 
mainly based on the file format specification sent to us by Martijn Fransen
<martijn.fransen@panalytical.com>. 

NOTE: V5 format has not been tested, because we cannot get sample files.

*/

#include "philips_raw.h"
#include "util.h"

using namespace std;
using namespace xylib::util;

namespace xylib {

const static string exts[4] = { "rd", "sd" };

const FormatInfo PhilipsRawDataSet::fmt_info(
    "philips_rd",
    "Philips RD raw scan format V3",
    vector<string>(exts, exts + sizeof(exts) / sizeof(string)),
    true,                        // whether binary
    false                        // whether has multi-blocks
);


bool PhilipsRawDataSet::check(istream &f)
{
    string head = read_string(f, 4);
    return head == "V3RD" || head == "V5RD";
}


void PhilipsRawDataSet::load_data(std::istream &f) 
{
    // mappers, translate the numbers to human-readable strings
    const static string diffractor_types[6] = {"PW1800", "PW1710 based system", "PW1840",
        "PW3710 based system", "Undefined", "X'Pert MPD"};
    const static string anode_materials[6] = { "Cu", "Mo", "Fe", "Cr", "Other"};
    const static string focus_types[4] = {"BF", "NF", "FF", "LFF"};

    string version = read_string(f, 2);    
    format_assert(version == "V3" || version == "V5");

    f.ignore(82);
    int dt_idx = static_cast<int>(read_char(f));
    if (0 <= dt_idx && dt_idx <= 5) {
        meta["diffractor type"]  = diffractor_types[dt_idx];
    }

    int anode_idx = static_cast<int>(read_char(f));
    if (0 <= anode_idx && anode_idx <= 5) {
        meta["tube anode material"] = anode_materials[anode_idx];
    }

    int ft_idx = static_cast<int>(read_char(f));
    if (0 <= ft_idx && ft_idx <= 3) {
        meta["focus type of x-ray tube"] = focus_types[ft_idx];
    }

    f.ignore(138 - 84 - 3);
    meta["name of the file"] = read_string(f, 8);
    meta["sample identification"] = read_string(f, 20);

    f.ignore(214 - 138 - 8 - 20);
    double x_step = read_dbl_le(f);
    double x_start = read_dbl_le(f);
    double x_end = read_dbl_le(f);
    unsigned pt_cnt = static_cast<unsigned>((x_end - x_start) / x_step + 1);

    StepColumn *p_xcol = new StepColumn(x_start, x_step, pt_cnt);
    
    // read in y data
    if ("V3" == version) {
        f.ignore(250 - 214 - 8*3);
    } else {
        f.ignore(810 - 214 - 8*3);
    }
    
    VecColumn *p_ycol = new VecColumn;
    for (unsigned i = 0; i < pt_cnt; ++i) {
        int packed_y = read_uint16_le(f);
        double y = packed_y * packed_y / 100;
        p_ycol->add_val(y);
    }

    Block *p_blk = new Block;
    p_blk->set_xy_columns(p_xcol, p_ycol);

    blocks.push_back(p_blk);
}

} // end of namespace xylib


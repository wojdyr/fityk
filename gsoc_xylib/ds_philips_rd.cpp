// Implementation of class PhilipsRdDataSet for reading meta-data and 
// xy-data from Philips RD raw scan format V3
// Licence: Lesser GNU Public License 2.1 (LGPL) 
// ds_philips_rd.cpp $

/*

FORMAT DESCRIPTION:
====================

Philips RD raw scan format V3

///////////////////////////////////////////////////////////////////////////////
    * Name in progam:   philips_rd
    * Extension name:   rd
    * Binary/Text:      binary
    * Multi-ranged:     N

///////////////////////////////////////////////////////////////////////////////
    * Format details: 
direct access, binary raw format without any record deliminators. 
All data are stored in records, and each record has a fixed offset and length. 
Note that in one record, data is stored as little-endian (see "endian" in 
wikipedia www.wikipedia.org for details), so on the platform other than 
little-endian (e.g. PDP and Sun SPARC), the byte-order needs be exchanged.

It contains ONLY 1 group/range in one file.

///////////////////////////////////////////////////////////////////////////////
    * Implementation Ref of xylib: 
mainly based on the file format specification sent to us by Martijn Fransen
<martijn.fransen@panalytical.com>. 

*/

#include "ds_philips_rd.h"
#include "util.h"

using namespace std;
using namespace xylib::util;

namespace xylib {

const FormatInfo PhilipsRdDataSet::fmt_info(
    FT_PHILIPS_RD,
    "philips_rd",
    "Philips RD raw scan format V3",
    vector<string>(1, "rd"),
    true,                        // whether binary
    false                        // whether multi-ranged
);


bool PhilipsRdDataSet::check(istream &f)
{
    // the first 4 letters must be "V3RD"
    f.clear();
    string head = read_string(f, 4);
    if(f.rdstate() & ios::failbit) {
        return false;
    }

    f.seekg(0);
    return ("V3RD" == head);
}


void PhilipsRdDataSet::load_data(std::istream &f) 
{
    if (!check(f)) {
        throw XY_Error("file is not the expected " + get_filetype() + " format");
    }
    clear();

    // mappers, translate the numbers to human-readable strings
    const static string diffractor_types[6] = {"PW1800", "PW1710 based system", "PW1840",
        "PW3710 based system", "Undefined", "X'Pert MPD"};
    const static string anode_materials[6] = { "Cu", "Mo", "Fe", "Cr", "Other"};
    const static string focus_types[4] = {"BF", "NF", "FF", "LFF"};

    f.ignore(84);
    int dt_idx = static_cast<int>(read_char(f));
    if (0 <= dt_idx && dt_idx <= 5) {
        add_meta("diffractor type", diffractor_types[dt_idx]);
    }

    int anode_idx = static_cast<int>(read_char(f));
    if (0 <= anode_idx && anode_idx <= 5) {
        add_meta("tube anode material", anode_materials[anode_idx]);
    }

    int ft_idx = static_cast<int>(read_char(f));
    if (0 <= ft_idx && ft_idx <= 3) {
        add_meta("focus type of x-ray tube", focus_types[ft_idx]);
    }

    f.ignore(138 - 84 - 3);
    add_meta("name of the file", read_string(f, 8));
    add_meta("sample identification", read_string(f, 20));

    f.ignore(214 - 138 - 8 - 20);
    double x_step = read_dbl_le(f);
    double x_start = read_dbl_le(f);
    double x_end = read_dbl_le(f);
    unsigned pt_cnt = static_cast<unsigned>((x_end - x_start) / x_step);

    StepColumn *p_xcol = new StepColumn(x_start, x_step, pt_cnt);
    
    // read in y data
    f.ignore(250 - 214 - 8*3);
    VecColumn *p_ycol = new VecColumn;
    for (unsigned i = 0; i < pt_cnt; ++i) {
        double packed_y = read_uint16_le(f);
        double y = packed_y * packed_y * 0.01;
        p_ycol->add_val(y);
    }

    Range *p_rg = new Range;
    p_rg->add_column(p_xcol, Range::CT_X);
    p_rg->add_column(p_ycol, Range::CT_Y);

    ranges.push_back(p_rg);
}

} // end of namespace xylib


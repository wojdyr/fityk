// one of Canberra MCA output formats
// Licence: Lesser GNU Public License 2.1 (LGPL)
// $Id$

#include <cmath>
#include <boost/cstdint.hpp>
#include "canberra_mca.h"
#include "util.h"

using namespace std;
using namespace xylib::util;
using boost::uint16_t;
using boost::uint32_t;


namespace xylib {


const FormatInfo CanberraMcaDataSet::fmt_info(
    "canberra_mca",
    "Canberra MCA",
    vector_string("mca"),
    true,                       // whether binary
    false,                      // whether has multi-blocks
    &CanberraMcaDataSet::ctor,
    &CanberraMcaDataSet::check
);

bool CanberraMcaDataSet::check(istream &f)
{
    const int file_size = 2*512+2048*4;
    char *all_data = new char[file_size];
    f.read(all_data, file_size);
    uint16_t word_at_0 = *reinterpret_cast<uint16_t*>(all_data + 0);
    le_to_host(&word_at_0, 2);
    uint16_t word_at_34 = *reinterpret_cast<uint16_t*>(all_data + 34);
    le_to_host(&word_at_34, 2);
    uint16_t word_at_36 = *reinterpret_cast<uint16_t*>(all_data + 36);
    le_to_host(&word_at_36, 2);
    uint16_t word_at_38 = *reinterpret_cast<uint16_t*>(all_data + 38);
    le_to_host(&word_at_38, 2);
    delete [] all_data;
    return f.gcount() == file_size
           && word_at_0 == 0
           && word_at_34 == 4
           && word_at_36 == 2048
           && word_at_38 == 1;
}

void CanberraMcaDataSet::load_data(std::istream &f)
{
    const int file_size = 2*512+2048*4;
    char *all_data = new char[file_size];
    f.read(all_data, file_size);
    if (f.gcount() != file_size) {
        delete [] all_data;
        throw FormatError("Unexpected end of file.");
    }

    double energy_offset = pdp11_f (all_data + 108);
    double energy_slope = pdp11_f (all_data + 112);
    double energy_quadr = pdp11_f (all_data + 116);

    Block* blk = new Block;

    Column *xcol = NULL;
    if (energy_quadr) {
        VecColumn *vc = new VecColumn;
        for (int i = 1; i <= 2048; i++) {
            //FIXME should it be from 1 ?
            // perhaps from 0 to 2047, description was not clear.
            double x = energy_offset + energy_slope * i + energy_quadr * i * i;
            vc->add_val(x);
        }
        xcol = vc;
    }
    else {
        xcol = new StepColumn(energy_offset+energy_slope, energy_slope);
    }
    blk->add_column(xcol);

    VecColumn *ycol = new VecColumn;
    uint16_t data_offset = *reinterpret_cast<uint16_t*>(all_data+24);
    le_to_host(&data_offset, 2);
    uint32_t* pw = reinterpret_cast<uint32_t*>(all_data + data_offset);
    for (int i = 1; i <= 2048; i++) {
        double y = *pw;
        pw++;
        le_to_host(&y, 4);
        ycol->add_val(y);
    }
    blk->add_column(ycol);

    blocks.push_back(blk);
}

// function that converts:
//   single precision 32-bit floating point DEC PDP-11 format
//   to double
double CanberraMcaDataSet::pdp11_f (char* p)

{
    int sign = (p[1] & 0x80) == 0 ? 1 : -1;
    int unbiased = ((p[1] & 0x7F) << 1) + ((p[0] & 0x80) >> 7) - 128;
    if (unbiased == -128)
        return 0;
    double h = (p[2] & 0x7F) / 256. / 256. / 256.
               + (p[3] & 0x7F) / 256. / 256.
               + (128 + (p[0] & 0x7F)) / 256.;
    return sign * h * pow(2., unbiased);
}



} // namespace xylib


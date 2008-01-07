// Princeton Instruments WinSpec SPE Format
// Licence: Lesser GNU Public License 2.1 (LGPL) 
// $Id: ds_winspec_spe.cpp $


// Princeton Instruments WinSpec SPE format. One kind of spectroscopy formats
// used by Princeton Instruments (PI). The Official programs to deal with this
// format is WinView/WinSpec.
// 
// According to the format specification, SPE format has several versions 
// (v1.43, v1.6 and the newest v2.25). But we need not implement every version 
// of it, because it's backward compatible.
//     
// Data begins after a 4100-byte header.
// 
// Based on the file format specification sent us by David Hovis <dbh6@case.edu>
// (the documents came with his equipment) and a SPE reading program written by
// Pablo Bianucci <pbian@physics.utexas.edu>.

#include "winspec_spe.h"
#include "util.h"
#include <cmath>

using namespace std;
using namespace xylib::util;

namespace xylib {

const FormatInfo WinspecSpeDataSet::fmt_info(
    "spe",
    "Princeton Instruments WinSpec SPE Format",
    vector<string>(1, "spe"),
    true,                       // whether binary
    true,                       // whether has multi-blocks
    &WinspecSpeDataSet::ctor,
    &WinspecSpeDataSet::check
);

bool WinspecSpeDataSet::check(istream &f) {
    // make sure file size > 4100
    f.seekg(-1, ios_base::end);
    long file_sz = f.tellg();
    if (file_sz <= 4100) {
        return false;
    }

    // datatype field in header ONLY can be 0~3
    f.seekg(108);
    spe_dt data_type = static_cast<spe_dt>(read_uint16_le(f));
    if (data_type < SPE_DATA_FLOAT || data_type > SPE_DATA_UINT) 
        return false;

    return true;
}


void WinspecSpeDataSet::load_data(std::istream &f) 
{
    // only read necessary params from file header
    f.ignore(42);
    int xdim = read_uint16_le(f);
    f.ignore(64);
    spe_dt data_type = static_cast<spe_dt>(read_uint16_le(f));
    
    f.ignore(546);
    int ydim = read_uint16_le(f);
    f.ignore(788);
    size_t num_frames = read_uint32_le(f);
    
    spe_calib x_calib, y_calib;
    f.ignore(1550);     // move ptr to x_calib start
    read_calib(f, x_calib);
    read_calib(f, y_calib);

    int dim;
    spe_calib *calib;
    if (ydim == 1) {
        dim = xdim;
        calib = &x_calib;
    } else if (xdim == 1) {
        dim = ydim;
        calib = &y_calib;
    } else {
        throw XY_Error("xylib does not support 2-D images");
    }

    f.ignore(122);      // move ptr to frames-start
    for (unsigned frm = 0; frm < num_frames; ++frm) {
        Column *xcol = get_calib_column(calib, dim);

        VecColumn *ycol = new VecColumn;
        for (int i = 0; i < dim; ++i) {
            double y = 0;
            switch (data_type) {
                case SPE_DATA_FLOAT:
                    y = read_flt_le(f);
                    break;
                case SPE_DATA_LONG:
                    y = read_uint32_le(f);
                    break;
                case SPE_DATA_INT:
                    y = read_int16_le(f);
                    break;
                case SPE_DATA_UINT:
                    y = read_uint16_le(f);
                    break;
                default:
                    break;
            }
            ycol->add_val(y);
        }
        
        Block *blk = new Block;
        blk->set_xy_columns(xcol, ycol);
        blocks.push_back(blk);
    }
}


// internally-used helper functions
///////////////////////////////////////////////////////////////////////////////

Column* WinspecSpeDataSet::get_calib_column(const spe_calib *calib, int dim)
{
    my_assert(calib->polynom_order <= 6, "bad polynom header found");

    if (!calib->calib_valid)    //use idx as X instead
        return new StepColumn(0, 1); 
    else if (calib->polynom_order == 1) { // linear
        return new StepColumn(calib->polynom_coeff[0], 
                              calib->polynom_coeff[1]); 
    }
    else {
        VecColumn *xcol = new VecColumn;
        for (int i = 0; i < dim; ++i) {
            double x = 0;
            for (int j = 0; j <= calib->polynom_order; ++j) 
                x += calib->polynom_coeff[j] * pow(i + 1., double(j));
            xcol->add_val(x);
        }
        return xcol;
    }
}


// read some fields of calib. 'offset' is the offset of the structure in file
void WinspecSpeDataSet::read_calib(istream &f, spe_calib &calib)
{
    f.ignore(98);
    my_read(f, &calib.calib_valid, 1);

    f.ignore(2);
    my_read(f, &calib.polynom_order, 1);

    f.ignore(161);
    for (int i = 0; i < 6; ++i) {
        calib.polynom_coeff[i] = read_dbl_le(f);
    }

    f.ignore(178);  // skip all of the left fields in calib
}

} // end of namespace xylib



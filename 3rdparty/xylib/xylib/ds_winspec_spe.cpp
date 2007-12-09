// Implementation of class WinspecSpeDataSet for reading meta-info 
// and xy-data from WinSpec SPE Format
// Licence: Lesser GNU Public License 2.1 (LGPL) 
// $Id: ds_winspec_spe.cpp $

/*

FORMAT DESCRIPTION:
====================
Princeton Instruments WinSpec SPE format. One kind of spectroscopy formats
used by Princeton Instruments (PI). The Official programs to deal with this
format is WinView/WinSpec.

///////////////////////////////////////////////////////////////////////////////
    * Name in progam:   spe
    * Extension name:   spe
    * Binary/Text:      binary
    * Multi-blocks:     Y
    
///////////////////////////////////////////////////////////////////////////////
    * Format details: 
According to the format specification, SPE format has several versions (v1.43, 
v1.6 and the newest v2.25). But we need not implement every version of it, 
because it's back-ward compatible.
    
All data files must begin with a 4100-byte header. There are lots of fields 
stored in the header in a fixed offset. 
NOTE: there are 2 issues must be aware of when implementing this format.
1. Data are stored in Little-Endian, binary raw format. 
2. Its own types (SPE_DATA_LONG etc) have different lengths compared with those
types in C/C++

Data begins right after the header, with offset 4100.

///////////////////////////////////////////////////////////////////////////////
    * Implementation Ref of xylib: 
mainly based on the file format specification: A file format specification
sent us by David Hovis <dbh6@case.edu> (the documents came with his equipment) 
and a SPE reading program written by Pablo Bianucci <pbian@physics.utexas.edu>.

*/

#include "ds_winspec_spe.h"
#include "util.h"
#include <cmath>

using namespace std;
using namespace xylib::util;

namespace xylib {

const FormatInfo WinspecSpeDataSet::fmt_info(
    FT_SPE,
    "spe",
    "Princeton Instruments WinSpec SPE Format",
    vector<string>(1, "spe"),
    true,                       // whether binary
    true                        // whether has multi-blocks
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
    if (data_type < SPE_DATA_FLOAT || data_type > SPE_DATA_UINT) {
        return false;
    }

    // additional if-condition can be added

    f.seekg(0);
    return true;
}


void WinspecSpeDataSet::load_data(std::istream &f) 
{
    if (!check(f)) {
        throw XY_Error("file is not the expected " + get_filetype() + " format");
    }
    clear();

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
    if (1 == ydim) {
        dim = xdim;
        calib = &x_calib;
    } else if (1 == xdim) {
        dim = ydim;
        calib = &y_calib;
    } else {
        throw XY_Error("xylib does not support 2-D images");
    }

    f.ignore(122);      // move ptr to frames-start
    for (unsigned frm = 0; frm < num_frames; ++frm) {
        VecColumn *p_xcol = new VecColumn;
        if ((1 == calib->polynom_order) || (!calib->calib_valid)) {
            // it's linear, so step is fixed
            p_xcol->fixed_step = true;
        }

        VecColumn *p_ycol = new VecColumn;
        Block *p_blk = new Block;
        p_blk->add_column(p_xcol, Block::CT_X);
        p_blk->add_column(p_ycol, Block::CT_Y);
        
        for (int pt = 0; pt < dim; ++pt) {
            double x = idx_to_calib_val(pt, calib);
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

            p_xcol->add_val(x);
            p_ycol->add_val(x);
        }
        
        blocks.push_back(p_blk);
    }
}


// internally-used helper functions
///////////////////////////////////////////////////////////////////////////////

// get the calibration value of index 'idx'
double WinspecSpeDataSet::idx_to_calib_val(int idx, const spe_calib *calib)
{
    double re = 0;

    // Sanity checks
    if (!calib) {
        throw XY_Error("invalid calib structure");
    }
    
    my_assert(calib->polynom_order <= 6, "bad polynom header found");

    if (!calib->calib_valid) {
        return idx;        // if invalid, use idx as X instead
    }

    for (int i = 0; i <= calib->polynom_order; ++i) {
        re += calib->polynom_coeff[i] * pow(double(idx + 1), double(i));
    }

    return re;
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



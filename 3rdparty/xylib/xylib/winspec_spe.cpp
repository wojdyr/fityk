// Princeton Instruments WinSpec SPE Format - spectroscopy data
// Licence: Lesser GNU Public License 2.1 (LGPL) 
// $Id$

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

enum {
    SPE_HEADER_SIZE = 4100,   // fixed binary header size
};

/* datatypes of DATA point in spe_file */
enum spe_dt {
    SPE_DATA_FLOAT = 0,     // size 4
    SPE_DATA_LONG  = 1,     // size 4
    SPE_DATA_INT   = 2,     // size 2
    SPE_DATA_UINT  = 3,     // size 2
};

// Calibration structure in SPE format.
// NOTE: fields that we don't care have been commented out
struct spe_calib {
//    double offset;                // +0 offset for absolute data scaling
//    double factor;                // +8 factor for absolute data scaling 
//    char current_unit;            // +16 selected scaling unit 
//    char reserved1;               // +17 reserved 
//    char string[40];              // +18 special string for scaling 
//    char reserved2[40];           // +58 reserved
    char calib_valid;             // +98 flag of whether calibration is valid
//    char input_unit;              // +99 current input units for 
                                  // "calib-value" 
//    char polynom_unit;            // +100 linear UNIT and used 
                                  // in the "polynom-coeff" 
    char polynom_order;           // +101 ORDER of calibration POLYNOM 
//    char calib_count;             // +102 valid calibration data pairs 
//    double pixel_position[10];    // +103 pixel pos. of calibration data 
//    double calib_value[10];       // +183 calibration VALUE at above pos 
    double polynom_coeff[6];      // +263 polynom COEFFICIENTS 
//    double laser_position;        // +311 laser wavenumber for relativ WN 
//    char reserved3;               // +319 reserved 
//    unsigned char new_calib_flag; // +320 If set to 200, valid label below 
//    char calib_label[81];         // +321 Calibration label (NULL term'd) 
//    char expansion[87];           // +402 Calibration Expansion area 
};


bool WinspecSpeDataSet::check(istream &f) {
    // make sure file size > 4100 (data begins after a 4100-byte header)
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
        throw FormatError("xylib does not support 2-D images");
    }

    f.ignore(122);      // move ptr to frames-start
    for (unsigned frm = 0; frm < num_frames; ++frm) {
        Block *blk = new Block;
        Column *xcol = get_calib_column(calib, dim);
        blk->add_column(xcol);

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
        blk->add_column(ycol);
        
        blocks.push_back(blk);
    }
}


Column* WinspecSpeDataSet::get_calib_column(const spe_calib *calib, int dim)
{
    format_assert(calib->polynom_order <= 6, "bad polynom header");

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
    f.read(&calib.calib_valid, 1);

    f.ignore(2);
    f.read(&calib.polynom_order, 1);

    f.ignore(161);
    for (int i = 0; i < 6; ++i) 
        calib.polynom_coeff[i] = read_dbl_le(f);

    f.ignore(178);  // skip all of the left fields in calib
}

} // end of namespace xylib



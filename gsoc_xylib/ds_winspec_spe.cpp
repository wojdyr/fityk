// Implementation of class WinspecSpeDataSet for reading meta-data 
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
    * Extension name:   Princeton Instruments WinSpec SPE Format 
    * Binary/Text:      binary
    * Multi-ranged:     Y
    
///////////////////////////////////////////////////////////////////////////////
    * Format details: 
According to the format specification, SPE format has several versions (v1.43, 
v1.6 and the newest v2.25). But we need not implement every version of it, 
because it's back-ward compatible.
    
All data files must begin with a 4100 byte header. There are lots of fields 
stored in the header in a fixed offset. 
NOTE: there are 2 issues must be aware of when implementing this format.
1. Data are stored in Little-Endian, binary raw format. 
2. Its type SPE_DATA_LONG etc has a different length, compared with in C/C++

Data begins right after the header, with offset 4100.

///////////////////////////////////////////////////////////////////////////////
    * Implementation Ref of xylib: 
mainly based on the file format specification: A file format specification
sent us by David Hovis <dbh6@case.edu> (the documents came with his equipment) 
and a SPE reading program written by Pablo Bianucci <pbian@physics.utexas.edu>.

*/

#include "ds_winspec_spe.h"
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
    true                        // whether multi-ranged
);

bool WinspecSpeDataSet::is_filetype() const
{
    ifstream &f = *p_ifs;

    // make sure file size > 4100
    f.seekg(-1, ios_base::end);
    long file_sz = f.tellg();
    if (file_sz <= 4100) {
        return false;
    }

    // datatype field in header ONLY can be 0~3
    spe_dt data_type = static_cast<spe_dt>(read_uint16_le(f, 108));
    if (data_type < SPE_DATA_FLOAT || data_type > SPE_DATA_UINT) {
        return false;
    }

    // additional if-condition can be added

    return true;
}

void WinspecSpeDataSet::load_data() 
{
    init();
    ifstream &f = *p_ifs;

    // only get neccesary params from file header
    
    int xdim = read_uint16_le(f, 42);
    int ydim = read_uint16_le(f, 656);
    spe_dt data_type = static_cast<spe_dt>(read_uint16_le(f, 108));
    size_t num_frames = read_uint32_le(f, 1446);
    
    size_t dt_size = get_datatype_size(data_type);
    size_t frame_size = xdim * ydim * dt_size;    // in Byte

    spe_calib x_calib, y_calib;
    read_calib(x_calib, 3000);
    read_calib(y_calib, 3489);

    int dim;
    spe_calib *calib;
    if (1 == ydim) {
        dim = xdim;
        calib = &x_calib;
    } else if (1 == ydim) {
        dim = xdim;
        calib = &y_calib;
    } else {
        throw XY_Error("xylib does not support 2-D images");
    }

    for (unsigned frm = 0; frm < num_frames; ++frm) {
        Range *p_rg = new Range;
        size_t frm_off = SPE_HEADER_SIZE + frame_size * frm;
        
        for (int pt = 0; pt < dim; ++pt) {
            double x = idx_to_calib_val(pt, calib);
            double y = 0;
            switch (data_type) {
            case SPE_DATA_FLOAT:
                y = read_flt_le(f, frm_off + dt_size * pt);
                break;
            case SPE_DATA_LONG:
                y = read_uint32_le(f, frm_off + dt_size * pt);
                break;
            case SPE_DATA_INT:
                y = read_int16_le(f, frm_off + dt_size * pt);                    y = read_uint32_le(f, frm_off + dt_size * pt);
                break;
            case SPE_DATA_UINT:
                y = read_uint16_le(f, frm_off + dt_size * pt);
                break;
            default:
                break;
            }
            
            p_rg->add_pt(x, y);
        }
        
        ranges.push_back(p_rg);
    }
}


// internally-used helper functions
///////////////////////////////////////////////////////////////////////////////

// get the calibration value of index 'idx'
double WinspecSpeDataSet::idx_to_calib_val(int idx, const spe_calib *calib)
{
    double re = 0;

	// Sanity checks
	if (calib->polynom_order > 6) {
        throw XY_Error("bad polynom header found");
	}
	if (!calib->calib_valid) {
	    return idx;	    // if invalid, use idx as X instead
	}

	for (int i = 0; i <= calib->polynom_order; ++i) {
        re += calib->polynom_coeff[i] * pow(double(idx + 1), double(i));
	}

	return re;
}


// get the size of data types in SPEC files
size_t WinspecSpeDataSet::get_datatype_size(int dt)
{
    static size_t dt_size[] = {
        4,  // SPE_DATA_FLOAT 
        4,  // SPE_DATA_LONG  
        2,  // SPE_DATA_INT   
        2,  // SPE_DATA_UINT  
    };

	return dt_size[dt];
}

// read some fields of calib. 'offset' is the offset of the structure in file
void WinspecSpeDataSet::read_calib(spe_calib &calib, int offset)
{
    ifstream &f = *p_ifs;

    calib.calib_valid = (read_string(f, offset + 98, 1).c_str())[0];
    calib.polynom_order = (read_string(f, offset + 101, 1).c_str())[0];

    for (int i = 0; i < 6; ++i) {
        calib.polynom_coeff[i] = read_dbl_le(f, offset + 263 + 8 * i);
    }
}

} // end of namespace xylib



// Header of class WinspecSpeDataSet
// Licence: Lesser GNU Public License 2.1 (LGPL) 
// $Id: ds_winspec_spe.h $

#ifndef WINSPEC_SPE_DATASET_H
#define WINSPEC_SPE_DATASET_H
#include "xylib.h"
#include "util.h"

namespace xylib {

#define SPE_HEADER_SIZE 4100	// fixed binary header size

/* datatypes of DATA point in spe_file */
enum spe_dt {
    SPE_DATA_FLOAT = 0,     // size 4
    SPE_DATA_LONG  = 1,     // size 4
    SPE_DATA_INT   = 2,     // size 2
    SPE_DATA_UINT  = 3,     // size 2
};

struct spe_calib;

class WinspecSpeDataSet : public DataSet
{
public:
    WinspecSpeDataSet(const std::string &filename)
        : DataSet(filename, FT_SPE) 
    {}

    // implement the interfaces specified by DataSet
    bool is_filetype() const;
    void load_data();

    const static FormatInfo fmt_info;
    
protected:
    size_t get_datatype_size(int dt);
    double idx_to_calib_val(int idx, const spe_calib *calib);
    void read_calib(spe_calib &calib, int offset);
    
}; 


// used internally to parse the file structure
///////////////////////////////////////////////////////////////////////////////

/*
 * Calibration structure in SPE format.
 * NOTE: fields that we don't care have been commented out
 */
struct spe_calib {
/*
    double offset;        // +0 offset for absolute data scaling
    double factor;        // +8 factor for absolute data scaling 
    char current_unit;    // +16 selected scaling unit 
    char reserved1;       // +17 reserved 
    char string[40];      // +18 special string for scaling 
    char reserved2[40];   // +58 reserved
*/
    char calib_valid;     // +98 flag if calibration is valid 
/*
    char input_unit;      // +99 current input units for 
                          // "calib-value" 
    char polynom_unit;    // +100 linear UNIT and used 
                          // in the "polynom-coeff" 
*/
    char polynom_order;   // +101 ORDER of calibration POLYNOM 
/*
    char calib_count;             // +102 valid calibration data pairs 
    double pixel_position[10];    // +103 pixel pos. of calibration data 
    double calib_value[10];       // +183 calibration VALUE at above pos 
*/
    double polynom_coeff[6];      // +263 polynom COEFFICIENTS 
/*
    double laser_position;        // +311 laser wavenumber for relativ WN 
    char reserved3;               // +319 reserved 
    unsigned char new_calib_flag; // +320 If set to 200, valid label below 
    char calib_label[81];         // +321 Calibration label (NULL term'd) 
    char expansion[87];           // +402 Calibration Expansion area 
*/
};


}
#endif // #ifndef WINSPEC_SPE_DATASET_H

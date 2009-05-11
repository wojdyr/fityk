// Header of class PhilipsRawDataSet
// Licence: Lesser GNU Public License 2.1 (LGPL)
// $Id$

//Based on the file format specification sent to us by Martijn Fransen.
//
//NOTE: V5 format has not been tested, because we cannot get sample files.

#ifndef XYLIB_PHILIPS_RAW_H_
#define XYLIB_PHILIPS_RAW_H_

#include "xylib.h"

namespace xylib {

    class PhilipsRawDataSet : public DataSet
    {
        OBLIGATORY_DATASET_MEMBERS(PhilipsRawDataSet)
    };

}
#endif // XYLIB_PHILIPS_RAW_H_


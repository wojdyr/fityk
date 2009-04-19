// Philips UDF format - powder diffraction data from Philips diffractometers
// Licence: Lesser GNU Public License 2.1 (LGPL) 
// $Id$

//    Implementation based on the analysis of the sample files.

#ifndef XYLIB_PHILIPS_UDF_H_
#define XYLIB_PHILIPS_UDF_H_
#include "xylib.h"

namespace xylib {

    class UdfDataSet : public DataSet
    {
        OBLIGATORY_DATASET_MEMBERS(UdfDataSet)
    }; 

} // namespace

#endif // XYLIB_PHILIPS_UDF_H_


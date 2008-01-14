// Philips UDF format - powder diffraction data from Philips diffractometers
// Licence: Lesser GNU Public License 2.1 (LGPL) 
// $Id$

//    Implementation based on the analysis of the sample files.

#ifndef UDF_DATASET
#define UDF_DATASET
#include "xylib.h"

namespace xylib {

    class UdfDataSet : public DataSet
    {
        OBLIGATORY_DATASET_MEMBERS(UdfDataSet)
    }; 

} // namespace

#endif // UDF_DATASET


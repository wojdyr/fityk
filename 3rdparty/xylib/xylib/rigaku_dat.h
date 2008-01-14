// Rigaku .dat format - powder diffraction data from Rigaku diffractometers
// Licence: Lesser GNU Public License 2.1 (LGPL) 
// $Id$

// Implementation based on the analysis of the sample files.

#ifndef RIGAKU_DATASET
#define RIGAKU_DATASET
#include "xylib.h"


namespace xylib {

    class RigakuDataSet : public DataSet
    {
        OBLIGATORY_DATASET_MEMBERS(RigakuDataSet)
    }; 

} // namespace

#endif // RIGAKU_DATASET


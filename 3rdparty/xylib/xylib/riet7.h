// RIET7/LHPM/CSRIET DAT,  ILL_D1A5 and PSI_DMC formats
// Licence: Lesser GNU Public License 2.1 (LGPL) 
// $Id$

// RIET7/LHPM/CSRIET DAT (*.dat) constant count time XRD format is implemented
// as described at: http://www.ccp14.ac.uk/tutorial/xfit-95/dformat.htm
// i.e. start, step, stop and counts are read as free format.
// Moreover, the number of header lines can be smaller than four - we look at
// each line and and guess if it starts with start-step-stop sequence or not.
// The rest of the line with start-step-stop is discarded.
// In this way ILL_D1A5 and PSI_DMC formats can be also read - these formats
// are read by ObjCryst library

#ifndef XYLIB_RIET7_H_
#define XYLIB_RIET7_H_
#include "xylib.h"

namespace xylib {

    class Riet7DataSet : public DataSet
    {
        OBLIGATORY_DATASET_MEMBERS(Riet7DataSet)
    };

}
#endif // XYLIB_RIET7_H_


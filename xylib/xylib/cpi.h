// Sietronics Sieray CPI format
// Licence: Lesser GNU Public License 2.1 (LGPL)
// $Id$

// Implementation based on analysis of sample files
// and description from http://www.ccp14.ac.uk/tutorial/xfit-95/dformat.htm

#ifndef XYLIB_CPI_H_
#define XYLIB_CPI_H_
#include "xylib.h"

namespace xylib {

    class CpiDataSet : public DataSet
    {
        OBLIGATORY_DATASET_MEMBERS(CpiDataSet)
    };

}
#endif // XYLIB_CPI_H_


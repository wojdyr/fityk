// GSAS File Format
// Licence: Lesser GNU Public License 2.1 (LGPL)
// $Id$

// based on GSAS Manual, chapter "GSAS Standard Powder Data File"

#ifndef XYLIB_GSAS_H_
#define XYLIB_GSAS_H_

#include "xylib.h"

namespace xylib {

    class GsasDataSet : public DataSet
    {
    public:
        OBLIGATORY_DATASET_MEMBERS(GsasDataSet)
    };

} // namespace

#endif // XYLIB_GSAS_H_


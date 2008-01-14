// GSAS File Format
// Licence: Lesser GNU Public License 2.1 (LGPL) 
// $Id$

// based on GSAS Manual, chapter "GSAS Standard Powder Data File"

#ifndef GSAS_DATASET_H
#define GSAS_DATASET_H

#include "xylib.h"

namespace xylib {

    class GsasDataSet : public DataSet
    {
    public:
        OBLIGATORY_DATASET_MEMBERS(GsasDataSet)
    }; 

} // namespace

#endif // GSAS_DATASET_H


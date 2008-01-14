// Siemens/Bruker Diffrac-AT UXD text format (for powder diffraction data)
// Licence: Lesser GNU Public License 2.1 (LGPL) 
// $Id$

// Binary files can be converted to UXD format with XCH tool.
// Implementation based on the analysis of sample files.

#ifndef UXD_DATASET_H
#define UXD_DATASET_H
#include "xylib.h"

namespace xylib {

    class UxdDataSet : public DataSet
    {
        OBLIGATORY_DATASET_MEMBERS(UxdDataSet)
    };

}
#endif // UXD_DATASET_H


// Siemens/Bruker Diffrac-AT UXD text format (for powder diffraction data)
// Licence: Lesser GNU Public License 2.1 (LGPL)
// $Id$

// Binary files can be converted to UXD format with XCH tool.
// Implementation based on the analysis of sample files.

#ifndef XYLIB_UXD_H_
#define XYLIB_UXD_H_
#include "xylib.h"

namespace xylib {

    class UxdDataSet : public DataSet
    {
        OBLIGATORY_DATASET_MEMBERS(UxdDataSet)
    };

}
#endif // XYLIB_UXD_H_


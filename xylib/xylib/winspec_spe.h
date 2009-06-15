// Princeton Instruments WinSpec SPE Format
// Licence: Lesser GNU Public License 2.1 (LGPL)
// $Id$

// According to the format specification, SPE format has several versions
// (v1.43, v1.6 and the newest v2.25). But we need not implement every version
// of it, because it's backward compatible.
// The official programs to deal with this format is WinView/WinSpec.
//
// Implementation is based on the file format specification sent us by
// David Hovis (the documents came with his equipment)
// and source code of a program written by Pablo Bianucci.

#ifndef XYLIB_WINSPEC_SPE_H_
#define XYLIB_WINSPEC_SPE_H_
#include "xylib.h"

namespace xylib {

    struct spe_calib;

    class WinspecSpeDataSet : public DataSet
    {
        OBLIGATORY_DATASET_MEMBERS(WinspecSpeDataSet)

    protected:
        Column* get_calib_column(const spe_calib *calib, int dim);
        void read_calib(std::istream &f, spe_calib &calib);
    };

} // namespace
#endif // XYLIB_WINSPEC_SPE_H_


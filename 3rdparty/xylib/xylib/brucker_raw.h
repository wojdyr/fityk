// Siemens/Bruker Diffrac-AT Raw Format version 1/2/3
// Licence: Lesser GNU Public License 2.1 (LGPL) 
// $Id$

// Contains data from Siemens/Brucker X-ray diffractometers.
// Implementation based on the file format specification:
// "Appendix B: DIFFRAC-AT Raw Data File Format" from a diffractometer manual 

#ifndef BRUCKER_RAW_V1_H
#define BRUCKER_RAW_V1_H

#include "xylib.h"

namespace xylib {

    class BruckerRawDataSet : public DataSet
    {
        OBLIGATORY_DATASET_MEMBERS(BruckerRawDataSet)

    protected:
        void load_version1(std::istream &f);
        void load_version2(std::istream &f);
    }; 

} // namespace

#endif // BRUCKER_RAW_V1_H


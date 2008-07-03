// Siemens/Bruker Diffrac-AT Raw Format version 1/2/3
// Licence: Lesser GNU Public License 2.1 (LGPL) 
// $Id$

// Contains data from Siemens/Brucker X-ray diffractometers.
// Implementation based on:
// ver. 1 and 2: the file format specification from a diffractometer manual,
//               chapter "Appendix B: DIFFRAC-AT Raw Data File Format" 
// ver. with magic string "RAW1.01", that probably is v. 4, because 
//               corresponding ascii files start with ";RAW4.00",
//               was contributed by Andreas Breslau, who analysed binary files
//               and corresponding ascii files.

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
        void load_version1_01(std::istream &f);
    }; 

} // namespace

#endif // BRUCKER_RAW_V1_H


// Header of class BruckerV1RawDataSet
// Siemens/Bruker Diffrac-AT Raw File v1 format
// Licence: Lesser GNU Public License 2.1 (LGPL) 
// $Id: ds_brucker_raw_v1.h $

#ifndef BRUCKER_RAW_V1_H
#define BRUCKER_RAW_V1_H

#include "xylib.h"

namespace xylib {
    class BruckerV1RawDataSet : public DataSet
    {
    public:
        BruckerV1RawDataSet(std::istream &is)
            : DataSet(is, FT_BR_RAW1) {}
        
        // implement the interfaces specified by DataSet
        void load_data();

        static bool check(std::istream &f);

        const static FormatInfo fmt_info;
    }; // end of BruckerV1RawDataSet
}
#endif // #ifndef BRUCKER_RAW_V1_H


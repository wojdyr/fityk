// Header of class BruckerV23RawDataSet
// Siemens/Bruker Diffrac-AT Raw File v2/v3 format
// Licence: Lesser GNU Public License 2.1 (LGPL) 
// $Id: ds_brucker_raw_v23.h $

#ifndef BRUCKER_RAW_V23_H
#define BRUCKER_RAW_V23_H

#include "xylib.h"


namespace xylib {
    class BruckerV23RawDataSet : public DataSet
    {
    public:
        BruckerV23RawDataSet(std::istream &is, const std::string &filename = "")
            : DataSet(is, FT_BR_RAW23, filename) {}

        // implement the interfaces specified by DataSet
        void load_data();

        static bool check(std::istream &f);

        const static FormatInfo fmt_info;
    }; 
}
#endif // #ifndef BRUCKER_RAW_V23_H


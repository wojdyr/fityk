// Header of class BruckerV23RawDataSet
// Siemens/Bruker Diffrac-AT Raw File v2/v3 format
// Licence: Lesser GNU Public License 2.1 (LGPL) 
// $Id: ds_brucker_raw_v23.h $

#ifndef BRUCKER_RAW_V23_H
#define BRUCKER_RAW_V23_H

#include "xylib.h"
#include "util.h"

namespace xylib {
    class BruckerV23RawDataSet : public DataSet
    {
    public:
        BruckerV23RawDataSet(const std::string &filename)
            : DataSet(filename, FT_BR_RAW23) {}

        BruckerV23RawDataSet(std::istream &is, const std::string &filename)
            : DataSet(is, filename, FT_BR_RAW23) {}

        // implement the interfaces specified by DataSet
        bool is_filetype() const;
        void load_data();

        static bool check(std::istream &f);

        const static FormatInfo fmt_info;
    }; 
}
#endif // #ifndef BRUCKER_RAW_V23_H


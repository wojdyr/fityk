// Header of class BruckerV1RawDataSet for reading meta-data and xy-data from 
// Siemens/Bruker Diffrac-AT Raw File v1 format
// Licence: GNU General Public License version 2
// $Id: __MY_FILE_ID__ $

#ifndef BRUCKER_RAW_V1_H
#define BRUCKER_RAW_V1_H

#include "xylib.h"
#include "util.h"

namespace xylib {
    class BruckerV1RawDataSet : public DataSet
    {
    public:
        BruckerV1RawDataSet(const std::string &filename)
            : DataSet(filename, FT_BR_RAW1) {}

        // implement the interfaces specified by DataSet
        bool is_filetype() const;
        void load_data();

        static bool check(std::ifstream &f);
    }; // end of BruckerV1RawDataSet
}
#endif // #ifndef BRUCKER_RAW_V1_H


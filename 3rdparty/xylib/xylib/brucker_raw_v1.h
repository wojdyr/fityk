// Siemens/Bruker Diffrac-AT Raw File v1 format
// Licence: Lesser GNU Public License 2.1 (LGPL) 
// $Id: ds_brucker_raw_v1.h $

#ifndef BRUCKER_RAW_V1_H
#define BRUCKER_RAW_V1_H

#include "xylib.h"

namespace xylib {
    class BruckerRawDataSet : public DataSet
    {
    public:
        const static FormatInfo fmt_info;

        static bool check(std::istream &f);

        BruckerRawDataSet() 
            : DataSet(&fmt_info) {}
        
        void load_data(std::istream &f);

    protected:
        void load_version1(std::istream &f);
        void load_version2(std::istream &f);
    }; 
}

#endif // #ifndef BRUCKER_RAW_V1_H


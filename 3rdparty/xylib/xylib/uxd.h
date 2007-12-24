// Header of class UxdDataSet
// Licence: Lesser GNU Public License 2.1 (LGPL) 
// $Id: ds_uxd.h $

#ifndef UXD_DATASET_H
#define UXD_DATASET_H
#include "xylib.h"

namespace xylib {
    class UxdDataSet : public DataSet
    {
    public:
        UxdDataSet()
            : DataSet(&fmt_info) {}

        // implement the interfaces specified by DataSet
        void load_data(std::istream &f);

        static bool check(std::istream &f);
        const static FormatInfo fmt_info;
    }; 
}
#endif // #ifndef UXD_DATASET_H


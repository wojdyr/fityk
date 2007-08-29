// Header of class GsasDataSet
// Licence: Lesser GNU Public License 2.1 (LGPL) 
// $Id: ds_gsas.h $

#ifndef GSAS_DATASET_H
#define GSAS_DATASET_H
#include "xylib.h"

namespace xylib {
    class GsasDataSet : public DataSet
    {
    public:
        GsasDataSet()
            : DataSet(fmt_info.ftype) {}

        // implement the interfaces specified by DataSet
        void load_data(std::istream &f);

        static bool check(std::istream &f);
        const static FormatInfo fmt_info;
    }; 
}
#endif // #ifndef GSAS_DATASET_H


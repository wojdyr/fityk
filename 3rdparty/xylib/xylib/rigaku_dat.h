// Header of class RigakuDataSet
// Licence: Lesser GNU Public License 2.1 (LGPL) 
// $Id: ds_rigaku_udf.h $

#ifndef RIGAKU_DATASET
#define RIGAKU_DATASET
#include "xylib.h"


namespace xylib {
    class RigakuDataSet : public DataSet
    {
    public:
        RigakuDataSet()
            : DataSet(&fmt_info) {}
        
        // implement the interfaces specified by DataSet
        void load_data(std::istream &f);
        static bool check(std::istream &f);
        const static FormatInfo fmt_info;

    }; 
}
#endif // #ifndef RIGAKU_DATASET


// Header of class UdfDataSet
// Licence: Lesser GNU Public License 2.1 (LGPL) 
// $Id: ds_philips_udf.h $

#ifndef UDF_DATASET
#define UDF_DATASET
#include "xylib.h"


namespace xylib {
    class UdfDataSet : public DataSet
    {
    public:
        UdfDataSet()
            : DataSet(&fmt_info) {}

        // implement the interfaces specified by DataSet
        void load_data(std::istream &f);

        static bool check(std::istream &f);

        const static FormatInfo fmt_info;
    }; 
}
#endif // #ifndef UDF_DATASET


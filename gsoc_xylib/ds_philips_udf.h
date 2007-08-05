// Header of class UdfDataSet
// Licence: Lesser GNU Public License 2.1 (LGPL) 
// $Id: ds_philips_udf.h $

#ifndef UDF_DATASET
#define UDF_DATASET
#include "xylib.h"
#include "util.h"

namespace xylib {
    class UdfDataSet : public DataSet
    {
    public:
        UdfDataSet(const std::string &filename)
            : DataSet(filename, FT_UDF) {}

        UdfDataSet(std::istream &is, const std::string &filename)
            : DataSet(is, filename, FT_UDF) {}

        // implement the interfaces specified by DataSet
        bool is_filetype() const;
        void load_data();

        const static FormatInfo fmt_info;

    protected:
        void get_key_val(std::istream &f, std::string &key, std::string &val);
    }; 
}
#endif // #ifndef UDF_DATASET


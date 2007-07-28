// Header of class UdfDataSet
// Licence: Lesser GNU Public License 2.1 (LGPL) 
// $Id: ds_philips_udf.h $

#ifndef UDF_DATASET
#define UDF_DATASET
#include "xylib.h"
#include "util.h"

namespace xylib {
    class UdfDataSet : public UxdLikeDataSet
    {
    public:
        UdfDataSet(const std::string &filename)
            : UxdLikeDataSet(filename, FT_UDF) 
        {
            rg_start_tag = "RawScan";
            x_start_key = "DataAngleRange";
            x_step_key = "ScanStepSize";
            meta_sep = ",";
            data_sep = ", ";
        }

        // implement the interfaces specified by DataSet
        bool is_filetype() const;
        void load_data();

        const static FormatInfo fmt_info;

    protected:
        void parse_range(FixedStepRange* p_rg);

    }; 
}
#endif // #ifndef UDF_DATASET


// Header of class UdfDataSet for reading meta-data and xy-data from 
// Siemens/Bruker Diffrac-AT Raw File v2/v3 format
// Licence: GNU General Public License version 2
// $Id: __MY_FILE_ID__ $

#ifndef UDF_DATASET
#define UDF_DATASET
#include "xylib.h"
#include "util.h"

namespace xylib {
    class UdfDataSet : public UxdLikeDataSet
    {
    public:
        UdfDataSet(const std::string &filename)
            : UxdLikeDataSet(filename, FT_RIGAKU) 
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

    protected:
        void parse_range(FixedStepRange* p_rg);

    }; 
}
#endif // #ifndef UDF_DATASET


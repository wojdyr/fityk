// Header of class RigakuDataSet
// Licence: Lesser GNU Public License 2.1 (LGPL) 
// $Id: ds_rigaku_udf.h $

#ifndef RIGAKU_DATASET
#define RIGAKU_DATASET
#include "xylib.h"
#include "util.h"

namespace xylib {
    class RigakuDataSet : public UxdLikeDataSet
    {
    public:
        RigakuDataSet(const std::string &filename)
            : UxdLikeDataSet(filename, FT_RIGAKU) 
        {
            rg_start_tag = "*BEGIN";
            x_start_key = "*START";
            x_step_key = "*STEP";
            meta_sep = "=";
            data_sep = ", ";
            cmt_start = ";#";
        }

        // implement the interfaces specified by DataSet
        bool is_filetype() const;
        void load_data();

        static bool check(std::ifstream &f);

        const static FormatInfo fmt_info;

    protected:
        void parse_range(FixedStepRange* p_rg);

    }; 
}
#endif // #ifndef RIGAKU_DATASET


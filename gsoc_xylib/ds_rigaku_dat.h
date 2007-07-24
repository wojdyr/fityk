// Header of class BruckerV23RawDataSet for reading meta-data and xy-data from 
// Siemens/Bruker Diffrac-AT Raw File v2/v3 format
// Licence: GNU General Public License version 2
// $Id: __MY_FILE_ID__ $

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

    protected:
        void parse_range(FixedStepRange* p_rg);

    }; 
}
#endif // #ifndef RIGAKU_DATASET


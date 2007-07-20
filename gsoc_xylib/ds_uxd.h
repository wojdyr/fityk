// Header of class BruckerV23RawDataSet for reading meta-data and xy-data from 
// Siemens/Bruker Diffrac-AT Raw File v2/v3 format
// Licence: GNU General Public License version 2
// $Id: __MY_FILE_ID__ $

#ifndef UXD_DATASET_H
#define UXD_DATASET_H
#include "xylib.h"
#include "util.h"

namespace xylib {
    class UxdDataSet : public UxdLikeDataSet
    {
    public:
        UxdDataSet(const std::string &filename)
            : UxdLikeDataSet(filename, FT_UXD) 
        {
            rg_start_tag = "_DRIVE";
            x_start_key = "_START";
            x_step_key = "_STEPSIZE";
            meta_sep = "=";
            data_sep = " ";
            cmt_start = ";";
        }

        // implement the interfaces specified by DataSet
        bool is_filetype() const;
        void load_data();
        
    protected:
        void parse_range(FixedStepRange *p_rg);
    }; 
}
#endif // #ifndef UXD_DATASET_H


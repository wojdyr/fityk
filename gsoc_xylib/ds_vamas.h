// Header of class VamasDataSet
// Licence: Lesser GNU Public License 2.1 (LGPL) 
// $Id: ds_vamas.h $

#ifndef VAMAS_DATASET
#define VAMAS_DATASET
#include "xylib.h"
#include "util.h"

namespace xylib {
    class VamasDataSet : public DataSet
    {
    public:
        VamasDataSet(const std::string &filename)
            : DataSet(filename, FT_VAMAS), include(40, false) {}

        // implement the interfaces specified by DataSet
        bool is_filetype() const;
        void load_data();

        const static FormatInfo fmt_info;

    protected:
        void vamas_read_blk(FixedStepRange *p_rg);

        void read_meta_line(int idx, FixedStepRange *p_rg, std::string meta_key);

        // data members
        //////////////////////////////////////////////////////
        
        // a complete blk/range contains 40 parts. 
        // include[i] indicates if the i-th part (0-based) is included 
        std::vector<bool> include;

        int blk_fue;            // number of future upgrade experiment entries
        int exp_fue;            // number of future upgrade block entries
        std::string exp_mode;   // experimental mode
        std::string scan_mode;  // scan mode
        int exp_var_cnt;        // count of experimental variables
        
       
    }; 
}
#endif // #ifndef BRUCKER_RAW_V23_H


// ISO14976 VAMAS Surface Chemical Analysis Standard Data Transfer Format File
// Licence: Lesser GNU Public License 2.1 (LGPL) 
// $Id$

// This implementation is based on [1] and on the analysis of sample files.
//
//[1] W.A. Dench, L. B. Hazell and M. P. Seah, VAMAS Surface Chemical Analysis 
//    Standard Data Transfer Format with Skeleton Decoding Programs, 
//    Surface and Interface Analysis, 13 (1988) 63-122 
//    or National Physics Laboratory Report DMA(A)164 July 1988
// 

#ifndef VAMAS_DATASET
#define VAMAS_DATASET
#include "xylib.h"

namespace xylib {

    class VamasDataSet : public DataSet
    {
        OBLIGATORY_DATASET_MEMBERS(VamasDataSet)

    protected:
        // a complete block contains 40 parts. 
        // include[i] indicates if the i-th part (0-based) is included 
        std::vector<bool> include;

        int blk_fue;            // number of future upgrade experiment entries
        int exp_fue;            // number of future upgrade block entries
        std::string exp_mode;   // experimental mode
        std::string scan_mode;  // scan mode
        int exp_var_cnt;        // count of experimental variables
        
        Block *read_block(std::istream &f);
    }; 

} // namespace xylib
#endif // #ifndef VAMAS_DATASET


// Header of class VamasDataSet
// Licence: Lesser GNU Public License 2.1 (LGPL) 
// $Id: ds_vamas.h $

#ifndef VAMAS_DATASET
#define VAMAS_DATASET
#include "xylib.h"

namespace xylib {

class VamasDataSet : public DataSet
{
public:
    static const FormatInfo fmt_info;
    VamasDataSet()
        : DataSet(&fmt_info), include(40, false) {}

    void load_data(std::istream &f);

    static bool check(std::istream &f);

protected:
    // a complete blk/range contains 40 parts. 
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


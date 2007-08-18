// Header of class PdCifDataSet
// Licence: Lesser GNU Public License 2.1 (LGPL) 
// $Id: ds_pdcif_spe.h $

#ifndef PDCIF_DATASET_H
#define PDCIF_DATASET_H
#include "xylib.h"

namespace xylib {

class PdCifDataSet : public DataSet
{
public:
    PdCifDataSet()
        : DataSet(FT_PDCIF) {}

    // implement the interfaces specified by DataSet
    static bool check(std::istream &f);
    void load_data(std::istream &f);

    const static FormatInfo fmt_info;
    
protected:
    void get_all_values(const std::string &line, std::istream &f, 
        std::vector<std::string> &values);
    bool add_key_val(Range *p_rg, const std::string &key, const std::string &val);
    void add_range(Range* p_rg);

    VecColumn* get_col_ptr(Range *p_rg, const std::string name, 
        std::map<std::string, VecColumn*>& mapper);
}; 

}
#endif // #ifndef PDCIF_DATASET_H

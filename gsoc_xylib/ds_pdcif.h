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
    PdCifDataSet(std::istream &is)
        : DataSet(is, FT_PDCIF) {}

    // implement the interfaces specified by DataSet
    static bool check(std::istream &f);
    void load_data();

    const static FormatInfo fmt_info;
    
protected:
    enum {
        OUT_OF_LOOP = -1,
        IN_LOOP_HEAD = 0,
        IN_LOOP_BODY = 1,
    };

    void get_all_values(const std::string &line, std::istream &f, std::vector<std::string> &values);
    bool add_key_val(FixedStepRange *p_rg, const std::string &key, const std::string &val);
}; 

}
#endif // #ifndef PDCIF_DATASET_H

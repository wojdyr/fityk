// Header of class TextDataSet
// Siemens/Bruker Diffrac-AT Raw File v2/v3 format
// Licence: Lesser GNU Public License 2.1 (LGPL) 
// $Id: ds_text.h $

#ifndef TEXT_DATASET_H
#define TEXT_DATASET_H
#include "xylib.h"

namespace xylib {
    class TextDataSet : public DataSet
    {
    public:
        TextDataSet()
            : DataSet(fmt_info.ftype) {}

        // implement the interfaces specified by DataSet
        void load_data(std::istream &f);

        static bool check(std::istream &f);

        const static FormatInfo fmt_info;
    }; 
}
#endif // #ifndef TEXT_DATASET_H


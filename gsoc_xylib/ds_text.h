// Header of class TextDataSet
// Siemens/Bruker Diffrac-AT Raw File v2/v3 format
// Licence: Lesser GNU Public License 2.1 (LGPL) 
// $Id: ds_text.h $

#ifndef TEXT_DATASET_H
#define TEXT_DATASET_H
#include "xylib.h"
#include "util.h"

namespace xylib {
    class TextDataSet : public DataSet
    {
    public:
        TextDataSet(const std::string &filename)
            : DataSet(filename, FT_TEXT) {}

        TextDataSet(std::istream &is, const std::string &filename)
            : DataSet(is, filename, FT_TEXT) {}

        // implement the interfaces specified by DataSet
        bool is_filetype() const;
        void load_data();

        const static FormatInfo fmt_info;
    }; 
}
#endif // #ifndef TEXT_DATASET_H


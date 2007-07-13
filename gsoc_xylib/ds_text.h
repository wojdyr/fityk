// Header of class TextDataSet for reading meta-data and xy-data from 
// Siemens/Bruker Diffrac-AT Raw File v2/v3 format
// Licence: GNU General Public License version 2
// $Id: __MY_FILE_ID__ $

#ifndef TEXT_DATASET_H
#define TEXT_DATASET_H
#include "xylib.h"

namespace xylib {
    class TextDataSet : public DataSet
    {
    public:
        TextDataSet(const std::string &filename)
            : DataSet(filename, FT_TEXT) {}

        // implement the interfaces specified by DataSet
        bool is_filetype() const;
        void load_data();
        
    }; 
}
#endif // #ifndef TEXT_DATASET_H


// Header of class TextDataSet
// Siemens/Bruker Diffrac-AT Raw File v2/v3 format
// Licence: Lesser GNU Public License 2.1 (LGPL) 
// $Id$

#ifndef TEXT_DATASET_H
#define TEXT_DATASET_H
#include "xylib.h"

namespace xylib {

class TextDataSet : public DataSet
{
    OBLIGATORY_DATASET_MEMBERS(TextDataSet)
}; 

} // namespace
#endif // TEXT_DATASET_H


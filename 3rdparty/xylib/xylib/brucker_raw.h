// Siemens/Bruker Diffrac-AT Raw File v1 format
// Licence: Lesser GNU Public License 2.1 (LGPL) 
// $Id$

#ifndef BRUCKER_RAW_V1_H
#define BRUCKER_RAW_V1_H

#include "xylib.h"

namespace xylib {

class BruckerRawDataSet : public DataSet
{
    OBLIGATORY_DATASET_MEMBERS(BruckerRawDataSet)

protected:
    void load_version1(std::istream &f);
    void load_version2(std::istream &f);
}; 

} // namespace

#endif // BRUCKER_RAW_V1_H


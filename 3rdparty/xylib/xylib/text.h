// ascii plain text 
// Licence: Lesser GNU Public License 2.1 (LGPL) 
// $Id$

// Multi-column ascii text file.
// In each line, we try to read as many numbers as possible.
// If there are at least two numbers, the line is considered a valid
// data line, otherwise it is silently skipped (regarded a comment).
// The number of "columns" in file is the minimal number of columns of valid
// lines. 
// 
// The following lines will be skipped:
// # foo bar
// ; 1.2 3.4 5.6
// 1.2 ** 3.4
// foo 2 bar 4

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


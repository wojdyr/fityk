// ascii plain text 
// Licence: Lesser GNU Public License 2.1 (LGPL) 
// $Id$

// Multi-column ascii text file.
// In each line, we try to read as many numbers as possible.
// The lines that do not start with number are skipped.
// If there are at least two numbers, the line is considered a valid
// data line, otherwise it is silently skipped (regarded a comment).
// In non-strict mode, the line that start with numbers, but is shorter
// than the previous line, in some cases (see TextDataSet::load_data())
// can also be skipped.
// If valid (numeric) lines have different number of numbers, the smallest
// number is used as the number of columns and the longer lines are truncated.
// 
// The following lines will be skipped:
// # foo bar
// ; 1.2 3.4 5.6
// foo 2 bar 4

#ifndef XYLIB_TEXT_H_
#define XYLIB_TEXT_H_
#include "xylib.h"

namespace xylib {

    class TextDataSet : public DataSet
    {
        OBLIGATORY_DATASET_MEMBERS(TextDataSet)
        static const char*  read_numbers(std::string const& s, 
                                         std::vector<double>& row);
    }; 

} // namespace
#endif // XYLIB_TEXT_H_


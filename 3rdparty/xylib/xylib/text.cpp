// ascii plain text 
// Licence: Lesser GNU Public License 2.1 (LGPL) 
// $Id$

#include <cerrno>
#include "text.h"
#include "util.h"

using namespace std;
using namespace xylib::util;

namespace xylib {

const FormatInfo TextDataSet::fmt_info(
    "text",
    "ascii text / CSV / TSV",
    vector<string>(),//vector_string("txt", "dat", "asc", "csv"),
    false,                       // whether binary
    false,                       // whether has multi-blocks
    &TextDataSet::ctor,
    &TextDataSet::check
);

bool TextDataSet::check(istream & /*f*/) 
{
    return true;
}


void TextDataSet::load_data(std::istream &f) 
{
    vector<VecColumn*> cols;  
    vector<double> row; // temporary storage for values from one line
    string s;
    while (getline(f, s)) {
        row.clear();
        const char *p = s.c_str();
        while (*p != 0) {
            char *endptr = NULL;
            errno = 0; // to distinguish success/failure after call 
            double val = strtod(p, &endptr);
            if (p == endptr) // no more numbers
                break;
            if (errno != 0)
                throw FormatError("Numeric overflow or underflow in line:\n" 
                                  + s);
            row.push_back(val);
            p = endptr;
            while (isspace(*p) || *p == ',' || *p == ';' || *p == ':')
                ++p;
        }

        if (row.size() < 2) // no data in this line
            continue;

        if (cols.size() == 0)  // first line - initialization
            for (size_t i = 0; i != row.size(); ++i) 
                cols.push_back(new VecColumn);

        if (cols.size() > row.size()) { 
            // it's not clear what to do in this situation
            // if it's the last line, we ignore the line
            // otherwise we decrease the number of columns
            if (f.eof())
                break;
            for (size_t i = row.size(); i != cols.size(); ++i)
                delete cols[i];
            cols.resize(row.size());
        }

        for (size_t i = 0; i != cols.size(); ++i) 
            cols[i]->add_val(row[i]);
    }

    format_assert (cols.size() >= 2 && cols[0]->get_point_count() >= 2,
                   "data not found in file.");

    Block* blk = new Block;
    for (unsigned i = 0; i < cols.size(); ++i) 
        blk->add_column(cols[i]);

    blocks.push_back(blk);
}

} // end of namespace xylib


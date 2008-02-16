// FOURYA/XFIT/Koalariet XDD file
// Licence: Lesser GNU Public License 2.1 (LGPL) 
// $Id$

#include <cmath>
#include <cstdlib>
#include "xfit_xdd.h"
#include "util.h"

using namespace std;
using namespace xylib::util;

namespace xylib {


const FormatInfo XfitXddDataSet::fmt_info(
    "xfit_xdd",
    "XFIT XDD",
    vector_string("xdd"),
    false,                       // whether binary
    false,                       // whether has multi-blocks
    &XfitXddDataSet::ctor,
    &XfitXddDataSet::check
);

namespace {

void skip_whitespace(istream &f)
{
    while (isspace(f.peek()))
        f.ignore();
}

void skip_c_style_comments(istream& f)
{
    skip_whitespace(f);
    int a = f.get();
    if (a != '/' || f.peek() != '*') {
        f.unget();
        return;
    }
    f.ignore(); // '*'
    while (f) {
        f.ignore(2048, '*');
        if (f.peek() == '/') {
            f.ignore();
            break;
        }
    }
    skip_whitespace(f);
}

// read line (popular e.g. in powder data ascii file types) in free format:
// start step count
// example:
//   15.000   0.020 110.000
// returns NULL on error
StepColumn* read_start_step_end_line(istream& f)
{
    string line;
    getline(f, line); 
    // the first line should contain start, step and stop
    char *endptr;
    const char *startptr = line.c_str();
    double start = strtod(startptr, &endptr);
    if (startptr == endptr)
        return NULL;

    startptr = endptr;
    double step = strtod(startptr, &endptr);
    if (startptr == endptr || step == 0.)
        return NULL;

    startptr = endptr;
    double stop = strtod(endptr, &endptr);
    if (startptr == endptr)
        return NULL;

    double dcount = (stop - start) / step + 1;
    int count = iround(dcount);
    if (count < 4 || fabs(count - dcount) > 1e-2)
        return NULL;

    return new StepColumn(start, step, count);
}

Block* read_ssel_and_data(istream &f)
{
    StepColumn *xcol = read_start_step_end_line(f);
    if (!xcol)
        return NULL;

    Block* blk = new Block;
    blk->add_column(xcol);
    
    VecColumn *ycol = new VecColumn;
    string s;
    while (getline(f, s)) 
        ycol->add_values_from_str(s);
    blk->add_column(ycol);

    // both xcol and ycol should have known and same number of points
    if (xcol->get_point_count() != ycol->get_point_count()) {
        delete blk;
        return NULL;
    }
    return blk;
}

} // anonymous namespace

bool XfitXddDataSet::check(istream &f) 
{
    skip_c_style_comments(f);
    StepColumn *c = read_start_step_end_line(f);
    delete c;
    return c != NULL;
}

void XfitXddDataSet::load_data(std::istream &f) 
{
    skip_c_style_comments(f);
    Block *block = read_ssel_and_data(f);
    format_assert(block);
    blocks.push_back(block);
}

} // namespace xylib


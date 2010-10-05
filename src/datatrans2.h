// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
// $Id$

#ifndef FITYK__DATATRANS2__H__
#define FITYK__DATATRANS2__H__

/// big grammars in Spirit take a lot of time and memory to compile
/// so they must be splitted into separate compilation units
/// that's the only reason why this file is not a part of datatrans.cpp
/// code here was originally part of datatrans.cpp (yes, .cpp)
///
/// this file is included only by datatrans*.cpp

#include "datatrans.h"
#include "common.h"
#include "data.h"
#include "var.h"
#include "func.h"
#include "numfuncs.h"
#include "logic.h"
#include "fityk.h"
#include "eparser.h"
#include <boost/spirit/include/classic_core.hpp>

using namespace std;
using namespace boost::spirit::classic;

using namespace dataVM;

namespace datatrans {

extern vector<int> code;        //  VM code
extern vector<fp> numbers;  //  VM data
extern const int stack_size;  //should be enough,
                              //there are no checks for stack overflow

//-- functors used in the grammar for putting VM code and data into vectors --

struct push_double
{
    void operator()(const double& d) const;
};

struct push_the_double: public push_double
{
    push_the_double(double d_) : d(d_) {}
    void operator()(char const*, char const*) const
                                               { push_double::operator()(d); }
    void operator()(const char) const { push_double::operator()(d); }
    double d;
};

#ifndef STANDALONE_DATATRANS
struct push_var: public push_double
{
    void operator()(char const* a, char const* b) const;
};

struct push_func_param: public push_double
{
    void operator()(char const* a, char const* b) const;
};

struct push_func
{
    void operator()(char const* a, char const* b) const;
};

#endif //not STANDALONE_DATATRANS

struct push_op
{
    push_op(int op_, int op2_=0) : op(op_), op2(op2_) {}
    void push() const;
    void operator()(char const*, char const*) const { push(); }
    void operator()(char) const { push(); }

    int op, op2;
};

// optimization, to parse -3 as number, not as 3 OP_NEG
inline void push_neg_op(char const*, char const*)
{
    if (code.size() >= 2 && *(code.end() - 2) == OP_NUMBER)
        numbers[*(code.end() - 1)] *= -1;
    else
        code.push_back(OP_NEG);
}

} //namespace

#endif

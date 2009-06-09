// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
// $Id$

#ifndef FITYK__CMD3__H__
#define FITYK__CMD3__H__

/// big grammars in Spirit take a lot of time and memory to compile
/// so they must be splitted into separate compilation units
/// that's the only reason why this file is not a part of datatrans.cpp
/// code here was originally part of datatrans.cpp (yes, .cpp)
///
/// this file is included only by datatrans*.cpp

#include "common.h"
#include <boost/spirit/core.hpp>


using namespace boost::spirit;

/// a part of command grammar
struct Cmd3Grammar : public grammar<Cmd3Grammar>
{
  template <typename ScannerT>
  struct definition
  {
    definition(Cmd3Grammar const& self);

    rule<ScannerT> in_data, optional_plus,
                   set_arg, commands_arg, fit_arg, statement;

    rule<ScannerT> const& start() const { return statement; }
  };
};

extern Cmd3Grammar cmd3G;


#endif


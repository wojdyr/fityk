// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#ifndef FITYK__DATATRANS3__H__
#define FITYK__DATATRANS3__H__

/// big grammars in Spirit take a lot of time and memory to compile
/// so they must be splitted into separate compilation units
/// that's the only reason why this file is not a part of datatrans.cpp
/// code here was originally part of datatrans.cpp (yes, .cpp)
///
/// this file is included only by datatrans*.cpp

#include <boost/spirit/core.hpp>

using namespace boost::spirit;

/// a part of data expression grammar
struct DataE2Grammar : public grammar<DataE2Grammar>
{
  template <typename ScannerT>
  struct definition
  {
    definition(DataE2Grammar const& /*self*/);

    rule<ScannerT> rprec6, real_constant, real_variable, parameterized_args,
                   index;

    rule<ScannerT> const& start() const { return rprec6; }
  };
};

extern DataE2Grammar DataE2G;

#endif

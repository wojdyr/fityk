// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#ifndef FITYK__DATATRANS4__H__
#define FITYK__DATATRANS4__H__

/// big grammars in Spirit take a lot of time and memory to compile
/// so they must be splitted into separate compilation units
/// that's the only reason why this file is not a part of datatrans.cpp
/// code here was originally part of datatrans.cpp (yes, .cpp)
///
/// this file is included only by datatrans*.cpp

#include <boost/spirit/core.hpp>

using namespace boost::spirit;

/// a part of data expression grammar
struct DataExprFunGrammar: public grammar<DataExprFunGrammar>
{
  template <typename ScannerT>
  struct definition
  {
    definition(DataExprFunGrammar const& /*self*/);

    rule<ScannerT> dfunc, func_or_f_or_z;

    rule<ScannerT> const& start() const { return dfunc; }
  };
};

extern DataExprFunGrammar DataExprFunG;

#endif

// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
// $Id$

#ifndef FITYK__CMD2__H__
#define FITYK__CMD2__H__

/// big grammars in Spirit take a lot of time and memory to compile
/// so they must be splitted into separate compilation units
/// that's the only reason why this file is not a part of datatrans.cpp
/// code here was originally part of datatrans.cpp (yes, .cpp)
///
/// this file is included only by datatrans*.cpp

#include <limits.h>
#include <boost/spirit/core.hpp>
#include <boost/spirit/actor/increment_actor.hpp>

#include "common.h"


using namespace boost::spirit;
class DataAndModel;

namespace cmdgram {

extern bool with_plus, deep_cp;
extern std::string t, t2, t3;
extern int tmp_int, tmp_int2, dm_pref;
extern double tmp_real, tmp_real2;
extern std::vector<std::string> vt, vr;
extern std::vector<int> vn, vds;
extern const int new_dataset;
extern const int all_datasets;
extern bool no_info_output;
extern std::string prepared_info;

std::vector<DataAndModel*> get_datasets_from_indata();

/// a part of command grammar
struct IntRangeGrammar : public grammar<IntRangeGrammar>
{
    template <typename ScannerT>
    struct definition
    {
      definition(IntRangeGrammar const& /*self*/)
      {
          static const int zero = 0;
          static const int int_max = INT_MAX;

          t
              = '[' >> (int_p[assign_a(tmp_int)]
                       | eps_p[assign_a(tmp_int, zero)]
                       )
                    >> (':'
                        >> (int_p[assign_a(tmp_int2)]
                           | eps_p[assign_a(tmp_int2, int_max)]
                           )
                        >> ']'
                       | ch_p(']')[assign_a(tmp_int2, tmp_int)]
                              [increment_a(tmp_int2)]//see assign_a error above
                       )
              ;
      }
      rule<ScannerT> t;
      rule<ScannerT> const& start() const { return t; }
    };
};

extern IntRangeGrammar  IntRangeG;

struct CompactStrGrammar : public grammar<CompactStrGrammar>
{
    template <typename ScannerT>
    struct definition
    {
      definition(CompactStrGrammar const& /*self*/)
      {
          main
            = lexeme_d['\'' >> (+~chset_p("'\n"))[assign_a(t)]
                       >> '\'']
            | lexeme_d[+~chset_p(" \t\n\r;,#")] [assign_a(t)]
            ;
      }
      rule<ScannerT> main;
      rule<ScannerT> const& start() const { return main; }
    };
};

extern CompactStrGrammar CompactStrG;

} // namespace cmdgram


/// a part of command grammar
struct Cmd2Grammar : public grammar<Cmd2Grammar>
{
  template <typename ScannerT>
  struct definition
  {
    definition(Cmd2Grammar const& self);

    rule<ScannerT> transform, type_name, function_param,
                   in_data, dm_prefix,
                   dataset_handling, guess,
                   dataset_lhs, dataset_nr,
                   optional_plus,
                   plot_range, info_arg, statement;

    rule<ScannerT> const& start() const { return statement; }
  };
};

extern Cmd2Grammar cmd2G;

#endif

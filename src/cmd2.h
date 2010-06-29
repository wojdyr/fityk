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
#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_increment_actor.hpp>

#include "common.h"


using namespace boost::spirit::classic;
class DataAndModel;

namespace cmdgram {

extern bool with_plus;
extern std::string t, t2, t3;
extern int tmp_int, tmp_int2, dm_pref;
extern double tmp_real;
extern std::vector<std::string> vt, vr;
extern std::vector<int> vn, vds;
extern const int new_dataset;
extern const int all_datasets;

std::vector<DataAndModel*> get_datasets_from_indata();

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

struct InDataGrammar : public grammar<InDataGrammar>
{
    template <typename ScannerT>
    struct definition
    {
      definition(InDataGrammar const& /*self*/)
      {
          in_data
              = eps_p [clear_a(vds)]
              >> !("in" >> (lexeme_d['@' >> uint_p [push_back_a(vds)]
                                     ]
                             % ','
                           | str_p("@*") [push_back_a(vds, all_datasets)]
                           )
                  )
              ;
      }
      rule<ScannerT> in_data;
      rule<ScannerT> const& start() const { return in_data; }
    };
};

extern InDataGrammar InDataG;

} // namespace cmdgram


/// a part of command grammar
struct Cmd2Grammar : public grammar<Cmd2Grammar>
{
  template <typename ScannerT>
  struct definition
  {
    definition(Cmd2Grammar const& self);

    rule<ScannerT> transform, type_name, function_param,
                   dm_prefix,
                   dataset_handling, guess,
                   dataset_lhs, dataset_nr,
                   optional_plus,
                   plot_range, info_arg, statement;

    rule<ScannerT> const& start() const { return statement; }
  };
};

extern Cmd2Grammar cmd2G;

#endif

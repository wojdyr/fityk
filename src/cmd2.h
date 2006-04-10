// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#ifndef FITYK__CMD2__H__
#define FITYK__CMD2__H__

/// big grammars in Spirit take a lot of time and memory to compile
/// so they must be splitted into separate compilation units
/// that's the only reason why this file is not a part of datatrans.cpp
/// code here was originally part of datatrans.cpp (yes, .cpp)
///
/// this file is included only by datatrans*.cpp

#include "common.h"
#include <boost/spirit/core.hpp>


using namespace boost::spirit;
class DataWithSum;

namespace cmdgram {

    extern bool with_plus, deep_cp;
    extern std::string t, t2;
    extern int tmp_int, tmp_int2, ds_pref;
    extern double tmp_real, tmp_real2;
    extern std::vector<std::string> vt, vr;
    extern std::vector<int> vn, vds;
    extern const int new_dataset;
    extern const int all_datasets;
    extern bool outdated_plot;

    std::vector<DataWithSum*> get_datasets_from_indata();

}


struct Cmd2Grammar : public grammar<Cmd2Grammar>
{
  template <typename ScannerT>
  struct definition
  {
    definition(Cmd2Grammar const& self);

    rule<ScannerT> transform, type_name, function_param, 
                   in_data, ds_prefix,
                   dataset_handling, compact_str, guess,
                   existing_dataset_nr, dataset_nr, 
                   optional_plus, 
                   plot_range, info_arg, statement;  

    rule<ScannerT> const& start() const { return statement; }
  };
};

extern Cmd2Grammar cmd2G;

#endif

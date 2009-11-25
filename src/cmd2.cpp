// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
// $Id$

/// big grammars in Spirit take a lot of time and memory to compile
/// so they must be splitted into separate compilation units
/// that's the only reason why this file is not a part of cmd.cpp

#include <boost/spirit/core.hpp>
#include <boost/spirit/actor/assign_actor.hpp>
#include <boost/spirit/actor/push_back_actor.hpp>
#include <boost/spirit/actor/clear_actor.hpp>
#include <boost/spirit/utility/chset.hpp>
#include <boost/spirit/utility/chset_operators.hpp>
#include <math.h>

#include "cmd2.h"
#include "optional_suffix.h"
#include "info.h"
#include "logic.h"
#include "data.h"
#include "model.h"
#include "guess.h"
#include "var.h"

using namespace std;

namespace cmdgram {

bool with_plus;
string t, t2, t3;
int tmp_int, tmp_int2, dm_pref;
double tmp_real;
vector<string> vt, vr;
vector<int> vn, vds;
const int new_dataset = -1;
const int all_datasets = -2;


vector<int> get_dm_indices_from_indata()
{
    vector<int> result;
    // no datasets specified
    if (vds.empty()) {
        if (AL->get_dm_count() == 1)
            result.push_back(0);
        else
            throw ExecuteError("Dataset must be specified (eg. 'in @0').");
    }
    // @*
    else if (vds.size() == 1 && vds[0] == all_datasets)
        for (int i = 0; i < AL->get_dm_count(); ++i)
            result.push_back(i);
    // general case
    else
        for (vector<int>::const_iterator i = vds.begin(); i != vds.end(); ++i)
            if (*i == all_datasets) {
                for (int j = 0; j < AL->get_dm_count(); ++j) {
                    if (!contains_element(result, j))
                        result.push_back(j);
                }
                return result;
            }
            else
                result.push_back(*i);
    return result;
}

vector<DataAndModel*> get_datasets_from_indata()
{
    vector<int> indices = get_dm_indices_from_indata();
    vector<DataAndModel*> result(indices.size());
    for (size_t i = 0; i < indices.size(); ++i)
        result[i] = AL->get_dm(indices[i]);
    return result;
}

IntRangeGrammar  IntRangeG;
CompactStrGrammar  CompactStrG;

} //namespace cmdgram

using namespace cmdgram;

namespace {

void do_import_dataset(char const*, char const*)
{
    if (t == ".") {
        if (tmp_int == new_dataset)
            throw ExecuteError("New dataset can't be reverted");
        if (!vt.empty())
            throw ExecuteError("Options can't be given when reverting data");
        AL->get_data(tmp_int)->revert();
    }
    else
        AL->import_dataset(tmp_int, t, vt);
    AL->outdated_plot();
}
void do_revert_data(char const*, char const*)
{
    AL->get_data(tmp_int)->revert();
    AL->outdated_plot();
}
void do_load_data_sum(char const*, char const*)
{
    vector<Data const*> dd;
    for (vector<int>::const_iterator i = vn.begin(); i != vn.end(); ++i)
        dd.push_back(AL->get_data(*i));
    if (tmp_int == new_dataset)
        tmp_int = AL->append_dm();
    AL->get_data(tmp_int)->load_data_sum(dd, t);
    AL->outdated_plot();
}

void do_plot(char const*, char const*)
{
    AL->view.parse_and_set(vr, get_dm_indices_from_indata());
    AL->get_ui()->draw_plot(1, true);
}

void do_output_info(char const* a, char const* b)
{
    output_info(AL, string(a,b), with_plus);
}

void do_guess(char const*, char const*)
{
    vector<DataAndModel*> v = get_datasets_from_indata();
    string const& name = t;
    string const& function = t2;

    // "%name = guess Linear in @0, @1" makes no sense
    if (!name.empty() && v.size() > 1)
        // SyntaxError actually
        throw ExecuteError("many functions can't be assigned to one name.");

    for (vector<DataAndModel*>::const_iterator i = v.begin();
                                                          i != v.end(); ++i) {
        DataAndModel *dm = *i;
        vector<string> vars = vt;
        assert(vr.size() == 2);
        Guess(AL, dm).guess(name, function, vr[0], vr[1], vars);
        string real_name = AL->assign_func(name, function, vars);
        dm->model()->add_function_to(real_name, Model::kF);
    }
    AL->outdated_plot();
}

void set_data_title(char const*, char const*)  {
    AL->get_data(dm_pref)->title = t;
}

} //namespace

template <typename ScannerT>
Cmd2Grammar::definition<ScannerT>::definition(Cmd2Grammar const& /*self*/)
{
    //these static constants for assign_a are workaround for assign_a
    //problems, as proposed by Joao Abecasis at Spirit-general ML
    //Message-ID: <435FB3DD.8030205@gmail.com>
    //Subject: [Spirit-general] Re: weird assign_a(x,y) problem
    static const bool true_ = true;
    static const bool false_ = false;
    static const int minus_one = -1;
    static const char *dot = ".";
    static const char *empty = "";


    in_data
        = eps_p [clear_a(vds)]
        >> !("in" >> (lexeme_d['@' >> (uint_p [push_back_a(vds)]
                                      |ch_p('*')[push_back_a(vds, all_datasets)]
                                      )
                              ]
                       % ','
                     )
            )
        ;

    dm_prefix
        = lexeme_d['@' >> uint_p [assign_a(dm_pref)]
           >> '.']
        | eps_p [assign_a(dm_pref, minus_one)]
        ;

    type_name
        = lexeme_d[(upper_p >> +alnum_p)]
        ;

    function_param
        = lexeme_d[alpha_p >> *(alnum_p | '_')]
        ;

    dataset_nr
        = lexeme_d['@' >> ( uint_p [assign_a(tmp_int)]
                          | ch_p('*') [assign_a(tmp_int, all_datasets)]
                          )
                  ]
        ;

    dataset_lhs
        = lexeme_d['@' >> ( uint_p [assign_a(tmp_int)]
                          | ch_p('+') [assign_a(tmp_int, new_dataset)]
                          )
                  ]
        ;

    dataset_handling
          //load from file
        = (dataset_lhs >> '<' >> CompactStrG [clear_a(vt)]
           >> !(lexeme_d[+(alnum_p | '-' | '_')] [push_back_a(vt)]
                % ',')
          ) [&do_import_dataset]
          //sum / duplicate
        | dataset_lhs >> ch_p('=') [clear_a(vn)] [assign_a(t, empty)]
          >> !(lexeme_d[lower_p >> +(alnum_p | '-' | '_')] [assign_a(t)])
          >> (lexeme_d['@' >> uint_p [push_back_a(vn)]
                      | "0"
                                   ] % '+') [&do_load_data_sum]
        ;

    plot_range  //first clear vr if needed
        = (ch_p('[') >> ']') [push_back_a(vr,empty)][push_back_a(vr,empty)]
        | '[' >> (real_p|"."|eps_p) [push_back_a(vr)]
          >> ':' >> (real_p|"."|eps_p) [push_back_a(vr)]
          >> ']'
        | str_p(".") [push_back_a(vr,dot)][push_back_a(vr,dot)] // [.:.]
        | eps_p [push_back_a(vr,empty)][push_back_a(vr,empty)] // [:]
        ;

    guess
        = (FunctionLhsG [assign_a(t)] >> '='
          | eps_p [assign_a(t, empty)]
          )
          >> optional_suffix_p("g","uess")[clear_a(vt)] [clear_a(vr)]
          >> type_name [assign_a(t2)]
          >> plot_range
          >> !((function_param >> '=' >> no_actions_d[FuncG])
                                                       [push_back_a(vt)]
               % ',')
          >> in_data
        ;

    optional_plus
        = str_p("+") [assign_a(with_plus, true_)]
        | eps_p [assign_a(with_plus, false_)]
        ;

    statement
        = (optional_suffix_p("i","nfo")
           >> optional_plus
           >> (+chset<>(anychar_p - chset<>(";#"))) [&do_output_info]
          )
        | (optional_suffix_p("p","lot") [clear_a(vr)]
           >> plot_range >> plot_range >> in_data) [&do_plot]
        | guess [&do_guess]
        | dataset_handling
        | optional_suffix_p("s","et")
          >> (dm_prefix >> "title" >> '=' >> CompactStrG)[&set_data_title]
        ;
}


template Cmd2Grammar::definition<scanner<char const*, scanner_policies<skipper_iteration_policy<iteration_policy>, match_policy, action_policy> > >::definition(Cmd2Grammar const&);

template Cmd2Grammar::definition<scanner<char const*, scanner_policies<skipper_iteration_policy<iteration_policy>, match_policy, no_actions_action_policy<action_policy> > > >::definition(Cmd2Grammar const&);

Cmd2Grammar cmd2G;



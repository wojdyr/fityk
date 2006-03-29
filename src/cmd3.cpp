// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

/// big grammars in Spirit take a lot of time and memory to compile
/// so they must be splitted into separate compilation units
/// that's the only reason why this file is not a part of cmd.cpp

#include <boost/spirit/core.hpp>
#include <boost/spirit/actor/assign_actor.hpp>
#include <boost/spirit/actor/push_back_actor.hpp>
#include <boost/spirit/actor/clear_actor.hpp>
#include <boost/spirit/actor/increment_actor.hpp>
#include <boost/spirit/utility/chset.hpp>
#include <boost/spirit/utility/chset_operators.hpp>
#include <boost/spirit/utility.hpp>

#include "cmd3.h"
#include "cmd2.h"
#include "optional_suffix.h"
#include "logic.h"
#include "data.h"
#include "ui.h"
#include "settings.h"
#include "datatrans.h"

using namespace std;
using namespace cmdgram;

namespace {

void do_transform(char const*, char const*)  { 
    vector<DataWithSum*> v = get_datasets_from_indata();
    for (vector<DataWithSum*>::const_iterator i = v.begin(); i != v.end(); ++i)
        (*i)->get_data()->transform(t); 
    outdated_plot=true; 
}

void do_reset(char const*, char const*)   { AL->reset_all(); }

void do_dump(char const*, char const*)   { AL->dump_all_as_script(t); }

void do_commands_logging(char const*, char const*)
{
    if (t == "/dev/null")
        getUI()->stopLog();
    else
        getUI()->startLog(t, with_plus);
}

void do_commands_print(char const*, char const*)
{
    vector<string> cc 
        = getUI()->getCommands().get_commands(tmp_int, tmp_int2, with_plus);
    string text = join_vector(cc, "\n");
    if (t.empty())
        mesg(text);
    else {
        ofstream f;
        f.open(t.c_str(), ios::app);
        if (!f) 
            throw ExecuteError("Can't open file for writing: " + t);
        f << text << endl;
    }
}

void do_exec_file(char const*, char const*) 
{ 
    vector<pair<int,int> > vpn;
    for (int i = 0; i < size(vn); i+=2)
        vpn.push_back(make_pair(vn[i],vn[i+1]));
    getUI()->execScript(t, vpn); 
}

void do_set(char const*, char const*) { getSettings()->setp(t2, t); }

void do_set_show(char const*, char const*)  { mesg(getSettings()->infop(t2)); }

void do_sleep(char const*, char const*) { getUI()->wait(tmp_real); }

} //namespace

template <typename ScannerT>
Cmd3Grammar::definition<ScannerT>::definition(Cmd3Grammar const& /*self*/)
{
    //these static constants for assign_a are workaround for assign_a
    //problems, as proposed by Joao Abecasis at Spirit-general ML
    //Message-ID: <435FB3DD.8030205@gmail.com>
    //Subject: [Spirit-general] Re: weird assign_a(x,y) problem
    static const bool true_ = true;
    static const bool false_ = false;
    static const int zero = 0;
    static const int int_max = INT_MAX;
    static const char *empty = "";


    in_data
        = eps_p [clear_a(vds)]
        >> !("in" >> (lexeme_d['@' >> uint_p [push_back_a(vds)]
                               ]
                       % ','
                     | str_p("@*") [push_back_a(vds, all_datasets)]
                     )
            )
        ;

    optional_plus
        = str_p("+") [assign_a(with_plus, true_)] 
        | eps_p [assign_a(with_plus, false_)] 
        ;

    compact_str
        = lexeme_d['\'' >> (+~ch_p('\''))[assign_a(t)] 
                   >> '\'']
        | lexeme_d[+chset<>(anychar_p - chset<>(" \t\n\r;,"))] [assign_a(t)]
        ;

    int_range
        = '[' >> (int_p[assign_a(tmp_int)] 
                 | eps_p[assign_a(tmp_int, zero)]
                 )
              >> (':'
                  >> (int_p[assign_a(tmp_int2)] 
                     | eps_p[assign_a(tmp_int2, int_max)]
                     )
                  >> ']'
                 | ch_p(']')[assign_a(tmp_int2, tmp_int)]
                        [increment_a(tmp_int2)] //see assign_a error above
                 )  
        ;

    commands_arg
        = eps_p [assign_a(t, empty)] 
          >> optional_plus
          >> ( (ch_p('>') >> compact_str) [&do_commands_logging] 
             | (int_range 
                 >> !(ch_p('>') >> compact_str)) [&do_commands_print]
             | (ch_p('<') [clear_a(vn)]
                 >> compact_str 
                 >> *(int_range[push_back_a(vn, tmp_int)]
                                             [push_back_a(vn, tmp_int2)])
               ) [&do_exec_file]
             ) 
        ;

    set_arg
        = (+(lower_p | '-'))[assign_a(t2)]
          >> ('=' >> compact_str[&do_set]
             | eps_p[&do_set_show]
             )
        ;


    statement 
        = str_p("reset") [&do_reset]
        | ("sleep" >> ureal_p[assign_a(tmp_real)])[&do_sleep]
        | (str_p("dump") >> '>' >> compact_str)[&do_dump]
        | (no_actions_d[DataTransformG][assign_a(t)] >> in_data)[&do_transform]
        | optional_suffix_p("s","et") >> (set_arg % ',')
        | optional_suffix_p("c","ommands") >> commands_arg
        ;
}

template Cmd3Grammar::definition<scanner<char const*, scanner_policies<skipper_iteration_policy<iteration_policy>, match_policy, no_actions_action_policy<action_policy> > > >::definition(Cmd3Grammar const&);

template Cmd3Grammar::definition<scanner<char const*, scanner_policies<skipper_iteration_policy<iteration_policy>, match_policy, action_policy> > >::definition(Cmd3Grammar const&);

Cmd3Grammar cmd3G;




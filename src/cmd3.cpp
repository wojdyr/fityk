// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
// $Id$

/// big grammars in Spirit take a lot of time and memory to compile
/// so they must be splitted into separate compilation units
/// that's the only reason why this file is not a part of cmd.cpp

#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_assign_actor.hpp>
#include <boost/spirit/include/classic_push_back_actor.hpp>
#include <boost/spirit/include/classic_clear_actor.hpp>
#include <boost/spirit/include/classic_chset.hpp>
#include <boost/spirit/include/classic_chset_operators.hpp>
#include <boost/spirit/include/classic_utility.hpp>

#include "cmd3.h"
#include "cmd2.h"
#include "optional_suffix.h"
#include "logic.h"
#include "data.h"
#include "ui.h"
#include "fit.h"
#include "settings.h"
#include "datatrans.h"
#include "stdio.h"

#ifdef _MSC_VER
#define popen _popen
#define pclose _pclose
#define HAVE_POPEN
#endif


using namespace std;
using namespace cmdgram;

namespace {

void do_transform(char const*, char const*)  {
    vector<DataAndModel*> v = get_datasets_from_indata();
    for (vector<DataAndModel*>::const_iterator i = v.begin(); i != v.end(); ++i)
        (*i)->data()->transform(t);
    AL->outdated_plot();
}

void do_reset(char const*, char const*)
{
    AL->reset();
    AL->outdated_plot();
}

void do_dump(char const*, char const*)  { AL->dump_all_as_script(t); }

void do_commands_logging(char const*, char const*)
{
    if (t == "/dev/null")
        AL->get_ui()->stop_log();
    else
        AL->get_ui()->start_log(t, with_plus);
}

void do_exec_file(char const*, char const*)
{
    AL->get_ui()->exec_script(t);
}

void do_exec_prog_output(char const* a, char const* b)
{
    string s(a, b);
    FILE* f = NULL;
#ifdef HAVE_POPEN
    f = popen(s.c_str(), "r");
#else
    AL->warn ("popen() was disabled during compilation.");
#endif

    if (!f)
        return;

    AL->get_ui()->exec_stream(f);
#ifdef HAVE_POPEN
    pclose(f);
#endif
}

void do_fit(char const*, char const*)
{
    if (with_plus) {
        if (!vds.empty())
            throw ExecuteError("No need to specify datasets to continue fit.");
        AL->get_fit()->continue_fit(tmp_int);
    }
    else
        AL->get_fit()->fit(tmp_int, get_datasets_from_indata());
    AL->outdated_plot();
}

void do_load_fit_history(int n)
{
    AL->get_fit_container()->load_param_history(n);
    AL->outdated_plot();
}

void do_clear_fit_history(char const*, char const*)
{
    AL->get_fit_container()->clear_param_history();
}

void do_undo_fit(char const*, char const*)
{
    AL->get_fit_container()->load_param_history(-1, true);
    AL->outdated_plot();
}

void do_redo_fit(char const*, char const*)
{
    AL->get_fit_container()->load_param_history(+1, true);
    AL->outdated_plot();
}


void do_set(char const*, char const*) { AL->get_settings()->setp(t2, t); }

void do_set_show(char const*, char const*)
                                   { AL->rmsg(AL->get_settings()->infop(t2)); }

void do_sleep(char const*, char const*) { AL->get_ui()->wait(tmp_real); }

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
    static const int minus_one = -1;
    static const char *empty = "";

    optional_plus
        = str_p("+") [assign_a(with_plus, true_)]
        | eps_p [assign_a(with_plus, false_)]
        ;

    commands_arg
        = eps_p [assign_a(t, empty)]
          >> optional_plus
          >> ( (ch_p('>') >> CompactStrG) [&do_commands_logging]
             | (ch_p('<') >> CompactStrG) [&do_exec_file]
             | ch_p('!') >> (+~ch_p('\n')) [&do_exec_prog_output]
             )
        ;

    set_arg
        = (+chset_p("a-z-")) [assign_a(t2)]
          >> ('=' >> CompactStrG [&do_set]
             | eps_p[&do_set_show]
             )
        ;

    fit_arg
        = optional_plus
          >> ( uint_p[assign_a(tmp_int)]
             | eps_p[assign_a(tmp_int, minus_one)]
             )
          //TODO >> !("only" >>  (%name | $name)[push_back_a()]
          //                                  % ',')
          //     >> !("not" >>   (%name | $name)[push_back_a()]
          //                                  % ',')
          >> InDataG
        ;


    statement
        = str_p("reset") [&do_reset]
        | ("sleep" >> ureal_p[assign_a(tmp_real)])[&do_sleep]
        | (str_p("dump") >> '>' >> CompactStrG)[&do_dump]
        | (no_actions_d[DataTransformG][assign_a(t)] >> InDataG)[&do_transform]
        | optional_suffix_p("s","et") >> (set_arg % ',')
        | optional_suffix_p("c","ommands") >> commands_arg
        | optional_suffix_p("f","it")
          >> (optional_suffix_p("h","istory")
               >> (int_p [&do_load_fit_history]
                  | optional_suffix_p("c","lear") [&do_clear_fit_history]
                  )
             | optional_suffix_p("u","ndo") [&do_undo_fit]
             | optional_suffix_p("r","edo") [&do_redo_fit]
             | fit_arg [&do_fit]
             )
        ;
}

template Cmd3Grammar::definition<scanner<char const*, scanner_policies<skipper_iteration_policy<iteration_policy>, match_policy, no_actions_action_policy<action_policy> > > >::definition(Cmd3Grammar const&);

template Cmd3Grammar::definition<scanner<char const*, scanner_policies<skipper_iteration_policy<iteration_policy>, match_policy, action_policy> > >::definition(Cmd3Grammar const&);

Cmd3Grammar cmd3G;




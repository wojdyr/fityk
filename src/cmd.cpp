// This file is part of fityk program. Copyright (C) 2005 Marcin Wojdyr
// $Id$

#include <boost/spirit/core.hpp>
#include <boost/spirit/actor/assign_actor.hpp>
#include <boost/spirit/actor/push_back_actor.hpp>
#include <boost/spirit/actor/clear_actor.hpp>
#include <boost/spirit/utility/chset.hpp>
//#include <boost/spirit/utility/chset_operators.hpp>
#include <stdlib.h>
#include <utility>
#include <algorithm>
#include <limits.h>

#include "cmd.h"
#include "cmd2.h"
#include "cmd3.h"
#include "ui.h"
#include "var.h"
#include "sum.h"
#include "func.h"
#include "logic.h"
#include "settings.h"
#include "optional_suffix.h"

using namespace std;
using namespace boost::spirit;

using namespace cmdgram;

namespace {

void do_assign_var(char const* a, char const* b) 
{ 
    AL->assign_variable(string(t, 1), string(a,b)); 
    outdated_plot=true;  //TODO only if...
}

void do_assign_func(char const*, char const*)
{
    t = AL->assign_func(t2, t, vt);
    outdated_plot=true;  //TODO only if function in @active
}

void do_assign_func_copy(char const*, char const*)
{
    t = AL->assign_func_copy(t2, t);
    outdated_plot=true;  //TODO only if function in @active
}


void do_subst_func_param(char const* a, char const* b)
{
    if (t == "F" || t == "Z") { 
        vector<string> const &names = AL->get_sum(ds_pref)->get_names(t[0]);
        for (vector<string>::const_iterator i = names.begin(); 
                                                   i != names.end(); ++i)
            AL->substitute_func_param(*i, t2, string(a,b));
    }
    else 
        AL->substitute_func_param(t, t2, string(a,b));
    outdated_plot=true;  //TODO only if...
}

void do_get_func_by_idx(char const* a, char const*)
{
    //TODO replace it with ApplicationLogic::find_function_any()
    vector<string> const &names = AL->get_sum(ds_pref)->get_names(*a);
    int idx = (tmp_int >= 0 ? tmp_int : tmp_int + names.size());
    if (!is_index(idx, names))
        throw ExecuteError("There is no item with index " + S(tmp_int));
    t = names[idx];
}

void do_assign_fz(char const*, char const*) 
{
    Sum* sum = AL->get_sum(tmp_int2);
    assert(t3 == "F" || t3 == "Z");
    if (!with_plus)
        sum->remove_all_functions_from(t3[0]);
    for (vector<string>::const_iterator i = vr.begin(); i != vr.end(); ++i)
        sum->add_function_to(*i, t3[0]);
    if (!with_plus)
        AL->auto_remove_functions();
    outdated_plot=true;  //TODO only if ds_pref == @active
}

void add_fz_copy(char const* a, char const*)
{
    vector<string> const &names = AL->get_sum(ds_pref)->get_names(*a);
    for (vector<string>::const_iterator i=names.begin(); i != names.end(); ++i)
        vr.push_back(AL->assign_func_copy("", *i));
}

void add_fz_links(char const* a, char const*)
{
    vector<string> const &names = AL->get_sum(ds_pref)->get_names(*a);
    vr.insert(vr.end(), names.begin(), names.end());
}

void do_remove_from_fz(char const* a, char const*)
{
    assert(*a == 'F' || *a == 'Z');
    AL->get_sum(ds_pref)->remove_function_from(t, *a);
    AL->auto_remove_functions();
    outdated_plot=true;  //TODO only if ds_pref == @active
}

void do_delete(char const*, char const*) 
{ 
    if (!vn.empty()) {
        sort(vn.begin(), vn.end());
        reverse(vn.begin(), vn.end());
        for (vector<int>::const_iterator i = vn.begin(); i != vn.end(); ++i) 
            AL->remove_ds(*i);
    }
    AL->delete_funcs_and_vars(vt);
    outdated_plot=true;  //TODO only if...
}

void do_define_func(char const* a, char const* b) 
{ 
    string s = string(a,b);
    UdfContainer::define(s); 
}

void do_undefine_func(char const*, char const*) 
{ 
    for (vector<string>::const_iterator i = vt.begin(); i != vt.end(); ++i) 
        UdfContainer::undefine(*i); 
}


void do_replot(char const*, char const*) 
{ 
    if (outdated_plot)
        getUI()->drawPlot(2); 
    outdated_plot=false;
}

void do_temporary_set(char const*, char const*)
{
    getSettings()->set_temporary(t2, t);
}

void do_temporary_unset(char const*, char const*)
{
    getSettings()->clear_temporary();
}


} //namespace


struct CmdGrammar : public grammar<CmdGrammar>
{
  template <typename ScannerT>
  struct definition
  {
    definition(CmdGrammar const& /*self*/)
    {
        //these static constants for assign_a are workaround for assign_a
        //problems, as proposed by Joao Abecasis at Spirit-general ML
        //Message-ID: <435FB3DD.8030205@gmail.com>
        //Subject: [Spirit-general] Re: weird assign_a(x,y) problem
        static const bool true_ = true;
        static const bool false_ = false;
        static const int minus_one = -1;
        static const char *empty = "";

        compact_str
            = lexeme_d['\'' >> (+~ch_p('\''))[assign_a(t)] 
                       >> '\'']
            | lexeme_d[+chset<>(anychar_p - chset<>(" \t\n\r;,"))] [assign_a(t)]
            ;

        assign_var 
            = VariableLhsG [assign_a(t)]
                      >> '=' >> no_actions_d[FuncG] [&do_assign_var]
            ;

        type_name
            = lexeme_d[(upper_p >> +alnum_p)] 
            ;

        function_param
            = lexeme_d[alpha_p >> *(alnum_p | '_')]
            ;

        subst_func_param
            = ( func_id 
              | ds_prefix >> (str_p("F")|"Z") [assign_a(t)]
              )
              >> "." >> function_param [assign_a(t2)]
              >> "=" >> no_actions_d[FuncG] [&do_subst_func_param]
            ;

        func_id  // stores the function name in `t'
            = FunctionLhsG [assign_a(t)]
            | ds_prefix >> ((str_p("F[")|"Z[") >> int_p[assign_a(tmp_int)] 
              >> ch_p(']')) [&do_get_func_by_idx]
            ;

        new_func_rhs  //assigns function name to `t'
            = type_name [assign_a(t)] 
                  >> str_p("(") [clear_a(vt)] 
                  >> !(
                       (!(function_param >> "=") >> no_actions_d[FuncG])
                                                             [push_back_a(vt)] 
                          % ','
                      )
                  >> str_p(")") [&do_assign_func]
            | "copy(" >> func_id >> str_p(")") [&do_assign_func_copy]
            ;

        assign_func
            = FunctionLhsG [assign_a(t2)] >> '=' >> new_func_rhs
            ;

        assign_fz 
            = ds_prefix [assign_a(tmp_int2, ds_pref)] 
              >> (str_p("F")|"Z") [assign_a(t3)] 
              >> ( str_p("+=") [assign_a(with_plus, true_)]
                 | str_p("=") [assign_a(with_plus, false_)] 
                 ) [clear_a(vr)]
              >> (('0' //nothing
                  | "copy(" >> ds_prefix >> (str_p("F")|"Z") [&add_fz_copy] 
                    >> ")"
                  | ds_prefix >> (str_p("F")|"Z") [&add_fz_links]
                  | eps_p [assign_a(t2, empty)]
                    >> (func_id | new_func_rhs) [push_back_a(vr, t)] 
                  )  % '+') [&do_assign_fz]
            ;

        remove_from_fz
            = ds_prefix >> ((ch_p('F')|'Z') >>  "-=" >> func_id) 
                                                         [&do_remove_from_fz]
            ;
        ds_prefix
            = lexeme_d['@' >> uint_p [assign_a(ds_pref)] 
               >> '.']
            | eps_p [assign_a(ds_pref, minus_one)]
            ;


        define_func
            = optional_suffix_p("def","ine") 
              >> (type_name >> '(' 
                  >> ((function_param >> !('=' >> no_actions_d[FuncG])
                      ) % ',')
                  >> ')' >> '='
                  >> (((type_name >> '('  // CompoundFunction
                      >> (no_actions_d[FuncG]  % ',')
                      >> ')'
                      ) % '+')
                     | no_actions_d[FuncG] //Custom Function
                     )
              ) [&do_define_func]
            ;

        temporary_set 
            = optional_suffix_p("w","ith") 
              >> ((+(lower_p | '-'))[assign_a(t2)]
                  >> '=' >> compact_str[&do_temporary_set]
                 ) % ','
            ;

        statement 
            = !temporary_set >>
            ( (optional_suffix_p("del","ete")[clear_a(vt)][clear_a(vn)] 
                >> ( VariableLhsG [push_back_a(vt)]
                   | func_id [push_back_a(vt, t)]
                   | lexeme_d['@'>>uint_p[push_back_a(vn)]]) % ',') [&do_delete]
            | assign_var 
            | subst_func_param 
            | assign_func 
            | assign_fz
            | remove_from_fz
            | define_func 
            | (optional_suffix_p("undef","ine")[clear_a(vt)] 
               >> type_name[push_back_a(vt)] 
                  % ',')[&do_undefine_func]
            | cmd2G
            | cmd3G
            ) [&do_temporary_unset]
            ;

        multi 
            = (!( (!statement) % ';') 
                    >> !('#' >> *~ch_p('\n'))) [&do_replot]
            ;
    }

    rule<ScannerT> assign_var, type_name, func_id, new_func_rhs, assign_func, 
                   function_param, subst_func_param, 
                   assign_fz, remove_from_fz, define_func,
                   ds_prefix, compact_str, temporary_set, statement, multi;  

    rule<ScannerT> const& start() const { return multi; }
  };
} cmdG;


bool check_command_syntax(string const& str)
{
    if (strip_string(str) == "quit")
        return true;
    parse_info<> result = parse(str.c_str(), no_actions_d[cmdG], space_p);
    return result.full;
}

Commands::Status parse_and_execute(string const& str)
{
    if (strip_string(str) == "quit")
        throw ExitRequestedException();
    try {
        parse_info<> result = parse(str.c_str(), no_actions_d[cmdG], space_p);
        if (result.full) {
            parse(str.c_str(), cmdG, space_p);
            return Commands::status_ok;
        }
        else {
            warn("Syntax error.");
            return Commands::status_syntax_error;
        }
    } catch (ExecuteError &e) {
        getSettings()->clear_temporary();
        warn(string("Error: ") + e.what());
        return Commands::status_execute_error;
    }
}



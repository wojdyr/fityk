// This file is part of fityk program. Copyright (C) 2005 Marcin Wojdyr
// $Id$

#include <boost/spirit/core.hpp>
#include <boost/spirit/actor/assign_actor.hpp>
#include <boost/spirit/actor/push_back_actor.hpp>
#include <boost/spirit/actor/clear_actor.hpp>
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
    t = AL->assign_func(t, t2, vt);
    vt = vector1(t); //for do_put_function()
    outdated_plot=true;  //TODO only if function in @active
}

void do_assign_func_copy(char const*, char const*)
{
    t = AL->assign_func_copy(t, t2);
    vt = vector1(t); //for do_put_function()
    outdated_plot=true;  //TODO only if function in @active
}


void do_subst_func_param(char const* a, char const* b)
{
    if (t == "F" || t == "Z") {
        Sum const* sum = AL->get_sum(ds_pref);
        vector<string> const &names = (t == "F" ? sum->get_ff_names() 
                                                : sum->get_zz_names());
        for (vector<string>::const_iterator i = names.begin(); 
                                                   i != names.end(); ++i)
            AL->substitute_func_param(*i, t2, string(a,b));
    }
    else 
        AL->substitute_func_param(t, t2, string(a,b));
    outdated_plot=true;  //TODO only if...
}

void do_put_function(char const* a, char const*)
{
    for (vector<string>::const_iterator i = vt.begin(); i != vt.end(); ++i)
        AL->get_sum(ds_pref)->add_function_to(*i, *a);
    outdated_plot=true;  //TODO only if...
}

void do_fz_assign(char const*, char const*)
{
    Sum* sum = AL->get_sum(ds_pref);
    sum->remove_all_functions_from(t[0]);
    for (vector<string>::const_iterator i = vt.begin(); i != vt.end(); ++i)
        sum->add_function_to(*i, t[0]);
    if (!t2.empty()) {
        assert(t2 == "F" || t2 == "Z");
        Sum const* from_sum = AL->get_sum(tmp_int);
        vector<string> const &names = (t2 == "F" ? from_sum->get_ff_names() 
                                                 : from_sum->get_zz_names());
        for (vector<string>::const_iterator i = names.begin(); 
                                                   i != names.end(); ++i) {
            sum->add_function_to(deep_cp ? AL->assign_func_copy("", *i) : *i, 
                                 t[0]);
        }
    }
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
    CompoundFunction::define(s); 
}

void do_replot(char const*, char const*) 
{ 
    if (outdated_plot)
        getUI()->drawPlot(2); 
    outdated_plot=false;
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

        assign_var 
            = VariableLhsG [assign_a(t)]
                      >> '=' >> no_actions_d[FuncG] [&do_assign_var]
            ;

        type_name
            = lexeme_d[(upper_p >> *alnum_p)] 
            ;

        function_param
            = lexeme_d[alpha_p >> *(alnum_p | '_')]
            ;

        assign_func
            = (FunctionLhsG [assign_a(t)] >> '=' 
              | eps_p [assign_a(t, empty)]
              )
              >> (type_name [assign_a(t2)]
                  >> str_p("(")[clear_a(vt)] 
                  >> !(
                       (!(function_param >> "=") >> no_actions_d[FuncG])
                                                             [push_back_a(vt)] 
                          % ','
                      )
                  >> str_p(")") [&do_assign_func]
                 | str_p("copy(") >> FunctionLhsG [assign_a(t2)] 
                   >> str_p(")") [&do_assign_func_copy]
                 )
              >> !put_func_to
            ;

        subst_func_param
            = (ds_prefix >> (ch_p('F')|'Z')[assign_a(t)]
              | FunctionLhsG [assign_a(t)]
              )
              >> "[" >> function_param [assign_a(t2)]
              >> "]" >> "="
              >> no_actions_d[FuncG][&do_subst_func_param]
            ;

        put_function
            = FunctionLhsG[clear_a(vt)] [push_back_a(vt)] 
              >> *("," >> FunctionLhsG [push_back_a(vt)])
              >> put_func_to
            ;

        put_func_to
            = "->" >> ds_prefix >> (ch_p('F')|'Z'|'N')[&do_put_function]
            ;

        fz_assign
            = ds_prefix >> (ch_p('F')|'Z') [assign_a(t)] 
              >> ch_p('=') [clear_a(vt)] [assign_a(t2, empty)]
              >> ("copy(" >> lexeme_d['@' >> uint_p [assign_a(tmp_int)]
                                      >> '.' >> (ch_p('F')|'Z') [assign_a(t2)]
                                     ] 
                    >> ch_p(')') [assign_a(deep_cp, true_)]
                 |  lexeme_d['@' >> uint_p [assign_a(tmp_int)]
                             >> '.' >> (ch_p('F')|'Z') [assign_a(t2)]
                            ] [assign_a(deep_cp, false_)]
                 | (FunctionLhsG [push_back_a(vt)] 
                     % ',')
                 | '0'
                 ) [&do_fz_assign]
            ;

        ds_prefix
            = lexeme_d['@' >> uint_p [assign_a(ds_pref)] 
               >> '.']
            | eps_p [assign_a(ds_pref, minus_one)]
            ;


        define_func
            = optional_suffix_p("def","ine") 
              >> (type_name >> '(' 
                  >> ((function_param >> !('=' >> function_param 
                                           >> !('*'>>ureal_p))
                      ) % ',')
                  >> ')' >> '='
                  >> ((type_name >> '('
                       >> (no_actions_d[FuncG]  % ',')
                       >> ')'
                      ) % '+')
                  ) [&do_define_func]
            ;

        statement 
            = (optional_suffix_p("del","ete")[clear_a(vt)][clear_a(vn)] 
                >> ( VariableLhsG [push_back_a(vt)]
                   | FunctionLhsG [push_back_a(vt)]
                   | lexeme_d['@'>>uint_p[push_back_a(vn)]]) % ',') [&do_delete]
            | assign_var 
            | subst_func_param 
            | assign_func 
            | put_function
            | fz_assign
            | define_func 
            | cmd2G
            | cmd3G
            ;

        multi 
            = (!( (!statement) % ';') 
                    >> !('#' >> *~ch_p('\n'))) [&do_replot]
            ;
    }

    rule<ScannerT> assign_var, type_name, assign_func, 
                   function_param, subst_func_param, put_function, 
                   put_func_to, fz_assign, define_func,
                   ds_prefix, statement, multi;  

    rule<ScannerT> const& start() const { return multi; }
  };
} cmdG;


void parse_and_execute(string const& str)
{
    if (strip_string(str) == "quit")
        throw ExitRequestedException();
    try {
        parse_info<> result = parse(str.c_str(), no_actions_d[cmdG], space_p);
        if (result.full) {
            parse_info<> result = parse(str.c_str(), cmdG, space_p);
        }
        else {
            warn("Syntax error.");
        }
    } catch (ExecuteError &e) {
        warn(string("Error: ") + e.what());
    }
}



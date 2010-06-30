// This file is part of fityk program. Copyright (C) 2005 Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
// $Id$

#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_assign_actor.hpp>
#include <boost/spirit/include/classic_push_back_actor.hpp>
#include <boost/spirit/include/classic_clear_actor.hpp>
#include <boost/spirit/include/classic_chset.hpp>
//#include <boost/spirit/include/classic_chset_operators.hpp>

#include <stdlib.h>
#include <utility>
#include <algorithm>
#include <limits.h>

#include "cmd.h"
#include "cmd2.h"
#include "cmd3.h"
#include "ui.h"
#include "var.h"
#include "model.h"
#include "func.h"
#include "udf.h"
#include "logic.h"
#include "settings.h"
#include "optional_suffix.h"
#include "datatrans.h"

using namespace std;
using namespace boost::spirit::classic;

using namespace cmdgram;

namespace {

void do_assign_var(char const* a, char const* b)
{
    AL->assign_variable(string(t, 1), string(a,b));
    AL->use_parameters();
    AL->outdated_plot();  //TODO only if...
}

void do_assign_func(char const*, char const*)
{
    t = AL->assign_func(t2, t, vt);
    AL->use_parameters();
    AL->outdated_plot(); //TODO only if function in @active
}

void do_assign_func_copy(char const*, char const*)
{
    t = AL->assign_func_copy(t2, t);
    AL->use_parameters();
    AL->outdated_plot(); //TODO only if function in @active
}


void do_subst_func_param(char const* a, char const* b)
{
    if (t == "F" || t == "Z") {
        vector<string> const &names =
                AL->get_model(dm_pref)->get_names(Model::parse_funcset(t[0]));
        for (vector<string>::const_iterator i = names.begin();
                                                   i != names.end(); ++i)
            AL->substitute_func_param(*i, t2, string(a,b));
    }
    else
        AL->substitute_func_param(t, t2, string(a,b));
    AL->use_parameters();
    AL->outdated_plot(); //TODO only if...
}

void do_get_func_by_idx(char const* a, char const*)
{
    //TODO replace it with ApplicationLogic::find_function_any()
    vector<string> const &names =
            AL->get_model(dm_pref)->get_names(Model::parse_funcset(*a));
    int idx = (tmp_int >= 0 ? tmp_int : tmp_int + names.size());
    if (!is_index(idx, names))
        throw ExecuteError("There is no item with index " + S(tmp_int));
    t = names[idx];
}

void do_assign_fz(char const*, char const*)
{
    Model* model = AL->get_model(tmp_int2);
    Model::FuncSet fz = Model::parse_funcset(t3[0]);
    bool remove = (!with_plus && !model->get_names(fz).empty());
    if (remove)
        model->remove_all_functions_from(fz);
    for (vector<string>::const_iterator i = vr.begin(); i != vr.end(); ++i)
        model->add_function_to(*i, fz);
    if (remove)
        AL->auto_remove_functions();
    AL->outdated_plot(); //TODO only if dm_pref == @active
}

void add_fz_copy(char const* a, char const*)
{
    Model::FuncSet fz = Model::parse_funcset(*a);
    vector<string> const &names = AL->get_model(dm_pref)->get_names(fz);
    for (vector<string>::const_iterator i=names.begin(); i != names.end(); ++i)
        vr.push_back(AL->assign_func_copy("", *i));
}

void add_fz_links(char const* a, char const*)
{
    Model::FuncSet fz = Model::parse_funcset(*a);
    vector<string> const &names = AL->get_model(dm_pref)->get_names(fz);
    vr.insert(vr.end(), names.begin(), names.end());
}

void do_remove_from_fz(char const* a, char const*)
{
    Model::FuncSet fz = Model::parse_funcset(*a);
    AL->get_model(dm_pref)->remove_function_from(t, fz);
    AL->auto_remove_functions();
    AL->outdated_plot(); //TODO only if dm_pref == @active
}

void do_delete(char const*, char const*)
{
    // delete datasets
    if (!vn.empty()) {
        sort(vn.begin(), vn.end());
        reverse(vn.begin(), vn.end());
        for (vector<int>::const_iterator i = vn.begin(); i != vn.end(); ++i)
            AL->remove_dm(*i);
    }

    // delete functions/variables
    vector<string> vars, funcs;
    for (vector<string>::const_iterator i = vt.begin(); i != vt.end(); ++i) {
        if ((*i)[0] == '$')
            vars.push_back(string(*i, 1));
        else if ((*i)[0] == '%')
            funcs.push_back(string(*i, 1));
    }
    AL->delete_funcs(funcs);
    AL->delete_variables(vars);

    // delete data points
    if (vt.size() == 1 && vt[0][0] == '(') {
        vector<DataAndModel*> dsds = get_datasets_from_indata();
        for (vector<DataAndModel*>::const_iterator i = dsds.begin();
                                                        i != dsds.end(); ++i)
            (*i)->data()->delete_points(vt[0]);
    }

    AL->outdated_plot(); //TODO only if...
}

void do_quit(char const*, char const*) { throw ExitRequestedException(); }

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
    if (AL->is_plot_outdated()) {
        AL->get_ui()->draw_plot(2, UserInterface::kRepaint);
    }
}

void do_temporary_set(char const*, char const*)
{
    AL->get_settings()->set_temporary(t2, t);
}

void do_temporary_unset(char const*, char const*)
{
    AL->get_settings()->clear_temporary();
}

void do_system(char const* a, char const* b)
{
    string s(a, b);
    system(s.c_str());
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
            = lexeme_d[(upper_p >> +alnum_p)]
            ;

        function_param
            = lexeme_d[alpha_p >> *(alnum_p | '_')]
            ;

        subst_func_param
            = ( func_id
              | dm_prefix >> (str_p("F")|"Z") [assign_a(t)]
              )
              >> "." >> function_param [assign_a(t2)]
              >> "=" >> no_actions_d[FuncG] [&do_subst_func_param]
            ;

        func_id  // stores the function name in `t'
            = FunctionLhsG [assign_a(t)]
            | dm_prefix >> ((str_p("F[")|"Z[") >> int_p[assign_a(tmp_int)]
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
            = dm_prefix [assign_a(tmp_int2, dm_pref)]
              >> (str_p("F")|"Z") [assign_a(t3)]
              >> ( str_p("+=") [assign_a(with_plus, true_)]
                 | str_p("=") [assign_a(with_plus, false_)]
                 ) [clear_a(vr)]
              >> (('0' //nothing
                  | eps_p [assign_a(t2, empty)]
                    >> (func_id | new_func_rhs) [push_back_a(vr, t)]
                  | "copy(" >> dm_prefix >> (str_p("F")|"Z") [&add_fz_copy]
                    >> ")"
                  | dm_prefix >> (str_p("F")|"Z") [&add_fz_links]
                  )  % '+') [&do_assign_fz]
            ;

        remove_from_fz
            = dm_prefix >> ((ch_p('F')|'Z') >>  "-=" >> func_id)
                                                         [&do_remove_from_fz]
            ;
        dm_prefix
            = lexeme_d['@' >> uint_p [assign_a(dm_pref)]
               >> '.']
            | eps_p [assign_a(dm_pref, minus_one)]
            ;


        define_func
            = optional_suffix_p("def","ine")
              >> (type_name >> '('
                  >> !((function_param >> !('=' >> no_actions_d[FuncG])
                       ) % ',')
                  >> ')' >> '='
                  >> (((type_name >> '('  // CompoundFunction
                      >> (no_actions_d[FuncG]  % ',')
                      >> ')'
                      ) % '+')
                     | str_p("x") >> str_p("<") >> +~chset_p("\n;#")
                     | no_actions_d[FuncG] //Custom Function
                       >> !("where"
                            >> (function_param >> '=' >> no_actions_d[FuncG])
                                % ','
                           )
                     )
              ) [&do_define_func]
            ;

        temporary_set
            = optional_suffix_p("w","ith")
              >> ((+(lower_p | '_'))[assign_a(t2)]
                  >> '=' >> CompactStrG[&do_temporary_set]
                 ) % ','
            ;

        statement
            = !temporary_set
            >> ( (optional_suffix_p("del","ete")[clear_a(vt)][clear_a(vn)]
                  >> (( lexeme_d[(ch_p('%')|'$') >> +(alnum_p|'_'|'*')]
                                                              [push_back_a(vt)]
                      | func_id [push_back_a(vt, t)]
                      | lexeme_d['@' >> uint_p[push_back_a(vn)]]
                      ) % ','
                     | ('(' >> DataExpressionG >> ')') [push_back_a(vt)]
                       >> InDataG)
                     ) [&do_delete]
               | str_p("quit") [&do_quit]
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
               | '!' >> (+~ch_p('\n')) [&do_system]
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
                   dm_prefix, temporary_set, statement, multi;

    rule<ScannerT> const& start() const { return multi; }
  };
} cmdG;


bool check_command_syntax(string const& str)
{
    return parse(str.c_str(), no_actions_d[cmdG >> end_p], space_p).full;
}

bool parse_and_execute_e(string const& str)
{
    bool r = check_command_syntax(str);
    if (r)
        parse(str.c_str(), cmdG, space_p);
    return r;
}

Commands::Status parse_and_execute(string const& str)
{
    try {
        bool r = parse_and_execute_e(str);
        if (r)
            return Commands::status_ok;
        else {
            AL->warn("Syntax error.");
            return Commands::status_syntax_error;
        }
    } catch (ExecuteError &e) {
        AL->get_settings()->clear_temporary();
        AL->warn(string("Error: ") + e.what());
        return Commands::status_execute_error;
    }
}


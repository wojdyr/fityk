// This file is part of fityk program. Copyright (C) 2005 Marcin Wojdyr
// $Id$

#include "cmd.h"
#include "ui.h"
#include "data.h"
#include "datatrans.h"
#include "var.h"
#include "func.h"
#include "logic.h"
#include <boost/spirit/core.hpp>
#include <boost/spirit/actor/assign_actor.hpp>
#include <boost/spirit/actor/push_back_actor.hpp>
#include <boost/spirit/actor/clear_actor.hpp>
#include <stdlib.h>

using namespace std;
using namespace boost::spirit;

namespace {

bool extended_print;
string t, t2;
int tmp_int;
vector<string> vt;
vector<int> vn;

void set_data_title(char const*, char const*)  { my_data->title = t; }

void do_transform(char const* a, char const* b)  
                                         { my_data->transform(string(a,b)); }

void do_assign_var(char const* a, char const* b) 
               { AL->assign_variable(string(t, 1), string(a,b)); }

void do_assign_func(char const*, char const*)
{
   AL->assign_func(string(t,1), t2, vt);
}

void do_change_func_param(char const* a, char const* b)
{
    AL->substitute_func_param(string(t,1), t2, string(a,b));
}

void do_delete(char const*, char const*) 
{ 
    AL->remove_ds(vn);
    AL->delete_funcs_and_vars(vt);
}

void do_print(char const* a, char const* b)
{
    string s = string(a,b);
    string m;
    vector<Variable*> const &variables = AL->get_variables(); 
    vector<Function*> const &functions = AL->get_functions(); 
    if (s == "variables") {
        m = "Defined variables: ";
        for (vector<Variable*>::const_iterator i = variables.begin(); 
                i != variables.end(); ++i)
            if (extended_print)
                m += "\n" + (*i)->get_info(AL->get_parameters(), false);
            else
                if ((*i)->is_visible())
                    m += (*i)->xname + " ";
    }
    else if (s[0] == '$') {
        const Variable* v = AL->find_variable(string(s, 1));
        m = v ? v->get_info(AL->get_parameters(), extended_print) 
              : "Undefined variable: " + s;
    }
    if (s == "functions") {
        m = "Defined functions: ";
        for (vector<Function*>::const_iterator i = functions.begin(); 
                i != functions.end(); ++i)
            if (extended_print)
                m += "\n" + (*i)->get_info(variables, AL->get_parameters());
            else
                m += (*i)->xname + " ";
    }
    else if (s[0] == '%') {
        const Function* f = AL->find_function(string(s, 1));
        m = f ? f->get_info(variables, AL->get_parameters(), extended_print) 
              : "Undefined function: " + s;
    }
    else if (s[0] == '@') {
        int k = (s[1]=='.' ? AL->get_active_ds_position() : atoi(s.c_str()+1));
        if (k >= 0 && k < AL->get_ds_count())
            m = AL->get_ds(k)->get_data()->getInfo();
        else
            m = "There is no dataset: " + s;
    }
    mesg(m);
}


void do_print_func_value(char const*, char const*)
{
    string m;
    const Function* f = AL->find_function(string(t, 1));
    if (f) {
        fp x = get_transform_expression_value(t2);
        AL->use_parameters();
        fp y = f->calculate_value(x);
        m = f->xname + "(" + S(x) + ") = " + S(y);
        if (extended_print) {
            //TODO? derivatives
        }
    }
    else
      m = "Undefined function: " + t;
    mesg(m);
}

void do_import_dataset(char const*, char const*)
{
    if (tmp_int == -1) {
        auto_ptr<Data> data(new Data);
        data->load_file(t, 0, vector<int>()); //TODO columns, type
        tmp_int = AL->append_ds(data.release());
    }
    else {
        //TODO columns, type
        AL->get_active_ds()->get_data()->load_file(t, 0, vector<int>()); 
    }
}

void do_select_data(char const*, char const*)
{
    if (tmp_int == -1) 
        tmp_int = AL->append_ds();
    AL->activate_ds(tmp_int);
}

void do_load_data_sum(char const*, char const*)
{
    //TODO
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
        static const int new_dataset = -1;

        transform 
            = "title">>ch_p('=') >> lexeme_d['"' >> (+~ch_p('"'))[assign_a(t)] 
                                             >> '"']  [&set_data_title]
            | no_actions_d[DataTransformG][&do_transform] 
            ;

        assign_var 
            = VariableLhsG [assign_a(t)]
                      >> '=' >> no_actions_d[VariableRhsG] [&do_assign_var]
            ;

        function_name
            = lexeme_d[(upper_p >> *alnum_p)] [assign_a(t2)]
            ;

        assign_func
            = FunctionLhsG [assign_a(t)]
              >> '=' 
              >> function_name 
              >> str_p("(")[clear_a(vt)] 
              >> !(no_actions_d[VariableRhsG][push_back_a(vt)] 
                   % ',')
              >> str_p(")")[&do_assign_func]
            ;

        subst_func_param
            = FunctionLhsG [assign_a(t)]
              >> "[" >> (+(alnum_p | '_')) [assign_a(t2)]
              >> "]" >> "="
              >> no_actions_d[VariableRhsG][&do_change_func_param]
            ;

        filename_str
            = lexeme_d['\'' >> (+~ch_p('\''))[assign_a(t)] 
                       >> '\'']
                      | (+~space_p) [assign_a(t)]
            ;

        dataset_nr
            = lexeme_d['@' >> (uint_p [assign_a(tmp_int)]
                              | ch_p('*') [assign_a(tmp_int, new_dataset)]
                              )
                      ]
            ;

        dataset_sum
            = lexeme_d['@' >> uint_p[clear_a(vn)][push_back_a(vn)]]
              >> *('+' >> lexeme_d['@' >> uint_p[push_back_a(vn)]])
            ;

        dataset_handling
            = (dataset_nr >> '<' >> filename_str)[&do_import_dataset]
                                                             [&do_select_data]
            | (filename_str >> '>' >> dataset_nr)[&do_import_dataset]
            | (dataset_nr >> '<' >> dataset_sum)[&do_load_data_sum]
                                                             [&do_select_data]
            | (dataset_sum >> '>' >> dataset_nr)[&do_load_data_sum] 
            | dataset_nr[&do_select_data]
            ;

        print_arg
            = str_p("variables") [&do_print]
            | VariableLhsG [&do_print]
            | str_p("functions")[&do_print]
            | (FunctionLhsG[assign_a(t)] 
               >> "(" 
               >> (+~ch_p(')'))[assign_a(t2)]
               //TODO
               //>> no_actions_d[DataTransformG.use_parser<1>()][assign_a(t2)]
               >> ")")[&do_print_func_value]
            | FunctionLhsG[&do_print]
            | lexeme_d['@' >> (uint_p | '.')][&do_print]
            ;

        statement 
            = transform % ','
            | assign_var % ','
            | assign_func % ','
            | subst_func_param % ','
            | (str_p("delete")[clear_a(vt)][clear_a(vn)] 
                >> ( VariableLhsG [push_back_a(vt)]
                   | FunctionLhsG [push_back_a(vt)]
                   | lexeme_d['@'>>uint_p[push_back_a(vn)]]) % ',') [&do_delete]
            | dataset_handling
            | str_p("print") 
                >> (str_p("-v") [assign_a(extended_print, true_)] 
                   | eps_p[assign_a(extended_print, false_)] 
                   )
                >> (print_arg % ',')
            ;

        multi 
            = statement % ';';
    }

    rule<ScannerT> transform, assign_var, function_name, assign_func, 
                   subst_func_param, dataset_handling, filename_str,
                   dataset_nr, dataset_sum,
                   print_arg, statement, multi;  

    rule<ScannerT> const& start() const { return multi; }
  };
} cmdG;


bool spirit_parser(string const& str)
{
    parse_info<> result = parse(str.c_str(), cmdG, space_p);
    if (result.full) {
    }
    return (bool) result.full;
}



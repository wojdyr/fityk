// This file is part of fityk program. Copyright (C) 2005 Marcin Wojdyr
// $Id$

#include "cmd.h"
#include "ui.h"
#include "data.h"
#include "datatrans.h"
#include "var.h"
#include "sum.h"
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

void do_subst_func_param(char const* a, char const* b)
{
    AL->substitute_func_param(string(t,1), t2, string(a,b));
}

void do_put_function(char const* a, char const* b)
{
    string s(a,b);
    for (vector<string>::const_iterator i = vt.begin(); i != vt.end(); ++i)
        if (s.size() == 1)
            AL->get_active_ds()->get_sum()->add_function_to(string(*i,1), s[0]);
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
        if (v) {
            m = v->get_info(AL->get_parameters(), extended_print);
            if (extended_print) {
                vector<string> refs = AL->get_variable_references(string(s, 1));
                if (!refs.empty())
                    m += "\nreferenced by: " + join_vector(refs, ", ");
            }
        }
        else 
            m = "Undefined variable: " + s;
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
        m = AL->get_data(tmp_int)->getInfo();
    }
    else if (s == "view") {
        m = AL->view.str();
    }
    else if (s == "F") {
        m = "F: "; 
        vector<int> const &idx = AL->get_active_ds()->get_sum()->get_ff_idx();
        for (vector<int>::const_iterator i = idx.begin(); i != idx.end(); ++i){
            Function const* f = functions[*i];
            if (extended_print)
                m += "\n" + f->get_info(variables, AL->get_parameters());
            else
                m += f->xname + " ";
        }
    }
    else if (s == "Z") {
        m = "Z: "; 
        vector<int> const &idx = AL->get_active_ds()->get_sum()->get_zz_idx();
        for (vector<int>::const_iterator i = idx.begin(); i != idx.end(); ++i){
            Function const* f = functions[*i];
            if (extended_print)
                m += "\n" + f->get_info(variables, AL->get_parameters());
            else
                m += f->xname + " ";
        }
    }
    else if (s == "sum-formula") {
        AL->use_parameters();
        m = AL->get_active_ds()->get_sum()->get_formula(!extended_print);
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
        AL->get_data(tmp_int)->load_file(t, 0, vector<int>()); 
    }
}

void do_export_dataset(char const*, char const*)
{
    AL->get_data(tmp_int)->export_to_file(t); 
}

void do_select_data(char const*, char const*)
{
    if (tmp_int == -1) 
        tmp_int = AL->append_ds();
    AL->activate_ds(tmp_int);
}

void do_load_data_sum(char const*, char const*)
{
    //TODO do_load_data_sum
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
            //TODO data tranform with "in @3" or "in @3, @4
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
              >> no_actions_d[VariableRhsG][&do_subst_func_param]
            ;

        put_function
            = FunctionLhsG[clear_a(vt)] [push_back_a(vt)] 
              >> *("," >> FunctionLhsG [push_back_a(vt)])
              >> "->" >> (str_p("F")|"Z"|"N")[&do_put_function]
            ;

        filename_str
            = lexeme_d['\'' >> (+~ch_p('\''))[assign_a(t)] 
                       >> '\'']
                      | (+~space_p) [assign_a(t)]
            ;

        existing_dataset_nr
            = lexeme_d['@' >> (uint_p [assign_a(tmp_int)]
                              | ch_p('.') [assign_a(tmp_int, 
                                                 AL->get_active_ds_position())]
                              )
                      ]
            ;

        dataset_nr
            = existing_dataset_nr
            | str_p("@*") [assign_a(tmp_int, new_dataset)]
            ;

        dataset_sum
            = lexeme_d['@' >> uint_p[clear_a(vn)][push_back_a(vn)]]
              >> *('+' >> lexeme_d['@' >> uint_p[push_back_a(vn)]])
            ;

        dataset_handling
            = (dataset_nr >> '<' >> filename_str) [&do_import_dataset]
                                                             [&do_select_data]
            | (filename_str >> '>' >> dataset_nr) [&do_import_dataset]
            | (dataset_nr >> '<' >> dataset_sum) [&do_load_data_sum]
                                                             [&do_select_data]
            | (dataset_sum >> '>' >> dataset_nr) [&do_load_data_sum] 
            | dataset_nr[&do_select_data]
            | (dataset_nr >> '>' >> filename_str) [&do_export_dataset]
            ;

        print_arg
            = str_p("variables") [&do_print]
            | VariableLhsG [&do_print]
            | str_p("functions")[&do_print]
            | (FunctionLhsG[assign_a(t)] 
               >> "(" 
               >> no_actions_d[DataExpressionG][assign_a(t2)]
               >> ")")[&do_print_func_value]
            | FunctionLhsG[&do_print]
            | existing_dataset_nr[&do_print]
            | str_p("view")[&do_print]
            | (str_p("F")|"Z")[&do_print]
            //TODO F(3.2), Z(2.1)
            | str_p("sum-formula")[&do_print]
            // gnuplot formula... 
            ;

        statement 
            = transform 
            | assign_var 
            | assign_func 
            | subst_func_param 
            | put_function
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
                   subst_func_param, put_function, 
                   dataset_handling, filename_str,
                   existing_dataset_nr, dataset_nr, dataset_sum,
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



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
#include "fit.h"
#include "manipul.h"
#include <boost/spirit/core.hpp>
#include <boost/spirit/actor/assign_actor.hpp>
#include <boost/spirit/actor/push_back_actor.hpp>
#include <boost/spirit/actor/clear_actor.hpp>
#include <stdlib.h>

using namespace std;
using namespace boost::spirit;

namespace {

bool extended_info;
string t, t2;
int tmp_int;
bool tmp_bool;
double tmp_real, tmp_real2;
vector<string> vt;
vector<int> vn;
static const int new_dataset = -1;
static const int active_dataset = -2;
bool outdated_plot = false;

void set_data_title(char const*, char const*)  { my_data->title = t; }

void do_transform(char const* a, char const* b)  { 
    my_data->transform(string(a,b)); 
    outdated_plot=true; 
}

void do_assign_var(char const* a, char const* b) 
{ 
    AL->assign_variable(string(t, 1), string(a,b)); 
    outdated_plot=true;  //TODO only if...
}

void do_assign_func(char const*, char const*)
{
    AL->assign_func(t, t2, vt);
    vt = vector1(t); //for do_put_function()
    outdated_plot=true;  //TODO only if...
}

void do_subst_func_param(char const* a, char const* b)
{
    AL->substitute_func_param(t, t2, string(a,b));
    outdated_plot=true;  //TODO only if...
}

void do_put_function(char const* a, char const* b)
{
    string s(a,b);
    for (vector<string>::const_iterator i = vt.begin(); i != vt.end(); ++i)
        if (s.size() == 1)
            AL->get_active_ds()->get_sum()->add_function_to(*i, s[0]);
    outdated_plot=true;  //TODO only if...
}

void do_delete(char const*, char const*) 
{ 
    AL->remove_ds(vn);
    AL->delete_funcs_and_vars(vt);
    outdated_plot=true;  //TODO only if...
}

void do_print_info(char const* a, char const* b)
{
    string s = string(a,b);
    string m;
    vector<Variable*> const &variables = AL->get_variables(); 
    vector<Function*> const &functions = AL->get_functions(); 
    if (s.empty())
        m = "info about what?";
    if (s == "variables") {
        m = "Defined variables: ";
        for (vector<Variable*>::const_iterator i = variables.begin(); 
                i != variables.end(); ++i)
            if (extended_info)
                m += "\n" + (*i)->get_info(AL->get_parameters(), false);
            else
                if ((*i)->is_visible())
                    m += (*i)->xname + " ";
    }
    else if (s[0] == '$') {
        const Variable* v = AL->find_variable(string(s, 1));
        if (v) {
            m = v->get_info(AL->get_parameters(), extended_info);
            if (extended_info) {
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
            if (extended_info)
                m += "\n" + (*i)->get_info(variables, AL->get_parameters());
            else
                m += (*i)->xname + " ";
    }
    else if (s[0] == '%') {
        const Function* f = AL->find_function(string(s, 1));
        m = f ? f->get_info(variables, AL->get_parameters(), extended_info) 
              : "Undefined function: " + s;
    }
    else if (s == "datasets") {
        m = S(AL->get_ds_count()) + " datasets.";
        if (extended_info)
            for (int i = 0; i < AL->get_ds_count(); ++i)
                m += "\n@" + S(i) + ": " + AL->get_data(i)->get_title();
    }
    else if (s[0] == '@') {
        if (tmp_int == active_dataset)
            tmp_int = AL->get_active_ds_position();
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
            if (extended_info)
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
            if (extended_info)
                m += "\n" + f->get_info(variables, AL->get_parameters());
            else
                m += f->xname + " ";
        }
    }
    else if (s == "sum-formula") {
        m = AL->get_active_ds()->get_sum()->get_formula(!extended_info);
    }
    else if (s == "fit")
        m = my_fit->getInfo();
    else if (s == "errors")
        m = my_fit->getErrorInfo(extended_info);

    mesg(m);
}

void do_print_sum_derivatives_info(char const*, char const*)
{
    fp x = get_transform_expression_value(t);
    Sum const* sum = AL->get_active_ds()->get_sum();
    vector<fp> symb = sum->get_symbolic_derivatives(x);
    vector<fp> num = sum->get_numeric_derivatives(x, 1e-4);
    assert (symb.size() == num.size());
    string m = "F(" + S(x) + ")=" + S(sum->value(x));
    for (int i = 0; i < size(num); ++i) {
        if (is_neq(symb[i], 0) || is_neq(num[i], 0))
            m += "\ndF / d " + AL->find_variable_handling_param(i)->xname 
                + " = (symb.) " + S(symb[i]) + " = (num.) " + S(num[i]);
    }
    mesg(m);
}


void do_print_func_value(char const*, char const*)
{
    string m;
    const Function* f = AL->find_function(t);
    if (f) {
        fp x = get_transform_expression_value(t2);
        fp y = f->calculate_value(x);
        m = f->xname + "(" + S(x) + ") = " + S(y);
        if (extended_info) {
            //TODO? derivatives
        }
    }
    else
      m = "Undefined function: " + t;
    mesg(m);
}

void do_import_dataset(char const*, char const*)
{
    if (tmp_int == new_dataset) {
        auto_ptr<Data> data(new Data);
        data->load_file(t, 0, vector<int>()); //TODO columns, type
        tmp_int = AL->append_ds(data.release());
    }
    else {
        if (tmp_int == active_dataset)
            tmp_int = AL->get_active_ds_position();
        //TODO columns, type
        AL->get_data(tmp_int)->load_file(t, 0, vector<int>()); 
    }
    outdated_plot=true;  //TODO only if...
}

void do_export_dataset(char const*, char const*)
{
    if (tmp_int == active_dataset)
        tmp_int = AL->get_active_ds_position();
    AL->get_data(tmp_int)->export_to_file(t); 
}

void do_select_data(char const*, char const*)
{
    if (tmp_int == new_dataset) 
        tmp_int = AL->append_ds();
    else if (tmp_int == active_dataset)
        tmp_int = AL->get_active_ds_position();
    AL->activate_ds(tmp_int);
    outdated_plot=true;  //TODO only if...
}

void do_load_data_sum(char const*, char const*)
{
    if (tmp_int == active_dataset)
        tmp_int = AL->get_active_ds_position();
    //TODO do_load_data_sum
    outdated_plot=true;  //TODO only if...
}

void do_plot(char const*, char const*)
{
    AL->view.parse_and_set(vt);
    getUI()->drawPlot(1, true);
    outdated_plot=false;
}

void do_replot(char const*, char const*) 
{ 
    if (outdated_plot)
        getUI()->drawPlot(2); 
    outdated_plot=false;
}

void do_fit(char const*, char const*)
{
    my_fit->fit(tmp_bool, tmp_int);
    outdated_plot=true;  //TODO only if...
}

void do_sleep(char const*, char const*)
{
    getUI()->wait(tmp_real);
}

void do_guess(char const*, char const*)
{
    my_manipul->guess_and_add(t, t2, tmp_bool, tmp_real, tmp_real2, vt);
    outdated_plot=true;  //TODO only if...
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
        static const char *dot = ".";
        static const char *empty = "";
        transform 
            //TODO data tranform with "in @3" or "in @3, @4
            = "title">>ch_p('=') >> lexeme_d['"' >> (+~ch_p('"'))[assign_a(t)] 
                                             >> '"']  [&set_data_title]
            | no_actions_d[DataTransformG][&do_transform] 
            ;

        assign_var 
            = VariableLhsG [assign_a(t)]
                      >> '=' >> no_actions_d[FuncG] [&do_assign_var]
            ;

        function_name
            = lexeme_d[(upper_p >> *alnum_p)] 
            ;

        function_param
            = +(alnum_p | '_')
            ;

        assign_func
            = (FunctionLhsG [assign_a(t)] >> '=' 
              | eps_p [assign_a(t, empty)]
              )
              >> function_name [assign_a(t2)]
              >> str_p("(")[clear_a(vt)] 
              >> !((no_actions_d[FuncG][push_back_a(vt)] 
                   % ',')
                  | ((function_param >> "=" >> no_actions_d[FuncG])
                                                              [push_back_a(vt)] 
                   % ',')
                  )
              >> str_p(")")[&do_assign_func]
              >> !("->" >> (str_p("F")|"Z"|"N")[&do_put_function])
            ;

        subst_func_param
            = FunctionLhsG [assign_a(t)]
              >> "[" >> function_param [assign_a(t2)]
              >> "]" >> "="
              >> no_actions_d[FuncG][&do_subst_func_param]
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
                              |eps_p [assign_a(tmp_int, active_dataset)]
                              )
                      ]
            ;

        dataset_nr
            = str_p("@*") [assign_a(tmp_int, new_dataset)]
            | existing_dataset_nr
            ;

        dataset_sum //TODO use existing_dataset_nr here
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
            | (existing_dataset_nr >> '>' >> filename_str) [&do_export_dataset]
            | dataset_nr[&do_select_data]
            ;

        plot_range 
            = (ch_p('[') >> ']') [push_back_a(vt,empty)][push_back_a(vt,empty)]
            | '[' >> (real_p|"."|eps_p) [push_back_a(vt)] 
              >> ':' >> (real_p|"."|eps_p) [push_back_a(vt)] 
              >> ']'  
            | str_p(".") [push_back_a(vt,dot)][push_back_a(vt,dot)] // [.:.]
            | eps_p [push_back_a(vt,empty)][push_back_a(vt,empty)] // [:]
            ;

        info_arg
            = str_p("variables") [&do_print_info]
            | VariableLhsG [&do_print_info]
            | str_p("functions") [&do_print_info]
            | (FunctionLhsG [assign_a(t)] 
               >> "(" 
               >> no_actions_d[DataExpressionG] [assign_a(t2)]
               >> ")") [&do_print_func_value] //TODO -> DataExpressionG
            | FunctionLhsG [&do_print_info]
            | str_p("datasets") [&do_print_info]
            | existing_dataset_nr [&do_print_info]
            | str_p("view") [&do_print_info]
            | (str_p("F")|"Z") [&do_print_info]
            | str_p("sum-formula") [&do_print_info]
            | (str_p("dF") >> '(' 
              >> no_actions_d[DataExpressionG][assign_a(t)] 
              >> ')') [&do_print_sum_derivatives_info]
            | str_p("fit") [&do_print_info]
            | str_p("errors") [&do_print_info]
            ;

        fit
            =
            (str_p("fit") 
             >> (str_p("+")[assign_a(tmp_bool, false_)]
                |eps_p[assign_a(tmp_bool, true_)] 
                )
             >> (uint_p[assign_a(tmp_int)]
                |eps_p[assign_a(tmp_int, minus_one)]
                )
            //TODO [@n, ...] 
            //TODO [only %name, $name, not $name2, ...] 
            //TODO [using method]
            ) [&do_fit]
            ;

        guess
            = (str_p("guess") [clear_a(vt)] [assign_a(t, empty)] 
                               [assign_a(tmp_bool, false_)]
              >> function_name [assign_a(t2)]
              >> !('[' >> real_p [assign_a(tmp_real)][assign_a(tmp_bool,true_)]
                  >> ':' >> real_p [assign_a(tmp_real2)]
                  >> ']') 
              >> !((function_param >> '=' >> no_actions_d[FuncG])
                                                           [push_back_a(vt)]
                   % ',')
              >> !("as" >> FunctionLhsG [assign_a(t)])) [&do_guess]
            ;

        statement 
            = transform 
            | assign_var 
            | subst_func_param 
            | assign_func 
            | put_function
            | (str_p("delete")[clear_a(vt)][clear_a(vn)] 
                >> ( VariableLhsG [push_back_a(vt)]
                   | FunctionLhsG [push_back_a(vt)]
                   | lexeme_d['@'>>uint_p[push_back_a(vn)]]) % ',') [&do_delete]
            | dataset_handling
            | str_p("info") 
                >> (str_p("+") [assign_a(extended_info, true_)] 
                   | eps_p[assign_a(extended_info, false_)] 
                   )
                >> (info_arg % ',')
            | (str_p("plot") [clear_a(vt)] 
              >> plot_range >> plot_range) [&do_plot]
            | fit
            | guess
            | (str_p("sleep") >> ureal_p[assign_a(tmp_real)])[&do_sleep]
            ;

        multi 
            = (statement % ';') [&do_replot];
    }

    rule<ScannerT> transform, assign_var, function_name, assign_func, 
                   function_param, subst_func_param, put_function, 
                   dataset_handling, filename_str, guess,
                   existing_dataset_nr, dataset_nr, dataset_sum,
                   plot_range, info_arg, fit, statement, multi;  

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



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
#include "settings.h"
#include <boost/spirit/core.hpp>
#include <boost/spirit/actor/assign_actor.hpp>
#include <boost/spirit/actor/push_back_actor.hpp>
#include <boost/spirit/actor/clear_actor.hpp>
#include <boost/spirit/actor/increment_actor.hpp>
#include <stdlib.h>
#include <fstream>
#include <utility>
#include <limits.h>

using namespace std;
using namespace boost::spirit;

namespace {

bool with_plus, deep_cp;
string t, t2;
int tmp_int, tmp_int2, ds_pref;
double tmp_real, tmp_real2;
vector<string> vt, vr;
vector<int> vn, vds;
static const int new_dataset = -1;
static const int all_datasets = -2;
bool outdated_plot = false;

vector<DataWithSum*> get_datasets_from_indata()
{
    vector<DataWithSum*> result;
    if (vds.empty()) {
        if (AL->get_ds_count() == 1)
            result.push_back(AL->get_ds(0));
        else
            throw ExecuteError("Dataset must be specified (eg. 'in @0').");
    }
    else if (vds.size() == 1 && vds[0] == all_datasets)
        for (int i = 0; i < AL->get_ds_count(); ++i)
            result.push_back(AL->get_ds(i));
    else
        for (vector<int>::const_iterator i = vds.begin(); i != vds.end(); ++i)
            result.push_back(AL->get_ds(*i));
    return result;
}

void set_data_title(char const*, char const*)  { 
    AL->get_data(ds_pref)->title = t; 
}

void do_transform(char const*, char const*)  { 
    vector<DataWithSum*> v = get_datasets_from_indata();
    for (vector<DataWithSum*>::const_iterator i = v.begin(); i != v.end(); ++i)
        (*i)->get_data()->transform(t); 
    outdated_plot=true; 
}

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
    else if (s == "variables") {
        if (variables.empty())
            m = "No variables found.";
        else {
            m = "Defined variables: ";
            for (vector<Variable*>::const_iterator i = variables.begin(); 
                    i != variables.end(); ++i)
                if (with_plus)
                    m += "\n" + (*i)->get_info(AL->get_parameters(), false);
                else
                    if ((*i)->is_visible())
                        m += (*i)->xname + " ";
        }
    }
    else if (s[0] == '$') {
        const Variable* v = AL->find_variable(string(s, 1));
        if (v) {
            m = v->get_info(AL->get_parameters(), with_plus);
            if (with_plus) {
                vector<string> refs = AL->get_variable_references(string(s, 1));
                if (!refs.empty())
                    m += "\nreferenced by: " + join_vector(refs, ", ");
            }
        }
        else 
            m = "Undefined variable: " + s;
    }
    else if (s == "functions") {
        if (functions.empty())
            m = "No functions found.";
        else {
            m = "Defined functions: ";
            for (vector<Function*>::const_iterator i = functions.begin(); 
                    i != functions.end(); ++i)
                if (with_plus)
                    m += "\n" + (*i)->get_info(variables, AL->get_parameters());
                else
                    m += (*i)->xname + " ";
        }
    }
    else if (s == "types") {
        m = "Defined function types: ";
        vector<string> const& types = Function::get_all_types();
        for (vector<string>::const_iterator i = types.begin(); 
                i != types.end(); ++i)
            if (with_plus)
                m += "\n" + Function::get_formula(*i);
            else
                m += *i + " ";
    }
    else if (s[0] == '%') {
        const Function* f = AL->find_function(string(s, 1));
        m = f ? f->get_info(variables, AL->get_parameters(), with_plus) 
              : "Undefined function: " + s;
    }
    else if (s == "datasets") {
        m = S(AL->get_ds_count()) + " datasets.";
        if (with_plus)
            for (int i = 0; i < AL->get_ds_count(); ++i)
                m += "\n@" + S(i) + ": " + AL->get_data(i)->get_title();
    }
    else if (s[0] == '@') {
        m = AL->get_data(tmp_int)->getInfo();
    }
    else if (s == "view") {
        m = AL->view.str();
    }
    else if (s == "fit")
        m = my_fit->getInfo();
    else if (s == "errors")
        m = my_fit->getErrorInfo(with_plus);
    else if (s == "commands")
        m = getUI()->getCommands().get_info();
    else if (string(s, 0, 5) == "peaks") {
        vector<DataWithSum*> v = get_datasets_from_indata();
        for (vector<DataWithSum*>::const_iterator i = v.begin(); 
                                                           i != v.end(); ++i)
            m += print_multiple_peakfind(*i, tmp_int, vr);
    }
    mesg(m);
}

void do_print_sum_info(char const* a, char const* b)
{
    string s = string(a,b);
    string m;
    Sum const* sum = AL->get_sum(ds_pref);
    if (s == "F" || s == "Z") {
        m = s + ": "; 
        vector<int> const &idx = (s == "F" ? sum->get_ff_idx() 
                                           : sum->get_zz_idx());
        for (vector<int>::const_iterator i = idx.begin(); i != idx.end(); ++i){
            Function const* f = AL->get_function(*i);
            if (with_plus)
                m += "\n" 
                    + f->get_info(AL->get_variables(), AL->get_parameters());
            else
                m += f->xname + " ";
        }
    }
    else if (s == "formula") {
        m = sum->get_formula(!with_plus);
    }
    mesg(m);
}

void do_print_sum_derivatives_info(char const*, char const*)
{
    fp x = get_transform_expression_value(t, AL->get_data(ds_pref));
    Sum const* sum = AL->get_sum(ds_pref);
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
    Data const* data = 0;
    try {
        data = AL->get_data(ds_pref);
    } catch (ExecuteError &) {
        // leave data=0
    }
    if (f) {
        fp x = get_transform_expression_value(t2, data);
        fp y = f->calculate_value(x);
        m = f->xname + "(" + S(x) + ") = " + S(y);
        if (with_plus) {
            //TODO? derivatives
        }
    }
    else
      m = "Undefined function: " + t;
    mesg(m);
}

void do_print_data_expr(char const*, char const*)
{
    string s;
    vector<DataWithSum*> v = get_datasets_from_indata();
    if (v.size() == 1)
        s = S(get_transform_expression_value(t, v[0]->get_data()));
    else {
        map<DataWithSum const*, int> m;
        for (int i = 0; i < AL->get_ds_count(); ++i)
            m[AL->get_ds(i)] = i;
        for (vector<DataWithSum*>::const_iterator i = v.begin(); 
                i != v.end(); ++i) {
            fp k = get_transform_expression_value(t, (*i)->get_data());
            s += "in @" + S(m[*i]) + ": " + S(k);
        }
    }
    mesg(s);
}

void do_print_func_type(char const* a, char const* b)
{
    string s = string(a,b);
    string m = Function::get_formula(s);
    if (m.empty())
        m = "Undefined function type: " + s;
    mesg(m);
}

void do_import_dataset(char const*, char const*)
{
    if (tmp_int == new_dataset
            && (AL->get_ds_count() != 1 || AL->get_data(0)->has_any_info()
                || AL->get_sum(0)->has_any_info())) {
        auto_ptr<Data> data(new Data);
        data->load_file(t, 0, vector<int>()); //TODO columns, type
        tmp_int = AL->append_ds(data.release());
    }
    else {
        //TODO columns, type
        AL->get_data(tmp_int)->load_file(t, 0, vector<int>()); 
        if (AL->get_ds_count() == 1)
            AL->view.fit();
    }
    AL->activate_ds(tmp_int);
    outdated_plot=true;  
}

void do_export_dataset(char const*, char const*)
{
    AL->get_data(tmp_int)->export_to_file(t); 
}

void do_append_data(char const*, char const*)
{
    int n = AL->append_ds();
    AL->activate_ds(n);
    outdated_plot=true;  
}

void do_load_data_sum(char const*, char const*)
{
    vector<Data const*> dd;
    for (vector<int>::const_iterator i = vn.begin(); i != vn.end(); ++i)
        dd.push_back(AL->get_data(*i));
    if (tmp_int == new_dataset) 
        tmp_int = AL->append_ds();
    AL->get_data(tmp_int)->load_data_sum(dd);
    AL->activate_ds(tmp_int);
    outdated_plot=true;  
}

void do_plot(char const*, char const*)
{
    if (tmp_int != -1)
        AL->activate_ds(tmp_int);
    AL->view.parse_and_set(vr);
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
    if (with_plus)
        my_fit->continue_fit(tmp_int);
    else
        my_fit->fit(tmp_int, get_datasets_from_indata());
    outdated_plot=true;  
}

void do_sleep(char const*, char const*)
{
    getUI()->wait(tmp_real);
}

void do_guess(char const*, char const*)
{
    vector<DataWithSum*> v = get_datasets_from_indata();
    for (vector<DataWithSum*>::const_iterator i = v.begin(); i != v.end(); ++i)
        guess_and_add(*i, t, t2, vr, vt);
    outdated_plot=true;  
}

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

void do_reset(char const*, char const*)   { AL->reset_all(); }

void do_dump(char const*, char const*)   { AL->dump_all_as_script(t); }

void do_exec_file(char const*, char const*) 
{ 
    vector<pair<int,int> > vpn;
    for (int i = 0; i < size(vn); i+=2)
        vpn.push_back(make_pair(vn[i],vn[i+1]));
    getUI()->execScript(t, vpn); 
}

void do_set(char const* a, char const* b) {getSettings()->setp(t, string(a,b));}

void do_set_show(char const*, char const*)  { mesg(getSettings()->infop(t)); }

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
        static const int one = 1;
        static const int zero = 0;
        static const int int_max = INT_MAX;
        static const char *dot = ".";
        static const char *empty = "";

        transform 
            = (ds_prefix >> "title" >> ch_p('=') 
               >> lexeme_d['"' >> (+~ch_p('"'))[assign_a(t)] 
                           >> '"'] 
              ) [&set_data_title]
            | (no_actions_d[DataTransformG] [assign_a(t)] 
               >> in_data) [&do_transform] 
            ;

        assign_var 
            = VariableLhsG [assign_a(t)]
                      >> '=' >> no_actions_d[FuncG] [&do_assign_var]
            ;

        function_name
            = lexeme_d[(upper_p >> *alnum_p)] 
            ;

        function_param
            = lexeme_d[alpha_p >> *(alnum_p | '_')]
            ;

        functionname_assign
            = (FunctionLhsG [assign_a(t)] >> '=' 
              | eps_p [assign_a(t, empty)]
              )
            ;

        assign_func
            = functionname_assign
              >> (function_name [assign_a(t2)]
                  >> str_p("(")[clear_a(vt)] 
                  >> !((no_actions_d[FuncG][push_back_a(vt)] 
                       % ',')
                      | ((function_param >> "=" >> no_actions_d[FuncG])
                                                             [push_back_a(vt)] 
                         % ',')
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

        in_data
            = eps_p [clear_a(vds)]
            >> !("in" >> (lexeme_d['@' >> uint_p [push_back_a(vds)]
                                   ]
                           % ','
                         | str_p("@*") [push_back_a(vds, all_datasets)]
                         )
                )
            ;

        ds_prefix
            = lexeme_d['@' >> uint_p [assign_a(ds_pref)] 
               >> '.']
            | eps_p [assign_a(ds_pref, minus_one)]
            ;

        filename_str
            = lexeme_d['\'' >> (+~ch_p('\''))[assign_a(t)] 
                       >> '\'']
                      | (+~space_p) [assign_a(t)]
            ;

        existing_dataset_nr
            = lexeme_d['@' >> uint_p [assign_a(tmp_int)]
                      ]
            ;

        dataset_nr
            = str_p("@+") [assign_a(tmp_int, new_dataset)]
            | existing_dataset_nr
            ;

        dataset_handling
            = dataset_nr >> '<' >> (eps_p[clear_a(vn)]
                                    >> (lexeme_d['@' >> uint_p [push_back_a(vn)]
                                                ]
                                        % '+') [&do_load_data_sum]
                                   | filename_str [&do_import_dataset]
                                   )
            | (existing_dataset_nr >> '>' >> filename_str) [&do_export_dataset]
            | str_p("@+")[&do_append_data] 
            ;

        plot_range  //first clear vr if needed 
            = (ch_p('[') >> ']') [push_back_a(vr,empty)][push_back_a(vr,empty)]
            | '[' >> (real_p|"."|eps_p) [push_back_a(vr)] 
              >> ':' >> (real_p|"."|eps_p) [push_back_a(vr)] 
              >> ']'  
            | str_p(".") [push_back_a(vr,dot)][push_back_a(vr,dot)] // [.:.]
            | eps_p [push_back_a(vr,empty)][push_back_a(vr,empty)] // [:]
            ;
        
        info_arg
            = str_p("variables") [&do_print_info]
            | VariableLhsG [&do_print_info]
            | str_p("functions") [&do_print_info]
            | (FunctionLhsG [assign_a(t)] 
               >> "(" 
               >> no_actions_d[DataExpressionG] [assign_a(t2)]
               >> ")" >> in_data) [&do_print_func_value] 
            | FunctionLhsG [&do_print_info]
            | str_p("datasets") [&do_print_info]
            | existing_dataset_nr [&do_print_info]
            | str_p("commands") [&do_print_info]
            | str_p("view") [&do_print_info]
            | ds_prefix >> (ch_p('F')|'Z'|"formula") [&do_print_sum_info]
            | (ds_prefix >> str_p("dF") >> '(' 
               >> no_actions_d[DataExpressionG][assign_a(t)] 
               >> ')') [&do_print_sum_derivatives_info]
            | str_p("fit") [&do_print_info] //TODO in_data?
            | str_p("errors") [&do_print_info] //TODO in_data?
            | (str_p("peaks") [clear_a(vr)]
               >> ( uint_p [assign_a(tmp_int)]
                  | eps_p [assign_a(tmp_int, one)])
               >> plot_range >> in_data) [&do_print_info]
            | (no_actions_d[DataExpressionG][assign_a(t)] 
                 >> in_data) [&do_print_data_expr]
            | function_name[&do_print_func_type]
            ;

        fit_arg
            = optional_plus
              >> ( uint_p[assign_a(tmp_int)]
                 | eps_p[assign_a(tmp_int, minus_one)]
                 )
              //TODO >> [only %name, $name, not $name2, ...] 
              >> in_data
            //TODO [with fitting-method=NM]
            ;

        guess_arg
            = eps_p [clear_a(vt)] [clear_a(vr)] 
              >> function_name [assign_a(t2)]
              >> plot_range
              >> !((function_param >> '=' >> no_actions_d[FuncG])
                                                           [push_back_a(vt)]
                   % ',')
              >> in_data
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
              >> ( (ch_p('>') >> filename_str) [&do_commands_logging] 
                 | (int_range 
                     >> !(ch_p('>') >> filename_str)) [&do_commands_print]
                 | (ch_p('<') [clear_a(vn)]
                     >> filename_str 
                     >> *(int_range[push_back_a(vn, tmp_int)]
                                                 [push_back_a(vn, tmp_int2)])
                   ) [&do_exec_file]
                 ) 
            ;

        optional_plus
            = str_p("+") [assign_a(with_plus, true_)] 
            | eps_p [assign_a(with_plus, false_)] 
            ;

        set_arg
            = (+(lower_p | '-'))[assign_a(t)]
              >> ('=' >> ('"' >> (*~ch_p('"'))[&do_set]
                          >> '"'
                         | (*(alnum_p | '-' | '+' | '.'))[&do_set]
                         )
                 | eps_p[&do_set_show]
                 )
            ;

        statement 
            = "info" >> optional_plus >> (info_arg % ',')
            | (str_p("delete")[clear_a(vt)][clear_a(vn)] 
                >> ( VariableLhsG [push_back_a(vt)]
                   | FunctionLhsG [push_back_a(vt)]
                   | lexeme_d['@'>>uint_p[push_back_a(vn)]]) % ',') [&do_delete]
            | (str_p("plot") [clear_a(vr)] [assign_a(tmp_int, minus_one)]
              >> !existing_dataset_nr >> plot_range >> plot_range) [&do_plot]
            | ("fit" >> fit_arg) [&do_fit]
            | ("sleep" >> ureal_p[assign_a(tmp_real)])[&do_sleep]
            | "commands" >> commands_arg
            | str_p("reset") [&do_reset]
            | (str_p("dump") >> '>' >> filename_str)[&do_dump]
            | "set" >> (set_arg % ',')
            | transform 
            | assign_var 
            | (functionname_assign >> "guess" >> guess_arg) [&do_guess]
            | subst_func_param 
            | assign_func 
            | put_function
            | fz_assign
            | dataset_handling
            ;

        multi 
            = (!(statement % ';') >> !('#' >> *~ch_p('\n'))) [&do_replot];

    }

    rule<ScannerT> transform, assign_var, function_name, assign_func, 
                   function_param, subst_func_param, put_function, 
                   functionname_assign, put_func_to, fz_assign, 
                   in_data, ds_prefix,
                   dataset_handling, filename_str, guess_arg,
                   existing_dataset_nr, dataset_nr, 
                   optional_plus, int_range, commands_arg, set_arg,
                   plot_range, info_arg, fit_arg, statement, multi;  

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



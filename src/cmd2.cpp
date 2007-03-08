// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License version 2
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
#include "logic.h"
#include "data.h"
#include "sum.h"
#include "ui.h"
#include "fit.h"
#include "settings.h"
#include "datatrans.h"
#include "guess.h"
#include "var.h"
#include "func.h"
#include "ast.h"

using namespace std;

namespace cmdgram {

bool with_plus, deep_cp;
string t, t2, t3, output_redir;
string prepared_info;
bool info_append;
int tmp_int, tmp_int2, ds_pref;
double tmp_real, tmp_real2;
vector<string> vt, vr;
vector<int> vn, vds;
const int new_dataset = -1;
const int all_datasets = -2;
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

IntRangeGrammar  IntRangeG;

} //namespace cmdgram

using namespace cmdgram;


namespace {

void do_import_dataset(char const*, char const*)
{
    if (tmp_int == new_dataset) {
        if (AL->get_ds_count() != 1 || AL->get_data(0)->has_any_info()
                                    || AL->get_sum(0)->has_any_info()) {
            // load data into new slot
            auto_ptr<Data> data(new Data);
            data->load_file(t, t2, vn); 
            tmp_int = AL->append_ds(data.release());
        }
        else { // there is only one and empty slot -- load data there
            AL->get_data(-1)->load_file(t, t2, vn); 
            AL->view.set_datasets(vector1(AL->get_ds(0)));
            AL->view.fit();
            tmp_int = 0;
        }
    }
    else { // slot number was specified -- load data there
        AL->get_data(tmp_int)->load_file(t, t2, vn); 
        if (AL->get_ds_count() == 1) {
            AL->view.set_datasets(vector1(AL->get_ds(0)));
            AL->view.fit();
        }
    }
    AL->activate_ds(tmp_int);
    outdated_plot=true;  
}

void do_export_dataset(char const*, char const*)
{
    vector<string> const& ff_names = AL->get_sum(tmp_int)->get_ff_names();
    AL->get_data(tmp_int)->export_to_file(t, vt, ff_names); 
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
    AL->get_data(tmp_int)->load_data_sum(dd, t);
    AL->activate_ds(tmp_int);
    outdated_plot=true;  
}

void do_plot(char const*, char const*)
{
    if (vds.size() == 1 && vds[0] >= 0)
        AL->activate_ds(vds[0]);
    bool need_ds = false;
    for (vector<string>::const_iterator i = vr.begin(); i != vr.end(); ++i)
        if (i->empty())
            need_ds = true;
    if (need_ds) {
        vector<DataWithSum*> dsds = get_datasets_from_indata();
        //move active ds to the front
        DataWithSum* ads = AL->get_ds(AL->get_active_ds_position());
        vector<DataWithSum*>::iterator pos = find(dsds.begin(),dsds.end(), ads);
        if (pos != dsds.end() && pos != dsds.begin()) {
            *pos = dsds[0];
            dsds[0] = ads;
        }
        AL->view.set_datasets(dsds);
    }
    AL->view.parse_and_set(vr);
    getUI()->draw_plot(1, true);
    outdated_plot = false;
}

void do_output_info(char const*, char const*)
{
    prepared_info = strip_string(prepared_info);
    if (output_redir.empty())
        rmsg(prepared_info);
    else {
        ofstream os(output_redir.c_str(), 
                    ios::out | (info_append ? ios::app : ios::trunc));
        if (!os) 
            throw ExecuteError("Can't open file: " + output_redir);
        os << prepared_info << endl;
    }
}

void do_print_info(char const* a, char const* b)
{
    string s = string(a,b);
    string m;
    vector<Variable*> const &variables = AL->get_variables(); 
    vector<Function*> const &functions = AL->get_functions(); 
    if (s.empty())
        m = "info about what?";
    else if (s == "version") {
        m = VERSION;
    }
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
        m = v->get_info(AL->get_parameters(), with_plus);
        if (with_plus) {
            vector<string> refs = AL->get_variable_references(string(s, 1));
            if (!refs.empty())
                m += "\nreferenced by: " + join_vector(refs, ", ");
        }
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
        m = f->get_info(variables, AL->get_parameters(), with_plus);
    }
    else if (s == "datasets") {
        m = S(AL->get_ds_count()) + " datasets.";
        if (with_plus)
            for (int i = 0; i < AL->get_ds_count(); ++i)
                m += "\n@" + S(i) + ": " + AL->get_data(i)->get_title();
    }
    else if (s[0] == '@') {
        Data const* data = AL->get_data(tmp_int);
        if (s.find(".title") != string::npos)
            m = data->get_title();
        else if (s.find(".filename") != string::npos)
            m = data->get_filename();
        else
            m = data->getInfo();
    }
    else if (s == "view") {
        m = AL->view.str();
    }
    else if (s == "set")
        m = getSettings()->print_usage();
    else if (startswith(s, "fit-history"))
        m = FitMethodsContainer::getInstance()->param_history_info();
    else if (startswith(s, "fit"))
        m = getFit()->getInfo(get_datasets_from_indata());
    else if (startswith(s, "errors"))
        m = getFit()->getErrorInfo(get_datasets_from_indata(), with_plus);
    else if (startswith(s, "peaks")) {
        vector<fp> errors;
        vector<DataWithSum*> v = get_datasets_from_indata();
        if (with_plus) 
            errors = getFit()->get_symmetric_errors(v);
        for (vector<DataWithSum*>::const_iterator i=v.begin(); i!=v.end(); ++i){
            m += "# " + (*i)->get_data()->get_title() + "\n";
            m += (*i)->get_sum()->get_peak_parameters(errors) + "\n";
        }
    }
    else if (startswith(s, "formula")) {
        vector<DataWithSum*> v = get_datasets_from_indata();
        for (vector<DataWithSum*>::const_iterator i=v.begin(); i!=v.end(); ++i){
            m += "# " + (*i)->get_data()->get_title() + "\n";
            bool gnuplot = getSettings()->get_e("formula-export-style") == 1;
            m += (*i)->get_sum()->get_formula(!with_plus, gnuplot) + "\n";
        }
    }
    else if (s == "commands")
        m = getUI()->get_commands().get_info(with_plus);
    else if (startswith(s, "guess")) {
        vector<DataWithSum*> v = get_datasets_from_indata();
        for (vector<DataWithSum*>::const_iterator i = v.begin(); 
                                                           i != v.end(); ++i)
            m += get_guess_info(*i, vr);
    }
    prepared_info += "\n" + m;
}

void do_print_func(char const*, char const*)
{
    vector<string> const &names = AL->get_sum(ds_pref)->get_names(t2[0]);
    if (tmp_int < 0)
        tmp_int += names.size();
    if (is_index(tmp_int, names)) 
        prepared_info += "\n" + AL->find_function(names[tmp_int])
            ->get_info(AL->get_variables(), AL->get_parameters(), with_plus);
    else
        prepared_info += "\nNot found.";
}

void do_print_sum_info(char const*, char const*)
{
    string m = t2 + ": "; 
    vector<int> const &idx = AL->get_sum(ds_pref)->get_indices(t2[0]);
    for (vector<int>::const_iterator i = idx.begin(); i != idx.end(); ++i) {
        Function const* f = AL->get_function(*i);
        if (with_plus)
            m += "\n" + f->get_info(AL->get_variables(), AL->get_parameters());
        else
            m += f->xname + " ";
    }
    prepared_info += "\n" + m;
}

void do_print_sum_derivatives_info(char const*, char const*)
{
    fp x = get_transform_expression_value(t2, AL->get_data(ds_pref));
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
    prepared_info += "\n" + m;
}

void do_print_deriv(char const* a, char const* b)
{
    string s = string(a, b);
    prepared_info += "\n" + get_derivatives_str(s);
}

void do_print_debug_info(char const*, char const*)  
{ 
    string m;
    if (t == "idx") {   // show varnames and var_idx from VariableUser
        for (int i = 0; i < size(AL->get_functions()); ++i) 
            m += S(i) + ": " + AL->get_function(i)->get_debug_idx_info() +"\n";
        for (int i = 0; i < size(AL->get_variables()); ++i) 
            m += S(i) + ": " + AL->get_variable(i)->get_debug_idx_info() +"\n";
    }
    else if (t == "rd") { // show values of derivatives for all variables
        for (int i = 0; i < size(AL->get_variables()); ++i) {
            Variable const* var = AL->get_variable(i);
            m += var->xname + ": ";
            vector<Variable::ParMult> const& rd 
                                        = var->get_recursive_derivatives();
            for (vector<Variable::ParMult>::const_iterator i = rd.begin(); 
                                                           i != rd.end(); ++i)
                m += S(i->p) 
                    + "/" + AL->find_variable_handling_param(i->p)->xname 
                    + "/" + S(i->mult) + "  ";
            m += "\n";
        }
    }
    else if (t.size() > 0 && t[0] == '%') { // show bytecode
        Function const* f = AL->find_function(t);
        m = f->get_bytecode();
    }
    rmsg(m);
}


void do_print_data_expr(char const*, char const*)
{
    string s;
    if (vds.empty() && !is_data_dependent_expression(t2)) //no data
        s = S(get_transform_expression_value(t2, 0));
    else {
        vector<DataWithSum*> v = get_datasets_from_indata();
        if (v.size() == 1)
            s = S(get_transform_expression_value(t2, v[0]->get_data()));
        else {
            map<DataWithSum const*, int> m;
            for (int i = 0; i < AL->get_ds_count(); ++i)
                m[AL->get_ds(i)] = i;
            for (vector<DataWithSum*>::const_iterator i = v.begin(); 
                    i != v.end(); ++i) {
                fp k = get_transform_expression_value(t2, (*i)->get_data());
                if (i != v.begin())
                    s += "\n";
                s += "in @" + S(m[*i]); 
                if (with_plus)
                    s += " " + (*i)->get_data()->get_title();
                s += ": " + S(k);
            }
        }
    }
    prepared_info += "\n" + s;
}

void do_print_func_type(char const* a, char const* b)
{
    string s = string(a,b);
    string m = Function::get_formula(s);
    if (m.empty())
        m = "Undefined function type: " + s;
    prepared_info += "\n" + m;
}

void do_guess(char const*, char const*)
{
    vector<DataWithSum*> v = get_datasets_from_indata();
    for (vector<DataWithSum*>::const_iterator i = v.begin(); i != v.end(); ++i)
        guess_and_add(*i, t, t2, vr, vt);
    outdated_plot=true;  
}

void set_data_title(char const*, char const*)  { 
    AL->get_data(ds_pref)->title = t; 
}

void do_list_commands(char const*, char const*)
{
    vector<string> cc 
        = getUI()->get_commands().get_commands(tmp_int, tmp_int2, with_plus);
    prepared_info += "\n" + join_vector(cc, "\n");
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

    compact_str
        = lexeme_d['\'' >> (+~ch_p('\''))[assign_a(t)] 
                   >> '\'']
        | lexeme_d[+chset<>(anychar_p - chset<>(" \t\n\r;,#"))] [assign_a(t)]
        ;

    type_name
        = lexeme_d[(upper_p >> +alnum_p)] 
        ;

    function_param
        = lexeme_d[alpha_p >> *(alnum_p | '_')]
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
        = (dataset_nr >> ch_p('<') >> compact_str //load from file
           >> lexeme_d[!(alpha_p >> *alnum_p)] [assign_a(t2)] [clear_a(vn)]
           >> !(uint_p [push_back_a(vn)] 
                % ',')
          ) [&do_import_dataset]
        | dataset_nr >> ch_p('=') [clear_a(vn)] [assign_a(t, empty)] //sum/dup
          >> !(lexeme_d[lower_p >> +(alnum_p | '-' | '_')] [assign_a(t)])
          >> (lexeme_d['@' >> uint_p [push_back_a(vn)]  
                                           ] % '+') [&do_load_data_sum]
        | (existing_dataset_nr [clear_a(vt)]
           >> !('(' >> ((DataExpressionG
                        | ("*F(" >> DataExpressionG >> ")")
                        ) [push_back_a(vt)] 
                        % ',')
                >> ')')
           >> '>' >> compact_str) [&do_export_dataset]
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
        = (str_p("commands") >> IntRangeG) [&do_list_commands] 
        |  ( str_p("version") 
           | str_p("variables") 
           | VariableLhsG 
           | str_p("types") 
           | str_p("functions") 
           | str_p("datasets") 
           | str_p("commands") 
           | str_p("view") 
           | str_p("set")) [&do_print_info] 
        | ((str_p("fit-history") 
            | "fit"
            | "errors"
            | "peaks"
            | "formula"
                    ) >> in_data) [&do_print_info] 
        | (str_p("guess") [clear_a(vr)]
           >> plot_range >> in_data) [&do_print_info]
        | type_name[&do_print_func_type]
        | (no_actions_d[DataExpressionG][assign_a(t2)] 
             >> in_data) [&do_print_data_expr]
        | FunctionLhsG [&do_print_info]
        | ds_prefix >> (str_p("F")|"Z")[assign_a(t2)] 
          >> ( ('[' >> int_p [assign_a(tmp_int)] >> ']') [&do_print_func] 
             | eps_p [&do_print_sum_info]
             )
        | (ds_prefix >> str_p("dF") >> '(' 
           >> no_actions_d[DataExpressionG][assign_a(t2)] 
           >> ')') [&do_print_sum_derivatives_info]
        | (existing_dataset_nr 
                >> (str_p(".filename") | ".title" | eps_p)) [&do_print_info]
        | "debug " >> compact_str [&do_print_debug_info] //no output_redir
        // this will eat also ',', because voigt(x,y) needs it
        // unfortunatelly "i der x^2, sin(x)" is made impossible
        | "der " >> (+chset<>(anychar_p - chset<>(";#"))) [&do_print_deriv]
        | eps_p [&do_print_info] 
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
        = (optional_suffix_p("i","nfo") [assign_a(output_redir, empty)]
                                        [assign_a(prepared_info, empty)]
           >> optional_plus >> (info_arg % ',')
           >> !( ( str_p(">>") [assign_a(info_append, true_)]
                 | str_p(">") [assign_a(info_append, false_)]
                 )
                >> compact_str [assign_a(output_redir, t)]
               )
          ) [&do_output_info]
        | (optional_suffix_p("p","lot") [clear_a(vr)] 
           >> plot_range >> plot_range >> in_data) [&do_plot]
        | guess [&do_guess]
        | (ds_prefix >> "title" >> '=' >> compact_str)[&set_data_title]
        | dataset_handling
        ;
}


template Cmd2Grammar::definition<scanner<char const*, scanner_policies<skipper_iteration_policy<iteration_policy>, match_policy, action_policy> > >::definition(Cmd2Grammar const&);

template Cmd2Grammar::definition<scanner<char const*, scanner_policies<skipper_iteration_policy<iteration_policy>, match_policy, no_actions_action_policy<action_policy> > > >::definition(Cmd2Grammar const&);

Cmd2Grammar cmd2G;



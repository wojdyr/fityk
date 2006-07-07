// This file is part of fityk program. Copyright (C) Marcin Wojdyr
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

using namespace std;

namespace cmdgram {

bool with_plus, deep_cp;
string t, t2;
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
            AL->view.fit();
            tmp_int = 0;
        }
    }
    else { // slot number was specified -- load data there
        AL->get_data(tmp_int)->load_file(t, t2, vn); 
        if (AL->get_ds_count() == 1)
            AL->view.fit();
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
        m = AL->get_data(tmp_int)->getInfo();
    }
    else if (s == "view") {
        m = AL->view.str();
    }
    else if (s == "set")
        m = getSettings()->print_usage();
    else if (startswith(s, "fit"))
        m = getFit()->getInfo(get_datasets_from_indata());
    else if (startswith(s, "errors"))
        m = getFit()->getErrorInfo(get_datasets_from_indata(), with_plus);
    else if (s == "commands")
        m = getUI()->getCommands().get_info();
    else if (startswith(s, "guess")) {
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

void do_print_debug_info(char const*, char const*)  { 
    string m;
    if (t == "idx") {
        for (int i = 0; i < size(AL->get_functions()); ++i) 
            m += S(i) + ": " + AL->get_function(i)->get_debug_idx_info() +"\n";
        for (int i = 0; i < size(AL->get_variables()); ++i) 
            m += S(i) + ": " + AL->get_variable(i)->get_debug_idx_info() +"\n";
    }
    else if (t == "rd") {
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
    else if (t.size() > 0 && t[0] == '%') {
        Function const* f = AL->find_function(t);
        m = f->get_bytecode();
    }
    mesg(m);
}


void do_print_data_expr(char const*, char const*)
{
    string s;
    //TODO "2+2" case
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
            if (i != v.begin())
                s += "\n";
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

void do_guess(char const*, char const*)
{
    vector<DataWithSum*> v = get_datasets_from_indata();
    for (vector<DataWithSum*>::const_iterator i = v.begin(); i != v.end(); ++i)
        guess_and_add(*i, t, t2, vr, vt);
    outdated_plot=true;  
}

void do_export_sum(char const*, char const*)   
{ 
    vector<DataWithSum*> v = get_datasets_from_indata();
    ofstream os(t.c_str(), ios::out | ios::trunc);
    if (!os) 
        throw ExecuteError("Can't open file: " + t);
    os << "# exported by fityk " VERSION << endl;
    if (t2 == "peaks") {
        vector<fp> errors;
        if (with_plus) 
            errors = getFit()->get_symmetric_errors(v);
        for (vector<DataWithSum*>::const_iterator i=v.begin(); i!=v.end(); ++i){
            os << "# " << (*i)->get_data()->get_title() << endl;
            os << (*i)->get_sum()->get_peak_parameters(errors);
        }
    }
    else if (t2 == "formula") {
        for (vector<DataWithSum*>::const_iterator i=v.begin(); i!=v.end(); ++i){
            os << "# " << (*i)->get_data()->get_title() << endl;
            os << (*i)->get_sum()->get_formula(!with_plus);
        }
    }
}

void set_data_title(char const*, char const*)  { 
    AL->get_data(ds_pref)->title = t; 
}

void do_list_commands(char const*, char const*)
{
    vector<string> cc 
        = getUI()->getCommands().get_commands(tmp_int, tmp_int2, with_plus);
    mesg(join_vector(cc, "\n"));
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
    static const int one = 1;
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

    ds_multiprefix
        = eps_p [clear_a(vds)]
        >> (lexeme_d['@' >> uint_p [push_back_a(vds)]
            >> '.']
           | str_p("@*.") [push_back_a(vds, all_datasets)]
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
        | lexeme_d[+chset<>(anychar_p - chset<>(" \t\n\r;,"))] [assign_a(t)]
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
        = dataset_nr >> ch_p('<') [clear_a(vn)] 
          >> ((lexeme_d['@' >> uint_p [push_back_a(vn)]  //sum/duplicate
                       ] % '+') [&do_load_data_sum]
             | (compact_str                              //load from file
                 >> lexeme_d[!(alpha_p >> *alnum_p)][assign_a(t2)]
                 >> !(uint_p [push_back_a(vn)]
                        % ',')
               ) [&do_import_dataset]
             )
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
        = str_p("variables") [&do_print_info]
        | VariableLhsG [&do_print_info]
        | str_p("types") [&do_print_info]
        | str_p("functions") [&do_print_info]
        | str_p("datasets") [&do_print_info]
        | (str_p("commands") >> optional_plus >> IntRangeG) [&do_list_commands] 
        | str_p("commands") [&do_print_info] 
        | str_p("view") [&do_print_info]
        | str_p("set") [&do_print_info] 
        | (str_p("fit") >> in_data) [&do_print_info] 
        | (str_p("errors") >> in_data) [&do_print_info] 
        | (str_p("guess") [clear_a(vr)]
           >> ( uint_p [assign_a(tmp_int)]
              | eps_p [assign_a(tmp_int, one)])
           >> plot_range >> in_data) [&do_print_info]
        | type_name[&do_print_func_type]
        | (no_actions_d[DataExpressionG][assign_a(t)] 
             >> in_data) [&do_print_data_expr]
        | FunctionLhsG [&do_print_info]
        | ds_prefix >> (ch_p('F')|'Z'|"formula") [&do_print_sum_info]
        | (ds_prefix >> str_p("dF") >> '(' 
           >> no_actions_d[DataExpressionG][assign_a(t)] 
           >> ')') [&do_print_sum_derivatives_info]
        | existing_dataset_nr [&do_print_info]
        | "debug" >> compact_str [&do_print_debug_info] 
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
        = optional_suffix_p("i","nfo") >> optional_plus >> (info_arg % ',')
        | (optional_suffix_p("p","lot") 
                              [clear_a(vr)] [assign_a(tmp_int, minus_one)]
          >> !existing_dataset_nr >> plot_range >> plot_range) [&do_plot]
        | guess [&do_guess]
        | (ds_prefix >> "title" >> '=' >> compact_str)[&set_data_title]
        | ds_multiprefix >> (str_p("peaks")|"formula")[assign_a(t2)] 
          >> optional_plus >> '>' >> compact_str [&do_export_sum]
        | dataset_handling
        ;
}


template Cmd2Grammar::definition<scanner<char const*, scanner_policies<skipper_iteration_policy<iteration_policy>, match_policy, action_policy> > >::definition(Cmd2Grammar const&);

template Cmd2Grammar::definition<scanner<char const*, scanner_policies<skipper_iteration_policy<iteration_policy>, match_policy, no_actions_action_policy<action_policy> > > >::definition(Cmd2Grammar const&);

Cmd2Grammar cmd2G;



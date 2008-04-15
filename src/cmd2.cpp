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
#include <xylib/xylib.h> //get_version()

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

#ifndef CONFIGURE_BUILD
# define CONFIGURE_BUILD "UNKNOWN"
#endif
#ifndef CONFIGURE_ARGS
# define CONFIGURE_ARGS "UNKNOWN"
#endif

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
bool no_info_output = false;


vector<int> get_ds_indices_from_indata()
{
    vector<int> result;
    // no datasets specified
    if (vds.empty()) {
        if (AL->get_ds_count() == 1)
            result.push_back(0);
        else
            throw ExecuteError("Dataset must be specified (eg. 'in @0').");
    }
    // @*
    else if (vds.size() == 1 && vds[0] == all_datasets)
        for (int i = 0; i < AL->get_ds_count(); ++i)
            result.push_back(i);
    // general case
    else
        for (vector<int>::const_iterator i = vds.begin(); i != vds.end(); ++i)
            if (*i == all_datasets) {
                for (int j = 0; j < AL->get_ds_count(); ++j) {
                    if (!contains_element(result, j))
                        result.push_back(j);
                }
                return result;
            }
            else
                result.push_back(*i);
    return result;
}

vector<DataWithSum*> get_datasets_from_indata()
{
    vector<int> indices = get_ds_indices_from_indata();
    vector<DataWithSum*> result(indices.size());
    for (size_t i = 0; i < indices.size(); ++i)
        result[i] = AL->get_ds(indices[i]);
    return result;
}

IntRangeGrammar  IntRangeG;
CompactStrGrammar  CompactStrG;

} //namespace cmdgram

using namespace cmdgram;

namespace {

void do_import_dataset(char const*, char const*)
{
    if (t == ".") {
        if (tmp_int == new_dataset)
            throw ExecuteError("New dataset can't be reverted");
        if (!vt.empty())
            throw ExecuteError("Options can't be given when reverting data");
        AL->get_data(tmp_int)->revert();
    }
    else
        AL->import_dataset(tmp_int, t, vt);
    outdated_plot=true;  
}
void do_revert_data(char const*, char const*)
{
    AL->get_data(tmp_int)->revert();
    outdated_plot=true;  
}

// returns true if x columns in all datasets have the same x values
bool equal_x_colums(bool only_active)
{
    vector<fp> xx;
    vector<Point> const& points = AL->get_data(0)->points();
    for (size_t i = 0; i < points.size(); ++i) {
        if (!only_active || points[i].is_active)
            xx.push_back(points[i].x);
    }

    for (int j = 1; j < AL->get_ds_count(); ++j) {
        vector<Point> const& points2 = AL->get_data(j)->points();
        size_t n = 0;
        for (size_t i = 0; i < points2.size(); ++i) {
            if (!only_active || points2[i].is_active) {
                double x = points2[i].x;
                if (n >= xx.size() || fabs(xx[n] - x) > 1e-6 * fabs(xx[n] + x))
                    return false;
                ++n;
            }
        }
    }
    return true;
}


// change F(...) to @n.F(...), and the same with Z
string add_ds_to_sum(string const& s, int nr)
{
    string s2;
    s2.reserve(s.size());
    for (string::const_iterator i = s.begin(); i != s.end(); ++i) {
        if ((*i == 'F' || *i == 'Z')
             && (i == s.begin() || (*(i-1) != '.' && !isalnum(*(i-1)) 
                                    && *(i-1) != '_' && *(i-1) != '$' 
                                    && *(i-1) != '%')) 
             && (i+1 == s.end() || (!isalnum(*(i+1)) && *(i+1) != '_'))) {
            s2 += "@" + S(nr) + ".";
        }
        s2 += *i;
    }
    return s2;
}

void do_export_dataset(char const*, char const*)
{
    vector<string>& cols = vt;
    int idx = tmp_int; 

    ostringstream os;
    os << "# exported by fityk " VERSION << endl;

    bool only_active = !contains_element(cols, "a");
    int ds_count = AL->get_ds_count();
    vector<vector<fp> > r;

    // first dataset
    Data *data = AL->get_data(idx == all_datasets ? 0 : idx);
    string title = data->get_title();
    for (vector<string>::const_iterator i = cols.begin(); i != cols.end(); ++i){
        os << (i == cols.begin() ? "#" : "\t") << title << ":" << *i;
        r.push_back(get_all_point_expressions(add_ds_to_sum(*i, idx), 
                                              data, only_active));
    }

    // the rest of datasets (if any)
    if (idx == all_datasets && ds_count > 1) { 
        // special exception - remove redundant x columns
        if (cols[0] == "x" && equal_x_colums(only_active))
            cols.erase(cols.begin());

        for (int i = 1; i < ds_count; ++i) {
            data = AL->get_data(i);
            string title = data->get_title();
            for (vector<string>::const_iterator j = cols.begin(); 
                                                    j != cols.end(); ++j) {
                os << "\t" << title << ":" << *j;
                r.push_back(get_all_point_expressions(add_ds_to_sum(*j, idx), 
                                                      data, only_active));
            }
        }
    }
    os << endl; // after title

    // output 2D table r as TSV
    size_t nc = r.size();
    for (size_t i = 0; i != r[0].size(); ++i) {
        for (size_t j = 0; j != nc; ++j) {
            os << r[j][i] << (j < nc-1 ? '\t' : '\n'); 
        }
    }

    prepared_info += "\n" + os.str();
}


void do_load_data_sum(char const*, char const*)
{
    vector<Data const*> dd;
    for (vector<int>::const_iterator i = vn.begin(); i != vn.end(); ++i)
        dd.push_back(AL->get_data(*i));
    if (tmp_int == new_dataset) 
        tmp_int = AL->append_ds();
    AL->get_data(tmp_int)->load_data_sum(dd, t);
    outdated_plot=true;  
}

void do_plot(char const*, char const*)
{
    AL->view.parse_and_set(vr, get_ds_indices_from_indata());
    AL->get_ui()->draw_plot(1, true);
    outdated_plot = false;
}

void do_output_info(char const*, char const*)
{
    prepared_info = strip_string(prepared_info);
    if (no_info_output)
        return;
    if (output_redir.empty()) {
        int max_screen_info_length = 2048;
        int more = prepared_info.length() - max_screen_info_length;
        if (more > 0) {
            prepared_info.resize(max_screen_info_length);
            prepared_info += "\n[... " + S(more) + " characters more...]";
        }
        AL->rmsg(prepared_info);
    }
    else {
        ofstream os(output_redir.c_str(), 
                    ios::out | (info_append ? ios::app : ios::trunc));
        if (!os) 
            throw ExecuteError("Can't open file: " + output_redir);
        os << prepared_info << endl;
    }
}

void do_print_info_in_data(char const* a, char const* b)
{
    string s = string(a,b);
    vector<DataWithSum*> v = get_datasets_from_indata();

    if (startswith(s, "fit"))
        prepared_info += "\n" + AL->get_fit()->get_info(v);
    else if (startswith(s, "errors"))
        prepared_info += "\n" + AL->get_fit()->getErrorInfo(v, with_plus);

    else if (startswith(s, "peaks")) {
        vector<fp> errors;
        if (with_plus) 
            errors = AL->get_fit()->get_symmetric_errors(v);
        for (vector<DataWithSum*>::const_iterator i = v.begin(); i != v.end(); ++i) {
            prepared_info += "\n# " + (*i)->get_data()->get_title() + "\n"
                          + (*i)->get_sum()->get_peak_parameters(errors) + "\n";
        }
    }
    else if (startswith(s, "formula")) {
        bool gnuplot = AL->get_settings()->get_e("formula-export-style") == 1;
        for (vector<DataWithSum*>::const_iterator i = v.begin(); i != v.end(); ++i) {
            prepared_info += "\n# " + (*i)->get_data()->get_title() + "\n"
                   + (*i)->get_sum()->get_formula(!with_plus, gnuplot);
        }
    }
    else if (startswith(s, "title")) {
        for (vector<DataWithSum*>::const_iterator i = v.begin(); i != v.end(); ++i) {
            prepared_info += "\n" + (*i)->get_data()->get_title();
        }
    }
    else if (startswith(s, "filename")) {
        for (vector<DataWithSum*>::const_iterator i = v.begin(); i != v.end(); ++i) {
            prepared_info += "\n" + (*i)->get_data()->get_filename();
        }
    }
    else if (startswith(s, "data")) {
        for (vector<DataWithSum*>::const_iterator i = v.begin(); i != v.end(); ++i) {
            prepared_info += "\n" + (*i)->get_data()->get_info();
        }
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
        if (with_plus) {
            m = "Version: " VERSION 
                "\nBuild system type: " CONFIGURE_BUILD
                "\nConfigured with: " CONFIGURE_ARGS
#ifdef __GNUC__
                "\nCompiler: GCC"
#endif
#ifdef __VERSION__
                "\nCompiler version: " __VERSION__ 
#endif
                "\nCompilation date: " __DATE__ 
                "\nBoost.Spirit version: " + S(SPIRIT_VERSION / 0x1000) 
                                + "." + S(SPIRIT_VERSION % 0x1000 / 0x0100)
                                + "." + S(SPIRIT_VERSION % 0x0100)
                + "\nxylib version:" + xylib::get_version();
 
        }
        else
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
    else if (s == "view") {
        m = AL->view.str();
    }
    else if (s == "set")
        m = AL->get_settings()->print_usage();
    else if (startswith(s, "fit-history"))
        m = AL->get_fit_container()->param_history_info();
    else if (s == "commands")
        m = AL->get_ui()->get_commands().get_info(with_plus);
    else if (startswith(s, "guess")) {
        vector<DataWithSum*> v = get_datasets_from_indata();
        for (vector<DataWithSum*>::const_iterator i = v.begin(); 
                                                           i != v.end(); ++i)
            m += Guess(AL, *i).get_guess_info(vr);
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
    AL->rmsg(m);
}


void do_print_data_expr(char const*, char const*)
{
    string s;

    // special case, only one variable, e.g. "info $_1"
    if (t2[0] == '$' && parse(t2.c_str(), VariableLhsG).full) {
        string varname = t2.substr(1);
        const Variable* v = AL->find_variable(varname);
        s = v->get_info(AL->get_parameters(), with_plus);
        if (with_plus) {
            vector<string> refs = AL->get_variable_references(varname);
            if (!refs.empty())
                s += "\nreferenced by: " + join_vector(refs, ", ");
        }
        prepared_info += "\n" + s;
        return;
    }

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
    string const& name = t;
    string const& function = t2;

    // "%name = guess Linear in @0, @1" makes no sense
    if (!name.empty() && v.size() > 1)
        // SyntaxError actually
        throw ExecuteError("many functions can't be assigned to one name."); 

    for (vector<DataWithSum*>::const_iterator i = v.begin(); i != v.end(); ++i) {
        DataWithSum *ds = *i;
        vector<string> vars = vt;
        Guess(AL, ds).guess(name, function, vr, vars);
        string real_name = AL->assign_func(name, function, vars);
        ds->get_sum()->add_function_to(real_name, 'F');
    }
    outdated_plot=true;  
}

void set_data_title(char const*, char const*)  { 
    AL->get_data(ds_pref)->title = t; 
}

void do_list_commands(char const*, char const*)
{
    vector<string> cc 
      = AL->get_ui()->get_commands().get_commands(tmp_int, tmp_int2, with_plus);
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
        >> !("in" >> (lexeme_d['@' >> (uint_p [push_back_a(vds)]
                                      |ch_p('*')[push_back_a(vds, all_datasets)]
                                      )
                              ]
                       % ','
                     )
            )
        ;

    ds_prefix
        = lexeme_d['@' >> uint_p [assign_a(ds_pref)] 
           >> '.']
        | eps_p [assign_a(ds_pref, minus_one)]
        ;

    type_name
        = lexeme_d[(upper_p >> +alnum_p)] 
        ;

    function_param
        = lexeme_d[alpha_p >> *(alnum_p | '_')]
        ;

    dataset_nr
        = lexeme_d['@' >> ( uint_p [assign_a(tmp_int)]
                          | ch_p('*') [assign_a(tmp_int, all_datasets)]
                          )
                  ]
        ;

    dataset_lhs
        = lexeme_d['@' >> ( uint_p [assign_a(tmp_int)]
                          | ch_p('+') [assign_a(tmp_int, new_dataset)]
                          )
                  ]
        ;

    dataset_handling
          //load from file
        = (dataset_lhs >> '<' >> CompactStrG [clear_a(vt)]
           >> !(lexeme_d[+(alnum_p | '-' | '_')] [push_back_a(vt)] 
                % ',')
          ) [&do_import_dataset]
          //sum / duplicate
        | dataset_lhs >> ch_p('=') [clear_a(vn)] [assign_a(t, empty)] 
          >> !(lexeme_d[lower_p >> +(alnum_p | '-' | '_')] [assign_a(t)])
          >> (lexeme_d['@' >> uint_p [push_back_a(vn)]  
                      | "0"
                                   ] % '+') [&do_load_data_sum]
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
        | ( str_p("version") 
          | "variables" 
          | "types" 
          | "functions" 
          | "datasets" 
          | "commands" 
          | "view" 
          | "set"
          | "fit-history"
          ) [&do_print_info] 
        | (( str_p("fit")
           | "errors"
           | "peaks"
           | "formula"
           | "filename" 
           | "title" 
           | "data"
           ) 
           >> in_data) [&do_print_info_in_data] 
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
          // export data
        | dataset_nr [clear_a(vt)]
          >> str_p("(") 
          >> (DataExpressionG [push_back_a(vt)] 
              % ',')
          >> str_p(")") [&do_export_dataset]
        | "debug " >> CompactStrG [&do_print_debug_info] //no output_redir
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
                >> CompactStrG [assign_a(output_redir, t)]
               )
          ) [&do_output_info]
        | (optional_suffix_p("p","lot") [clear_a(vr)] 
           >> plot_range >> plot_range >> in_data) [&do_plot]
        | guess [&do_guess]
        | dataset_handling
        | optional_suffix_p("s","et") 
          >> (ds_prefix >> "title" >> '=' >> CompactStrG)[&set_data_title]
        ;
}


template Cmd2Grammar::definition<scanner<char const*, scanner_policies<skipper_iteration_policy<iteration_policy>, match_policy, action_policy> > >::definition(Cmd2Grammar const&);

template Cmd2Grammar::definition<scanner<char const*, scanner_policies<skipper_iteration_policy<iteration_policy>, match_policy, no_actions_action_policy<action_policy> > > >::definition(Cmd2Grammar const&);

Cmd2Grammar cmd2G;



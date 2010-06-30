// This file is part of fityk program. Copyright 2009 Marcin Wojdyr.
// Licence: GNU General Public License ver. 2+
// $Id: $

/// Parsing arguments of info command (fityk's mini-language).
/// Boost.Spirit is not used for this now, because it creates more problems
/// than its worth.

#include "info.h"
#include <string>
#include <vector>
#include <ctype.h>
#include <string.h>
//#include <iostream>

#include <xylib/xylib.h> //get_version()
#include <boost/version.hpp> // BOOST_VERSION

#include "logic.h"
#include "func.h"
#include "data.h"
#include "fit.h"
#include "ast.h"
#include "model.h"
#include "datatrans.h"
#include "guess.h"

#ifndef CONFIGURE_BUILD
# define CONFIGURE_BUILD "UNKNOWN"
#endif
#ifndef CONFIGURE_ARGS
# define CONFIGURE_ARGS "UNKNOWN"
#endif

using namespace std;

typedef vector<DataAndModel*> VecDM;

template <typename T>
inline T at_signed_pos(std::vector<T> const& vec, int idx)
{
    if (idx < 0)
        idx += vec.size();
    if (idx >= 0 && idx < (int) vec.size())
        return vec[idx];
    else
        throw ExecuteError("Wrong index: " + S(idx));
}

string get_word(string const& t, size_t &pos, const char* end_chars=" \t\r\n,")
{
    if (pos >= t.size())
        return "";
    pos = t.find_first_not_of(" \t\r\n", pos);
    size_t end = t.find_first_of(end_chars, pos);
    string word = t.substr(pos, end - pos);
    pos = (end == string::npos ? t.size() : end);
    return word;
}

// find the next char from the given string that is not inside brackets
const char* find_outer_char(const char* p, const char* char_list)
{
    int level = 0;
    while (*p != '\0' && level >= 0) {
        if (*p == '(')
            ++level;
        else if (*p == ')')
            --level;
        else if (level == 0 && strchr(char_list, *p) != NULL)
            return p;
        ++p;
    }
    return NULL;
}

// parse string like "@4" or "@*" or "@2 @5"
void parse_datasets(Ftk const* F, string const& t, size_t &pos,
                    vector<int>& ret)
{
    for (;;) {
        if (pos >= t.size())
            break;
        pos = t.find_first_not_of(" \t\r\n", pos);
        if (pos == string::npos || t[pos] != '@')
            break;
        ++pos;
        int ds;
        if (t[pos] == '*') {
            ds = -1;
            ++pos;
            for (int i = 0; i < F->get_dm_count(); ++i)
                ret.push_back(i);
        }
        else {
            const char *p = t.c_str() + pos;
            char *endptr;
            ds = strtol(p, &endptr, 10);
            if (endptr == p)
                throw ExecuteError("Expected number or `*' after `@'");
            pos += (endptr - p);
            ret.push_back(ds);
        }
    }
}

// parse string like "in @4" or "in @*" or "in @2 @5"
VecDM parse_in_data(Ftk const* F, string const& t, size_t &pos)
{
    VecDM ret;
    size_t old_pos = pos;
    if (get_word(t, pos) != "in") {
        if (F->get_dm_count() == 1)
            ret.push_back(const_cast<DataAndModel*>(F->get_dm(0)));
        pos = old_pos;
        return ret;
    }
    vector<int> numbers;
    parse_datasets(F, t, pos, numbers);
    if (numbers.empty())
        throw ExecuteError("Expected @dataset-number after `in'");
    ret.resize(numbers.size());
    for (size_t i = 0; i != numbers.size(); ++i)
        ret[i] = const_cast<DataAndModel*>(F->get_dm(numbers[i]));
    return ret;
}

bool is_blank(string const& s)
{
    for (const char *p = s.c_str(); *p != '\0'; ++p)
        if (!isspace(*p))
            return false;
    return true;
}

// parse string like "[12.3:56.7]" or "[:45.6]" or ""
RealRange parse_real_range(string const& t, size_t &pos)
{
    RealRange range;
    if (pos < t.size())
        pos = t.find_first_not_of(" \t\r\n", pos);
    if (pos < t.size() && t[pos] == '[') {
        size_t right_b = find_matching_bracket(t, pos);
        size_t colon = t.find(':', pos);
        if (colon == string::npos)
            throw ExecuteError("Expected [from:to] range, `:' not found");
        string from_str = t.substr(pos+1, colon-(pos+1));
        string to_str = t.substr(colon+1, right_b-(colon+1));
        if (is_blank(from_str))
            range.from = RealRange::kInf;
        else {
            range.from = RealRange::kNumber;
            range.from_val = get_transform_expression_value(from_str, NULL);
        }
        if (is_blank(to_str))
            range.to = RealRange::kInf;
        else {
            range.to = RealRange::kNumber;
            range.to_val = get_transform_expression_value(to_str, NULL);
        }
        pos = right_b + 1;
    }
    else {
        range.from = range.to = RealRange::kNone;
        // to handle '.' as none:
        //else if (pos < t.size() && t[pos] == '.') { ... }
        //else throw ExecuteError("Expected [from:to] range");
    }
    return range;
}

pair<int,int> parse_index_range(string const& t, size_t &pos, size_t size)
{
    pair<int,int> range;
    RealRange real_range = parse_real_range(t, pos);

    if (real_range.from == RealRange::kInf)
        range.first = 0;
    else if (real_range.from == RealRange::kNumber)
        range.first = iround(real_range.from_val);

    if (real_range.to == RealRange::kInf)
        range.second = size;
    else if (real_range.to == RealRange::kNumber)
        range.second = iround(real_range.to_val);

    if (range.first < 0)
        range.first += size;
    if (range.second < 0)
        range.second += size;
    // we accept some empty ranges that make no sense, e.g. [4:1]
    if (range.first < 0 || range.second > (int) size)
        throw ExecuteError("Limits [from:to] outside of the range.");
    return range;
}

void get_info_guess(Ftk const* F, string const& args, size_t& pos,
                    string& result)
{
    RealRange range = parse_real_range(args, pos);
    VecDM v = parse_in_data(F, args, pos);
    for (vector<DataAndModel*>::const_iterator i = v.begin(); i != v.end(); ++i)
        Guess(F, *i).get_guess_info(range, result);
}

void get_info_commands(Ftk const* F, string const& args, size_t& pos,
                       bool full, string& result)
{
    Commands const& commands = F->get_ui()->get_commands();
    if (pos < args.size())
        pos = args.find_first_not_of(" \t\r\n", pos);
    if (pos < args.size() && args[pos] == '[') {
        vector<Commands::Cmd> const& cmds = commands.get_cmds();
        pair<int,int> range = parse_index_range(args, pos, cmds.size());
        if (cmds.empty())
            return;
        for (int i = range.first; i < range.second; ++i)
            result += (full ? cmds[i].str() : cmds[i].cmd) + "\n";
    }
    else
        result += commands.get_info(full);
}

void get_info_debug(Ftk const* F, string const& arg, string& result)
{
    if (arg == "idx") {   // show varnames and var_idx from VariableUser
        for (int i = 0; i < size(F->get_functions()); ++i)
            result += S(i) + ": " + F->get_function(i)->get_debug_idx_info()
                + "\n";
        for (int i = 0; i < size(F->get_variables()); ++i)
            result += S(i) + ": " + F->get_variable(i)->get_debug_idx_info()
                + "\n";
    }
    else if (arg == "rd") { // show values of derivatives for all variables
        for (int i = 0; i < size(F->get_variables()); ++i) {
            Variable const* var = F->get_variable(i);
            result += var->xname + ": ";
            vector<Variable::ParMult> const& rd
                                        = var->get_recursive_derivatives();
            for (vector<Variable::ParMult>::const_iterator i = rd.begin();
                                                           i != rd.end(); ++i)
                result += S(i->p)
                    + "/" + F->find_variable_handling_param(i->p)->xname
                    + "/" + S(i->mult) + "  ";
            result += "\n";
        }
    }
    else if (arg.size() > 0 && arg[0] == '%') { // show bytecode
        Function const* f = F->find_function(arg);
        result += f->get_bytecode();
    }
}

void get_info_der(string const& args, size_t& pos, string& result)
{
    if (pos < args.size())
        pos = args.find_first_not_of(" \t\r\n", pos);
    if (pos == string::npos)
        throw ExecuteError("Missing 'info der' argument");
    // The function get_derivatives_str() gets all the rest of the args,
    // but it may consume only a part of it.
    // Using comma, e.g. "i der x^2, der sin(x)", should be possible.
    size_t length = get_derivatives_str(args.c_str() + pos, result);
    // the returned lenght is not the real length. It doesn't count
    // spaces (which are removed by lexer).
    for (size_t i = 0; i != length; ++i) {
        assert(pos < args.size());
        pos = args.find_first_not_of(" \t\r\n", pos);
        ++pos;
    }
    if (pos < args.size())
        pos = args.find_first_not_of(" \t\r\n", pos);
}

void get_info_variable(Ftk const* F, string const& name, bool full,
                       string& result)
{
    assert(name[0] == '$');
    string varname = name.substr(1);
    result += F->get_variable_info(varname, full);
    if (full) {
        vector<string> refs = F->get_variable_references(varname);
        if (!refs.empty())
            result += "\n  referenced by: " + join_vector(refs, ", ");
    }
}

void get_info_data_expr(Ftk const* F, string const& args, size_t& pos,
                        bool full, string& result)
{
    size_t in_pos = args.find(" in ", pos);
    size_t end_pos;
    const char* expr_end;
    vector<int> ds;
    if (in_pos != string::npos) {
        end_pos = in_pos + 4;
        expr_end = args.c_str() + in_pos;
        parse_datasets(F, args, end_pos, ds); // end_pos is modified here
        if (ds.empty())
            throw ExecuteError("Expected @dataset-number after `in'");
    }
    else {
        end_pos = args.size();
        expr_end = args.c_str() + args.size();
    }

    vector<string> expressions;
    const char* start = args.c_str() + pos;
    while (start < expr_end) {
        const char *comma = find_outer_char(start, ",>");
        if (comma == NULL)
            comma = expr_end;
        expressions.push_back(string(start, comma));
        if (*comma == '>') {
            end_pos = comma - args.c_str();
            break;
        }
        start = comma + 1;
    }

    if (ds.empty()) {
        Data const* data = F->get_dm_count() == 1 ? F->get_data(0) : NULL;
        for (vector<string>::const_iterator j = expressions.begin();
                                                j != expressions.end(); ++j) {
            fp k = get_transform_expression_value(*j, data);
            result += F->get_settings()->format_double(k)
                      + (j == expressions.end() - 1 ? "\n" : " ");
        }
    }
    else {
        for (vector<int>::const_iterator i = ds.begin(); i != ds.end(); ++i) {
            result += "in @" + S(*i);
            if (full)
                result += " " + F->get_data(*i)->get_title();
            result += ": ";
            for (vector<string>::const_iterator j = expressions.begin();
                                                j != expressions.end(); ++j) {
                fp k = get_transform_expression_value(*j, F->get_data(*i));
                result += F->get_settings()->format_double(k)
                          + (j == expressions.end() - 1 ? "\n" : " ");
            }
        }
    }
    pos = end_pos;
}

void get_info_version(bool full, string& result)
{
    if (full)
        result += "Version: " VERSION
            "\nBuild system type: " CONFIGURE_BUILD
            "\nConfigured with: " CONFIGURE_ARGS
#ifdef __GNUC__
            "\nCompiler: GCC"
#endif
#ifdef __VERSION__
            "\nCompiler version: " __VERSION__
#endif
            "\nCompilation date: " __DATE__
            "\nBoost version: " + S(BOOST_VERSION / 100000)
                          + "." + S(BOOST_VERSION / 100 % 1000)
                          + "." + S(BOOST_VERSION % 100)
            + "\nxylib version: " + xylib_get_version();
    else
        result += VERSION;
}

void get_info_variables(Ftk const* F, bool full, string& result)
{
    vector<Variable*> const &variables = F->get_variables();
    if (variables.empty())
        result += "No variables found.";
    else {
        result += "Defined variables: ";
        for (vector<Variable*>::const_iterator i = variables.begin();
                i != variables.end(); ++i)
            if (full)
                result += "\n" + F->get_variable_info(*i, false);
            else
                if ((*i)->is_visible())
                    result += (*i)->xname + " ";
    }
}

void get_info_types(bool full, string& result)
{
    result += "Defined function types: ";
    vector<string> const& types = Function::get_all_types();
    for (vector<string>::const_iterator i = types.begin();
            i != types.end(); ++i)
        if (full)
            result += "\n" + Function::get_formula(*i);
        else
            result += *i + " ";
}

void get_info_functions(Ftk const* F, bool full, string& result)
{
    vector<Function*> const &functions = F->get_functions();
    if (functions.empty())
        result += "No functions found.";
    else {
        result += "Defined functions: ";
        for (vector<Function*>::const_iterator i = functions.begin();
                i != functions.end(); ++i)
            if (full)
                result += "\n" + (*i)->get_info(F, false);
            else
                result += (*i)->xname + " ";
    }
}

void get_info_datasets(Ftk const* F, bool full, string& result)
{
    result += S(F->get_dm_count()) + " datasets.";
    if (full)
        for (int i = 0; i < F->get_dm_count(); ++i)
            result += "\n@" + S(i) + ": " + F->get_data(i)->get_title();
}

void get_info_peaks(Ftk const* F, VecDM const& v, bool full,
                    string& result)
{
    vector<fp> errors;
    if (full)
        errors = F->get_fit()->get_standard_errors(v);
    for (VecDM::const_iterator i = v.begin(); i != v.end(); ++i)
        result += "\n# " + (*i)->data()->get_title()
                + "\n" + (*i)->model()->get_peak_parameters(errors) + "\n";
}

void get_info_formula(Ftk const* F, VecDM const& v, bool full,
                    string& result)
{
    bool gnuplot = F->get_settings()->get_e("formula_export_style") == 1;
    for (VecDM::const_iterator i = v.begin(); i != v.end(); ++i)
        result += "\n# " + (*i)->data()->get_title()
                + "\n" + (*i)->model()->get_formula(!full, gnuplot);
}

void get_info_func_type(string const& functype, bool full, string& result)
{
    string m = Function::get_formula(functype);
    if (m.empty()) {
        result += "Undefined function type: " + functype;
        return;
    }
    result += m;
    if (full && m.find(" where ") != string::npos)
        result += "\n = " + Function::get_rhs_from_formula(m);
}

void get_info_model(Ftk const* F, int ds,
                    string const& args, size_t& pos,
                    Model::FuncSet funcset,
                    bool full,
                    string& result)
{
    if (pos < args.size())
        pos = args.find_first_not_of(" \t\r\n", pos);
    if (pos < args.size() && args[pos] == '[') {
        size_t right_b = find_matching_bracket(args, pos);
        if (right_b == string::npos)
            throw ExecuteError("Missing closing bracket `]'");
        string expr = args.substr(pos+1, right_b-(pos+1));
        int idx = iround(get_transform_expression_value(expr, NULL));
        string name = at_signed_pos(F->get_model(ds)->get_names(funcset), idx);
        result += F->find_function(name)->get_info(F, full);
        pos = right_b + 1;
    }
    else {
        result += Model::str(funcset) +  ":";
        vector<int> const &idx = F->get_model(ds)->get_indices(funcset);
        for (vector<int>::const_iterator i = idx.begin(); i != idx.end(); ++i)
            result += (full ? "\n" + F->get_function(*i)->get_info(F, false)
                            : " " + F->get_function(*i)->xname);
    }
}

void get_info_model_der(Ftk const* F, int ds, string const& args, size_t& pos,
                        string& result)
{
    if (pos < args.size())
        pos = args.find_first_not_of(" \t\r\n", pos);
    if (pos >= args.size() || args[pos] != '(')
        throw ExecuteError("expected (expression) in brackets after `dF'");
    size_t right_b = find_matching_bracket(args, pos);
    if (right_b == string::npos)
        throw ExecuteError("Missing closing bracket `)'");
    string expr = args.substr(pos+1, right_b-(pos+1));
    fp x = get_transform_expression_value(expr, F->get_data(ds));
    Model const* model = F->get_model(ds);
    vector<fp> symb = model->get_symbolic_derivatives(x);
    vector<fp> num = model->get_numeric_derivatives(x, 1e-4);
    assert (symb.size() == num.size());
    result += "F(" + S(x) + ")=" + S(model->value(x));
    for (int i = 0; i < size(num); ++i) {
        if (is_neq(symb[i], 0) || is_neq(num[i], 0))
            result += "\ndF / d " + F->find_variable_handling_param(i)->xname
                + " = (symb.) " + S(symb[i]) + " = (num.) " + S(num[i]);
    }
    pos = right_b + 1;
}

// change F(...) to @n.F(...), and the same with Z
string add_dm_to_model(string const& s, int nr)
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

void get_info_points(Ftk const* F, string const& args, size_t& pos,
                     string& result)
{
    vector<int> v;
    parse_datasets(F, args, pos, v);

    // split the expressions in brackets
    if (pos < args.size())
        pos = args.find_first_not_of(" \t\r\n", pos);
    if (pos >= args.size() || args[pos] != '(')
        throw ExecuteError("Expected (expression) in brackets after @n");
    size_t right_b = find_matching_bracket(args, pos);
    if (right_b == string::npos)
        throw ExecuteError("Missing closing bracket `)'");
    const char *p = args.c_str() + pos + 1;
    vector<string> expressions;
    for (;;) {
        const char *end = find_outer_char(p, ",");
        if (end == NULL) {
            expressions.push_back(string(p, args.c_str() + right_b));
            break;
        }
        else {
            expressions.push_back(string(p, end));
            p = end+1;
        }
    }

    pos = right_b + 1;
    result += "# exported by fityk " VERSION "\n";

    // make tables of points and column titles
    bool only_active = !contains_element(expressions, "a");
    int ncol = v.size() * expressions.size();
    vector<vector<fp> > r(ncol);
    vector<string> titles(ncol);
    int n = 0;
    for (vector<int>::const_iterator i = v.begin(); i != v.end(); ++i)
        for (vector<string>::const_iterator j = expressions.begin();
                                                j != expressions.end(); ++j) {
            Data const* data = F->get_data(*i);
            titles[n] = data->get_title() + ":" + *j;
            string expr = add_dm_to_model(*j, *i);
            r[n] = get_all_point_expressions(expr, data, only_active);
            ++n;
        }

    // add TSV to `result'
    assert (titles.size() == r.size());
    for (size_t i = 0; i != titles.size(); ++i)
        result += (i == 0 ? "#" : "\t") + titles[i];
    result += "\n";
    const char *format= F->get_settings()->get_s("info_numeric_format").c_str();
    char buf[64];
    for (size_t i = 0; i != r[0].size(); ++i)
        for (size_t j = 0; j != r.size(); ++j) {
            snprintf(buf, 63, format, r[j][i]);
            result += buf;
            result += (j != r.size() - 1 ? '\t' : '\n');
        }
}

bool has_one_word(const char* p)
{
    while (isspace(*p) || ispunct(*p))
        ++p;
    while (isalnum(*p) || *p == '_')
        ++p;
    return (*p == '\0' || *p == ',');
}

size_t get_info_string(Ftk const* F, string const& args, bool full,
                       string& result,
                       size_t pos)
{
    if (!result.empty())
        result += "\n";
    string word = get_word(args, pos, " \t\r\n,[(");
    if (word.empty()) {
        // either args[pos:] is empty or it starts with bracket
        if (is_blank(args.c_str() + pos))
            throw ExecuteError("Info for what? No arguments supplied.");
        get_info_data_expr(F, args, pos, full, result);
    }
    else if (word == "version")
        get_info_version(full, result);
    else if (word == "variables")
        get_info_variables(F, full, result);
    else if (word == "types")
        get_info_types(full, result);
    else if (word == "functions")
        get_info_functions(F, full, result);
    else if (word == "datasets")
        get_info_datasets(F, full, result);
    else if (word == "commands")
        get_info_commands(F, args, pos, full, result);
    else if (word == "view")
        result += F->view.str();
    else if (word == "set")
        result += F->get_settings()->print_usage();
    else if (word == "fit_history")
        result += F->get_fit_container()->param_history_info();
    else if (word == "guess")
        get_info_guess(F, args, pos, result);
    else if (word == "debug") {
        string arg = get_word(args, pos);
        get_info_debug(F, arg, result);
    }
    else if (word == "der") {
        get_info_der(args, pos, result);
    }
    else if (word == "dops") {
        result += get_trans_repr(args.substr(pos));
        pos = args.size();
    }
    else if (word == "fit") {
        VecDM v = parse_in_data(F, args, pos);
        result += F->get_fit()->get_info(v);
    }
    else if (word == "errors") {
        VecDM v = parse_in_data(F, args, pos);
        result += F->get_fit()->get_error_info(v, full);
    }
    else if (word == "peaks") {
        VecDM v = parse_in_data(F, args, pos);
        get_info_peaks(F, v, full, result);
    }
    else if (word == "formula") {
        VecDM v = parse_in_data(F, args, pos);
        get_info_formula(F, v, full, result);
    }
    else if (word == "filename") {
        VecDM v = parse_in_data(F, args, pos);
        for (VecDM::const_iterator i=v.begin(); i!=v.end(); ++i)
            result += (*i)->data()->get_filename() + "\n";
    }
    else if (word == "title") {
        VecDM v = parse_in_data(F, args, pos);
        for (VecDM::const_iterator i=v.begin(); i!=v.end(); ++i)
            result += (*i)->data()->get_title() + "\n";
    }
    else if (word == "data") {
        VecDM v = parse_in_data(F, args, pos);
        for (VecDM::const_iterator i=v.begin(); i!=v.end(); ++i)
            result += (*i)->data()->get_info() + "\n";
    }
    else if (isupper(word[0]) && word.size() > 1)
        get_info_func_type(word, full, result);
    else if (word[0] == '%' && has_one_word(args.c_str()))
        result += F->find_function(word)->get_info(F, full);
    else if (word[0] == '$' && has_one_word(args.c_str()))
        get_info_variable(F, word, full, result);
    else if (word[0] == '@') {
        //TODO: parse expressions that start with @ of F,
        // e.g. F[0].height^2
        char *endptr;
        int ds = strtol(word.c_str() + 1, &endptr, 10);
        if (word == "@*" || *endptr == '\0'/*word == @number*/) {
            pos -= word.size(); // we will parse it again
            get_info_points(F, args, pos, result);
        }
        else if (endptr == word.c_str() + 1)
            throw ExecuteError("The `@' should be followed by number.");
        else if (strcmp(endptr, ".F") == 0)
            get_info_model(F, ds, args, pos, Model::kF, full, result);
        else if (strcmp(endptr, ".Z") == 0)
            get_info_model(F, ds, args, pos, Model::kZ, full, result);
        else if (strcmp(endptr, ".dF") == 0)
            get_info_model_der(F, ds, args, pos, result);
    }
    else if (word == "F")
        get_info_model(F, -1, args, pos, Model::kF, full, result);
    else if (word == "Z")
        get_info_model(F, -1, args, pos, Model::kZ, full, result);
    else if (word == "dF")
        get_info_model_der(F, -1, args, pos, result);
    else {
        pos -= word.size(); // we don't use `word' here
        get_info_data_expr(F, args, pos, full, result);
    }

    if (pos < args.size())
        pos = args.find_first_not_of(" \t\r\n", pos);
    if (pos < args.size() && args[pos] == ',')
        return get_info_string(F, args, full, result, pos+1);
    else {
        if (!result.empty() && *(result.end() - 1) == '\n')
            result.resize(result.size() - 1);
        return pos;
    }
}

bool get_info_string_safe(Ftk const* F, string const& args, bool full,
                          string& result)
{
    try {
        size_t pos = get_info_string(F, args, full, result);
        return (pos >= args.size());
    } catch (ExecuteError &) {
        return false;
    }
    return true;
}

void output_info(Ftk const* F, string const& args, bool full)
{
    string info;
    size_t pos = get_info_string(F, args, full, info);
    if (pos >= args.size()) {
        int max_screen_info_length = 2048;
        int more = info.length() - max_screen_info_length;
        if (more > 0) {
            info.resize(max_screen_info_length);
            info += "\n[... " + S(more) + " characters more...]";
        }
        F->rmsg(info);
    }
    else if (pos < args.size() - 2 && args[pos] == '>') {
        ++pos;
        ios::openmode mode = ios::trunc;
        if (args[pos] == '>') {
            ++pos;
            mode = ios::app;
        }
        pos = args.find_first_not_of(" \t\r\n", pos);
        size_t end;
        if (args[pos] == '\'') {
            ++pos;
            end = args.find('\'', pos);
            if (end == string::npos)
                //SyntaxError
                throw ExecuteError("missing ' in output filename");
        }
        else {
            end = args.find_first_of(" \t\r\n", pos);
        }
        string filename = args.substr(pos, end-pos);
        if (end < args.size() && args[end] == '\'')
            ++end;
        if (args.find_first_not_of(" \t\r\n", end) != string::npos)
            //SyntaxError
            throw ExecuteError("unexpected trailing chars after filename `"
                               + filename + "'");
        ofstream os(filename.c_str(), ios::out | mode);
        if (!os)
            throw ExecuteError("Can't open file: " + filename);
        os << info << endl;
    }
    else {
        //SyntaxError
        throw ExecuteError("Unknown info argument: " + args.substr(pos));
    }
}


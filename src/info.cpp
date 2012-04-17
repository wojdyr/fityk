// This file is part of fityk program. Copyright 2009 Marcin Wojdyr.
// Licence: GNU General Public License ver. 2+

/// Functions to execute commands: info, debug, print.

#include "info.h"
#include <string>
#include <vector>
#include <ctype.h>
#include <string.h>

#include <xylib/xylib.h> //get_version()
#include <boost/version.hpp> // BOOST_VERSION
extern "C" {
#include <lua.h> // LUA_RELEASE
}

#include "logic.h"
#include "func.h"
#include "tplate.h"
#include "data.h"
#include "fit.h"
#include "ast.h"
#include "model.h"
#include "guess.h"
#include "cparser.h"
#include "eparser.h"
#include "lexer.h"
#include "ui.h"
#include "runner.h" // args2range

using namespace std;


// get standard formula and make it parsable by the gnuplot program
string& gnuplotize_formula(string& formula)
{
    replace_all(formula, "^", "**");
    replace_words(formula, "ln", "log");
    // avoid integer division (1/2 == 0)
    size_t pos = 0;
    size_t len = formula.length();
    while ((pos = formula.find('/', pos)) != string::npos) {
        pos = formula.find_first_not_of(' ', pos+1);
        if (pos == string::npos || !isdigit(formula[pos]))
            continue;
        while (pos < len && isdigit(formula[pos]))
            ++pos;
        if (pos == formula.length() || formula[pos] != '.')
            formula.insert(pos, ".");
    }
    return formula;
}


void models_as_script(const Ftk* F, string& r, bool commented_defines)
{
    r += "# ------------  (un)defines  ------------";
    TplateMgr default_tpm;
    default_tpm.add_builtin_types(F->ui()->parser());
    v_foreach (Tplate::Ptr, i, default_tpm.tpvec()) {
        const Tplate* t = F->get_tpm()->get_tp((*i)->name);
        if (t == NULL || t->as_formula() != (*i)->as_formula())
            r += "\nundefine " + (*i)->name;
    }
    v_foreach (Tplate::Ptr, i, F->get_tpm()->tpvec()) {
        string formula = (*i)->as_formula();
        const Tplate* t = default_tpm.get_tp((*i)->name);
        if (t == NULL || t->as_formula() != formula)
            r += "\ndefine " + formula;
        else if (commented_defines)
            r += "\n# define " + formula;
    }
    r += "\n\n# ------------  variables and functions  ------------";
    // The script must not trigger VariableManager::remove_unreferred()
    // or VariableManager::auto_remove_functions() until all references
    // are reproduced.
    v_foreach (Variable*, i, F->variables())
        r += "\n$" + (*i)->name + " = " + (*i)->get_formula(F->parameters());
    r += "\n";
    v_foreach (Function*, i, F->functions())
        r +="\n" + (*i)->get_basic_assignment();
    r += "\n\n# ------------  models  ------------";
    for (int i = 0; i != F->get_dm_count(); ++i) {
        const Model* model = F->get_model(i);
        const vector<string>& ff = model->get_ff().names;
        if (!ff.empty())
            r += "\n@" + S(i) +  ": F = %" + join_vector(ff, " + %");
        vector<string> const& zz = model->get_zz().names;
        if (!zz.empty())
            r += "\n@" + S(i) +  ": Z = %" + join_vector(zz, " + %");
    }
}


namespace {

string info_compiler()
{
    return
        "Build system type: "
#ifdef CONFIGURE_BUILD
        CONFIGURE_BUILD
#else
        "UNKNOWN"
#endif

        "\nConfigured with: "
#ifdef CONFIGURE_ARGS
        CONFIGURE_ARGS
#else
        "UNKNOWN"
#endif

        "\nCompiler: "
#if defined(__GNUC__)
        "GCC"
#elif defined(_MSC_VER)
        "MSVC++"
#else
        "UNKNOWN"
#endif

#ifdef __VERSION__
        " " __VERSION__
#endif

        "\nCompilation date: " __DATE__
        "\nBoost version: " + S(BOOST_VERSION / 100000)
                      + "." + S(BOOST_VERSION / 100 % 1000)
                      + "." + S(BOOST_VERSION % 100)
        + "\nxylib version: " + xylib_get_version()
        + "\n" LUA_RELEASE
        ;
}

string get_variable_info(const Ftk* F, const Variable* v)
{
    string s = "$" + v->name + " = " + v->get_formula(F->parameters()) + " = "
               + F->settings_mgr()->format_double(v->get_value());
    const RealRange& d = v->domain;
    if (!d.from_inf() || !d.to_inf())
        s += "  [" + (d.from_inf() ? S("") : S(d.from)) + " : "
             + (d.to_inf() ? S("") : S(d.to)) + "]";
    if (VariableManager::is_auto(v->name))
        s += "  [auto]";
    return s;
}


void info_functions(const Ftk* F, const string& name, string& result)
{
    if (!contains_element(name, '*')) {
        const Function *f = F->find_function(name);
        result += f->get_basic_assignment();
    }
    else {
        v_foreach (Function*, i, F->functions())
            if (match_glob((*i)->name.c_str(), name.c_str()))
                result += (result.empty() ? "" : "\n")
                          + (*i)->get_basic_assignment();
    }
}

void info_variables(const Ftk* F, const string& name, string& result)
{
    if (!contains_element(name, '*')) {
        const Variable* var = F->find_variable(name);
        result += get_variable_info(F, var);
    }
    else {
        v_foreach (Variable*, i, F->variables())
            if (match_glob((*i)->name.c_str(), name.c_str()))
                result += (result.empty() ? "" : "\n")
                          + get_variable_info(F, *i);
    }
}

void info_func_type(const Ftk* F, const string& functype, string& result)
{
    const Tplate* tp = F->get_tpm()->get_tp(functype);
    if (tp == NULL)
        result += "undefined";
    else {
        result += tp->as_formula();
        if (!tp->op_trees.empty()) {
            vector<string> args = tp->fargs;
            args.push_back("x");
            const char* num_format = F->get_settings()->numeric_format.c_str();
            OpTreeFormat fmt = { num_format, &args };
            result += "\n = " + tp->op_trees.back()->str(fmt);
        }
    }
}

string info_func_props(const Ftk* F, const string& name)
{
    const Function* f = F->find_function(name);
    string s = f->tp()->as_formula();
    for (int i = 0; i < f->used_vars().get_count(); ++i) {
        Variable const* v = F->get_variable(f->used_vars().get_idx(i));
        s += "\n" + f->get_param(i) + " = " + get_variable_info(F, v);
    }
    realt a;
    const vector<string>& fargs = f->tp()->fargs;;
    if (f->get_center(&a) && !contains_element(fargs, string("center")))
        s += "\nCenter: " + S(a);
    if (f->get_height(&a) && !contains_element(fargs, string("height")))
        s += "\nHeight: " + S(a);
    if (f->get_fwhm(&a) && !contains_element(fargs, string("fwhm")))
        s += "\nFWHM: " + S(a);
    if (f->get_area(&a) && !contains_element(fargs, string("area")))
        s += "\nArea: " + S(a);
    v_foreach (string, i, f->get_other_prop_names())
        s += "\n" + *i + ": " + S(f->get_other_prop(*i));
    return s;
}


void info_history(const Ftk* F, const Token& t1, const Token& t2,
                  string& result)
{
    const vector<UserInterface::Cmd>& cmds = F->ui()->cmds();
    int from = 0, to = cmds.size();
    if (t1.type == kTokenExpr) {
        from = iround(t1.value.d);
        if (from < 0)
            from += cmds.size();
    }
    if (t2.type == kTokenExpr) {
        to = iround(t2.value.d);
        if (to < 0)
            to += cmds.size();
    }
    if (from < 0 || to > (int) cmds.size())
        throw ExecuteError("wrong history range");
    for (int i = from; i < to; ++i)
        result += cmds[i].str() + "\n";
}


void info_guess(const Ftk* F, int ds, const RealRange& range, string& result)
{
    if (range.from >= range.to)
        result += "invalid range";
    else {
        int lb = F->get_data(ds)->get_lower_bound_ac(range.from);
        int rb = F->get_data(ds)->get_upper_bound_ac(range.to);
        Guess g(F->get_settings());
        g.initialize(F->get_dm(ds), lb, rb, -1);
        boost::array<double,4> peak_v = g.estimate_peak_parameters();
        for (int i = 0; i != 4; ++i)
            result += (i != 0 ? ", " : "")
                      + Guess::peak_traits[i] + ": " + S(peak_v[i]);
        result += "\n";
        boost::array<double,3> lin_v = g.estimate_linear_parameters();
        for (int i = 0; i != 3; ++i)
            result += (i != 0 ? ", " : "")
                      + Guess::linear_traits[i] + ": " + S(lin_v[i]);
    }
}

void save_state(const Ftk* F, string& r)
{
    if (!r.empty())
        r += "\n";
    r += fityk_version_line + S(". Created: ") + time_now();
    r += "\nset verbosity = -1 #the rest of the file is not shown";
    r += "\nset autoplot = 0";
    r += "\nreset";
    r += "\n# ------------  settings  ------------";
    // do not set autoplot and verbosity here
    vector<string> e = F->settings_mgr()->get_key_list("");
    v_foreach(string, i, e) {
        if (*i == "autoplot" || *i == "verbosity")
            continue;
        string v = F->settings_mgr()->get_as_string(*i);
        if (*i == "cwd" && v == "''") // avoid this: set cwd=''
            continue;
        r += "\nset " + *i + " = " + v;
    }
    r += "\n";
    r += "\n# ------------  datasets ------------";
    for (int i = 0; i != F->get_dm_count(); ++i) {
        const Data* data = F->get_data(i);
        if (i != 0)
            r += "\n@+ = 0";
        r += "\nuse @" + S(i);
        r += "\ntitle = '" + data->get_title() + "'";
        int m = data->points().size();
        r += "\nM=" + S(m);
        r += "\nX=" + eS(data->get_x_max()) + "# =max(x), prevents sorting.";
        for (int j = 0; j != m; ++j) {
            const Point& p = data->points()[j];
            string idx = "[" + S(j) + "]=";
            r += "\nX" + idx + eS(p.x) +
                 ", Y" + idx + eS(p.y) +
                 ", S" + idx + eS(p.sigma) +
                 ", A" + idx + (p.is_active ? "1" : "0");
        }
        r += "\n";
    }
    r += "\n\n";
    models_as_script(F, r, true);
    r += "\n";
    r += "\nplot " + F->view.str();
    r += "\nuse @" + S(F->default_dm());
    r += "\nset autoplot = " + F->settings_mgr()->get_as_string("autoplot");
    r += "\nset verbosity = " + F->settings_mgr()->get_as_string("verbosity");
}

static
string format_error_info(const Ftk* F, const vector<realt>& errors)
{
    string s;
    const SettingsMgr *sm = F->settings_mgr();
    const vector<realt>& pp = F->parameters();
    assert(pp.size() == errors.size());
    const Fit* fit = F->get_fit();
    for (size_t i = 0; i != errors.size(); ++i) {
        if (fit->is_param_used(i)) {
            realt err = errors[i];
            s += "\n$" + F->find_variable_handling_param(i)->name
                + " = " + sm->format_double(pp[i])
                + " +- " + (err == 0. ? string("??") : sm->format_double(err));
        }
    }
    return s;
}

int eval_one_info_arg(const Ftk* F, int ds, const vector<Token>& args, int n,
                      string& result)
{
    int ret = 0;
    if (args[n].type == kTokenLname) {
        const string word = args[n].as_string();

        // no args
        if (word == "version")
            result += "Fityk " VERSION;
        else if (word == "compiler")
            result += info_compiler();
        else if (word == "variables")
            for (size_t i = 0; i < F->variables().size(); ++i)
                result += (i > 0 ? " $" : "$") + F->get_variable(i)->name;
        else if (word == "types")
            v_foreach (Tplate::Ptr, i, F->get_tpm()->tpvec())
                result += (*i)->name + " ";
        else if (word == "functions")
            for (size_t i = 0; i < F->functions().size(); ++i)
                result += (i > 0 ? " %" : "%") + F->get_function(i)->name;
        else if (word == "dataset_count")
            result += S(F->get_dm_count());
        else if (word == "view")
            result += F->view.str();
        else if (word == "fit_history")
            result += F->get_fit_container()->param_history_info();
        else if (word == "filename") {
            result += F->get_data(ds)->get_filename();
        }
        else if (word == "title") {
            result += F->get_data(ds)->get_title();
        }
        else if (word == "data") {
            result += F->get_data(ds)->get_info();
        }
        else if (word == "formula") {
            const char* fmt = F->get_settings()->numeric_format.c_str();
            result += F->get_model(ds)->get_formula(false, fmt, false);
        }
        else if (word == "gnuplot_formula") {
            const char* fmt = F->get_settings()->numeric_format.c_str();
            string formula = F->get_model(ds)->get_formula(false, fmt, false);
            result += gnuplotize_formula(formula);
        }
        else if (word == "simplified_formula") {
            const char* fmt = F->get_settings()->numeric_format.c_str();
            result += F->get_model(ds)->get_formula(true, fmt, false);
        }
        else if (word == "simplified_gnuplot_formula") {
            const char* fmt = F->get_settings()->numeric_format.c_str();
            string formula = F->get_model(ds)->get_formula(true, fmt, false);
            result += gnuplotize_formula(formula);
        }
        else if (word == "models") {
            models_as_script(F, result, false);
        }
        else if (word == "state") {
            save_state(F, result);
        }
        else if (word == "peaks") {
            vector<realt> no_errors; // empty vec -> no errors
            result += F->get_model(ds)->get_peak_parameters(no_errors);
        }
        else if (word == "peaks_err") {
            //FIXME: assumes the dataset was fitted separately
            DataAndModel* dm = const_cast<DataAndModel*>(F->get_dm(ds));
            vector<DataAndModel*> dms(1, dm);
            vector<realt> errors = F->get_fit()->get_standard_errors(dms);
            result += F->get_model(ds)->get_peak_parameters(errors);
        }
        else if (word == "history_summary")
            result += F->ui()->get_history_summary();

        else if (word == "set") {
            if (args[n+1].type == kTokenLname) {
                string key = args[n+1].as_string();
                result += F->settings_mgr()->get_as_string(key) + "\ntype: "
                        + F->settings_mgr()->get_type_desc(key);
            }
            else {
                result += "Available options:";
                vector<string> e = F->settings_mgr()->get_key_list("");
                v_foreach(string, i, e)
                    result += "\n " + *i
                             + " <" + F->settings_mgr()->get_type_desc(*i)
                             + "> = " + F->settings_mgr()->get_as_string(*i);
            }
            ++ret;
        }

        // optional range
        else if (word == "history") {
            info_history(F, args[n+1], args[n+2], result);
            ret += 2;
        }
        else if (word == "guess") {
            RealRange range = args2range(args[n+1], args[n+2]);
            info_guess(F, ds, range, result);
            ret += 2;
        }

        // optionally takes datasets as args
        else if (word == "fit" || word == "errors" || word == "cov" ||
                 word == "confidence") {
            double level = 0.;
            if (word == "confidence") {
                level = args[n+1].value.d;
                if (level <= 0 || level >= 100)
                    throw ExecuteError("confidence level outside of (0,100)");
                ++n;
                ++ret;
            }
            vector<DataAndModel*> v;
            while (args[n+1].type == kTokenDataset) {
                int k = args[n+1].value.i;
                DataAndModel* dm = const_cast<DataAndModel*>(F->get_dm(k));
                v.push_back(dm);
                ++n;
                ++ret;
            }
            assert(args[n+1].type == kTokenNop); // separator
            ++ret;
            if (v.empty()) {
                DataAndModel* dm = const_cast<DataAndModel*>(F->get_dm(ds));
                v.push_back(dm);
            }
            if (word == "fit")
                result += F->get_fit()->get_goodness_info(v);
            else if (word == "errors") {
                result += "Standard errors:";
                vector<realt> errors = F->get_fit()->get_standard_errors(v);
                result += format_error_info(F, errors);
            }
            else if (word == "confidence") {
                result += S(level) + "% confidence intervals:";
                vector<realt> limits =
                    F->get_fit()->get_confidence_limits(v, level);
                result += format_error_info(F, limits);
            }
            else //if (word == "cov")
                result += F->get_fit()->get_cov_info(v);
        }

        // one arg: $var
        else if (word == "refs") {
            string name = Lexer::get_string(args[n+1]);
            vector<string> refs = F->get_variable_references(name);
            result += join_vector(refs, ", ");
            ++ret;
        }

        // one arg: %func
        else if (word == "prop") {
            string name = Lexer::get_string(args[n+1]);
            result += info_func_props(F, name);
            ++ret;
        }
    }

    // FuncType
    else if (args[n].type == kTokenCname)
        info_func_type(F, args[n].as_string(), result);

    // %func
    else if (args[n].type == kTokenFuncname)
        info_functions(F, Lexer::get_string(args[n]), result);

    // $var
    else if (args[n].type == kTokenVarname)
        info_variables(F, Lexer::get_string(args[n]), result);

    // handle [@n.]F/Z['['expr']']
    else if ((args[n].type == kTokenUletter &&
                             (*args[n].str == 'F' || *args[n].str == 'Z'))
             || args[n].type == kTokenDataset) {
        int k = ds;
        if (args[n].type == kTokenDataset) {
            k = args[n].value.i;
            ++n;
            ++ret;
        }
        const Model* model = F->get_model(k);
        char fz = *args[n].str;
        if (is_index(n+1, args) && args[n+1].type == kTokenExpr) {
            ++ret;
            int idx = iround(args[n+1].value.d);
            const string& name = model->get_func_name(fz, idx);
            const Function *f = F->find_function(name);
            result += f->get_basic_assignment();
        }
        else {
            const vector<string>& names = model->get_fz(fz).names;
            if (!names.empty())
                result += "%" + join_vector(names, " + %");
        }
    }
    ++ret;
    return ret;
}

void eval_one_print_arg(const Ftk* F, int ds, const Token& t, string& result)
{
    if (t.type == kTokenString)
        result += Lexer::get_string(t);
    else if (t.type == kTokenExpr)
        result += F->settings_mgr()->format_double(t.value.d);
    else if (t.as_string() == "filename")
        result += F->get_data(ds)->get_filename();
    else if (t.as_string() == "title")
        result += F->get_data(ds)->get_title();
    else
        assert(0);
}

int eval_print_args(const Ftk* F, int ds, const vector<Token>& args, int len,
                    string& result)
{
    // args: condition (expr|string|"filename"|"title")+
    string sep = " ";
    if (args[0].type == kTokenNop) {
        for (int n = 1; n < len; ++n) {
            if (n != 1)
                result += sep;
            eval_one_print_arg(F, ds, args[n], result);
        }
    }
    else {
        vector<ExpressionParser> expr_parsers(args.size() + 1, F);
        for (int i = 0; i < len; ++i)
            if (args[i].type == kTokenExpr) {
                Lexer lex(args[i].str);
                expr_parsers[i].parse_expr(lex, ds);
            }
        const vector<Point>& points = F->get_data(ds)->points();
        for (int k = 0; k != (int) points.size(); ++k) {
            if (args[0].type == kTokenExpr) {
                double cond = expr_parsers[0].calculate(k, points);
                if (fabs(cond) < 0.5)
                    continue;
            }
            if (!result.empty())
                result += "\n";
            for (int n = 1; n < len; ++n) {
                if (n != 1)
                    result += sep;
                if (args[n].type == kTokenExpr) {
                    double value = expr_parsers[n].calculate(k, points);
                    result += F->settings_mgr()->format_double(value);
                }
                else
                    eval_one_print_arg(F, ds, args[n], result);
            }
        }
    }
    return len;
}

} // anonymous namespace

int eval_info_args(const Ftk* F, int ds, const vector<Token>& args, int len,
                   string& result)
{
    int n = 0;
    while (n < len) {
        if (!result.empty())
            result += "\n";
        n += eval_one_info_arg(F, ds, args, n, result);
    }
    if (len == 0) { // special case
        result += "Available arguments:\n";
        const char** arg = info_args;
        while (*arg != NULL) {
            result += *arg + S(" ");
            ++arg;
        }
        result += "%* $* AnyFunctionT";
    }
    return n;
}

void command_redirectable(const Ftk* F, int ds,
                          CommandType cmd, const vector<Token>& args)
{
    string info;
    int len = args.size();
    bool redir = (len >= 2 && (args[len-2].type == kTokenGT ||
                               args[len-2].type == kTokenAppend));
    int n_args = redir ? len - 2 : len;
    if (cmd == kCmdInfo)
        eval_info_args(F, ds, args, n_args, info);
    else // cmd == kCmdPrint
        eval_print_args(F, ds, args, n_args, info);
    if (!redir) { // no redirection to file
        int max_screen_info_length = 2048;
        int more = (int) info.length() - max_screen_info_length;
        if (more > 0) {
            info.resize(max_screen_info_length);
            info += "\n[... " + S(more) + " characters more...]";
        }
        F->rmsg(info);
    }
    else {
        assert(args.back().type == kTokenFilename ||
               args.back().type == kTokenString);
        string filename = Lexer::get_string(args.back());
        const char* mode = args[len-2].type == kTokenGT ? "w" : "a";
        FILE *f = fopen(filename.c_str(), mode);
        if (!f)
            throw ExecuteError("Can't open file: " + filename);
        fprintf(f, "%s\n", info.c_str());
        fclose(f);
    }
}

void command_debug(const Ftk* F, int ds, const Token& key, const Token& rest)
{
    // args: any-token rest-of-line
    string r;
    string word = key.as_string();

    if (word == "parse") {
        Parser parser(const_cast<Ftk*>(F));
        try {
            Lexer lex(rest.str);
            while (parser.parse_statement(lex))
                r += parser.get_statements_repr();
        }
        catch (fityk::SyntaxError& e) {
            r += string("ERR: ") + e.what();
        }
    }

    else if (word == "lex") {
        Lexer lex(rest.str);
        for (Token t = lex.get_token(); t.type != kTokenNop; t =lex.get_token())
            r += token2str(t) + "\n";
    }

    else if (word == "expr") {
        Lexer lex(rest.str);
        try {
            ExpressionParser parser(F);
            parser.parse_expr(lex, -1);
            r += vm2str(parser.vm());
        }
        catch (fityk::SyntaxError& e) {
            r += "ERROR at " + S(lex.scanned_chars()) + ": " + e.what();
        }
        if (lex.peek_token().type != kTokenNop)
            r += "\nnext token: " + token2str(lex.peek_token());
    }

    else if (word == "der") {
        get_derivatives_str(rest.str, r);
    }

    // show values of derivatives for all variables
    else if (word == "rd") {
        for (int i = 0; i < size(F->variables()); ++i) {
            const Variable* var = F->get_variable(i);
            r += "$" + var->name + ": ";
            v_foreach (Variable::ParMult, i, var->recursive_derivatives())
                r += "p" + S(i->p) + "=$"
                    + F->find_variable_handling_param(i->p)->name
                    + " *" + S(i->mult) + "    ";
            r += "\n";
        }
    }

    // show used_vars from VariableUser
    else if (word == "idx") {
        for (size_t i = 0; i != F->functions().size(); ++i) {
            const Function *f = F->get_function(i);
            r += S(i) + ": %" + f->name + ": ";
            const IndexedVars& uv = f->used_vars();
            for (int i = 0; i != uv.get_count(); ++i)
                r += uv.get_name(i) + "/" + S(uv.get_idx(i)) + " ";
            r += "\n";
        }
        for (size_t i = 0; i != F->variables().size(); ++i) {
            const Variable *v = F->get_variable(i);
            r += S(i) + ": $" + v->name + ": ";
            const IndexedVars& uv = v->used_vars();
            for (int i = 0; i != uv.get_count(); ++i)
                r += uv.get_name(i) + "/" + S(uv.get_idx(i)) + " ";
            r += "\n";
        }
    }

    // compares numeric and symbolic derivatives
    else if (word == "df") {
        Lexer lex(rest.str);
        ExpressionParser ep(F);
        ep.parse_expr(lex, ds);
        realt x = ep.calculate();
        const Model* model = F->get_model(ds);
        vector<realt> symb = model->get_symbolic_derivatives(x);
        vector<realt> num = model->get_numeric_derivatives(x, 1e-4);
        assert (symb.size() == num.size());
        int n = symb.size() - 1;
        r += "F(" + S(x) + ")=" + S(model->value(x));
        for (int i = 0; i < n; ++i) {
            if (is_neq(symb[i], 0) || is_neq(num[i], 0))
                r += "\ndF / d$" + F->find_variable_handling_param(i)->name
                    + " = (symb.) " + S(symb[i]) + " = (num.) " + S(num[i]);
        }
        r += "\ndF / dx = (symb.) " + S(symb[n]) + " = (num.) " + S(num[n]);
    }

    // show %function's bytecode
    else if (key.type == kTokenFuncname) {
        const Function* f = F->find_function(Lexer::get_string(key));
        r += f->get_bytecode();
    }

    // show derivatives of $variable
    else if (key.type == kTokenVarname) {
        const Variable* v = F->find_variable(Lexer::get_string(key));
        vector<string> vn;
        v_foreach (string, i, v->used_vars().names())
            vn.push_back("$" + *i);
        for (int i = 0; i < v->used_vars().get_count(); ++i) {
            const char* num_format = F->get_settings()->numeric_format.c_str();
            OpTreeFormat fmt = { num_format, &vn };
            string formula = v->get_op_trees()[i]->str(fmt);
            double value = v->get_derivative(i);
            if (i != 0)
                r += "\n";
            r += "d($" + v->name + ")/d($" + v->used_vars().get_name(i) + "): "
              + formula + " == " + F->settings_mgr()->format_double(value);
        }
    }

    // tests the match_glob()
    else if (word == "glob") {
        Lexer lex(rest.str);
        string pattern = lex.get_filename_token().as_string();
        Token t;
        while ((t = lex.get_filename_token()).type != kTokenNop) {
            string s = t.as_string();
            if (match_glob(s.c_str(), pattern.c_str()))
                r += s + " ";
        }
    }

    else
        r += "unexpected arg: " + word;
    F->rmsg(r);
}

void parse_and_eval_info(Ftk *F, const string& s, int dataset,
                         string& result)
{
    Lexer lex(s.c_str());
    Parser parser(F);
    parser.statement().datasets.push_back(dataset);
    vector<Token> args;
    parser.parse_info_args(lex, args);
    if (lex.peek_token().type != kTokenNop)
        lex.throw_syntax_error("unexpected argument");
    eval_info_args(F, dataset, args, args.size(), result);
}


// This file is part of fityk program. Copyright 2009 Marcin Wojdyr.
// Licence: GNU General Public License ver. 2+
// $Id: $

/// Functions to execute commands: info, debug, print.

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
        "MS VC++"
#else
        "UNKNOWN"
#endif

        "\nCompiler version: "
#ifdef __VERSION__
        __VERSION__
#else
        "UNKNOWN"
#endif

        "\nCompilation date: " __DATE__
        "\nBoost version: " + S(BOOST_VERSION / 100000)
                      + "." + S(BOOST_VERSION / 100 % 1000)
                      + "." + S(BOOST_VERSION % 100)
        + "\nxylib version: " + xylib_get_version();
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
        result += F->get_variable_info(var);
    }
    else {
        v_foreach (Variable*, i, F->variables())
            if (match_glob((*i)->name.c_str(), name.c_str()))
                result += (result.empty() ? "" : "\n")
                          + F->get_variable_info(*i);
    }
}

void info_func_type(const Ftk* F, const string& functype, string& result)
{
    const Tplate* tp = F->get_tpm()->get_tp(functype);
    if (tp == NULL)
        result += "undefined";
    else {
        result += tp->as_formula();
        if (tp->rhs.find(" where ") != string::npos)
            result += "\n = " + Function::do_substitutions(tp->rhs);
    }
}

void info_history(const Ftk* F, const Token& t1, const Token& t2,
                  string& result)
{
    const vector<Commands::Cmd>& cmds = F->get_ui()->get_commands().get_cmds();
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

void save_state(const Ftk* F, string& r)
{
    if (!r.empty())
        r += "\n";
    r += fityk_version_line;
    r += "\n## dumped at: " + time_now();
    r += "\nset verbosity = quiet #the rest of the file is not shown";
    r += "\nset autoplot = never";
    r += "\nreset";
    r += "\n# ------------  settings  ------------";
    r += "\n" + F->get_settings()->set_script();
    r += "\n# ------------  (un)defines  ------------";
    TplateMgr default_tpm;
    default_tpm.add_builtin_types(F->get_ui()->parser());
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
        else
            r += "\n# define " + formula;
    }
    r += "\n";
    r += "\n# ------------  variables and functions  ------------";
    // The script must not trigger VariableManager::remove_unreferred()
    // or VariableManager::auto_remove_functions() until all references
    // are reproduced.
    v_foreach (Variable*, i, F->variables())
        r += "\n" + (*i)->xname + " = " + (*i)->get_formula(F->parameters());
    r += "\n";
    v_foreach (Function*, i, F->functions())
        r +="\n" + (*i)->get_basic_assignment();
    r += "\n";
    r += "\n# ------------  datasets and models  ------------";
    for (int i = 0; i != F->get_dm_count(); ++i) {
        const Data* data = F->get_data(i);
        if (i != 0)
            r += "\n@+ = 0";
        // TODO set default dataset "use %s"
        //r += "\nuse @" + S(i);
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
        const Model* model = F->get_model(i);
        const vector<string>& ff = model->get_ff().names;
        if (!ff.empty())
            r += "\nF = %" + join_vector(ff, " + %");
        vector<string> const& zz = model->get_zz().names;
        if (!zz.empty())
            r += "\nZ = %" + join_vector(zz, " + %");
        r += "\n";
    }
    r += "\nplot " + F->view.str();
    // TODO set current dataset "use %s"
    r += "\nset autoplot = " + F->get_settings()->getp("autoplot");
    r += "\nset verbosity = " + F->get_settings()->getp("verbosity");
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
                result += (i > 0 ? " " : "") + F->get_variable(n)->xname;
        else if (word == "types")
            v_foreach (Tplate::Ptr, i, F->get_tpm()->tpvec())
                result += (*i)->name + " ";
        else if (word == "functions")
            for (size_t i = 0; i < F->functions().size(); ++i)
                result += (i > 0 ? " " : "") + F->get_function(n)->xname;
        else if (word == "dataset_count")
            result += S(F->get_dm_count());
        else if (word == "datasets") {
            for (int i = 0; i < F->get_dm_count(); ++i)
                result += (i > 0 ? "\n@" : "@") + S(i) + ": "
                        + F->get_data(i)->get_title();
        }
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
            bool gnuplot =
                F->get_settings()->get_e("formula_export_style") == 1;
            result += F->get_model(ds)->get_formula(false, gnuplot);
        }
        else if (word == "simplified_formula") {
            bool gnuplot =
                F->get_settings()->get_e("formula_export_style") == 1;
            result += F->get_model(ds)->get_formula(true, gnuplot);
        }
        else if (word == "state") {
            save_state(F, result);
        }
        else if (word == "peaks") {
            vector<fp> errors;
            result += F->get_model(ds)->get_peak_parameters(errors);
        }
        else if (word == "peaks_err") {
            //FIXME: assumes the dataset was fitted separately
            DataAndModel* dm = const_cast<DataAndModel*>(F->get_dm(ds));
            vector<DataAndModel*> dms(1, dm);
            vector<fp> errors = F->get_fit()->get_standard_errors(dms);
            result += F->get_model(ds)->get_peak_parameters(errors);
        }
        else if (word == "history_summary")
            result += F->get_ui()->get_commands().get_history_summary();

        else if (word == "set") {
            if (args[n+1].type == kTokenLname) {
                string key = args[n+1].as_string();
                result += F->get_settings()->infop(key);
            }
            else
                result += F->get_settings()->print_usage();
            ++ret;
        }

        // optional range
        else if (word == "history") {
            info_history(F, args[n+1], args[n+2], result);
            ret += 2;
        }
        else if (word == "guess") {
            RealRange range = args2range(args[n+1], args[n+2]);
            if (range.from >= range.to)
                result += "invalid range";
            else {
                int lb = F->get_data(ds)->get_lower_bound_ac(range.from);
                int rb = F->get_data(ds)->get_upper_bound_ac(range.to);
                Guess g(F->get_settings());
                g.initialize(F->get_dm(ds), lb, rb, -1);
                g.get_guess_info(result);
            }
            ret += 2;
        }

        // optionally takes datasets as args
        else if (word == "fit" || word == "errors" || word == "cov") {
            vector<DataAndModel*> v;
            ++n;
            while (args[n].type == kTokenDataset) {
                int k = args[n].value.i;
                DataAndModel* dm = const_cast<DataAndModel*>(F->get_dm(k));
                v.push_back(dm);
                ++n;
                ++ret;
            }
            if (v.empty()) {
                DataAndModel* dm = const_cast<DataAndModel*>(F->get_dm(ds));
                v.push_back(dm);
            }
            if (word == "fit")
                result += F->get_fit()->get_goodness_info(v);
            else if (word == "errors")
                result += F->get_fit()->get_error_info(v);
            else //if (word == "cov")
                result += F->get_fit()->get_cov_info(v);
        }

        // one arg: $var
        else if (word == "refs") {
            string name = Lexer::get_string(args[n]);
            vector<string> refs = F->get_variable_references(name);
            result += join_vector(refs, ", ");
            ++ret;
        }

        // one arg: %func
        else if (word == "par") {
            string name = Lexer::get_string(args[n]);
            result += F->find_function(name)->get_par_info(F);
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
            ++ret;
        }
        const Model* model = F->get_model(k);
        char fz = *args[n].str;
        if (is_index(n+1, args) && args[n+1].type == kTokenExpr) {
            ++ret;
            int idx = iround(args[n].value.d);
            const string& name = model->get_func_name(fz, idx);
            const Function *f = F->find_function(name);
            result += f->get_basic_assignment();
        }
        else {
            result += join_vector(model->get_fz(fz).names, ", ");
        }
    }
    ++ret;
    return ret;
}

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

void eval_one_print_arg(const Ftk* F, int ds, const Token& t, string& result)
{
    if (t.type == kTokenString)
        result += Lexer::get_string(t);
    else if (t.type == kTokenExpr)
        result += F->get_settings()->format_double(t.value.d);
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
                    result += F->get_settings()->format_double(value);
                }
                else
                    eval_one_print_arg(F, ds, args[n], result);
            }
        }
    }
    return len;
}

string get_info_string(Ftk const* F, string const& args)
{
    Lexer lex(args.c_str());
    Parser cp(const_cast<Ftk*>(F));
    vector<Token> tt;
    cp.parse_info_args(lex, tt);
    if (lex.peek_token().type != kTokenNop)
        lex.throw_syntax_error("unexpected argument");
    string result;
    eval_info_args(F, -1, tt, tt.size(), result);
    return result;
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
        assert(args.back().type == kTokenFilename);
        ios::openmode mode = ((args.end() - 1)->type == kTokenGT ? ios::trunc
                                                                 : ios::app);
        string filename = args.back().as_string();
        ofstream os(filename.c_str(), ios::out | mode);
        if (!os)
            throw ExecuteError("Can't open file: " + filename);
        os << info << endl;
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
            r += parser.list_ops();
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
            Variable const* var = F->get_variable(i);
            r += var->xname + ": ";
            v_foreach (Variable::ParMult, i, var->recursive_derivatives())
                r += "p" + S(i->p) + "="
                    + F->find_variable_handling_param(i->p)->xname
                    + " *" + S(i->mult) + "    ";
            r += "\n";
        }
    }

    // show varnames and var_idx from VariableUser
    else if (word == "idx") {
        for (size_t i = 0; i != F->functions().size(); ++i)
            r += S(i) + ": " + F->get_function(i)->get_debug_idx_info() + "\n";
        for (size_t i = 0; i != F->variables().size(); ++i)
            r += S(i) + ": " + F->get_variable(i)->get_debug_idx_info() + "\n";
    }

    // compares numeric and symbolic derivatives
    else if (word == "df") {
        Lexer lex(rest.str);
        ExpressionParser ep(F);
        ep.parse_expr(lex, ds);
        double x = ep.calculate();
        Model const* model = F->get_model(ds);
        vector<fp> symb = model->get_symbolic_derivatives(x);
        vector<fp> num = model->get_numeric_derivatives(x, 1e-4);
        assert (symb.size() == num.size());
        r += "F(" + S(x) + ")=" + S(model->value(x));
        for (int i = 0; i < size(num); ++i) {
            if (is_neq(symb[i], 0) || is_neq(num[i], 0))
                r += "\ndF / d " + F->find_variable_handling_param(i)->xname
                    + " = (symb.) " + S(symb[i]) + " = (num.) " + S(num[i]);
        }
    }

    // show %function's bytecode
    else if (key.type == kTokenFuncname) {
        Function const* f = F->find_function(Lexer::get_string(key));
        r += f->get_bytecode();
    }

    // show derivatives of $variable
    else if (key.type == kTokenVarname) {
        Variable const* v = F->find_variable(Lexer::get_string(key));
        vector<string> vn = concat_pairs("$", v->get_varnames());
        for (int i = 0; i < v->get_vars_count(); ++i) {
            string formula = v->get_op_trees()[i]->str(&vn);
            double value = v->get_derivative(i);
            if (i != 0)
                r += "\n";
            r += "d(" + v->xname + ")/d($" + v->get_var_name(i) + "): "
              + formula + " == " + F->get_settings()->format_double(value);
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


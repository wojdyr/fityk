// This file is part of fityk program. Copyright 2009 Marcin Wojdyr.
// Licence: GNU General Public License ver. 2+
// $Id: $

/// Parse and execute the info command

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
#include "cparser.h"
#include "eparser.h"
#include "lexer.h"
#include "ui.h"

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

void info_variables(Ftk const* F, string& result)
{
    result += "Defined variables:";
    vector<Variable*> const &vv = F->get_variables();
    for (vector<Variable*>::const_iterator i = vv.begin(); i != vv.end(); ++i)
        result += "\n" + F->get_variable_info(*i);
}

void info_types(string& result)
{
    result += "Defined function types:";
    vector<string> const& tt = Function::get_all_types();
    for (vector<string>::const_iterator i = tt.begin(); i != tt.end(); ++i)
        result += "\n" + Function::get_formula(*i);
}

void info_functions(Ftk const* F, string& result)
{
    vector<Function*> const &ff = F->get_functions();
    result += "Defined functions:";
    for (vector<Function*>::const_iterator i = ff.begin(); i != ff.end(); ++i)
        result += "\n" + (*i)->get_basic_assignment();
}

void info_func_type(string const& functype, string& result)
{
    string m = Function::get_formula(functype);
    if (m.empty())
        result += "undefined";
    else {
        result += m;
        if (m.find(" where ") != string::npos)
            result += "\n = " + Function::get_rhs_from_formula(m);
    }
}

void info_history(Ftk const* F, const Token& t1, const Token& t2,
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

void args2range(const Token& t1, const Token& t2, RealRange &range)
{
    if (t1.type == kTokenExpr) {
        range.from = RealRange::kNumber;
        range.from_val = t1.value.d;
    }
    else
        range.from = RealRange::kInf;
    if (t2.type == kTokenExpr) {
        range.to = RealRange::kNumber;
        range.to_val = t2.value.d;
    }
    else
        range.to = RealRange::kInf;
}

int eval_info_args(const Ftk* F, int ds, const vector<Token>& args,
                   string& result)
{
    int n = 0;
    while (n < (int) args.size()) {
        if (!result.empty())
            result += "\n";

        if (args[n].type == kTokenLname) {
            const string word = args[n].as_string();

            // no args
            if (word == "version")
                result += "Fityk " VERSION;
            else if (word == "compiler")
                result += info_compiler();
            else if (word == "variables")
                for (size_t i = 0; i < F->get_variables().size(); ++i)
                    result += (i > 0 ? " " : "") + F->get_variable(n)->xname;
            else if (word == "variables_full")
                info_variables(F, result);
            else if (word == "types")
                result += join_vector(Function::get_all_types(), " ");
            else if (word == "types_full")
                info_types(result);
            else if (word == "functions")
                for (size_t i = 0; i < F->get_functions().size(); ++i)
                    result += (i > 0 ? " " : "") + F->get_function(n)->xname;
            else if (word == "functions_full")
                info_functions(F, result);
            else if (word == "dataset_count")
                result += S(F->get_dm_count());
            else if (word == "datasets") {
                for (int i = 0; i < F->get_dm_count(); ++i)
                    result += (i > 0 ? "\n@" : "@") + S(i) + ": "
                            + F->get_data(i)->get_title();
            }
            else if (word == "view")
                result += F->view.str();
            else if (word == "set")
                result += F->get_settings()->print_usage();
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

            // optional range
            else if (word == "history") {
                info_history(F, args[n+1], args[n+2], result);
                n += 2;
            }
            else if (word == "guess") {
                RealRange range;
                args2range(args[n+1], args[n+2], range);
                Guess(F, F->get_dm(ds)).get_guess_info(range, result);
                n += 2;
            }

            // optionally takes datasets as args
            else if (word == "fit" || word == "errors" || word == "cov") {
                vector<DataAndModel*> v;
                ++n;
                while (args[n].type != kTokenNop) {
                    int k = args[n].value.i;
                    DataAndModel* dm = const_cast<DataAndModel*>(F->get_dm(k));
                    v.push_back(dm);
                    ++n;
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
                ++n;
                string name = Lexer::get_string(args[n]);
                vector<string> refs = F->get_variable_references(name);
                result += join_vector(refs, ", ");
            }

            // one arg: %func
            else if (word == "par") {
                ++n;
                string name = Lexer::get_string(args[n]);
                result += F->find_function(name)->get_par_info(F);
            }
        }

        // no keyword arg
        else if (args[n].type == kTokenCname)
            info_func_type(args[n].as_string(), result);

        // %func
        else if (args[n].type == kTokenFuncname) {
            const Function *f = F->find_function(Lexer::get_string(args[n]));
            result += f->get_basic_assignment();
        }
        // $var
        else if (args[n].type == kTokenVarname)
            result += F->get_variable_info(Lexer::get_string(args[n]));

        // handle [@n.]F/Z['['expr']']
        else if ((args[n].type == kTokenUletter &&
                                 (*args[n].str == 'F' || *args[n].str == 'Z'))
                 || args[n].type == kTokenDataset) {
            int k = ds;
            if (args[n].type == kTokenDataset) {
                k = args[n].value.i;
                ++n;
            }
            const Model* model = F->get_model(k);
            Model::FuncSet fz = Model::parse_funcset(*args[n].str);
            if (is_index(n+1, args) && args[n+1].type == kTokenExpr) {
                ++n;
                int idx = iround(args[n].value.d);
                const string& name = model->get_func_name(fz, idx);
                const Function *f = F->find_function(name);
                result += f->get_basic_assignment();
            }
            else {
                result += join_vector(model->get_names(fz), ", ");
            }
        }
        ++n;
    }
    return n;
}

string get_info_string(Ftk const* F, string const& args)
{
    Lexer lex(args.c_str());
    Parser cp(const_cast<Ftk*>(F));
    vector<Token> tt;
    cp.parse_info_args(lex, tt);
    if (lex.peek_token().type != kTokenNop)
        lex.throw_syntax_error("unexpected token");
    string result;
    eval_info_args(F, -1, tt, result);
    return result;
}

void do_command_info(const Ftk* F, int ds, const std::vector<Token>& args)
{
    string info;
    int n = eval_info_args(F, ds, args, info);
    if (n == (int) args.size()) { // no redirection to file
        int max_screen_info_length = 2048;
        int more = (int) info.length() - max_screen_info_length;
        if (more > 0) {
            info.resize(max_screen_info_length);
            info += "\n[... " + S(more) + " characters more...]";
        }
        F->rmsg(info);
    }
    else {
        assert(n == (int) args.size() - 2);
        assert(args[n].type == kTokenGT || args[n].type == kTokenAppend);
        assert(args[n+1].type == kTokenFilename);
        ios::openmode mode = (args[n].type == kTokenGT ? ios::trunc : ios::app);
        string filename = args[n+1].as_string();
        ofstream os(filename.c_str(), ios::out | mode);
        if (!os)
            throw ExecuteError("Can't open file: " + filename);
        os << info << endl;
    }
}

void do_command_debug(const Ftk* F, const string& args)
{
    string r;
    Lexer lex(args.c_str());
    Token token = lex.get_token();
    string word = token.as_string();

    if (word == "parse") {
        Parser parser(const_cast<Ftk*>(F));
        try {
            while (parser.parse_statement(lex))
                r += parser.get_statements_repr();
        }
        catch (fityk::SyntaxError& e) {
            r += string("ERR: ") + e.what();
        }
    }

    else if (word == "lex") {
        for (Token t = lex.get_token(); t.type != kTokenNop; t =lex.get_token())
            r += token2str(t) + "\n";
    }

    else if (word == "expr") {
        try {
            ExpressionParser parser(F);
            parser.parse2vm(lex, -1);
            r += parser.list_ops();
        }
        catch (fityk::SyntaxError& e) {
            r += "ERROR at " + S(lex.scanned_chars()) + ": " + e.what();
        }
        if (lex.peek_token().type != kTokenNop)
            r += "\nnext token: " + token2str(lex.peek_token());
    }

    else if (word == "der") {
        get_derivatives_str(lex.pchar(), r);
    }

    // show values of derivatives for all variables
    else if (word == "rd") {
        for (int i = 0; i < size(F->get_variables()); ++i) {
            Variable const* var = F->get_variable(i);
            r += var->xname + ": ";
            vector<Variable::ParMult> const& rd
                                        = var->get_recursive_derivatives();
            for (vector<Variable::ParMult>::const_iterator i = rd.begin();
                                                           i != rd.end(); ++i)
                r += "p" + S(i->p) + "="
                    + F->find_variable_handling_param(i->p)->xname
                    + " *" + S(i->mult) + "    ";
            r += "\n";
        }
    }

    // show varnames and var_idx from VariableUser
    else if (word == "idx") {
        for (size_t i = 0; i != F->get_functions().size(); ++i)
            r += S(i) + ": " + F->get_function(i)->get_debug_idx_info() + "\n";
        for (size_t i = 0; i != F->get_variables().size(); ++i)
            r += S(i) + ": " + F->get_variable(i)->get_debug_idx_info() + "\n";
    }

    // compares numeric and symbolic derivatives
    else if (word == "dF") {
        //TODO ds
        int ds = 0;
        ExpressionParser ep(F);
        ep.parse2vm(lex, ds);
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
    else if (token.type == kTokenFuncname) {
        Function const* f = F->find_function(Lexer::get_string(token));
        r += f->get_bytecode();
    }

    // show derivatives of $variable
    else if (token.type == kTokenVarname) {
        Variable const* v = F->find_variable(Lexer::get_string(token));
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

    else
        r += "unexpected arg: " + word;
    F->rmsg(r);
}


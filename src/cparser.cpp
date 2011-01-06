// This file is part of fityk program. Copyright 2009 Marcin Wojdyr.
// Licence: GNU General Public License ver. 2+

/// This parser is not used yet.
/// Command parser.

#include "cparser.h"

#include <string.h>
#include <stdlib.h>

#include "lexer.h"
#include "eparser.h"
#include "settings.h"
#include "logic.h"
#include "data.h"
#include "guess.h" // peak_traits, linear_traits
#include "ast.h" // prepare_ast_with_der()
#include "tplate.h"


using namespace std;
using fityk::SyntaxError;

const char *command_list[] = {
    "debug", "define", "delete", "exec", "fit", "guess", "info", "plot",
    "quit", "reset", "set", "sleep", "title", "undefine", "use"
};

const char* info_args[] = {
    "version", "compiler", "variables", "types", "functions",
    "dataset_count", "datasets", "view", "fit_history",
    "filename", "title", "data",
    "formula", "gnuplot_formula",
    "simplified_formula", "simplified_gnuplot_formula",
    "state", "history_summary", "peaks", "peaks_err",
    "set",
    "history", "guess",
    "fit", "errors", "cov",
    "refs", "prop",
    NULL
};

const char* debug_args[] = {
    // "%" and "$" mean that any %functions and $variables are accepted as args
    "parse", "lex", "expr", "der", "rd", "idx", "df", "%", "$", NULL
};


const char* commandtype2str(CommandType c)
{
    switch (c) {
        case kCmdDebug:   return "Debug";
        case kCmdDefine:  return "Define";
        case kCmdDelete:  return "Delete";
        case kCmdDeleteP: return "Delete";
        case kCmdExec:    return "Exec";
        case kCmdFit:     return "Fit";
        case kCmdGuess:   return "Guess";
        case kCmdInfo:    return "Info";
        case kCmdPlot:    return "Plot";
        case kCmdPrint:   return "Print";
        case kCmdQuit:    return "Quit";
        case kCmdReset:   return "Reset";
        case kCmdSet:     return "Set";
        case kCmdSleep:   return "Sleep";
        case kCmdTitle:   return "Title";
        case kCmdUndef:   return "Undef";
        case kCmdUse:     return "Use";
        case kCmdShell:   return "Shell";
        case kCmdLoad:    return "Load";
        case kCmdDatasetTr: return "DatasetTr";
        case kCmdNameFunc: return "NameFunc";
        case kCmdAssignParam: return "AssignParam";
        case kCmdAssignAll: return "AssignAll";
        case kCmdNameVar: return "NameVar";
        case kCmdChangeModel: return "ChangeModel";
        case kCmdPointTr: return "PointTr";
        case kCmdAllPointsTr: return "AllPointsTr";
        case kCmdResizeP: return "ResizeP";
        case kCmdNull:    return "Null";
    }
    return NULL; // avoid warning
}

Token nop()
{
    Token t;
    t.type = kTokenNop;
    return t;
}

bool is_command(const Token& token, const char* cmd_base,
                                    const char* cmd_suffix)
{
    assert(token.type == kTokenLname);
    int base_len = strlen(cmd_base);
    if (strncmp(token.str, cmd_base, base_len) != 0)
        return false;
    int left_chars = token.length - base_len;
    return left_chars == 0 ||
             (left_chars <= (int) strlen(cmd_suffix) &&
              strncmp(token.str + base_len, cmd_suffix, left_chars) == 0);
}


Parser::Parser(const Ftk* F)
    : F_(F), ep_(F)
{
}

Parser::~Parser()
{
}

Token Parser::read_expr(Lexer& lex)
{
    Token t;
    t.type = kTokenExpr;
    t.str = lex.pchar();
    ep_.clear_vm();
    assert(!st_.datasets.empty());
    int ds = st_.datasets[0];
    ep_.parse_expr(lex, ds);
    t.length = lex.pchar() - t.str;
    t.value.d = 0.;
    return t;
}

Token Parser::read_and_calc_expr(Lexer& lex)
{
    Token t = read_expr(lex);
    int ds = st_.datasets[0];
    const vector<Point>& points = F_->get_data(ds)->points();
    t.value.d = ep_.calculate(0, points);
    return t;
}


Token Parser::read_var(Lexer& lex)
{
    Token t;
    t.type = kTokenEVar;
    t.str = lex.pchar();
    int ds = st_.datasets[0];
    ep_.clear_vm();
    ep_.parse_expr(lex, ds, NULL, NULL, true);
    t.value.i = st_.vdlist.size();
    st_.vdlist.push_back(ep_.vm());
    t.length = lex.pchar() - t.str;
    return t;
}

// see the command above
Token Parser::read_define_arg(Lexer& lex, const vector<string>& allowed_names,
                              vector<string> *new_names)
{
    Token t;
    t.type = kTokenExpr;
    t.str = lex.pchar();
    ep_.clear_vm();
    ep_.parse_expr(lex, -1, &allowed_names, new_names);
    t.length = lex.pchar() - t.str;
    t.value.d = 0.;
    return t;
}

// '.' | ( '[' [Number] ':' [Number] ']' )
// appends two tokens (kTokenExpr/kTokenNop) to args
void Parser::parse_real_range(Lexer& lex, vector<Token>& args)
{
    if (lex.peek_token().type == kTokenLSquare) {
        lex.get_token(); // discard '['
        const Token& t = lex.peek_token();
        if (t.type == kTokenColon) {
            args.push_back(nop());
            lex.get_token(); // discard ':'
        }
        else if (t.type == kTokenRSquare) {
            // omitted ':', never mind
            args.push_back(nop());
        }
        else {
            args.push_back(read_and_calc_expr(lex));
            lex.get_expected_token(kTokenColon); // discard ':'
        }

        const Token& r = lex.peek_token();
        if (r.type == kTokenRSquare) {
            lex.get_token(); // discard ']'
            args.push_back(nop());
        }
        else {
            args.push_back(read_and_calc_expr(lex));
            lex.get_expected_token(kTokenRSquare); // discard ']'
        }
    }
    else {
        args.push_back(nop());
        args.push_back(nop()); // we always append two tokens in this function
    }
}

// %funcname | [@n.]('F'|'Z') '[' Number ']'
void Parser::parse_func_id(Lexer& lex, vector<Token>& args, bool accept_fz)
{
    Token t = lex.get_token();
    if (t.type == kTokenFuncname) {
        args.push_back(t);
        return;
    }
    if (t.type == kTokenDataset) {
        args.push_back(t);
        lex.get_expected_token(kTokenDot); // discard '.'
        t = lex.get_token();
    }
    else
        args.push_back(nop());
    if (t.as_string() != "F" && t.as_string() != "Z")
        lex.throw_syntax_error("expected %function ID");
    args.push_back(t);
    if (accept_fz && lex.peek_token().type != kTokenLSquare) {
        args.push_back(nop());
        return;
    }
    lex.get_expected_token(kTokenLSquare); // discard '['
    args.push_back(read_and_calc_expr(lex));
    lex.get_expected_token(kTokenRSquare); // discard ']'
}

void Parser::parse_set_args(Lexer& lex, vector<Token>& args)
{
    do {
        Token key = lex.get_expected_token(kTokenLname);
        lex.get_expected_token(kTokenAssign); // discard '='
        SettingsMgr::ValueType t =
            F_->settings_mgr()->get_value_type(key.as_string());
        if (t == SettingsMgr::kNotFound)
            lex.throw_syntax_error("no such option: " + key.as_string());
        Token value;
        if (t == SettingsMgr::kString)
            value = lex.get_expected_token(kTokenString);
        else if (t == SettingsMgr::kEnum)
            value = lex.get_expected_token(kTokenLname);
        else
            value = read_and_calc_expr(lex);
        args.push_back(key);
        args.push_back(value);
    } while (lex.discard_token_if(kTokenComma));
}


class ArgReader
{
public:
    ArgReader(Lexer& lex) : lex_(lex)
    {
        lex_.get_expected_token(kTokenOpen);
        flag_ = (lex.get_token_if(kTokenClose).type == kTokenClose ? 0 : 1);
    }

    bool next_arg()
    {
        if (flag_ == 0) // no args
            return false;
        else if (flag_ == 1) { // at the beginning
            flag_ = 2;
            return true;
        }
        else // now either ',' or ')'
            return (lex_.get_expected_token(kTokenComma, kTokenClose).type
                    == kTokenComma);
    }

private:
    Lexer& lex_;
    int flag_;
};

// FuncType "(" [ArgExpr % ","] ")"
void Parser::parse_component(Lexer& lex, const vector<string>& lhs_vars,
                             Tplate::Component* c)
{
    Token name = lex.get_expected_token(kTokenCname);
    c->p = F_->get_tpm()->get_shared_tp(name.as_string());
    c->cargs.clear();
    ArgReader ar(lex);
    while (ar.next_arg()) {
        read_define_arg(lex, lhs_vars, NULL);
        c->cargs.push_back(ep_.vm());
    }
}

void Parser::parse_define_rhs(Lexer& lex, Tplate *tp)
{
    Token t = lex.get_token();
    // CompoundFunction, RHS: Component % "+"
    if (t.type == kTokenCname) {
        lex.go_back(t);
        do {
            Tplate::Component c;
            parse_component(lex, tp->fargs, &c);
            tp->components.push_back(c);
        } while (lex.discard_token_if(kTokenPlus));
        tp->create = &create_CompoundFunction;
    }

    // SplitFunction, RHS: "x" "<" Arg ? Component : Component 
    else if (t.as_string() == "x" && lex.discard_token_if(kTokenLT)) {
        tp->components.resize(3);
        read_define_arg(lex, tp->fargs, NULL);
        tp->components[0].cargs.push_back(ep_.vm());
        lex.get_expected_token(kTokenQMark); // discard '?'
        parse_component(lex, tp->fargs, &tp->components[1]);
        lex.get_expected_token(kTokenColon); // discard ':'
        parse_component(lex, tp->fargs, &tp->components[2]);
        tp->create = &create_SplitFunction;
    }

    // CustomFunction
    else {
        lex.go_back(t);
        vector<string> extra_names;
        string rhs = read_define_arg(lex, tp->fargs, &extra_names).as_string();
        if (lex.peek_token().as_string() == "where") {
            lex.get_token(); // discard "where"
            do {
                string name = lex.get_expected_token(kTokenLname).as_string();
                lex.get_expected_token(kTokenAssign); // discard '='
                int idx = index_of_element(extra_names, name);
                if (idx == -1)
                    lex.throw_syntax_error("unused substitution: " + name);
                extra_names.erase(extra_names.begin() + idx);
                Token s = read_define_arg(lex, tp->fargs, &extra_names);
                replace_words(rhs, name, "("+s.as_string()+")");
            } while (lex.discard_token_if(kTokenComma));
        }
        v_foreach (string, i, extra_names) {
            if (*i != "x")
                lex.throw_syntax_error("unknown argument: " + *i);
        }

        Lexer lex2(rhs.c_str());
        ExpressionParser ep(NULL);
        ep.parse_expr(lex2, -1, &tp->fargs, NULL, true);
        tp->op_trees = prepare_ast_with_der(ep.vm(), tp->fargs.size() + 1);

        tp->create = &create_CustomFunction;
    }
}

// Tplate
Tplate::Ptr Parser::parse_define_args(Lexer& lex)
{
    boost::shared_ptr<Tplate> tp(new Tplate);
    // FuncType
    tp->name = lex.get_expected_token(kTokenCname).as_string();

    // '(' [(arg_name ['=' default_value]) % ','] ')'
    ArgReader arg_reader(lex);
    const vector<string> empty;
    vector<string> new_vars;
    while (arg_reader.next_arg()) {
        string name = lex.get_expected_token(kTokenLname).as_string();
        if (name == "x") {
            if (lex.peek_token().type == kTokenAssign)
                lex.throw_syntax_error("do not use x at left-hand side.");
            continue; // ignore this "x"
        }
        tp->fargs.push_back(name);
        string default_value;
        if (lex.discard_token_if(kTokenAssign))
            default_value = read_define_arg(lex, empty, &new_vars).as_string();
        else
            new_vars.push_back(name);
        tp->defvals.push_back(default_value);
    }

    tp->linear_d = false;
    v_foreach (string, i, new_vars)
        if (contains_element(Guess::linear_traits, *i)) {
            tp->linear_d = true;
            break;
        }
    tp->peak_d = false;
    v_foreach (string, i, new_vars)
        if (contains_element(Guess::peak_traits, *i)) {
            tp->peak_d = true;
            break;
        }

    // '='
    lex.get_expected_token(kTokenAssign); // discard '='

    const char* start_rhs = lex.pchar();
    while (isspace(*start_rhs))
        ++start_rhs;
    parse_define_rhs(lex, tp.get());
    tp->rhs = string(start_rhs, lex.pchar());
    return tp;
}

void parse_undefine_args(Lexer& lex, vector<Token>& args)
{
    do {
        args.push_back(lex.get_expected_token(kTokenCname));
    } while (lex.discard_token_if(kTokenComma));
}

void parse_delete_args(Lexer& lex, vector<Token>& args)
{
    do {
        // allow "delete %pd*" or %* or $foo* or $*.
        Token t = lex.get_glob_token();
        if (t.type == kTokenDataset ||      // @n
                t.type == kTokenFuncname || // %func
                t.type == kTokenVarname)    // $var
            args.push_back(t);
        else if (t.type == kTokenLname && t.as_string() == "file")
            args.push_back(lex.get_filename_token());
        else
            lex.throw_syntax_error("unexpected arg after `delete'");
    } while (lex.discard_token_if(kTokenComma));
}

void parse_exec_args(Lexer& lex, vector<Token>& args)
{
    if (lex.discard_token_if(kTokenBang))
        args.push_back(lex.get_rest_of_line());
    else
        args.push_back(lex.get_filename_token());
}

void parse_fit_args(Lexer& lex, vector<Token>& args)
{
    Token t = lex.get_token();
    if (t.type == kTokenLname) {
        string name = t.as_string();
        if (name == "undo" || name == "redo" || name == "clear_history") {
            args.push_back(t);
        }
        else if (name == "history") {
            args.push_back(t);
            args.push_back(t);
        }
        else
            lex.throw_syntax_error("unexpected name after `fit'");
    }
    else if (t.type == kTokenPlus) {
        args.push_back(t);
        args.push_back(lex.get_expected_token(kTokenNumber));
        if (lex.peek_token().type == kTokenDataset)
            throw ExecuteError("No need to specify datasets to continue fit.");
    }
    // [n_iter] @n*
    else if (t.type == kTokenNumber || kTokenDataset) {
        args.push_back(t);
        while (lex.peek_token().type == kTokenDataset)
            args.push_back(lex.get_token());
    }
    else // no args
        lex.go_back(t);
}

// [Funcname '='] Uname ['(' kwarg % ',' ')'] [range]
void Parser::parse_guess_args(Lexer& lex, vector<Token>& args)
{
    Token t = lex.get_expected_token(kTokenCname, kTokenFuncname);
    if (t.type == kTokenFuncname) {
        args.push_back(t);
        lex.get_expected_token(kTokenAssign); // discard '='
        t = lex.get_expected_token(kTokenCname);
    }
    else
        args.push_back(nop());
    args.push_back(t);
    if (lex.peek_token().type == kTokenOpen) {
        lex.get_expected_token(kTokenOpen);
        Token t = lex.get_token_if(kTokenClose);
        while (t.type != kTokenClose) {
            args.push_back(lex.get_expected_token(kTokenLname));
            lex.get_expected_token(kTokenAssign); // discard '='
            args.push_back(read_var(lex));
            t = lex.get_expected_token(kTokenComma, kTokenClose);
        }
    }
    parse_real_range(lex, args);
}

static
void parse_redir(Lexer& lex, vector<Token>& args)
{
    if (/*lex.peek_token().type == kTokenGT ||*/
            lex.peek_token().type == kTokenAppend) {
        args.push_back(lex.get_token());
        Token f = lex.get_filename_token();
        if (f.type == kTokenNop)
            lex.throw_syntax_error("expected filename");
        args.push_back(f);
    }
}

void Parser::parse_info_args(Lexer& lex, vector<Token>& args)
{
    if (lex.peek_token().type == kTokenNop) // no args
        return;
    parse_one_info_arg(lex, args);
    while (lex.discard_token_if(kTokenComma))
        parse_one_info_arg(lex, args);
    parse_redir(lex, args);
}

void Parser::parse_one_info_arg(Lexer& lex, vector<Token>& args)
{
    Token token = lex.get_glob_token();
    if (token.type == kTokenLname) {
        string word = token.as_string();
        const char** pos = info_args;
        while (*pos != NULL && *pos != word)
            ++pos;
        if (*pos == NULL)
            lex.throw_syntax_error("Unknown info argument: " + word);
        args.push_back(token);
        if (word == "set") {
            if (lex.peek_token().type == kTokenLname)
                args.push_back(lex.get_token());
            else
                args.push_back(nop());
        }
        else if (word == "history" || word == "guess") {
            parse_real_range(lex, args);
        }
        else if (word == "fit" || word == "errors" || word == "cov") {
            while (lex.peek_token().type == kTokenDataset)
                args.push_back(lex.get_token());
            args.push_back(nop()); // separator
        }
        else if (word == "refs") {
            args.push_back(lex.get_expected_token(kTokenVarname));
        }
        else if (word == "prop") {
            args.push_back(lex.get_expected_token(kTokenFuncname));
        }
    }
    else if (token.type == kTokenCname || token.type == kTokenFuncname ||
             token.type == kTokenVarname) {
        args.push_back(token);
    }
    // handle [@n.]F/Z['['expr']']
    else if ((token.type == kTokenUletter &&
                                (*token.str == 'F' || *token.str == 'Z'))
             || token.type == kTokenDataset) {
        args.push_back(token);
        if (token.type == kTokenDataset) {
            lex.get_expected_token(kTokenDot); // discard '.'
            args.push_back(lex.get_expected_token("F", "Z"));
        }
        if (lex.peek_token().type == kTokenLSquare) {
            lex.get_token(); // discard '['
            args.push_back(read_and_calc_expr(lex));
            lex.get_expected_token(kTokenRSquare); // discard ']'
        }
    }
    else
        lex.throw_syntax_error("Unknown info argument: " + token.as_string());
}

void Parser::parse_print_args(Lexer& lex, vector<Token>& args)
{
    // the first arg means:
    //  ':'   -> "all ":"
    //  expr  -> "if" expr ":"
    //  nop   -> execute once
    bool once = true;
    if (lex.peek_token().as_string() == "all") {
        lex.get_token(); // discard "all"
        // mark "all"
        args.push_back(lex.get_expected_token(kTokenColon));
        once = false;
    }
    else if (lex.peek_token().as_string() == "if") {
        lex.get_token(); // discard "if"
        args.push_back(read_expr(lex));
        lex.get_expected_token(kTokenColon); // discard ':'
        once = false;
    }
    else
        args.push_back(nop());
    do {
        if (lex.peek_token().type == kTokenString)
            args.push_back(lex.get_token());
        else if (lex.peek_token().as_string() == "filename" ||
                 lex.peek_token().as_string() == "title")
            args.push_back(lex.get_token());
        else {
            Token t = once ? read_and_calc_expr(lex) : read_expr(lex);
            args.push_back(t);
        }

    } while (lex.discard_token_if(kTokenComma));
    parse_redir(lex, args);
}

CommandType Parser::parse_xysa_args(Lexer& lex, vector<Token>& args)
{
    Token t = lex.get_expected_token(kTokenAssign, kTokenLSquare);
    // X =
    if (t.type == kTokenAssign) {
        for (;;) {
            args.push_back(read_expr(lex));
            if (!lex.discard_token_if(kTokenComma))
                break;
            Token a = lex.get_expected_token(kTokenUletter);
            char d = *a.str;
            if (d != 'X' && d != 'Y' && d != 'S' && d != 'A')
                lex.throw_syntax_error("unexpected letter");
            args.push_back(a);
            lex.get_expected_token(kTokenAssign); // discard '='
        }
        return kCmdAllPointsTr;
    }
    // X [expr] =
    else {
        for (;;) {
            args.push_back(read_and_calc_expr(lex));
            lex.get_expected_token(kTokenRSquare); // discard ']'
            lex.get_expected_token(kTokenAssign); // discard '='
            args.push_back(read_and_calc_expr(lex));
            if (!lex.discard_token_if(kTokenComma))
                break;
            Token a = lex.get_expected_token(kTokenUletter);
            char d = *a.str;
            if (d != 'X' && d != 'Y' && d != 'S' && d != 'A')
                lex.throw_syntax_error("unexpected letter");
            args.push_back(a);
            lex.get_expected_token(kTokenLSquare); // discard '['
        }
        return kCmdPointTr;
    }
}

// [Key] (Dataset | 0) % '+'
void parse_dataset_tr_args(Lexer& lex, vector<Token>& args)
{
    args.push_back(lex.get_token_if(kTokenLname));
    do {
        Token t = lex.get_expected_token(kTokenDataset, "0");
        if (t.type == kTokenDataset && (t.value.i == Lexer::kAll ||
                                        t.value.i == Lexer::kNew))
            lex.throw_syntax_error("expected @number");
        args.push_back(t);
    } while (lex.discard_token_if(kTokenPlus));
}

void Parser::parse_assign_func(Lexer& lex, vector<Token>& args)
{
    Token f = lex.get_expected_token(kTokenCname, "copy");
    if (f.type == kTokenCname) {
        // Uname '(' ([Lname '='] v_expr) % ',' ')'
        args.push_back(f);
        bool has_kwarg = false;
        ArgReader arg_reader(lex);
        while (arg_reader.next_arg()) {
            Token t = lex.get_token();
            if (lex.discard_token_if(kTokenAssign)) {
                if (t.type != kTokenLname)
                    lex.throw_syntax_error("wrong token before '='");
                args.push_back(t);
                has_kwarg = true;
            }
            else {
                if (has_kwarg)
                    lex.throw_syntax_error("non-keyword arg after keyword arg");
                args.push_back(nop());
                lex.go_back(t);
            }
            args.push_back(read_var(lex));
        }
    }
    else {
        // "copy" '(' func_id ')'
        args.push_back(f); // "copy"
        lex.get_expected_token(kTokenOpen); // discard '('
        parse_func_id(lex, args, false);
        lex.get_expected_token(kTokenClose); // discard ')'
    }
}

void Parser::parse_fz(Lexer& lex, Command &cmd)
{
    Token t = lex.get_token();
    // F=..., F+=...
    // ('='|'+=') (0 | %f | Type(...) | copy(%f) | F | copy(F)) % '+'
    if (t.type == kTokenAssign || t.type == kTokenAddAssign) {
        cmd.type = kCmdChangeModel;
        cmd.args.push_back(t);
        for (;;) {
            const Token& p = lex.peek_token();
            if (p.type == kTokenCname) {     // Type(...)
                parse_assign_func(lex, cmd.args);
            }
            else if (p.as_string() == "0") { // 0
                cmd.args.push_back(lex.get_token());
            }
            else if (p.as_string() == "copy") {
                cmd.args.push_back(lex.get_token()); // "copy"
                lex.get_expected_token(kTokenOpen); // discard '('
                parse_func_id(lex, cmd.args, true);
                lex.get_expected_token(kTokenClose); // discard ')'
            }
            else
                parse_func_id(lex, cmd.args, true);

            if (lex.peek_token().type == kTokenPlus)
                cmd.args.push_back(lex.get_token()); // '+'
            else
                break;
        }
    }
    // F.param= ...
    else if (t.type == kTokenDot) {
        cmd.type = kCmdAssignAll;
        cmd.args.push_back(lex.get_expected_token(kTokenLname));
        lex.get_expected_token(kTokenAssign); // discard '='
        cmd.args.push_back(read_var(lex));
    }
    // F[Number]...
    else if (t.type == kTokenRSquare) {
        cmd.args.push_back(read_and_calc_expr(lex));
        lex.get_expected_token(kTokenRSquare); // discard ']'
        Token k = lex.get_expected_token(kTokenAssign, kTokenDot);
        if (k.type == kTokenAssign) { // F[Number]=...
            cmd.type = kCmdChangeModel;
            if (lex.peek_token().type == kTokenFuncname) // ...=%func
                cmd.args.push_back(lex.get_token());
            else                                         // ...=Func(...)
                parse_assign_func(lex, cmd.args);
        }
        else { // F[Number].param=...
            cmd.type = kCmdAssignParam;
            cmd.args.push_back(lex.get_expected_token(kTokenLname));
            lex.get_expected_token(kTokenAssign); // discard '='
            cmd.args.push_back(read_var(lex));
        }
    }
    else
        lex.throw_syntax_error("unexpected token after F/Z");
}

void add_to_datasets(const Ftk* F_, vector<int>& datasets, int n)
{
    if (n == Lexer::kAll)
        for (int j = 0; j != F_->get_dm_count(); ++j)
            datasets.push_back(j);
    else
        datasets.push_back(n);
}

bool Parser::parse_statement(Lexer& lex)
{
    st_.datasets.clear();
    st_.with_args.clear();
    st_.vdlist.clear();
    st_.commands.resize(1);
    st_.commands[0].args.clear();

    Token first = lex.peek_token();

    if (first.type == kTokenNop)
        return false;

    if (first.type == kTokenDataset) {
        lex.get_token();
        Token t = lex.get_token();
        if (t.type == kTokenDataset || t.type == kTokenColon) {
            add_to_datasets(F_, st_.datasets, first.value.i);
            while (t.type == kTokenDataset) {
                add_to_datasets(F_, st_.datasets, t.value.i);
                t = lex.get_expected_token(kTokenDataset, kTokenColon);
            }
        }
        else {
            lex.go_back(first);
        }
    }
    if (st_.datasets.empty())
        st_.datasets.push_back(F_->default_dm());

    if (first.type == kTokenLname && is_command(first, "w","ith")) {
        lex.get_token(); // discard "with"
        parse_set_args(lex, st_.with_args);
    }

    parse_command(lex, st_.commands[0]);
    // the check for Nop below is to allows ';' at the end of line
    while (lex.discard_token_if(kTokenSemicolon) &&
           lex.peek_token().type != kTokenNop) {
        st_.commands.resize(st_.commands.size() + 1);
        parse_command(lex, st_.commands.back());
    }

    if (lex.peek_token().type != kTokenNop)
        lex.throw_syntax_error(S("unexpected token: `")
                               + tokentype2str(lex.peek_token().type) + "'");

    return true;
}


void Parser::parse_command(Lexer& lex, Command& cmd)
{
    cmd.type = kCmdNull; // initial value, will be changed
    const Token token = lex.get_token();
    if (token.type == kTokenLname) {
        if (is_command(token, "deb","ug")) {
            cmd.type = kCmdDebug;
            cmd.args.push_back(lex.get_token());
            cmd.args.push_back(lex.get_rest_of_line());
        }
        else if (is_command(token, "def","ine")) {
            cmd.type = kCmdDefine;
            cmd.defined_tp = parse_define_args(lex);
        }
        else if (is_command(token, "del","ete")) {
            if (lex.peek_token().type == kTokenOpen) {
                cmd.type = kCmdDeleteP;
                lex.get_expected_token(kTokenOpen); // discard '('
                cmd.args.push_back(read_expr(lex));
                lex.get_expected_token(kTokenClose); // discard ')'
            }
            else {
                cmd.type = kCmdDelete;
                parse_delete_args(lex, cmd.args);
            }
        }
        else if (is_command(token, "e","xecute")) {
            cmd.type = kCmdExec;
            parse_exec_args(lex, cmd.args);
        }
        else if (is_command(token, "f","it")) {
            cmd.type = kCmdFit;
            parse_fit_args(lex, cmd.args);
        }
        else if (is_command(token, "g","uess")) {
            cmd.type = kCmdGuess;
            parse_guess_args(lex, cmd.args);
        }
        else if (is_command(token, "i","nfo")) {
            cmd.type = kCmdInfo;
            parse_info_args(lex, cmd.args);
        }
        else if (is_command(token, "pl","ot")) {
            cmd.type = kCmdPlot;
            parse_real_range(lex, cmd.args);
            parse_real_range(lex, cmd.args);
            while (lex.peek_token().type == kTokenDataset)
                cmd.args.push_back(lex.get_token());
        }
        else if (is_command(token, "p","rint")) {
            cmd.type = kCmdPrint;
            parse_print_args(lex, cmd.args);
        }
        else if (is_command(token, "quit","")) {
            cmd.type = kCmdQuit;
            // no args
        }
        else if (is_command(token, "s","et")) {
            cmd.type = kCmdSet;
            parse_set_args(lex, cmd.args);
        }
        else if (is_command(token, "undef","ine")) {
            cmd.type = kCmdUndef;
            parse_undefine_args(lex, cmd.args);
        }
        else if (is_command(token, "reset","")) {
            cmd.type = kCmdReset;
            // no args
        }
        else if (is_command(token, "sleep","")) {
            cmd.type = kCmdSleep;
            Token value = read_and_calc_expr(lex);
            cmd.args.push_back(value);
        }
        else if (is_command(token, "title","")) {
            // [@n:] "title" '='
            cmd.type = kCmdTitle;
            lex.get_expected_token(kTokenAssign); // discard '='
            cmd.args.push_back(lex.get_filename_token());
        }
        else if (is_command(token, "use","")) {
            cmd.type = kCmdUse;
            cmd.args.push_back(lex.get_expected_token(kTokenDataset));
        }
        else if (token.length == 1 && (*token.str == 'x' || *token.str == 'y' ||
                                       *token.str == 's' || *token.str == 'a')){
            cmd.args.push_back(token);
            cmd.type = parse_xysa_args(lex, cmd.args);
        }
    }
    else if (token.type == kTokenUletter) {
        const char c = *token.str;
        if (c == 'F' || c == 'Z') {
            cmd.args.push_back(nop()); // dataset
            cmd.args.push_back(token); // F/Z
            parse_fz(lex, cmd);
        }
        else if (c == 'M') {
            cmd.type = kCmdResizeP;
            lex.get_expected_token(kTokenAssign);
            cmd.args.push_back(read_and_calc_expr(lex));
        }
        else if (c == 'X' || c == 'Y' || c == 'S' || c == 'A') {
            cmd.args.push_back(token);
            cmd.type = parse_xysa_args(lex, cmd.args);
        }
        else
            lex.throw_syntax_error("unknown name: " + token.as_string());
    }
    else if (token.type == kTokenBang) {
        cmd.type = kCmdShell;
        cmd.args.push_back(lex.get_rest_of_line());
    }
    // $var=...
    else if (token.type == kTokenVarname &&
             lex.peek_token().type == kTokenAssign) {
        cmd.type = kCmdNameVar;
        cmd.args.push_back(token);
        lex.get_token(); // discard '='
        cmd.args.push_back(read_var(lex));
        parse_real_range(lex, cmd.args);
    }
    // %func=...
    else if (token.type == kTokenFuncname &&
             lex.peek_token().type == kTokenAssign) {
        cmd.type = kCmdNameFunc;
        cmd.args.push_back(token);
        lex.get_token(); // discard '='
        parse_assign_func(lex, cmd.args);
    }
    // %func.param=...
    else if (token.type == kTokenFuncname &&
             lex.peek_token().type == kTokenDot) {
        cmd.type = kCmdAssignParam;
        cmd.args.push_back(token);
        lex.get_token(); // discard '.'
        cmd.args.push_back(lex.get_expected_token(kTokenLname));
        lex.get_expected_token(kTokenAssign); // discard '='
        cmd.args.push_back(read_var(lex));
    }
    else if (token.type == kTokenDataset &&
             lex.peek_token().type == kTokenDot) {
        cmd.args.push_back(token); // dataset
        lex.get_token(); // discard '.'
        string arg = lex.peek_token().as_string();
        if (arg == "F" || arg == "Z") {
            cmd.args.push_back(lex.get_token()); // F/Z
            parse_fz(lex, cmd);
        }
        else
            lex.throw_syntax_error("@n. must be followed by F or Z");
    }
    else if (token.type == kTokenDataset &&
             lex.peek_token().type == kTokenLT) {
        cmd.type = kCmdLoad;
        cmd.args.push_back(token);
        lex.get_token(); // discard '<'
        cmd.args.push_back(lex.get_filename_token());
    }
    else if (token.type == kTokenDataset &&
             lex.peek_token().type == kTokenAssign) {
        cmd.type = kCmdDatasetTr;
        cmd.args.push_back(token);
        lex.get_token(); // discard '='
        parse_dataset_tr_args(lex, cmd.args);
    }

    if (cmd.type == kCmdNull)
        lex.throw_syntax_error("unexpected token at command beginning");
}

bool Parser::check_syntax(const string& str)
{
    try {
        Lexer lex(str.c_str());
        parse_statement(lex);
    } catch (ExecuteError&) {
        return false;
    } catch (SyntaxError&) {
        // SyntaxError is thrown when resolving %functions and $variables
        // (in ExpressionParser) fails.
        return false;
    }
    return true;
}

string Parser::get_statements_repr() const
{
    string r = "datasets: " + join_vector(st_.datasets, " ");
    if (!st_.with_args.empty()) {
        r += "\nWith:";
        v_foreach (Token, i, st_.with_args)
            r += "\n\t" + token2str(*i);
    }
    v_foreach (Command, i, st_.commands) {
        r += S("\n") + commandtype2str(i->type);
        v_foreach (Token, j, i->args)
            r += "\n\t" + token2str(*j);
    }
    return r;
}


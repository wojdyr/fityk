// This file is part of fityk program. Copyright 2009 Marcin Wojdyr.
// Licence: GNU General Public License ver. 2+
// $Id: $

/// This parser is not used yet.
/// In the future it will replace the current parser (cmd* files)
/// Command parser.

#include "cparser.h"

#include <string.h>
#include <stdlib.h>

#include "lexer.h"
#include "eparser.h"
#include "settings.h"
#include "logic.h"

using namespace std;
using fityk::SyntaxError;

const char* info_args[] = {
    "version", "compiler", "variables", "variables_full",
    "types", "types_full", "functions", "functions_full",
    "dataset_count", "datasets", "view", "set", "fit_history",
    "filename", "title", "data", "formula", "simplified_formula",
    "history_summary", "peaks", "peaks_err",
    "history", "guess",
    "fit", "errors", "cov",
    "refs", "par",
    NULL
};

const char* debug_args[] = {
    "der", "ops", "parse", "lex", NULL
};


const char* commandtype2str(CommandType c)
{
    switch (c) {
        case kCmdDefine:  return "Define";
        case kCmdDelete:  return "Delete";
        case kCmdDeleteP: return "Delete";
        case kCmdExec:    return "Exec";
        case kCmdFit:     return "Fit";
        case kCmdGuess:   return "Guess";
        case kCmdInfo:    return "Info";
        case kCmdPlot:    return "Plot";
        case kCmdQuit:    return "Quit";
        case kCmdReset:   return "Reset";
        case kCmdSet:     return "Set";
        case kCmdSleep:   return "Sleep";
        case kCmdTitle:   return "Title";
        case kCmdUndef:   return "Undef";
        case kCmdShell:   return "Shell";
        case kCmdLoad:    return "Load";
        case kCmdDatasetTr: return "DatasetTr";
        case kCmdNameFunc: return "NameFunc";
        case kCmdAssignParam: return "AssignParam";
        case kCmdNameVar: return "NameVar";
        case kCmdChangeModel: return "ChangeModel";
        case kCmdPointTr: return "PointTr";
        case kCmdAllPointsTr: return "AllPointsTr";
        case kCmdResizeP: return "ResizeP";
        case kCmdNull:    return "Null";
    }
    return NULL; // avoid warning
}

// returns true if the command syntax ends with (optional) "in @n"
bool has_data_arg(CommandType c)
{
    switch (c) {
        case kCmdDeleteP:
        case kCmdFit:
        case kCmdGuess:
        case kCmdInfo:
        case kCmdPlot:
        case kCmdPointTr:
        case kCmdAllPointsTr:
        case kCmdResizeP:
            return true;
        default:
            return false;
    }
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

// how to evaluate expression:
// -> value
//   - [expr:expr]
//   - F[expr]
//   - set key=expr
//   - sleep expr
//   - X[expr]=...
//   - X[n]=expr
// -> ??
//   - define Func(par[=expr], ...) = ...
// -> VM code
//   - X=expr
//   - delete (expr)

Token Parser::read_expr(Lexer& lex)
{
    Token t;
    t.type = kTokenExpr;
    t.str = lex.pchar();
    ep_.clear_vm();
    int ds = st_->datasets.empty() ? -1 : st_->datasets[0];
    ep_.parse2vm(lex, ds);
    t.length = lex.pchar() - t.str;
    t.value.d = 0.;
    return t;
}

Token Parser::read_and_calc_expr(Lexer& lex)
{
    Token t = read_expr(lex);
    t.value.d = ep_.calculate();
    return t;
}


//TODO
// read variable RHS (expression -> AST)
// -> AST
//   - guess Func(par=expr) 
//   - %f = Func([par=]expr, ...)
//   - $var=expr
//   - F[Number].param=expr
//   - %func.param=expr
//   - F.param=expr
Token read_vr(Lexer& lex)
{
    Token t;
    t.type = kTokenExpr;
    t.str = lex.pchar();
    //TODO this is temporary
    ExpressionParser ep(NULL);
    ep.parse2vm(lex, -1);
    t.length = lex.pchar() - t.str;
    t.value.d = 0.;
    return t;
}

Parser::Parser(const Ftk* F)
    : F_(F), ep_(F), st_(new Statement)
{
}

Parser::~Parser()
{
    delete st_;
}


// '.' | ( '[' (Number | '.') ':' (Number | '.') ']' )
// appends two tokens (kTokenExpr/kTokenDot/kTokenNop) to args
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

/*
// parse ['in' @n[, @m...]]
void parse_in_data(Lexer& lex, vector<int>& datasets)
{
    const Token& t = lex.peek_token();
    if (t.type != kTokenLname || t.as_string() != "in")
        return;

    lex.get_token(); // discard "in"
    for (;;) {
        Token d = lex.get_expected_token(kTokenDataset);
        if (d.value.i == Lexer::kNew)
            lex.throw_syntax_error("unexpected @+ after 'in'");
        datasets.push_back(d.value.i);
        if (lex.peek_token().type == kTokenComma)
            lex.get_token(); // discard comma
        else
            break;
    }
}
*/

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

// member, because uses Settings::get_value_type()
void Parser::parse_set_args(Lexer& lex, vector<Token>& args)
{
    for (;;) {
        const Token key = lex.get_token();
        if (key.type != kTokenLname)
            lex.throw_syntax_error("expected option name");
        args.push_back(key);
        const Token eq = lex.get_token();
        if (eq.type != kTokenAssign)
            lex.throw_syntax_error("expected `='");
        const Settings *settings = F_->get_settings();
        Settings::ValueType t = settings->get_value_type(key.as_string());
        Token value;
        if (t == Settings::kNotFound) {
            lex.throw_syntax_error("no such option: " + key.as_string());
        }
        else if (t == Settings::kString) {
            value = lex.get_expected_token(kTokenString);
        }
        else if (t == Settings::kStringEnum) {
            value = lex.get_expected_token(kTokenLname);
        }
        else {
            value = read_and_calc_expr(lex);
        }
        args.push_back(value);

        if (lex.peek_token().type == kTokenComma)
            lex.get_token(); // discard comma
        else
            break;
    }
}

// '(' [(name ['=' expr]) % ','] ')'
void parse_kwargs(Lexer& lex, vector<Token>& args)
{
    lex.get_expected_token(kTokenOpen);
    while (lex.peek_token().type != kTokenClose) {
        Token t = lex.get_token();
        if (t.type == kTokenNop)
            lex.throw_syntax_error("mismatching '('");
        if (t.type != kTokenLname)
            lex.throw_syntax_error("expected parameter name");
        args.push_back(t);
        if (lex.peek_token().type == kTokenAssign) {
            lex.get_token(); // discard '='
            args.push_back(read_vr(lex));
        }
        else {
            args.push_back(nop());
        }
        if (lex.peek_token().type == kTokenComma)
            lex.get_token(); // discard ','
        else if (lex.peek_token().type != kTokenClose)
            lex.throw_syntax_error("unexpected token after arg #"
                                   + S(args.size()/2 + 1));
    }
    lex.get_token(); // discard ')'
}

void parse_define_args(Lexer& lex, vector<Token>& args)
{
    Token t = lex.get_expected_token(kTokenCname);
    args.push_back(t);
    parse_kwargs(lex, args);
    t = lex.get_expected_token(kTokenAssign);
    args.push_back(t);
    t = lex.get_token();
    if (t.type == kTokenCname) { // CompoundFunction
    }
    else if (t.as_string() == "x" && lex.peek_token().type == kTokenLT) {
        // SplitFunction
    }
    else {
        // CustomFunction
    }
    //TODO
    /*
   type_name
    >> '(' >> !((function_param >> !('=' >> no_actions_d[FuncG])) % ',')
    >> ')' >> '='
                  >> (((type_name >> '('  // CompoundFunction
                      >> (no_actions_d[FuncG]  % ',')
                      >> ')'
                      ) % '+')
                     | str_p("x") >> str_p("<") >> +~chset_p("\n;#")

                     | no_actions_d[FuncG] //Custom Function
                       >> !("where"
                            >> (function_param >> '=' >> no_actions_d[FuncG])
                                % ','
                           )
                     )
     */
}

void parse_undefine_args(Lexer& lex, vector<Token>& args)
{
    for (;;) {
        Token t = lex.get_expected_token(kTokenCname);
        args.push_back(t);
        if (lex.peek_token().type == kTokenComma)
            lex.get_token();
        else
            break;
    }
}

void parse_delete_args(Lexer& lex, vector<Token>& args)
{
    for (;;) {
        Token t = lex.get_token();
        if (t.type != kTokenDataset && t.type != kTokenFuncname
                && t.type != kTokenVarname)
            lex.throw_syntax_error("unexpected arg after `delete'");
        // TODO: allow "delete %pd*".
        args.push_back(t);
        if (lex.peek_token().type == kTokenComma)
            lex.get_token();
        else
            break;
    }
}

void parse_exec_args(Lexer& lex, vector<Token>& args)
{
    if (lex.peek_token().type == kTokenBang) {
        lex.get_token(); // discard '!'
        args.push_back(lex.get_rest_of_line());
    }
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
    else if (t.type == kTokenNumber) {
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
    args.push_back(t);
    if (lex.peek_token().type == kTokenOpen)
        parse_kwargs(lex, args);
    parse_real_range(lex, args);
}

void Parser::parse_info_args(Lexer& lex, vector<Token>& args)
{
    parse_one_info_arg(lex, args);
    while (lex.peek_token().type == kTokenComma) {
        lex.get_token(); // discard ','
        parse_one_info_arg(lex, args);
    }
    //TODO parse redir
}

void Parser::parse_one_info_arg(Lexer& lex, vector<Token>& args)
{
    Token token = lex.get_token();
    if (token.type == kTokenLname) {
        string word = token.as_string();
        const char** pos = info_args;
        while (*pos != NULL && *pos != word)
            ++pos;
        if (*pos == NULL)
            lex.throw_syntax_error("Unknown info argument: " + word);
        args.push_back(token);
        if (word == "history" || word == "guess") {
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
        else if (word == "par") {
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


// [Key] (Dataset | 0) % '+'
void parse_dataset_tr_args(Lexer& lex, vector<Token>& args)
{
    args.push_back(lex.get_token_if(kTokenLname));
    for (;;) {
        args.push_back(lex.get_expected_token(kTokenDataset, "0"));
        if (lex.peek_token().type == kTokenPlus)
            args.push_back(lex.get_token()); // append '+'
        else
            break;
    }
}

void Parser::parse_assign_func(Lexer& lex, vector<Token>& args)
{
    Token f = lex.get_expected_token(kTokenCname, "copy");
    lex.get_expected_token(kTokenOpen); // discard '('
    if (f.type == kTokenCname) {
        // Uname '(' ([Lname '='] var_rhs) % ',' ')'
        args.push_back(f);
        bool has_kwarg = false;
        while (lex.peek_token().type != kTokenClose) {
            Token t = lex.get_token();
            if (lex.peek_token().type == kTokenAssign) {
                if (t.type != kTokenLname)
                    lex.throw_syntax_error("wrong token before '='");
                args.push_back(t);
                lex.get_token(); // discard '='
                has_kwarg = true;
            }
            else {
                if (has_kwarg)
                    lex.throw_syntax_error("non-keyword arg after keyword arg");
                args.push_back(nop());
                lex.go_back(t);
            }
            args.push_back(read_vr(lex));
            if (lex.peek_token().type == kTokenComma)
                lex.get_token(); // discard ','
            else
                break;
        }
    }
    else {
        // "copy" '(' func_id ')'
        args.push_back(f); // "copy"
        lex.get_expected_token(kTokenOpen); // discard '('
        parse_func_id(lex, args, false);
    }
    lex.get_expected_token(kTokenClose); // discard ')'
}

void Parser::parse_fz(Lexer& lex, Statement &s)
{
    Token t = lex.get_token();
    // F=..., F+=...
    // ('='|'+=') (0 | %f | Type(...) | copy(%f) | F | copy(F)) % '+'
    if (t.type == kTokenAssign || t.type == kTokenAddAssign) {
        s.cmd = kCmdChangeModel;
        s.args.push_back(t);
        for (;;) {
            const Token& p = lex.peek_token();
            if (p.type == kTokenCname) {     // Type(...)
                parse_assign_func(lex, s.args);
            }
            else if (p.as_string() == "0") { // 0
                s.args.push_back(lex.get_token());
            }
            else if (p.as_string() == "copy") {
                s.args.push_back(lex.get_token()); // "copy"
                lex.get_expected_token(kTokenOpen); // discard '('
                parse_func_id(lex, s.args, true);
                lex.get_expected_token(kTokenClose); // discard ')'
            }
            else
                parse_func_id(lex, s.args, true);
            if (lex.peek_token().type == kTokenPlus)
                s.args.push_back(lex.get_token()); // '+'
            else
                break;
        }
    }
    // F.param= ...
    else if (t.type == kTokenDot) {
        s.cmd = kCmdAssignParam;
        lex.get_token(); // discard '.'
        s.args.push_back(lex.get_expected_token(kTokenLname));
        lex.get_expected_token(kTokenAssign); // discard '='
        s.args.push_back(read_vr(lex));
    }
    // F[Number]...
    else if (t.type == kTokenRSquare) {
        s.args.push_back(read_and_calc_expr(lex));
        lex.get_expected_token(kTokenRSquare); // discard ']'
        Token k = lex.get_expected_token(kTokenAssign, kTokenDot);
        if (k.type == kTokenAssign) { // F[Number]=...
            s.cmd = kCmdChangeModel;
            if (lex.peek_token().type == kTokenFuncname) // ...=%func
                s.args.push_back(lex.get_token());
            else                                         // ...=Func(...)
                parse_assign_func(lex, s.args);
        }
        else { // F[Number].param=...
            s.cmd = kCmdAssignParam;
            s.args.push_back(lex.get_expected_token(kTokenLname));
            lex.get_expected_token(kTokenAssign); // discard '='
            s.args.push_back(read_vr(lex));
        }
    }
    else
        lex.throw_syntax_error("unexpected token after F/Z");
}



//TODO 
// @*: Y=-y
// @1: Y=-y
// @1.Y=-@1.y
// @0 @1: fit # one by one
// fit in @0, @1 # all together
//
// handle all commands from has_data_arg()

bool Parser::parse_statement(Lexer& lex)
{
    st_->cmd = kCmdNull;
    st_->with_args.clear();
    st_->args.clear();
    st_->datasets.clear();

    Token first = lex.peek_token();

    if (first.type == kTokenNop)
        return false;

    if (first.type == kTokenDataset) {
        lex.get_token();
        Token t = lex.get_token();
        if (t.type == kTokenDataset || t.type == kTokenColon) {
            st_->datasets.push_back(first.value.i);
            while (t.type == kTokenDataset) {
                st_->datasets.push_back(t.value.i);
                t = lex.get_expected_token(kTokenDataset, kTokenColon);
            }
            expand_dataset_glob();
        }
        else {
            lex.go_back(first);
        }
    }
    if (st_->datasets.empty()) {
        int ds = -1; // flag for unset dataset
        if (F_->get_dm_count() == 1)
            ds = 0;
        st_->datasets.push_back(ds);
    }

    if (first.type == kTokenLname && is_command(first, "w","ith")) {
        lex.get_token(); // discard "with"
        parse_set_args(lex, st_->with_args);
    }

    parse_command(lex);

    //parse_in_data(lex, st_->datasets);

    if (lex.peek_token().type == kTokenSemicolon)
        lex.get_token(); // discard semicolon
    else if (lex.peek_token().type != kTokenNop)
        lex.throw_syntax_error("unexpected or extra token");

    return true;
}

void Parser::parse_command(Lexer& lex)
{
    Statement &s = *st_;
    const Token token = lex.get_token();
    if (token.type == kTokenLname) {
        if (is_command(token, "def","ine")) {
            s.cmd = kCmdDefine;
            parse_define_args(lex, s.args);
        }
        else if (is_command(token, "del","ete")) {
            if (lex.peek_token().type == kTokenOpen) {
                s.cmd = kCmdDeleteP;
                lex.get_expected_token(kTokenOpen); // discard '('
                s.args.push_back(read_expr(lex));
                lex.get_expected_token(kTokenOpen); // discard ')'
            }
            else {
                s.cmd = kCmdDelete;
                parse_delete_args(lex, s.args);
            }
        }
        else if (is_command(token, "e","xecute")) {
            s.cmd = kCmdExec;
            parse_exec_args(lex, s.args);
        }
        else if (is_command(token, "f","it")) {
            s.cmd = kCmdFit;
            parse_fit_args(lex, s.args);
        }
        else if (is_command(token, "g","uess")) {
            s.cmd = kCmdGuess;
            parse_guess_args(lex, s.args);
        }
        else if (is_command(token, "i","nfo")) {
            s.cmd = kCmdInfo;
            parse_info_args(lex, s.args);
        }
        else if (is_command(token, "p","lot")) {
            s.cmd = kCmdPlot;
            if (lex.peek_token().type == kTokenDot)
                s.args.push_back(lex.get_token());
            else
                parse_real_range(lex, s.args);
            parse_real_range(lex, s.args);
        }
        else if (is_command(token, "quit","")) {
            s.cmd = kCmdQuit;
            // no args
        }
        else if (is_command(token, "s","et")) {
            s.cmd = kCmdSet;
            parse_set_args(lex, s.args);
        }
        else if (is_command(token, "undef","ine")) {
            s.cmd = kCmdUndef;
            parse_undefine_args(lex, s.args);
        }
        else if (is_command(token, "reset","")) {
            s.cmd = kCmdReset;
            // no args
        }
        else if (is_command(token, "sleep","")) {
            s.cmd = kCmdSleep;
            Token value = read_and_calc_expr(lex);
            s.args.push_back(value);
        }
        else if (is_command(token, "title","")) {
            // [@n:] "title" '='
            s.cmd = kCmdTitle;
            lex.get_expected_token(kTokenAssign); // discard '='
            s.args.push_back(lex.get_filename_token());
        }
    }
    else if (token.type == kTokenUletter) {
        const char c = *token.str;
        if (c == 'F' || c == 'Z') {
            s.args.push_back(nop()); // dataset
            s.args.push_back(token); // F/Z
            parse_fz(lex, s);
        }
        else if (c == 'M') {
            s.cmd = kCmdResizeP;
            lex.get_expected_token(kTokenAssign);
            s.args.push_back(read_expr(lex));
        }
        else if (c == 'X' || c == 'Y' || c == 'S' || c == 'A') {
            s.args.push_back(token);
            Token t = lex.get_expected_token(kTokenAssign, kTokenLSquare);
            // X =
            if (t.type == kTokenAssign) {
                s.cmd = kCmdAllPointsTr;
                for (;;) {
                    s.args.push_back(read_expr(lex));
                    if (lex.peek_token().type != kTokenComma)
                        break;
                    lex.get_token(); // discard ','
                    Token a = lex.get_expected_token(kTokenUletter);
                    char d = *a.str;
                    if (d != 'X' && d != 'Y' && d != 'S' && d != 'A')
                        lex.throw_syntax_error("unexpected letter");
                    s.args.push_back(a);
                    lex.get_expected_token(kTokenAssign); // discard '='
                }
            }
            // X [expr] =
            else {
                s.cmd = kCmdPointTr;
                for (;;) {
                    s.args.push_back(read_and_calc_expr(lex));
                    lex.get_expected_token(kTokenRSquare); // discard ']'
                    lex.get_expected_token(kTokenAssign); // discard '='
                    s.args.push_back(read_and_calc_expr(lex));
                    if (lex.peek_token().type != kTokenComma)
                        break;
                    lex.get_token(); // discard ','
                    Token a = lex.get_expected_token(kTokenUletter);
                    char d = *a.str;
                    if (d != 'X' && d != 'Y' && d != 'S' && d != 'A')
                        lex.throw_syntax_error("unexpected letter");
                    s.args.push_back(a);
                    lex.get_expected_token(kTokenLSquare); // discard '['
                }
            }
        }
        else
            lex.throw_syntax_error("unknown name: " + token.as_string());
    }
    else if (token.type == kTokenBang) {
        s.cmd = kCmdShell;
        s.args.push_back(lex.get_rest_of_line());
    }
    // $var=...
    else if (token.type == kTokenVarname &&
             lex.peek_token().type == kTokenAssign) {
        s.cmd = kCmdNameVar;
        s.args.push_back(token);
        lex.get_token(); // discard '='
        s.args.push_back(read_vr(lex));
    }
    // %func=...
    else if (token.type == kTokenFuncname &&
             lex.peek_token().type == kTokenAssign) {
        s.cmd = kCmdNameFunc;
        s.args.push_back(token);
        lex.get_token(); // discard '='
        parse_assign_func(lex, s.args);
    }
    // %func.param=...
    else if (token.type == kTokenFuncname &&
             lex.peek_token().type == kTokenDot) {
        s.cmd = kCmdAssignParam;
        lex.get_token(); // discard '.'
        s.args.push_back(lex.get_expected_token(kTokenLname));
        lex.get_expected_token(kTokenAssign); // discard '='
        s.args.push_back(read_vr(lex));
    }
    else if (token.type == kTokenDataset &&
             lex.peek_token().type == kTokenDot) {
        s.args.push_back(token); // dataset
        lex.get_token(); // discard '.'
        string arg = lex.peek_token().as_string();
        if (arg == "F" || arg == "Z") {
            s.args.push_back(token); // dataset
            s.args.push_back(lex.get_token()); // F/Z
            parse_fz(lex, s);
        }
        else
            lex.throw_syntax_error("@n. must be followed by F or Z");
    }
    else if (token.type == kTokenDataset &&
             lex.peek_token().type == kTokenLT) {
        s.cmd = kCmdLoad;
        s.args.push_back(token);
        lex.get_token(); // discard '<'
        s.args.push_back(lex.get_filename_token());
    }
    else if (token.type == kTokenDataset &&
             lex.peek_token().type == kTokenAssign) {
        s.cmd = kCmdDatasetTr;
        s.args.push_back(token);
        lex.get_token(); // discard '='
        parse_dataset_tr_args(lex, s.args);
    }
    else {
        lex.throw_syntax_error("unexpected token at command beginning");
    }
}

bool Parser::check_syntax(const string& str)
{
    try {
        Lexer lex(str.c_str());
        while(parse_statement(lex)) {
        } // nothing
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
    string r;
    if (!st_->with_args.empty()) {
        r += "With:\n";
        for (vector<Token>::const_iterator i = st_->with_args.begin();
                                                i != st_->with_args.end(); ++i)
            r += "\t" + token2str(*i) + "\n";
    }
    r += commandtype2str(st_->cmd);
    if (!st_->datasets.empty())
        r += "\tdatasets: " + join_vector(st_->datasets, " ") ;
    for (vector<Token>::const_iterator i = st_->args.begin();
                                                i != st_->args.end(); ++i)
        r += "\n\t" + token2str(*i);
    r += "\n";
    return r;
}

void Parser::expand_dataset_glob()
{
    /*
    // no datasets specified
    if (ds.empty()) {
        if (F_->get_dm_count() == 1)
            result.push_back(0);
        else
            throw ExecuteError("Dataset must be specified (eg. 'in @0').");
    }
    else */

    vector<int> &ds = st_->datasets;
    // the most common case
    if (ds.size() == 1) {
        if (ds[0] == Lexer::kAll)
            ds = range_vector(0, F_->get_dm_count());
        return;
    }

    // general case
    for (vector<int>::iterator i = ds.begin(); i != ds.end(); ++i)
        if (*i == Lexer::kAll) {
            for (int j = 0; j != F_->get_dm_count(); ++j) {
                if (!contains_element(ds, j)) {
                    ds.insert(i, j);
                    ++i;
                }
            }
            ds.erase(i);
            --i;
        }
}


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
#include "udf.h"
#include "data.h"
#include "fityk.h"

using namespace std;
using fityk::SyntaxError;

enum CommandType
{
    kCmdWith,
    kCmdDefine,
    kCmdDelete,
    kCmdDeleteP,
    kCmdExec,
    kCmdFit,
    kCmdGuess,
    kCmdInfo,
    kCmdPlot,
    kCmdReset,
    kCmdSet,
    kCmdSleep,
    kCmdUndef,
    kCmdQuit,
    kCmdShell,
    kCmdLoad,
    kCmdDatasetTr,
    kCmdNameFunc,
    kCmdNameVar,
    kCmdAssignParam,
    kCmdTitle,
    kCmdChangeModel,
    kCmdPointTr,
    kCmdNull
};

struct Statement
{
    CommandType cmd;
    std::vector<Token> args;
    vector<int> datasets;
};

class StatementList : public std::vector<Statement>
{
public:
    StatementList() {}
};

const char* commandtype2str(CommandType c)
{
    switch (c) {
        case kCmdWith:    return "With";
        case kCmdDefine:  return "Define";
        case kCmdDelete:  return "Delete";
        case kCmdDeleteP: return "Delete";
        case kCmdExec:    return "Exec";
        case kCmdFit:     return "Fit";
        case kCmdGuess:   return "Guess";
        case kCmdInfo:    return "Info";
        case kCmdPlot:    return "Plot";
        case kCmdReset:   return "Reset";
        case kCmdSet:     return "Set";
        case kCmdSleep:   return "Sleep";
        case kCmdUndef:   return "Undef";
        case kCmdQuit:    return "Quit";
        case kCmdShell:   return "Shell";
        case kCmdLoad:    return "Load";
        case kCmdDatasetTr: return "DatasetTr";
        case kCmdNameFunc: return "NameFunc";
        case kCmdAssignParam: return "AssignParam";
        case kCmdNameVar: return "NameVar";
        case kCmdTitle:   return "Title";
        case kCmdChangeModel: return "ChangeModel";
        case kCmdPointTr: return "PointTr";
        case kCmdNull:    return "Null";
        default: return NULL; // avoid warning
    }
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

Token read_expr(Lexer& lex, bool calculate_value)
{
    Token t;
    t.type = kTokenExpr;
    t.str = lex.pchar();
    ExpressionParser ep;
    ep.parse(lex);
    t.length = lex.pchar() - t.str;
    if (calculate_value)
        t.value.d = ep.calculate_expression_value();
    return t;
}

Parser::Parser(Ftk* F)
    : F_(F), ok_(false), sts_(new StatementList)
{
}

Parser::~Parser()
{
    delete sts_;
}


// '.' | ( '[' (Number | '.') ':' (Number | '.') ']' )
// appends two tokens (kTokenNumber/kTokenDot/kTokenNop) to args
void parse_real_range(Lexer& lex, vector<Token>& args)
{
    if (lex.peek_token().type == kTokenDot) {
        Token t = lex.get_token();
        args.push_back(t);
        args.push_back(t); // twice, because '.' here means [.:.]
    }
    else if (lex.peek_token().type == kTokenLSquare) {
        lex.get_token(); // discard '['
        const Token& t = lex.peek_token();
        if (t.type == kTokenDot) {
            args.push_back(t);
            lex.get_token(); // discard '.'
            lex.get_expected_token(kTokenColon); // discard ':'
        }
        else if (t.type == kTokenColon) {
            args.push_back(nop());
            lex.get_token(); // discard ':'
        }
        else if (t.type == kTokenRSquare) {
            // omitted ':', never mind
            args.push_back(nop());
        }
        else {
            args.push_back(read_expr(lex, true));
            lex.get_expected_token(kTokenColon); // discard ':'
        }

        const Token& r = lex.peek_token();
        if (r.type == kTokenDot) {
            args.push_back(r);
            lex.get_token(); // discard '.'
            lex.get_expected_token(kTokenRSquare); // discard ']'
        }
        else if (r.type == kTokenRSquare) {
            lex.get_token(); // discard ']'
            args.push_back(nop());
        }
        else {
            args.push_back(read_expr(lex, true));
            lex.get_expected_token(kTokenRSquare); // discard ']'
        }

    }
    else {
        args.push_back(nop());
        args.push_back(nop()); // we always append two tokens in this function
    }
}

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

// %funcname | [@n.]('F'|'Z') '[' Number ']'
void parse_func_id(Lexer& lex, vector<Token>& args, bool accept_fz)
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
    if (accept_fz && lex.peek_token().type != kTokenLSquare)
        return;
    lex.get_expected_token(kTokenLSquare); // discard '['
    args.push_back(read_expr(lex, false));
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
        Settings *settings = F_->get_settings();
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
            value = read_expr(lex, true);
        }
        args.push_back(value);

        if (lex.peek_token().type == kTokenComma)
            lex.get_token(); // discard comma
        else
            break;
    }
}

void Parser::execute_command_set(const vector<Token>& args)
{
    Settings *settings = F_->get_settings();
    for (size_t i = 1; i < args.size(); i += 2)
        settings->setp(args[i-1].as_string(), args[i].as_string());
}

// '(' [(name '=' expr) % ','] ')'
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
            args.push_back(read_expr(lex, false));
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

void Parser::execute_command_define(const vector<Token>& args)
{
    //UdfContainer::define(s);
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

void Parser::execute_command_undefine(const vector<Token>& args)
{
    for (vector<Token>::const_iterator i = args.begin(); i != args.end(); ++i)
        UdfContainer::undefine(i->as_string());
}

void parse_delete_args(Lexer& lex, vector<Token>& args)
{
    for (;;) {
        Token t = lex.get_token();
        if (t.type != kTokenDataset && t.type != kTokenFuncname
                && t.type != kTokenVarname)
            lex.throw_syntax_error("unexpected arg after `delete'");
        args.push_back(t);
        if (lex.peek_token().type == kTokenComma)
            lex.get_token();
        else
            break;
    }
}

void Parser::execute_command_delete(const vector<Token>& args)
{
    vector<int> ds;
    vector<string> vars, funcs;
    for (vector<Token>::const_iterator i = args.begin(); i != args.end(); ++i) {
        if (i->type == kTokenDataset)
            ds.push_back(i->value.i);
        else if (i->type == kTokenFuncname)
            funcs.push_back(Lexer::get_string(*i));
        else if (i->type == kTokenVarname)
            vars.push_back(Lexer::get_string(*i));
    }
    if (!ds.empty()) {
        sort(ds.rbegin(), ds.rend());
        for (vector<int>::const_iterator j = ds.begin(); j != ds.end(); ++j)
            AL->remove_dm(*j);
    }
    F_->delete_funcs(funcs);
    F_->delete_variables(vars);
}

void parse_delete_points_args(Lexer& lex, vector<Token>& args)
{
    lex.get_expected_token(kTokenOpen); // discard '('
    args.push_back(read_expr(lex, false));
    lex.get_expected_token(kTokenOpen); // discard ')'
}

void Parser::execute_command_delete_points(const Statement& st)
{
    assert(st.args.size() == 1);
    vector<DataAndModel*> dm = get_datasets_from_indata(st.datasets);
    //Lexer lex(st.args[0].str);
    //ExpressionParser ep;
    //ep.parse(lex);
    for (vector<DataAndModel*>::const_iterator i = dm.begin();
                                                        i != dm.end(); ++i) {
        (*i)->data()->delete_points(st.args[0].as_string());
        //ep.calculate_expression_value();
    }
}

void parse_exec_args(Lexer& lex, vector<Token>& args)
{
    if (lex.peek_token().type == kTokenShell)
        args.push_back(lex.get_token());
    else
        args.push_back(lex.get_filename_token());
}

void Parser::execute_command_exec(const vector<Token>& args)
{
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
    }
    else if (t.type == kTokenNumber) {
        args.push_back(t);
    }
    else
        lex.go_back(t);
}

void Parser::execute_command_fit(const vector<Token>& args)
{
}

// [Funcname '='] Uname ['(' kwarg % ',' ')'] [range]
void parse_guess_args(Lexer& lex, vector<Token>& args)
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

void Parser::execute_command_guess(const vector<Token>& args)
{
}

void parse_info_args(Lexer& lex, vector<Token>& args)
{
    //TODO
}

void Parser::execute_command_info(const vector<Token>& args)
{
}

void parse_plot_args(Lexer& lex, vector<Token>& args)
{
    parse_real_range(lex, args);
    parse_real_range(lex, args);
}

void Parser::execute_command_plot(const vector<Token>& args)
{
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

void Parser::execute_command_dataset_tr(const vector<Token>& args)
{
}

void parse_assign_func(Lexer& lex, vector<Token>& args)
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
            args.push_back(read_expr(lex, false));
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

void parse_fz(Lexer& lex, Statement &s)
{
    Token t = lex.get_token();
    // F=..., F+=..., F-=...
    // ('='|'+='|'-=') (0 | %f | Type(...) | copy(%f) | F | copy(F)) % '+'
    if (t.type == kTokenAssign || t.type == kTokenAddAssign
            || t.type == kTokenSubAssign) {
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
            else if (p.as_string() == "copy") { // 0
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
        s.args.push_back(read_expr(lex, false));
    }
    // F[Number]...
    else if (t.type == kTokenRSquare) {
        s.args.push_back(read_expr(lex, true));
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
            s.args.push_back(read_expr(lex, false));
        }
    }
    else
        lex.throw_syntax_error("unexpected token after F/Z");
}


void Parser::execute_command_name_func(const vector<Token>& args)
{
}


void Parser::execute_command_load(const vector<Token>& args)
{
}


// Parses the string. Throws SyntaxError.
void Parser::parse(const string& str)
{
    str_ = str;
    ok_ = false;
    Lexer lex(str_.c_str());
    sts_->resize(1);

    for (;;) {
        Statement &s = sts_->back();
        s.cmd = kCmdNull;
        s.args.clear();

        const Token token = lex.get_token();
        if (token.type == kTokenLname) {
            if (is_command(token, "w","ith")) {
                s.cmd = kCmdWith;
                parse_set_args(lex, s.args);
            }
            else if (is_command(token, "def","ine")) {
                s.cmd = kCmdDefine;
                parse_define_args(lex, s.args);
            }
            else if (is_command(token, "del","ete")) {
                if (lex.peek_token().type == kTokenOpen) {
                    s.cmd = kCmdDeleteP;
                    parse_delete_points_args(lex, s.args);
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
                parse_plot_args(lex, s.args);
            }
            else if (is_command(token, "s","et")) {
                s.cmd = kCmdSet;
                parse_set_args(lex, s.args);
            }
            else if (is_command(token, "undef","ine")) {
                s.cmd = kCmdUndef;
                parse_undefine_args(lex, s.args);
            }
            else if (is_command(token, "quit","")) {
                s.cmd = kCmdQuit;
                // no args
            }
            else if (is_command(token, "reset","")) {
                s.cmd = kCmdReset;
                // no args
            }
            else if (is_command(token, "sleep","")) {
                s.cmd = kCmdSleep;
                Token value = read_expr(lex, true);
                s.args.push_back(value);
            }
        }
        else if (token.type == kTokenUletter) {
            const char c = *token.str;
            if (c == 'F' || c == 'Z') {
                s.args.push_back(nop()); // dataset
                s.args.push_back(token); // F/Z
                parse_fz(lex, s);
            }
            else if (c == 'M' || c == 'X' || c == 'Y' || c == 'S' || c == 'A') {
                s.cmd = kCmdPointTr;
                s.args.push_back(token);
                Token t = lex.get_expected_token(kTokenAssign, kTokenLSquare);
                // X =
                if (t.type == kTokenAssign)
                    s.args.push_back(nop());
                // X [expr] =
                else {
                    if (c == 'M')
                        lex.throw_syntax_error("M can not be indexed.");
                    s.args.push_back(read_expr(lex, true));
                    lex.get_expected_token(kTokenRSquare); // discard ']'
                    lex.get_expected_token(kTokenAssign); // discard '='
                }
                s.args.push_back(read_expr(lex, false));
            }
            else
                lex.throw_syntax_error("unknown name: " + token.as_string());
        }
        else if (token.type == kTokenShell) {
            s.cmd = kCmdShell;
            s.args.push_back(token);
        }
        else if (token.type == kTokenVarname &&
                 lex.peek_token().type == kTokenAssign) {
            s.cmd = kCmdNameVar;
            s.args.push_back(token);
            lex.get_token(); // discard '='
            s.args.push_back(read_expr(lex, false));
        }
        else if (token.type == kTokenFuncname &&
                 lex.peek_token().type == kTokenAssign) {
            s.cmd = kCmdNameFunc;
            s.args.push_back(token);
            lex.get_token(); // discard '='
            parse_assign_func(lex, s.args);
        }
        else if (token.type == kTokenFuncname &&
                 lex.peek_token().type == kTokenDot) {
            s.cmd = kCmdAssignParam;
            lex.get_token(); // discard '.'
            s.args.push_back(lex.get_expected_token(kTokenLname));
            lex.get_expected_token(kTokenAssign); // discard '='
            s.args.push_back(read_expr(lex, false));
        }
        else if (token.type == kTokenDataset &&
                 lex.peek_token().type == kTokenDot) {
            s.args.push_back(token); // dataset
            lex.get_token(); // discard '.'
            string arg = lex.peek_token().as_string();
            if (arg == "title") {
                // Dataset '.' "title" '='
                s.cmd = kCmdTitle;
                lex.get_token(); // discard "title"
                lex.get_expected_token(kTokenAssign); // discard '='
                s.args.push_back(lex.get_filename_token());
            }
            else if (arg == "F" || arg == "Z") {
                s.args.push_back(token); // dataset
                s.args.push_back(lex.get_token()); // F/Z
                parse_fz(lex, s);
            }
            else
                lex.throw_syntax_error("@n. must be followed by F, Z or title");
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

        parse_in_data(lex, s.datasets);

        if (s.cmd == kCmdWith) {
            sts_->resize(sts_->size() + 1);
        }
        else if (lex.peek_token().type == kTokenNop)
            break;
        else if (lex.peek_token().type == kTokenSemicolon) {
            lex.get_token(); // discard semicolon
            sts_->resize(sts_->size() + 1);
        }
        else
            lex.throw_syntax_error("unexpected or extra token");
    }
}

// Execute the last parsed string.
// Throws ExecuteError, ExitRequestedException.
void Parser::execute()
{
    bool active_with = false;
    for (StatementList::const_iterator i = sts_->begin();
                                                    i != sts_->end(); ++i) {
        switch (i->cmd) {
            case kCmdWith:
                execute_command_set(i->args);
                active_with = true;
                break;
            case kCmdDefine:
                execute_command_define(i->args);
                break;
            case kCmdDelete:
                execute_command_delete(i->args);
                break;
            case kCmdDeleteP:
                execute_command_delete_points(*i);
                break;
            case kCmdExec:
                execute_command_exec(i->args);
                break;
            case kCmdFit:
                execute_command_fit(i->args);
                break;
            case kCmdGuess:
                execute_command_guess(i->args);
                break;
            case kCmdInfo:
                execute_command_info(i->args);
                break;
            case kCmdPlot:
                execute_command_plot(i->args);
                break;
            case kCmdReset:
                F_->reset();
                F_->outdated_plot();
                break;
            case kCmdSet:
                execute_command_set(i->args);
                break;
            case kCmdSleep:
                //execute_command_sleep(i->args);
                break;
            case kCmdUndef:
                execute_command_undefine(i->args);
                break;
            case kCmdQuit:
                throw ExitRequestedException();
                break;
            case kCmdShell:
                system(i->args[0].str);
                break;
            case kCmdLoad:
                execute_command_load(i->args);
                break;
            case kCmdNameFunc:
                execute_command_name_func(i->args);
                break;
            case kCmdDatasetTr:
                execute_command_dataset_tr(i->args);
                break;
            case kCmdAssignParam:
            case kCmdNameVar:
            case kCmdChangeModel:
            case kCmdPointTr:
            case kCmdTitle:
            case kCmdNull:

                break;
        }
        if (active_with && i->cmd != kCmdWith) {
            F_->get_settings()->clear_temporary();
            active_with = false;
        }
    }
}

// Calls parse() and execute(), catches exceptions and returns status.
Commands::Status Parser::parse_and_execute(const string& str)
{
    try {
        parse(str);
        execute();
    } catch (SyntaxError &e) {
        //F_->warn(string("Syntax error. ") + e.what());
        return Commands::status_syntax_error;
    }
    catch (ExecuteError &e) {
        //F_->get_settings()->clear_temporary();
        //F_->warn(string("Error: ") + e.what());
        return Commands::status_execute_error;
    }
    return Commands::status_ok;
}

// The same as parse(), but it doesn't throw. Returns true on success.
bool Parser::check_command_syntax(const string& str)
{
    try {
        parse(str);
    } catch (SyntaxError &) {
        return false;
    }
    return true;
}

string Parser::get_statements_repr() const
{
    // if ok_ ...
    string r;
    for (StatementList::const_iterator s = sts_->begin();
                                                    s != sts_->end(); ++s) {
        r += commandtype2str(s->cmd);
        if (!s->datasets.empty())
            r += "\tdatasets: " + join_vector(s->datasets, " ") ;
        for (vector<Token>::const_iterator i = s->args.begin();
                                                    i != s->args.end(); ++i)
            r += "\n\t" + token2str(*i);
        r += "\n";
    }
    return r;
}

vector<int> Parser::expand_dataset_indices(const vector<int>& ds)
{
    vector<int> result;
    // no datasets specified
    if (ds.empty()) {
        if (F_->get_dm_count() == 1)
            result.push_back(0);
        else
            throw ExecuteError("Dataset must be specified (eg. 'in @0').");
    }
    // @*
    else if (ds.size() == 1 && ds[0] == Lexer::kAll)
        for (int i = 0; i < F_->get_dm_count(); ++i)
            result.push_back(i);
    // general case
    else
        for (vector<int>::const_iterator i = ds.begin(); i != ds.end(); ++i)
            if (*i == Lexer::kAll) {
                for (int j = 0; j < AL->get_dm_count(); ++j) {
                    if (!contains_element(result, j))
                        result.push_back(j);
                }
                return result;
            }
            else
                result.push_back(*i);
    return result;
}

vector<DataAndModel*> Parser::get_datasets_from_indata(const vector<int>& ds)
{
    vector<int> indices = expand_dataset_indices(ds);
    vector<DataAndModel*> result(indices.size());
    for (size_t i = 0; i < indices.size(); ++i)
        result[i] = F_->get_dm(indices[i]);
    return result;
}


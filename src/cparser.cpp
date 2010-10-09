// This file is part of fityk program. Copyright 2009 Marcin Wojdyr.
// Licence: GNU General Public License ver. 2+
// $Id: $

/// This parser is not used yet.
/// In the future it will replace the current parser (cmd* files)
/// Command parser.

#include "cparser.h"

#include <string.h>

#include "lexer.h"
#include "eparser.h"
#include "settings.h"
#include "logic.h"
#include "udf.h"
#include "fityk.h"

using namespace std;
using fityk::SyntaxError;

enum CommandType
{
    kCmdWith,
    kCmdDefine,
    kCmdDelete,
    kCmdFit,
    kCmdReset,
    kCmdSet,
    kCmdSleep,
    kCmdUndef,
    kCmdQuit,
    kCmdNull
};

struct Statement
{
    CommandType cmd;
    std::vector<Token> args;
};

class StatementList : public std::vector<Statement>
{
public:
    StatementList() {}
};

const char* commandtype2str(CommandType c)
{
    switch (c) {
        case kCmdWith:   return "With";
        case kCmdDefine: return "Define";
        case kCmdDelete: return "Delete";
        case kCmdFit:    return "Fit";
        case kCmdReset:  return "Reset";
        case kCmdSet:    return "Set";
        case kCmdSleep:  return "Sleep";
        case kCmdUndef:  return "Undef";
        case kCmdQuit:   return "Quit";
        case kCmdNull:   return "Null";
        default: return NULL; // avoid warning
    }
}

bool is_command(const Token& token, const char* cmd_base,
                                    const char* cmd_suffix)
{
    assert(token.type == kTokenName);
    int base_len = strlen(cmd_base);
    if (strncmp(token.str, cmd_base, base_len) != 0)
        return false;
    int left_chars = token.info.length - base_len;
    return left_chars == 0 ||
             (left_chars <= (int) strlen(cmd_suffix) &&
              strncmp(token.str + base_len, cmd_suffix, left_chars) == 0);
}

Token read_expr(Lexer& lex)
{
    Token t;
    t.type = kTokenName;
    t.str = lex.pchar();
    ExpressionParser ep;
    ep.parse(lex);
    t.info.length = lex.pchar() - t.str;
    return t;
}

Token parse_and_calculate_expr(Lexer& lex)
{
    Token ret;
    ret.type = kTokenNumber;
    ret.str = lex.peek_token().str;
    ExpressionParser ep;
    ep.parse(lex);
    ret.info.number = ep.calculate_expression_value();
    return ret;
}

Parser::Parser(Ftk* F)
    : F_(F), ok_(false), sts_(new StatementList)
{
}

Parser::~Parser()
{
    delete sts_;
}

void Parser::parse_set_args(Lexer& lex, vector<Token>& args)
{
    for (;;) {
        const Token key = lex.get_token();
        if (key.type != kTokenName)
            lex.throw_syntax_error("expected option name");
        args.push_back(key);
        const Token eq = lex.get_token();
        if (eq.type != kTokenAssign)
            lex.throw_syntax_error("expected `='");
        Settings *settings = F_->get_settings();
        Settings::ValueType t = settings->get_value_type(Lexer::get_raw(key));
        Token value;
        if (t == Settings::kNotFound) {
            lex.throw_syntax_error("no such option: " + Lexer::get_raw(key));
        }
        else if (t == Settings::kString || t == Settings::kStringEnum) {
            value = lex.get_token();
            if (value.type != kTokenName)
                lex.throw_syntax_error("a string was expected as option value"
                        " ('quote' strings with special characters)");
        }
        else {
            value = parse_and_calculate_expr(lex);
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
        settings->setp(Lexer::get_string(args[i-1]),
                       Lexer::get_string(args[i]));
}

bool is_function_type(const Token& tk)
{
    return tk.type == kTokenName && tk.info.length >= 2 && isupper(tk.str[0]);
}

void parse_define_args(Lexer& lex, vector<Token>& args)
{
    Token tk = lex.get_token();
    if (!is_function_type(tk))
        lex.throw_syntax_error("expected FunctionName after `define'");
    args.push_back(tk);
    lex.get_expected_token(kTokenOpen);
    for (;;) {
        tk = lex.get_token();
        if (tk.type == kTokenClose)
            break;
        if (tk.type != kTokenName)
            lex.throw_syntax_error("unexpected token in argument list");
        args.push_back(tk);
        if (lex.peek_token().type == kTokenAssign) {
            lex.get_token(); // discard '='
            args.push_back(read_expr(lex));
        }
        else {
            Token t;
            t.type = kTokenEOL;
            args.push_back(t);
        }
        if (lex.peek_token().type == kTokenComma)
            lex.get_token(); // discard ','
        else if (lex.peek_token().type != kTokenClose)
            lex.throw_syntax_error("unexpected token after arg #"
                                   + S(args.size()/2 + 1));
    }
    tk = lex.get_expected_token(kTokenAssign);
    args.push_back(tk);
    tk = lex.get_token();
    if (is_function_type(tk)) { // CompoundFunction
    }
    else if (tk.type == kTokenName && tk.info.length == 1 && tk.str[0] == 'x'
             && lex.peek_token().type == kTokenLT) {
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
    //TODO
    //UdfContainer::define(s);
}

void parse_undefine_args(Lexer& lex, vector<Token>& args)
{
    for (;;) {
        Token t = lex.get_expected_token(kTokenName);
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
        UdfContainer::undefine(Lexer::get_string(*i));
}

void parse_delete_args(Lexer& lex, vector<Token>& args)
{
    if (lex.peek_token().type == kTokenOpen) {
        //TODO delete (expression) in dataset
    }
    else {
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
}


void Parser::execute_command_delete(const vector<Token>& args)
{
    if (args[0].type == kTokenDataset ||
            args[0].type == kTokenFuncname ||
            args[0].type == kTokenVarname) {
        vector<int> ds;
        vector<string> vars, funcs;
        for (vector<Token>::const_iterator i = args.begin();
                i != args.end(); ++i) {
            if (i->type == kTokenDataset)
                ds.push_back(i->info.dataset);
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
    else {
        //TODO delete (expression) in dataset
    }
}

void parse_fit_args(Lexer& lex, vector<Token>& args)
{
    Token t = lex.get_token();
    if (t.type == kTokenName) {
        string name = Lexer::get_string(t);
        if (name == "undo" || name == "redo" || name == "clear_history") {
            args.push_back(t);
            return;
        }
        else if (name == "history") {
            args.push_back(t);
        }
    }
    args.push_back(read_expr(lex));
}

void Parser::execute_command_fit(const vector<Token>& args)
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
        if (token.type == kTokenName) {
            if (is_command(token, "w","ith")) {
                s.cmd = kCmdWith;
                parse_set_args(lex, s.args);
            }
            else if (is_command(token, "def","ine")) {
                s.cmd = kCmdDefine;
                parse_define_args(lex, s.args);
            }
            else if (is_command(token, "del","ete")) {
                s.cmd = kCmdDelete;
                parse_delete_args(lex, s.args);
            }
            else if (is_command(token, "e","xecute")) {
            }
            else if (is_command(token, "f","it")) {
                s.cmd = kCmdFit;
                parse_fit_args(lex, s.args);
            }
            else if (is_command(token, "g","uess")) {
            }
            else if (is_command(token, "i","nfo")) {
            }
            else if (is_command(token, "p","lot")) {
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
                const Token value = parse_and_calculate_expr(lex);
                s.args.push_back(value);
            }
            // M = 
            // X =
            // Y =
            // S =
            // A =
            // X [...] =
            // Y [...] =
            // S [...] =
            // A [...] =
            // 'F' '='
            // 'F' '.' Word '=' 
            // 'F' '[' Number ']' '='
            // 'F' '[' Number ']' '.' Word '=' 
        }
        else if (token.type == kTokenShell) {
            // dataset transformation
        }
        else if (token.type == kTokenVarname &&
                 lex.peek_token().type == kTokenAssign) {
            // assign variable
        }
        else if (token.type == kTokenFuncname &&
                 lex.peek_token().type == kTokenAssign) {
            // assign function
        }
        else if (token.type == kTokenFuncname &&
                 lex.peek_token().type == kTokenDot) {
            // assign function parameter
        }
        else if (token.type == kTokenDataset &&
                 lex.peek_token().type == kTokenDot) {
            // Dataset '.' "title" '='
            // Dataset '.' 'F' '='
            // Dataset '.' 'F' '.' Word '=' 
            // Dataset '.' 'F' '[' Number ']' '='
            // Dataset '.' 'F' '[' Number ']' '.' Word '=' 
        }
        else if (token.type == kTokenDataset &&
                 lex.peek_token().type == kTokenLT) {
            // load data file
        }
        else if (token.type == kTokenDataset &&
                 lex.peek_token().type == kTokenAssign) {
            // dataset transformation
        }
        else {
        }

        if (lex.peek_token().type == kTokenEOL)
            break;
        else
            sts_->resize(sts_->size() + 1);
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
            case kCmdFit:
                execute_command_fit(i->args);
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
            default:
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
        for (vector<Token>::const_iterator i = s->args.begin();
                                                    i != s->args.end(); ++i)
            r += "   " + token2str(*i);
        r += "\n";
    }
    return r;
}


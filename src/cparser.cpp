// This file is part of fityk program. Copyright 2009 Marcin Wojdyr.
// Licence: GNU General Public License ver. 2+
// $Id: $

/// Command parser.

#include "lexer.h"
using namespace std;

enum CommandType
{
    kCmdWith,
    kCmdDefine,
    kCmdDelete,
    kCmdSet,
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
    StatementList() {}
};

bool is_command(const Token& token, cmd_base, cmd_suffix)
{
    int base_len = strlen(cmd_base);
    if (strncmp(token.str, cmd_base, base_len) != 0)
        return false;
    int left_chars = token.length - base_len;
    return left_chars == 0 ||
             (left_chars <= strlen(cmd_suffix) &&
              strncmp(token.str + base_len, cmd_suffix, left_chars) == 0);
}


Parser::Parser()
    : ok_(false), sts_(new StatementList)
{
}

~Parser::Parser()
{
    delete sts_;
}

void parse_set_args(Lexer& lex, vector<Token>& args)
{
    for (;;) {
        const Token key = lex.get_token();
        if (key.type != kTokenName)
            lex.throw_syntax_error("expected option name");
        args.push_back(key);
        const Token eq = lex.get_token();
        if (eq.type != kTokenAssign)
            lex.throw_syntax_error("expected `='");
        Settings *settings = AL->get_settings();
        Settings::ValueType t = settings->get_value_type(Lexer::get_raw(key));
        Token value;
        if (t == Settings::kString || t == Settings::kStringEnum) {
            value = lex.get_token();
            if (value.type != Settings::kName)
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

void execute_command_with(const vector<Token>& args)
{
    Settings *settings = AL->get_settings();
    for (size_t i = 1; i < args.size(); i += 2)
        settings->set_temporary(args[i-1], args[i]);
}

void execute_command_set(const vector<Token>& args)
{
    Settings *settings = AL->get_settings();
    for (size_t i = 1; i < args.size(); i += 2)
        settings->setp(args[i-1], args[i]);
}

void parse_define_args(Lexer& lex, vector<Token>& args)
{
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

void execute_command_define(const vector<Token>& args)
{
    UdfContainer::define(s);
}

void parse_delete_args(Lexer& lex, vector<Token>& args)
{
}

void execute_command_delete(const vector<Token>& args)
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
        Statement &s = sts_.back();
        s.cmd = kCmdNull;
        s.args.clear();
        s.options.clear();

        const Token token = lex.get_token();
        if (token->type == kTokenName) {
            if (is_command(token, "w","ith")) {
                s.cmd = kCmdWith;
                parse_set_args(lex, s.args);
            }
            else if (is_command(*token, "def","ine")) {
                s.cmd = kCmdDefine;
                parse_define_args(lex, s.args);
            }
            else if (is_command(*token, "del","ete")) {
                s.cmd = kCmdDelete;
                parse_delete_args(lex, s.args);
            }
            else if (is_command(*token, "e","xecute")) {
            }
            else if (is_command(*token, "f","it")) {
                //s.cmd = kCmdDelete;
                //parse_delete_args(lex, s.args);
            }
            else if (is_command(*token, "g","uess")) {
            }
            else if (is_command(*token, "i","nfo")) {
            }
            else if (is_command(*token, "p","lot")) {
            }
            else if (is_command(*token, "s","et")) {
                s.cmd = kCmdSet;
                parse_set_args(lex, s.args);
            }
            else if (is_command(*token, "undef","ine")) {
            }
            else if (is_command(*token, "quit","")) {
                s.cmd = kCmdQuit;
                // no args
            }
            else if (is_command(*token, "reset","")) {
                s.cmd = kCmdReset;
                // no args
            }
            else if (is_command(*token, "sleep","")) {
                s.cmd = kCmdSleep;
                const Token value = parse_and_calculate_expr(lex);
                args.push_back(value);
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
        else if (token->type == kTokenShell) {
            // dataset transformation
        }
        else if (token->type == kTokenVarname &&
                 lex.peek_token().type == kTokenAssign) {
            // assign variable
        }
        else if (token->type == kTokenFuncname &&
                 lex.peek_token().type == kTokenAssign) {
            // assign function
        }
        else if (token->type == kTokenFuncname &&
                 lex.peek_token().type == kTokenDot) {
            // assign function parameter
        }
        else if (token->type == kTokenDataset &&
                 lex.peek_token().type == kTokenDot) {
            // Dataset '.' "title" '='
            // Dataset '.' 'F' '='
            // Dataset '.' 'F' '.' Word '=' 
            // Dataset '.' 'F' '[' Number ']' '='
            // Dataset '.' 'F' '[' Number ']' '.' Word '=' 
        }
        else if (token->type == kTokenDataset &&
                 lex.peek_token().type == kTokenLT) {
            // load data file
        }
        else if (token->type == kTokenDataset &&
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
    for (StatementList::const_iterator i = sts_.begin(); i != sts_.end(); ++i) {
        switch (i->cmd) {
            case kCmdWith:
                execute_command_with(i->args);
                active_with = true;
                break;
            case kCmdDefine:
                break;
        }
        if (active_with && i->cmd != kCmdWith) {
            AL->get_settings()->clear_temporary();
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
        //AL->warn(string("Syntax error. ") + e.what());
        return Commands::status_syntax_error;
    }
    catch (ExecuteError &e) {
        //AL->get_settings()->clear_temporary();
        //AL->warn(string("Error: ") + e.what());
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


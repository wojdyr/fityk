// This file is part of fityk program. Copyright 2009 Marcin Wojdyr.
// Licence: GNU General Public License ver. 2+
// $Id: $

/// Command parser.

#include "lexer.h"

enum CommandType
{
    kCmdWith,
    kCmdDefine,
    kCmdDelete,
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

void parse_set_command(Lexer& lex, std::vector<Token>& args)
{
    for (;;) {
        const Token key = lex.get_token();
        if (key.type != kTokenName)
            lex.throw_syntax_error("expected option name");
        const Token eq = lex.get_token();
        if (eq.type != kTokenAssign)
            lex.throw_syntax_error("expected `='");
        const Token value = lex.get_token();
        if (value.type != kTokenName && value.type != kTokenString )
            lex.throw_syntax_error("expected `='");
        args.push_back(key);
        args.push_back(value);
        token = lex.get_token();
        if (token != kTokenComma)
            break;
    }
}

// Parses the string. Throws SyntaxError.
void Parser::parse(const std::string& str)
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
                parse_set_command(lex, s.args);
            }
            else if (is_command(*token, "def","ine")) {
                s.cmd = kCmdDefine;
            }
            else if (is_command(*token, "del","ete")) {
                s.cmd = kCmdDelete;
            }
            else if (is_command(*token, "quit","")) {
                s.cmd = kCmdQuit;
            }
        }
        else {
        }
    }
}

// Execute the last parsed string.
// Throws ExecuteError, ExitRequestedException.
void Parser::execute()
{
}

// Calls parse() and execute(), catches exceptions and returns status.
Commands::Status Parser::parse_and_execute(const std::string& str)
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
bool Parser::check_command_syntax(const std::string& str)
{
    try {
        parse(str);
    } catch (SyntaxError &) {
        return false;
    }
    return true;
}


// This file is part of fityk program. Copyright 2009 Marcin Wojdyr.
// Licence: GNU General Public License ver. 2+
// $Id: $

/// This parser is not used yet.
/// In the future it will replace the current parser (cmd* files)
/// Command parser.

#ifndef FITYK_CPARSER_H_
#define FITYK_CPARSER_H_

#include <string>
#include "ui.h"
#include "lexer.h"
#include "eparser.h"

class DataAndModel;

enum CommandType
{
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
    kCmdAllPointsTr,
    kCmdResizeP,
    kCmdNull
};

struct Statement
{
    std::vector<Token> with_args;
    CommandType cmd;
    std::vector<Token> args;
    std::vector<int> datasets;
};

// NULL-terminated tables
extern const char* info_args[];
extern const char* debug_args[];

class Parser
{
public:
    Parser(const Ftk* F);
    ~Parser();

    // Parses statement. Throws SyntaxError.
    // Returns false if no tokens are left.
    bool parse_statement(Lexer& lex);

    Statement& get_statement() { return *st_; }

    // The same as parse_statement(), but it doesn't throw.
    // Returns true on success.
    bool check_command_syntax(const std::string& str);

    // for debugging only
    std::string get_statements_repr() const;

    // temporarily public
    void parse_info_args(Lexer& lex, std::vector<Token>& args);

private:
    const Ftk* F_;
    ExpressionParser ep_;
    Statement *st_;

    Token read_expr(Lexer& lex);
    Token read_and_calc_expr(Lexer& lex);
    void parse_fz(Lexer& lex, Statement &s);
    void parse_assign_func(Lexer& lex, std::vector<Token>& args);
    void parse_command(Lexer& lex);
    void parse_set_args(Lexer& lex, std::vector<Token>& args);
    void parse_real_range(Lexer& lex, std::vector<Token>& args);
    void parse_func_id(Lexer& lex, std::vector<Token>& args, bool accept_fz);
    void parse_guess_args(Lexer& lex, std::vector<Token>& args);
    //std::vector<DataAndModel*> get_datasets_from_statement();
    void parse_one_info_arg(Lexer& lex, std::vector<Token>& args);
    void expand_dataset_glob();
};

#endif //FITYK_CPARSER_H_

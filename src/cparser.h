// This file is part of fityk program. Copyright 2009 Marcin Wojdyr.
// Licence: GNU General Public License ver. 2+

/// Command parser.

#ifndef FITYK_CPARSER_H_
#define FITYK_CPARSER_H_

#include <string>
#include "ui.h"
#include "lexer.h"
#include "eparser.h"
#include "tplate.h"

class DataAndModel;

enum CommandType
{
    kCmdDebug,
    kCmdDefine,
    kCmdDelete,
    kCmdDeleteP,
    kCmdExec,
    kCmdFit,
    kCmdGuess,
    kCmdInfo,
    kCmdPlot,
    kCmdPrint,
    kCmdQuit,
    kCmdReset,
    kCmdSet,
    kCmdSleep,
    kCmdTitle,
    kCmdUndef,
    kCmdUse,
    kCmdShell,
    kCmdLoad,
    kCmdDatasetTr,
    kCmdNameFunc,
    kCmdNameVar,
    kCmdAssignParam,
    //kCmdAssignAll,
    kCmdChangeModel,
    kCmdPointTr,
    kCmdAllPointsTr,
    kCmdResizeP,
    kCmdNull
};

struct Command
{
    CommandType type;
    std::vector<Token> args;
    Tplate::Ptr defined_tp; // used to parse the "define" command
};

struct Statement
{
    std::vector<int> datasets;
    std::vector<Token> with_args;
    std::vector<Command> commands;
    std::vector<VMData> vdlist;
};

// NULL-terminated tables, used for tab-expansion.
extern const char* command_list[];
extern const char* info_args[];
extern const char* debug_args[];

const char* commandtype2str(CommandType c);

class Parser
{
public:
    Parser(const Ftk* F);
    ~Parser();

    // Parses statement. Throws SyntaxError.
    // Returns false if no tokens are left.
    bool parse_statement(Lexer& lex);

    Statement& statement() { return st_; }

    // Returns true on success.
    bool check_syntax(const std::string& str);

    // for debugging only
    std::string get_statements_repr() const;

    // these functions are used to parse command fragments from outside
    void parse_info_args(Lexer& lex, std::vector<Token>& args);
    void parse_print_args(Lexer& lex, std::vector<Token>& args);
    void parse_define_rhs(Lexer& lex, Tplate *tp);
    Tplate::Ptr parse_define_args(Lexer& lex);

private:
    const Ftk* F_;
    ExpressionParser ep_;
    Statement st_;

    Token read_expr(Lexer& lex);
    Token read_and_calc_expr(Lexer& lex);
    Token read_var(Lexer& lex);
    Token read_define_arg(Lexer& lex,
                          const std::vector<std::string>& allowed_names,
                          std::vector<std::string> *new_names);
    Token read_default_value(Lexer& lex);
    void parse_fz(Lexer& lex, Command &cmd);
    void parse_assign_func(Lexer& lex, std::vector<Token>& args);
    void parse_command(Lexer& lex, Command& cmd);
    void parse_component(Lexer& lex, const std::vector<std::string>& lhs_vars,
                         Tplate::Component* c);
    void parse_set_args(Lexer& lex, std::vector<Token>& args);
    CommandType parse_xysa_args(Lexer& lex, std::vector<Token>& args);
    void parse_real_range(Lexer& lex, std::vector<Token>& args);
    void parse_func_id(Lexer& lex, std::vector<Token>& args, bool accept_fz);
    void parse_guess_args(Lexer& lex, std::vector<Token>& args);
    void parse_one_info_arg(Lexer& lex, std::vector<Token>& args);
    void parse_fit_args(Lexer& lex, std::vector<Token>& args);
};

#endif //FITYK_CPARSER_H_

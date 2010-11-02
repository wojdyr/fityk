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

struct Statement;
class DataAndModel;


// Calls Parser::parse_statement() and Runner::execute_statement(),
// catches exceptions and returns status.
Commands::Status parse_and_execute_line(Ftk* F, const std::string& str);

class Parser
{
public:
    Parser(Ftk* F);
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

private:
    Ftk* F_;
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
    void expand_dataset_glob();
};

class Runner
{
public:
    Runner(Ftk* F) : F_(F) {}

    // Execute the last parsed string.
    // Throws ExecuteError, ExitRequestedException.
    // The statement is not const, because expressions in it can be re-parsed 
    // when executing for multiple datasets.
    void execute_statement(Statement& st);

private:
    Ftk* F_;
    int ds_;

    void command_set(const std::vector<Token>& args);
    void command_define(const std::vector<Token>& args);
    void command_delete(const std::vector<Token>& args);
    void command_delete_points(const Statement& st);
    void command_exec(const std::vector<Token>& args);
    void command_fit(const std::vector<Token>& args);
    void command_guess(const std::vector<Token>& args);
    void command_info(const std::vector<Token>& args);
    void command_plot(const std::vector<Token>& args);
    void command_undefine(const std::vector<Token>& args);
    void command_load(const std::vector<Token>& args);
    void command_dataset_tr(const std::vector<Token>& args);
    void command_name_func(const std::vector<Token>& args);
    void command_all_points_tr(const std::vector<Token>& args);

    void reparse_expressions(Statement& st, int ds);
};

#endif //FITYK_CPARSER_H_

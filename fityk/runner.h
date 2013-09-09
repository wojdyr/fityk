// This file is part of fityk program. Copyright 2009 Marcin Wojdyr.
// Licence: GNU General Public License ver. 2+

#ifndef FITYK_RUNNER_H_
#define FITYK_RUNNER_H_

#include <vector>
#include "lexer.h" // Token, TokenType
#include "fityk.h" // RealRange
#include "common.h" // DISALLOW_COPY_AND_ASSIGN
#include "eparser.h" // for ExpressionParser
#include "cparser.h" // Parser

namespace fityk {

class Full;
struct Statement;
struct Command;
class Data;

RealRange args2range(const Token& t1, const Token& t2);
void token_to_data(Full* F, const Token& token, std::vector<Data*>& dms);

class Runner
{
public:
    Runner(Full* F) : F_(F), ep_(F) {}

    // Execute the last parsed string.
    // Throws ExecuteError, ExitRequestedException.
    // The statement is not const, because expressions in it can be re-parsed 
    // when executing for multiple datasets.
    void execute_statement(Statement& st);

private:
    Full* F_;
    std::vector<VMData>* vdlist_;
    ExpressionParser ep_;

    void execute_command(Command& c, int ds);
    void command_set(const std::vector<Token>& args);
    void command_delete(const std::vector<Token>& args);
    void command_delete_points(const std::vector<Token>& args, int ds);
    void command_exec(TokenType tt, const std::string& str);
    void command_fit(const std::vector<Token>& args, int ds);
    void command_guess(const std::vector<Token>& args, int ds);
    void command_plot(const std::vector<Token>& args, int ds);
    void command_ui(const std::vector<Token>& args);
    void command_undefine(const std::vector<Token>& args);
    void command_load(const std::vector<Token>& args);
    void command_dataset_tr(const std::vector<Token>& args);
    void command_name_func(const std::vector<Token>& args, int ds);
    void command_all_points_tr(const std::vector<Token>& args, int ds);
    void command_point_tr(const std::vector<Token>& args, int ds);
    void command_resize_p(const std::vector<Token>& args, int ds);
    void command_assign_param(const std::vector<Token>& args, int ds);
    void command_assign_all(const std::vector<Token>& args, int ds);
    void command_name_var(const std::vector<Token>& args);
    void command_change_model(const std::vector<Token>& args, int ds);
    void recalculate_command(Command& c, int ds, Statement& st);
    int make_func_from_template(const std::string& name,
                                const std::vector<Token>& args, int pos);
    VMData* get_vm_from_token(const Token& t) const;
};

class CommandExecutor
{
public:
    CommandExecutor(Full* F) : parser_(F), runner_(F) {}

    /// share parser -- it can be safely reused
    Parser* parser() { return &parser_; }

    // Calls Parser::parse_statement() and Runner::execute_statement().
    void raw_execute_line(const std::string& str);

private:
    Parser parser_;
    Runner runner_;
    DISALLOW_COPY_AND_ASSIGN(CommandExecutor);
};

} // namespace fityk
#endif //FITYK_RUNNER_H_

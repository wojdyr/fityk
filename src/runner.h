// This file is part of fityk program. Copyright 2009 Marcin Wojdyr.
// Licence: GNU General Public License ver. 2+
// $Id: $

#ifndef FITYK_RUNNER_H_
#define FITYK_RUNNER_H_

#include <vector>
#include "lexer.h" // Token

class Ftk;
struct Statement;

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


#endif //FITYK_RUNNER_H_

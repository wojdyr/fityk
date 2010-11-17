// This file is part of fityk program. Copyright 2009 Marcin Wojdyr.
// Licence: GNU General Public License ver. 2+
// $Id: $

#ifndef FITYK_RUNNER_H_
#define FITYK_RUNNER_H_

#include <vector>
#include "lexer.h" // Token
#include "eparser.h"

class Ftk;
struct Statement;
class DataAndModel;

class Runner
{
public:
    Runner(Ftk* F) : F_(F), ep_(F) {}

    // Execute the last parsed string.
    // Throws ExecuteError, ExitRequestedException.
    // The statement is not const, because expressions in it can be re-parsed 
    // when executing for multiple datasets.
    void execute_statement(Statement& st);

private:
    Ftk* F_;
    ExpressionParser ep_;

    void command_set(const std::vector<Token>& args);
    void command_define(const std::vector<Token>& args);
    void command_delete(const std::vector<Token>& args);
    void command_delete_points(const Statement& st, int ds);
    void command_exec(const std::vector<Token>& args);
    void command_fit(const std::vector<Token>& args, int ds);
    void command_guess(const std::vector<Token>& args, int ds);
    void command_plot(const std::vector<Token>& args);
    void command_undefine(const std::vector<Token>& args);
    void command_load(const std::vector<Token>& args);
    void command_dataset_tr(const std::vector<Token>& args);
    void command_name_func(const std::vector<Token>& args);
    void command_all_points_tr(const std::vector<Token>& args, int ds);
    void command_point_tr(const std::vector<Token>& args, int ds);
    void command_resize_p(const std::vector<Token>& args, int ds);
    void command_assign_param(const std::vector<Token>& args, int ds);
    void command_name_var(const std::vector<Token>& args, int ds);
    void command_change_model(const std::vector<Token>& args, int ds);

    void reparse_expressions(Statement& st, int ds);
    void read_dms(std::vector<Token>::const_iterator first,
                  std::vector<Token>::const_iterator last,
                  std::vector<DataAndModel*>& dms);
};

#endif //FITYK_RUNNER_H_

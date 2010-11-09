// This file is part of fityk program. Copyright 2009 Marcin Wojdyr.
// Licence: GNU General Public License ver. 2+
// $Id: $

#include "runner.h"

#include "cparser.h"
#include "eparser.h"
#include "logic.h"
#include "udf.h"
#include "data.h"
#include "fityk.h"
#include "info.h"

using namespace std;

void Runner::command_set(const vector<Token>& args)
{
    Settings *settings = F_->get_settings();
    for (size_t i = 1; i < args.size(); i += 2)
        settings->setp(args[i-1].as_string(), args[i].as_string());
}

void Runner::command_define(const vector<Token>& /*args*/)
{
    //UdfContainer::define(s);
}


void Runner::command_undefine(const vector<Token>& args)
{
    for (vector<Token>::const_iterator i = args.begin(); i != args.end(); ++i)
        UdfContainer::undefine(i->as_string());
}

void Runner::command_delete(const vector<Token>& args)
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

void Runner::command_delete_points(const Statement& st)
{
    assert(st.args.size() == 1);
    //Lexer lex(st.args[0].str);
    //ExpressionParser ep(F_);
    //ep.parse2vm(lex);
    F_->get_data(ds_)->delete_points(st.args[0].as_string());
    //ep.calculate_expression_value();
}

void Runner::command_exec(const vector<Token>& /*args*/)
{
}

void Runner::command_fit(const vector<Token>& /*args*/)
{
}

void Runner::command_guess(const vector<Token>& /*args*/)
{
}

void Runner::command_info(const vector<Token>& args)
{
    do_command_info(F_, ds_, args);
}

void Runner::command_plot(const vector<Token>& /*args*/)
{
}

void Runner::command_dataset_tr(const vector<Token>& /*args*/)
{
}

void Runner::command_name_func(const vector<Token>& /*args*/)
{
}


void Runner::command_load(const vector<Token>& /*args*/)
{
}

void Runner::command_all_points_tr(const vector<Token>& args)
{
    // args: (kTokenUletter kTokenExpr)+
    ExpressionParser ep(F_);
    for (size_t i = 0; i < args.size(); i += 2) {
        Lexer lex(args[i+1].str);
        ep.parse2vm(lex, ds_);
        ep.push_assign_lhs(args[i]);
    }
    Data *data = F_->get_data(ds_);
    ep.transform_data(data->get_mutable_points());
    data->after_transform();
}

// Execute the last parsed string.
// Throws ExecuteError, ExitRequestedException.
void Runner::execute_statement(Statement& st)
{
    if (st.with_args.empty()) {
        Settings *settings = F_->get_settings();
        for (size_t i = 1; i < st.with_args.size(); i += 2)
            settings->set_temporary(st.with_args[i-1].as_string(),
                                    st.with_args[i].as_string());
    }

    try {
        for (vector<int>::const_iterator i = st.datasets.begin();
                                            i != st.datasets.end(); ++i) {
            ds_ = *i;
            // kCmdAllPointsTr is parsed in command_all_points_tr()
            if (i != st.datasets.begin() && st.cmd != kCmdAllPointsTr)
                reparse_expressions(st, *i);

            switch (st.cmd) {
                case kCmdDefine:
                    command_define(st.args);
                    break;
                case kCmdDelete:
                    command_delete(st.args);
                    break;
                case kCmdDeleteP:
                    command_delete_points(st);
                    break;
                case kCmdExec:
                    command_exec(st.args);
                    break;
                case kCmdFit:
                    command_fit(st.args);
                    break;
                case kCmdGuess:
                    command_guess(st.args);
                    break;
                case kCmdInfo:
                    command_info(st.args);
                    break;
                case kCmdPlot:
                    command_plot(st.args);
                    break;
                case kCmdReset:
                    F_->reset();
                    F_->outdated_plot();
                    break;
                case kCmdSet:
                    command_set(st.args);
                    break;
                case kCmdSleep:
                    //command_sleep(st.args);
                    break;
                case kCmdUndef:
                    command_undefine(st.args);
                    break;
                case kCmdQuit:
                    throw ExitRequestedException();
                    break;
                case kCmdShell:
                    system(st.args[0].str);
                    break;
                case kCmdLoad:
                    command_load(st.args);
                    break;
                case kCmdNameFunc:
                    command_name_func(st.args);
                    break;
                case kCmdDatasetTr:
                    command_dataset_tr(st.args);
                    break;
                case kCmdAllPointsTr:
                    command_all_points_tr(st.args);
                    break;
                case kCmdAssignParam:
                case kCmdNameVar:
                case kCmdChangeModel:
                case kCmdPointTr:
                case kCmdResizeP:
                case kCmdTitle:
                case kCmdNull:
                    break;
            }
        }
    }
    catch (...) {
        F_->get_settings()->clear_temporary();
        throw;
    }
    F_->get_settings()->clear_temporary();
}

void Runner::reparse_expressions(Statement& st, int ds)
{
    ExpressionParser ep(F_);
    for (vector<Token>::iterator j = st.args.begin(); j != st.args.end(); ++j)
        if (j->type == kTokenExpr) {
            Lexer lex(j->str);
            ep.clear_vm();
            ep.parse2vm(lex, ds);
            j->value.d = ep.calculate();
        }
}


/*
vector<DataAndModel*> Runner::get_datasets_from_statement()
{
    vector<DataAndModel*> result(st_->datasets.size());
    for (size_t i = 0; i != st_->datasets.size(); ++i)
        result[i] = F_->get_dm(st_->datasets[i]);
    return result;
}
*/


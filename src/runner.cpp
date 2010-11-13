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
#include "fit.h"

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
    vector<int> dd;
    vector<string> vars, funcs;
    for (vector<Token>::const_iterator i = args.begin(); i != args.end(); ++i) {
        if (i->type == kTokenDataset)
            dd.push_back(i->value.i);
        else if (i->type == kTokenFuncname)
            funcs.push_back(Lexer::get_string(*i));
        else if (i->type == kTokenVarname)
            vars.push_back(Lexer::get_string(*i));
    }
    if (!dd.empty()) {
        sort(dd.rbegin(), dd.rend());
        for (vector<int>::const_iterator j = dd.begin(); j != dd.end(); ++j)
            F_->remove_dm(*j);
    }
    F_->delete_funcs(funcs);
    F_->delete_variables(vars);
}

void Runner::command_delete_points(const Statement& st, int ds)
{
    assert(st.args.size() == 1);
    Lexer lex(st.args[0].str);
    ep_.clear_vm();
    ep_.parse2vm(lex, ds);

    Data *data = F_->get_data(ds);
    const vector<Point>& p = data->points();
    int len = data->points().size();
    vector<Point> new_p;
    new_p.reserve(len);
    for (int n = 0; n != len; ++n) {
        double val = ep_.calculate(n, p);
        if (fabs(val) < 0.5)
            new_p.push_back(p[n]);
    }
    data->set_points(new_p);
}

void Runner::command_exec(const vector<Token>& args)
{
    assert(args.size() == 1);
    const Token& t = args[0];

    // exec ! program
    if (t.type == kTokenRest) {
#ifdef HAVE_POPEN
        FILE* f = NULL;
        string s = t.as_string();
        f = popen(s.c_str(), "r");
        if (!f)
            return;
        F_->get_ui()->exec_stream(f);
        pclose(f);
#else
        F_->warn ("popen() was disabled during compilation.");
#endif
    }

    // exec filename
    else {
        string filename = (t.type == kTokenString ? Lexer::get_string(t)
                                                  : t.as_string());
        F_->get_ui()->exec_script(filename);
    }
}

void Runner::read_dms(vector<Token>::const_iterator first,
                      vector<Token>::const_iterator last,
                      vector<DataAndModel*>& dms)
{
    while (first != last) {
        assert(first->type == kTokenDataset);
        int d = first->value.i;
        if (d == Lexer::kAll) {
            dms = F_->get_dms();
            return;
        }
        else
            dms.push_back(F_->get_dm(d));
        ++first;
    }
}

void Runner::command_fit(const vector<Token>& args, int ds)
{
    /*
    //TODO
    if (args.empty() || args[0].type == ) {
        F_->get_fit()->fit(-1, dms);
        return;
    }
    */
    const Token& t = args[0];
    if (t.type == kTokenNumber) {
        int n_steps = iround(t.value.d);
        vector<DataAndModel*> dms;
        if (args.size() > 1)
            read_dms(args.begin() + 1, args.end(), dms);
        else
            dms.push_back(F_->get_dm(ds));
        F_->get_fit()->fit(n_steps, dms);
    }
    else if (t.type == kTokenPlus) {
        int n_steps = iround(args[1].value.d);
        F_->get_fit()->continue_fit(n_steps);
    }
    else if (t.as_string() == "undo") {
        F_->get_fit_container()->load_param_history(-1, true);
        F_->outdated_plot();
    }
    else if (t.as_string() == "redo") {
        F_->get_fit_container()->load_param_history(+1, true);
        F_->outdated_plot();
    }
    else if (t.as_string() == "clear_history") {
        F_->get_fit_container()->clear_param_history();
    }
    else if (t.as_string() == "history") {
        int n = (int) args[2].value.d;
        F_->get_fit_container()->load_param_history(n);
        F_->outdated_plot();
    }
}

void Runner::command_guess(const vector<Token>& /*args*/)
{
}

void Runner::command_info(const vector<Token>& args, int ds)
{
    do_command_info(F_, ds, args);
}

void Runner::command_plot(const vector<Token>& args)
{
    RealRange hor, ver;
    vector<int> dd; //TODO
    args2range(args[0], args[1], hor);
    args2range(args[0], args[1], ver);
    F_->view.parse_and_set(hor, ver, dd);
    F_->get_ui()->draw_plot(1, UserInterface::kRepaintDataset);
}

void Runner::command_dataset_tr(const vector<Token>& /*args*/)
{
}

void Runner::command_name_func(const vector<Token>& /*args*/)
{
}


void Runner::command_load(const vector<Token>& args)
{
    int dataset = args[0].value.i;
    string filename = args[1].as_string();
    if (filename == ".") { // revert from the file
        if (dataset == Lexer::kNew)
            throw ExecuteError("New dataset (@+) cannot be reverted");
        if (args.size() > 2)
            throw ExecuteError("Options can't be given when reverting data");
        F_->get_data(dataset)->revert();
    }
    else { // read given file
        string format, options;
        if (args.size() > 2) {
            format = args[2].as_string();
            for (size_t i = 3; i < args.size(); ++i)
                options += (i == 3 ? "" : " ") + args[i].as_string();
        }
        F_->import_dataset(dataset, filename, format, options);
    }
    F_->outdated_plot();
}

void Runner::command_all_points_tr(const vector<Token>& args, int ds)
{
    // args: (kTokenUletter kTokenExpr)+
    ep_.clear_vm();
    for (size_t i = 0; i < args.size(); i += 2) {
        Lexer lex(args[i+1].str);
        ep_.parse2vm(lex, ds);
        ep_.push_assign_lhs(args[i]);
    }
    Data *data = F_->get_data(ds);
    ep_.transform_data(data->get_mutable_points());
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
            // kCmdAllPointsTr is parsed in command_all_points_tr()
            if (i != st.datasets.begin() && st.cmd != kCmdAllPointsTr)
                reparse_expressions(st, *i);

            printf("ds:%d  cmd=%s  #args:%d\n", *i, commandtype2str(st.cmd), (int) st.args.size());
            switch (st.cmd) {
                case kCmdDefine:
                    command_define(st.args);
                    break;
                case kCmdDelete:
                    command_delete(st.args);
                    break;
                case kCmdDeleteP:
                    command_delete_points(st, *i);
                    break;
                case kCmdExec:
                    command_exec(st.args);
                    break;
                case kCmdFit:
                    command_fit(st.args, *i);
                    break;
                case kCmdGuess:
                    command_guess(st.args);
                    break;
                case kCmdInfo:
                    command_info(st.args, *i);
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
                    F_->get_ui()->wait(st.args[0].value.d);
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
                    command_all_points_tr(st.args, *i);
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
    for (vector<Token>::iterator j = st.args.begin(); j != st.args.end(); ++j)
        if (j->type == kTokenExpr) {
            Lexer lex(j->str);
            ep_.clear_vm();
            ep_.parse2vm(lex, ds);
            j->value.d = ep_.calculate();
        }
}



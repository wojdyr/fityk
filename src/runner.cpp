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
#include "model.h"
#include "guess.h"

using namespace std;

RealRange args2range(const Token& t1, const Token& t2)
{
    RealRange range;
    if (t1.type == kTokenExpr)
        range.from = t1.value.d;
    if (t2.type == kTokenExpr)
        range.to = t2.value.d;
    return range;
}

void Runner::command_set(const vector<Token>& args)
{
    Settings *settings = F_->get_settings();
    for (size_t i = 1; i < args.size(); i += 2)
        settings->setp(args[i-1].as_string(), args[i].as_string());
}

void Runner::command_define(const vector<Token>& /*args*/)
{
    //TODO
    //UdfContainer::define(s);
}


void Runner::command_undefine(const vector<Token>& args)
{
    vector_foreach (Token, i, args)
        UdfContainer::undefine(i->as_string());
}

void Runner::command_delete(const vector<Token>& args)
{
    vector<int> dd;
    vector<string> vars, funcs;
    vector_foreach (Token, i, args) {
        if (i->type == kTokenDataset)
            dd.push_back(i->value.i);
        else if (i->type == kTokenFuncname)
            funcs.push_back(Lexer::get_string(*i));
        else if (i->type == kTokenVarname)
            vars.push_back(Lexer::get_string(*i));
    }
    if (!dd.empty()) {
        sort(dd.rbegin(), dd.rend());
        vector_foreach (int, j, dd)
            F_->remove_dm(*j);
    }
    F_->delete_funcs(funcs);
    F_->delete_variables(vars);
}

void Runner::command_delete_points(const vector<Token>& args, int ds)
{
    assert(args.size() == 1);
    Lexer lex(args[0].str);
    ep_.clear_vm();
    ep_.parse_expr(lex, ds);

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
    if (args.empty())
        F_->get_fit()->fit(-1, vector1(F_->get_dm(ds)));
    else if (args[0].type == kTokenDataset) {
        vector<DataAndModel*> dms;
        read_dms(args.begin(), args.end(), dms);
        F_->get_fit()->fit(-1, dms);
    }
    if (args[0].type == kTokenNumber) {
        int n_steps = iround(args[0].value.d);
        vector<DataAndModel*> dms;
        if (args.size() > 1)
            read_dms(args.begin() + 1, args.end(), dms);
        else
            dms.push_back(F_->get_dm(ds));
        F_->get_fit()->fit(n_steps, dms);
    }
    else if (args[0].type == kTokenPlus) {
        int n_steps = iround(args[1].value.d);
        F_->get_fit()->continue_fit(n_steps);
    }
    else if (args[0].as_string() == "undo") {
        F_->get_fit_container()->load_param_history(-1, true);
        F_->outdated_plot();
    }
    else if (args[0].as_string() == "redo") {
        F_->get_fit_container()->load_param_history(+1, true);
        F_->outdated_plot();
    }
    else if (args[0].as_string() == "clear_history") {
        F_->get_fit_container()->clear_param_history();
    }
    else if (args[0].as_string() == "history") {
        int n = (int) args[2].value.d;
        F_->get_fit_container()->load_param_history(n);
        F_->outdated_plot();
    }
}


void Runner::command_guess(const vector<Token>& args, int ds)
{
    DataAndModel* dm = F_->get_dm(ds);
    Data const* data = dm->data();

    string name; // optional function name
    int ignore_idx = -1;
    if (args[0].type == kTokenFuncname) {
        name = Lexer::get_string(args[0]);
        ignore_idx = F_->find_function_nr(name);
    }

    // function type
    assert (args[1].type == kTokenCname);
    string ftype = args[1].as_string();

    // kwargs
    vector<string> par_names;
    vector<string> par_values;
    for (size_t n = 2; n < args.size() - 3; n += 2) {
        assert (args[n].type == kTokenLname);
        par_names.push_back(args[n].as_string());
        par_values.push_back(args[n+1].as_string());
    }

    RealRange range = args2range(*(args.end()-2), *(args.end()-1));
    if (range.from >= range.to)
        throw ExecuteError("invalid range");

    // handle a special case: guess Gaussian(center=$peak_center)
    if (range.from_inf() && range.to_inf() &&
            contains_element(par_names, "center")) {
        int ctr_pos = find(par_names.begin(), par_names.end(), "center")
                        - par_names.begin();
        string ctr_str = par_values[ctr_pos];
        Lexer lex(ctr_str.c_str());
        ep_.parse_expr(lex, ds);
        fp center = ep_.calculate();
        fp delta = fabs(F_->get_settings()->get_f("guess_at_center_pm"));
        range.from = center - delta;
        range.to = center + delta;
    }

    int lb = data->get_lower_bound_ac(range.from);
    int rb = data->get_upper_bound_ac(range.to);

    Guess g(F_->get_settings());
    g.initialize(dm, lb, rb, ignore_idx);
    g.guess(ftype, par_names, par_values);
    // for now use the old ugly interface
    vector<string> vars;
    for (size_t i = 0; i != par_names.size(); ++i)
        vars.push_back(par_names[i] + "=" + par_values[i]);

    string real_name = F_->assign_func(name, ftype, vars);
    int idx = F_->find_function_nr(real_name);
    FunctionSum& ff = dm->model()->get_ff();
    ff.names.push_back(real_name);
    ff.idx.push_back(idx);
    F_->use_parameters();
    F_->outdated_plot();
}

void Runner::command_plot(const vector<Token>& args, int ds)
{
    vector<int> dd = vector1(ds); //TODO (plot, view and storing datasets)
    RealRange hor = args2range(args[0], args[1]);
    RealRange ver = args2range(args[2], args[3]);
    F_->view.set_datasets(dd);
    F_->view.change_view(hor, ver);
    F_->get_ui()->draw_plot(1, UserInterface::kRepaintDataset);
}

void Runner::command_dataset_tr(const vector<Token>& args)
{
    int n = args[0].value.i;
    string tr = args[1].as_string();
    vector<Data const*> dd;
    for (size_t i = 2; i < args.size(); ++i)
        if (args[i].type == kTokenDataset)
            dd.push_back(F_->get_data(args[i].value.i));
    F_->get_data(n)->load_data_sum(dd, tr);
}

// should be reused from kCmdChangeModel
void Runner::command_name_func(const vector<Token>& args)
{
    string t;
    string name = Lexer::get_string(args[0]);
    if (args[1].as_string() == "copy") {
        t = F_->assign_func_copy(name, Lexer::get_string(args[2]));
    }
    else {
        string ftype = args[1].as_string();
        // for now use the old ugly interface
        vector<string> vars;
        for (size_t i = 2; i < args.size(); i += 2) {
            string key;
            if (args[i].type != kTokenNop) // no keyword
                key += args[i].as_string() + "=";
            string var = args[i+1].as_string();
            vars.push_back(key+var);
        }
        t = F_->assign_func(name, ftype, vars);
    }
    F_->use_parameters();
    F_->outdated_plot(); //TODO only if function in @active
    // return t;
}

void Runner::command_assign_param(const vector<Token>& /*args*/, int /*ds*/)
{
    //TODO
}

void Runner::command_name_var(const vector<Token>& args, int /*ds*/)
{
    assert(args.size() == 2);
    assert(args[0].type == kTokenVarname);
    F_->assign_variable(Lexer::get_string(args[0]), args[1].as_string());
    F_->use_parameters();
    F_->outdated_plot(); // TODO: only for replacing old variable
}

int get_fz_or_func(const Ftk *F, int ds, vector<Token>::const_iterator a,
                   vector<string>& added)
{
    // $func -> 1
    // (Dataset|Nop) (F|Z) (Expr|Nop)
    if (a->type == kTokenFuncname) {
        added.push_back(Lexer::get_string(*a));
        return 1;
    }
    else if (a->type == kTokenDataset || a->type == kTokenNop) {
        int r_ds = a->type == kTokenDataset ? a->value.i : ds;
        const Model* model = F->get_model(r_ds);
        assert((a+1)->type == kTokenUletter);
        char c = *(a+1)->str;
        if ((a+2)->type == kTokenNop) {
            const FunctionSum& s = model->get_fz(c);
            added.insert(added.end(), s.names.begin(), s.names.end());
        }
        else {
            int idx = iround((a+2)->value.d);
            added.push_back(model->get_func_name(c, idx));
        }
        return 3;
    }
    else
        return 0;
}

static
void add_functions_to(const Ftk* F, vector<string> const &names,
                      FunctionSum& sum)
{
    vector_foreach (string, i, names) {
        int n = F->find_function_nr(*i);
        if (n == -1)
            throw ExecuteError("undefined function: %" + *i);
        if (contains_element(sum.names, *i))
            throw ExecuteError("%" + *i + " added twice");
        sum.names.push_back(*i);
        sum.idx.push_back(n);
    }
}


void Runner::command_change_model(const vector<Token>& args, int ds)
{
    // args (Dataset|Nop) ("F"|"Z") ("+"|"+=")
    //      ("0" | $func | Type ... | ("F"|"Z") (Expr|Nop)
    //       ("copy" ($func | Dataset ("F"|"Z") (Expr|Nop)))
    //      )+
    int lhs_ds = (args[0].type == kTokenDataset ? args[0].value.i : ds);
    FunctionSum& sum = F_->get_model(lhs_ds)->get_fz(*args[1].str);
    if (args[2].type == kTokenAssign) {
        sum.names.clear();
        sum.idx.clear();
    }
    vector<string> new_names;
    for (size_t i = 3; i < args.size(); ++i) {
        // $func | Dataset ("F"|"Z")
        int n_tokens = get_fz_or_func(F_, ds, args.begin()+i, new_names);
        if (n_tokens > 0) {
            i += n_tokens - 1;
        }
        // "0"
        else if (args[i].type == kTokenNumber) {
            // nothing
        }
        // "copy" ...
        else if (args[i].type == kTokenLname && args[i].as_string() == "copy") {
            vector<string> v;
            int n_tokens = get_fz_or_func(F_, ds, args.begin()+i+1, v);
            vector_foreach (string, j, v)
                new_names.push_back(F_->assign_func_copy("", *j));
            i += n_tokens;
        }
        else if (args[i].type == kTokenCname) { // func rhs
            //TODO
        }
        else
            assert(0);
    }

    add_functions_to(F_, new_names, sum);

    if (args[2].type == kTokenAssign)
        F_->auto_remove_functions();
    F_->update_indices_in_models();
    if (lhs_ds == ds)
        F_->outdated_plot();
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
        ep_.parse_expr(lex, ds);
        ep_.push_assign_lhs(args[i]);
    }
    Data *data = F_->get_data(ds);
    ep_.transform_data(data->get_mutable_points());
    data->after_transform();
}


void Runner::command_point_tr(const vector<Token>& args, int ds)
{
    vector<Point>& points = F_->get_data(ds)->get_mutable_points();
    // args: (kTokenUletter kTokenExpr kTokenExpr)+
    for (size_t n = 0; n < args.size(); n += 3) {
        char c = *args[n].str;
        int idx = iround(args[n+1].value.d);
        double val = args[n+2].value.d;
        if (idx < 0)
            idx += points.size();
        if (idx < 0 || idx >= (int) points.size())
            throw ExecuteError("wrong point index: " + S(idx));
        Point& p = points[idx];
        if (c == 'x' || c == 'X')
            p.x = val;
        else if (c == 'y' || c == 'Y')
            p.y = val;
        else if (c == 's' || c == 'S')
            p.sigma = val;
        else if (c == 'a' || c == 'A')
            p.is_active = (fabs(val) >= 0.5);
    }
}


void Runner::command_resize_p(const vector<Token>& args, int ds)
{
    // args: kTokenExpr
    int val = iround(args[0].value.d);
    if (val < 0 || val > 1e6)
        throw ExecuteError("wrong length: " + S(val));
    Data *data = F_->get_data(ds);
    data->get_mutable_points().resize(val);
    data->after_transform();
}

void Runner::execute_command(Command& c, int ds)
{
    switch (c.type) {
        case kCmdDebug:
            run_debug(F_, ds, c.args[0], c.args[1]);
            break;
        case kCmdDefine:
            command_define(c.args);
            break;
        case kCmdDelete:
            command_delete(c.args);
            break;
        case kCmdDeleteP:
            command_delete_points(c.args, ds);
            break;
        case kCmdExec:
            command_exec(c.args);
            break;
        case kCmdFit:
            command_fit(c.args, ds);
            break;
        case kCmdGuess:
            command_guess(c.args, ds);
            break;
        case kCmdInfo:
            run_info(F_, ds, kCmdInfo, c.args);
            break;
        case kCmdPlot:
            command_plot(c.args, ds);
            break;
        case kCmdPrint:
            run_info(F_, ds, kCmdPrint, c.args);
            break;
        case kCmdReset:
            F_->reset();
            F_->outdated_plot();
            break;
        case kCmdSet:
            command_set(c.args);
            break;
        case kCmdSleep:
            F_->get_ui()->wait(c.args[0].value.d);
            break;
        case kCmdUndef:
            command_undefine(c.args);
            break;
        case kCmdQuit:
            throw ExitRequestedException();
            break;
        case kCmdShell:
            system(c.args[0].str);
            break;
        case kCmdLoad:
            command_load(c.args);
            break;
        case kCmdNameFunc:
            command_name_func(c.args);
            break;
        case kCmdDatasetTr:
            command_dataset_tr(c.args);
            break;
        case kCmdAllPointsTr:
            command_all_points_tr(c.args, ds);
            break;
        case kCmdPointTr:
            command_point_tr(c.args, ds);
            break;
        case kCmdResizeP:
            command_resize_p(c.args, ds);
            break;
        case kCmdTitle:
            F_->get_data(ds)->title = c.args[0].as_string();
            break;
        case kCmdAssignParam:
            command_assign_param(c.args, ds);
            break;
        case kCmdNameVar:
            command_name_var(c.args, ds);
            break;
        case kCmdChangeModel:
            command_change_model(c.args, ds);
            break;
        case kCmdNull:
            // nothing
            break;
    }
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
        vector_foreach (int, i, st.datasets) {
            const vector<Point>& points = F_->get_data(*i)->points();
            for (vector<Command>::iterator c = st.commands.begin();
                                                c != st.commands.end(); ++c) {
                // For all next datasets, re-evaluate expression in the
                // context of the current dataset.
                // Don't evaluate commands that are parsed in command_*().
                if (i != st.datasets.begin() && (c->type == kCmdAllPointsTr ||
                                                 c->type == kCmdDeleteP)) {
                    for (vector<Token>::iterator t = c->args.begin();
                                                       t != c->args.end(); ++t)
                        if (t->type == kTokenExpr) {
                            Lexer lex(t->str);
                            ep_.clear_vm();
                            ep_.parse_expr(lex, *i);
                            t->value.d = ep_.calculate(0, points);
                        }
                }
                execute_command(*c, *i);
            }
        }
    }
    catch (...) {
        F_->get_settings()->clear_temporary();
        throw;
    }
    F_->get_settings()->clear_temporary();
}


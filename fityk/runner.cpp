// This file is part of fityk program. Copyright 2009 Marcin Wojdyr.
// Licence: GNU General Public License ver. 2+

#define BUILDING_LIBFITYK
#include "runner.h"

#include <boost/scoped_ptr.hpp>

#include "cparser.h"
#include "eparser.h"
#include "logic.h"
#include "tplate.h"
#include "func.h"
#include "data.h"
#include "fityk.h"
#include "info.h"
#include "fit.h"
#include "model.h"
#include "guess.h"
#include "transform.h"
#include "ui.h"
#include "luabridge.h"

using namespace std;

namespace fityk {

RealRange args2range(const Token& t1, const Token& t2)
{
    RealRange range;
    if (t1.type == kTokenExpr)
        range.lo = t1.value.d;
    if (t2.type == kTokenExpr)
        range.hi = t2.value.d;
    return range;
}

void add_dms_from_token(Ftk* F, const Token& token, vector<DataAndModel*>& dms)
{
    assert(token.type == kTokenDataset);
    int d = token.value.i;
    if (d == Lexer::kAll) {
        dms = F->get_dms();
        return;
    }
    else
        dms.push_back(F->get_dm(d));
}

VMData* Runner::get_vm_from_token(const Token& t) const
{
    assert (t.type == kTokenEVar);
    return &(*vdlist_)[t.value.i];
}

void Runner::command_set(const vector<Token>& args)
{
    SettingsMgr *sm = F_->settings_mgr();
    for (size_t i = 1; i < args.size(); i += 2) {
        string key = args[i-1].as_string();
        if (key == "exit_on_warning" /*unused since 1.1.1*/) {
            F_->msg("Option `exit_on_warning' is obsolete.");
            continue;
        }
        if (args[i].type == kTokenExpr)
            sm->set_as_number(key, args[i].value.d);
        else
            sm->set_as_string(key, Lexer::get_string(args[i]));
    }
}

void Runner::command_undefine(const vector<Token>& args)
{
    v_foreach (Token, i, args)
        F_->get_tpm()->undefine(i->as_string());
}

void Runner::command_delete(const vector<Token>& args)
{
    vector<int> dd;
    vector<string> vars, funcs, files;
    v_foreach (Token, i, args) {
        if (i->type == kTokenDataset)
            dd.push_back(i->value.i);
        else if (i->type == kTokenFuncname)
            funcs.push_back(Lexer::get_string(*i));
        else if (i->type == kTokenVarname)
            vars.push_back(Lexer::get_string(*i));
        else if (i->type == kTokenFilename || i->type == kTokenString)
            files.push_back(Lexer::get_string(*i));
        else
            assert(0);
    }
    if (!dd.empty()) {
        sort(dd.rbegin(), dd.rend());
        v_foreach (int, j, dd)
            F_->remove_dm(*j);
    }
    F_->mgr.delete_funcs(funcs);
    F_->mgr.delete_variables(vars);
    v_foreach (string, f, files) {
        // stdio.h remove() should be portable, it is in C89
        int r = remove(f->c_str());
        if (r != 0 && F_->get_verbosity() >= 1)
            F_->ui()->mesg("Cannot remove file: " + *f);
    }
    if (!dd.empty() || !funcs.empty())
        F_->outdated_plot();
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
    F_->outdated_plot();
}

void Runner::command_exec(TokenType tt, const string& str)
{
    // exec ! program
    if (tt == kTokenRest) {
#ifdef HAVE_POPEN
        FILE* f = NULL;
        f = popen(str.c_str(), "r");
        if (!f)
            return;
        F_->ui()->exec_stream(f);
        pclose(f);
#else
        F_->ui()->warn("popen() was disabled during compilation.");
#endif
    }

    // exec filename
    else if (endswith(str, ".lua"))
        F_->lua_bridge()->exec_lua_script(str);
    else
        F_->ui()->exec_fityk_script(str);

}

void Runner::command_fit(const vector<Token>& args, int ds)
{
    if (args.empty()) {
        F_->get_fit()->fit(-1, vector1(F_->get_dm(ds)));
        F_->outdated_plot();
    }
    else if (args[0].type == kTokenDataset) {
        vector<DataAndModel*> dms;
        v_foreach(Token, i, args)
            add_dms_from_token(F_, *i, dms);
        F_->get_fit()->fit(-1, dms);
        F_->outdated_plot();
    }
    else if (args[0].type == kTokenNumber) {
        int n_steps = iround(args[0].value.d);
        vector<DataAndModel*> dms;
        for (size_t i = 1; i < args.size(); ++i)
            add_dms_from_token(F_, args[i], dms);
        if (dms.empty())
            dms.push_back(F_->get_dm(ds));
        F_->get_fit()->fit(n_steps, dms);
        F_->outdated_plot();
    }
    else if (args[0].as_string() == "undo") {
        F_->fit_manager()->load_param_history(-1, true);
        F_->outdated_plot();
    }
    else if (args[0].as_string() == "redo") {
        F_->fit_manager()->load_param_history(+1, true);
        F_->outdated_plot();
    }
    else if (args[0].as_string() == "clear_history") {
        F_->fit_manager()->clear_param_history();
    }
    else if (args[0].as_string() == "history") {
        int n = iround(args[1].value.d);
        F_->fit_manager()->load_param_history(n, false);
        F_->outdated_plot();
    }
}


void Runner::command_guess(const vector<Token>& args, int ds)
{
    DataAndModel* dm = F_->get_dm(ds);
    string name; // optional function name
    int ignore_idx = -1;
    if (args[0].type == kTokenFuncname) {
        name = Lexer::get_string(args[0]);
        ignore_idx = F_->mgr.find_function_nr(name);
    }
    else
        name = F_->mgr.next_func_name();

    // function type
    assert (args[1].type == kTokenCname);
    string ftype = args[1].as_string();
    Tplate::Ptr tp = F_->get_tpm()->get_shared_tp(ftype);
    if (!tp)
        throw ExecuteError("undefined function type: " + ftype);

    // kwargs
    vector<string> par_names;
    vector<VMData*> par_values;
    for (size_t n = 2; n < args.size() - 3; n += 2) {
        assert (args[n].type == kTokenLname);
        par_names.push_back(args[n].as_string());
        par_values.push_back(get_vm_from_token(args[n+1]));
    }
    vector<VMData*> func_args = reorder_args(tp, par_names, par_values);

    // range
    RealRange range = args2range(*(args.end()-2), *(args.end()-1));
    if (range.lo >= range.hi)
        throw ExecuteError("invalid range");

    // initialize guess
    Guess g(F_->get_settings());
    g.set_data(dm, range, ignore_idx);

    // guess
    vector<string> gkeys;
    vector<realt> gvals;
    if (tp->peak_d) {
        boost::array<double,4> peak_v = g.estimate_peak_parameters();
        gkeys.insert(gkeys.end(), Guess::peak_traits.begin(),
                                  Guess::peak_traits.end());
        gvals.insert(gvals.end(), peak_v.begin(), peak_v.end());
    }
    if (tp->linear_d) {
        boost::array<double,3> lin_v = g.estimate_linear_parameters();
        gkeys.insert(gkeys.end(), Guess::linear_traits.begin(),
                                  Guess::linear_traits.end());
        gvals.insert(gvals.end(), lin_v.begin(), lin_v.end());
    }

    // calculate default values
    vector<VMData> vd_storage(tp->fargs.size());
    for (size_t i = 0; i < tp->fargs.size(); ++i) {
        if (func_args[i] != NULL)
            continue;
        string dv = tp->defvals[i].empty() ? tp->fargs[i] : tp->defvals[i];
        ep_.clear_vm();
        Lexer lex(dv.c_str());
        bool r = ep_.parse_full(lex, 0, &gkeys);
        if (!r)
            throw ExecuteError("Cannot guess `" + dv + "' in " + tp->name);
        double value = ep_.calculate_custom(gvals);
        vd_storage[i].append_code(OP_TILDE);
        vd_storage[i].append_number(value);
        func_args[i] = &vd_storage[i];
    }

    // add function
    int idx = F_->mgr.assign_func(name, tp, func_args);

    FunctionSum& ff = dm->model()->get_ff();
    if (!contains_element(ff.names, name)) {
        ff.names.push_back(name);
        ff.idx.push_back(idx);
    }
    F_->mgr.use_parameters();
    F_->outdated_plot();
}

void Runner::command_plot(const vector<Token>& args, int ds)
{
    RealRange hor = args2range(args[0], args[1]);
    RealRange ver = args2range(args[2], args[3]);
    vector<int> dd;
    if (args.size() > 4) {
        for (size_t i = 4; i < args.size(); ++i) {
            int n = args[i].value.i;
            if (n == Lexer::kAll)
                for (int j = 0; j != F_->get_dm_count(); ++j)
                    dd.push_back(j);
            else
                dd.push_back(n);
        }
    }
    else
        dd.push_back(ds);
    F_->view.change_view(hor, ver, dd);
    F_->ui()->draw_plot(UserInterface::kRepaintImmediately);
}

void Runner::command_dataset_tr(const vector<Token>& args)
{
    assert(args.size() == 2);
    assert(args[0].type == kTokenDataset);
    assert(args[1].type == kTokenExpr);
    int n = args[0].value.i;
    Lexer lex(args[1].str);
    ep_.clear_vm();
    ep_.parse_expr(lex, F_->default_dm(), NULL, NULL,
                   ExpressionParser::kDatasetTrMode);
    DatasetTransformer dt(F_);
    dt.run_dt(ep_.vm(), n);
    F_->outdated_plot();
}

// returns the number of given args
int Runner::make_func_from_template(const string& name,
                                    const vector<Token>& args, int pos)
{
    string ftype = args[pos].as_string();
    vector<string> par_names;
    vector<VMData*> par_values;
    for (size_t n = pos+1; n < args.size(); n += 2) {
        if (args[n].type != kTokenLname && args[n].type != kTokenNop)
            break;
        if (args[n].type == kTokenLname)
            par_names.push_back(args[n].as_string());
        par_values.push_back(get_vm_from_token(args[n+1]));
    }
    if (!par_names.empty() && par_names.size() != par_values.size())
        throw ExecuteError("mixed keyword and non-keyword args");
    Tplate::Ptr tp = F_->get_tpm()->get_shared_tp(ftype);
    if (!tp)
        throw ExecuteError("undefined type of function: " + ftype);
    vector<VMData*> func_args;
    vector<VMData> vd_storage(tp->fargs.size());
    if (par_names.empty())
        func_args = par_values;
    else {
        func_args = reorder_args(tp, par_names, par_values);

        // calculate current values of VMs in par_values, it will be used
        // to handle default values
        vector<realt> cvals(par_values.size());
        vector<realt> dummy;
        for (size_t i = 0; i != par_values.size(); ++i)
            cvals[i] = run_code_for_variable(*par_values[i],
                                             F_->mgr.variables(), dummy);
        // calculate default values
        for (size_t i = 0; i != tp->fargs.size(); ++i) {
            if (func_args[i] != NULL)
                continue;
            // Default values are calculated as values
            // and converted to the "TILDE NUMBER" code.
            if (!tp->defvals.empty() && !tp->defvals[i].empty()) {
                string dv = tp->defvals[i];
                ep_.clear_vm();
                Lexer lex(dv.c_str());
                bool r = ep_.parse_full(lex, 0, &par_names);
                if (!r)
                    throw ExecuteError("Cannot calculate `" + dv + "' in "
                                       + tp->name);
                double value = ep_.calculate_custom(cvals);
                vd_storage[i].append_code(OP_TILDE);
                vd_storage[i].append_number(value);
                func_args[i] = &vd_storage[i];
            }
            else
                throw ExecuteError("missing parameter " + tp->fargs[i]);
        }
    }
    F_->mgr.assign_func(name, tp, func_args);
    return par_values.size();
}

static
string get_func(const Ftk *F, int ds, vector<Token>::const_iterator a)
{
    if (a->type == kTokenFuncname)
        return Lexer::get_string(*a);
    else {
        assert (a->type == kTokenDataset || a->type == kTokenNop);
        assert((a+1)->type == kTokenUletter);
        assert((a+2)->type == kTokenExpr);
        assert((a+1)->type == kTokenUletter);
        if (a->type == kTokenDataset)
            ds = a->value.i;
        char c = *(a+1)->str;
        int idx = iround((a+2)->value.d);
        return F->get_model(ds)->get_func_name(c, idx);
    }
}

// should be reused from kCmdChangeModel?
void Runner::command_name_func(const vector<Token>& args, int ds)
{
    string name = Lexer::get_string(args[0]);
    if (args[1].as_string() == "copy") { // copy(%f) or copy(@n.F[idx])
        string orig_name = get_func(F_, ds, args.begin()+2);
        F_->mgr.assign_func_copy(name, orig_name);
    }
    else                               // Foo(...)
        make_func_from_template(name, args, 1);
    F_->mgr.use_parameters();
    F_->outdated_plot(); //TODO only if function in @active
}

void Runner::command_assign_param(const vector<Token>& args, int ds)
{
    // args: Funcname Lname EVar
    // args: (Dataset|Nop) (F|Z) Expr Lname EVar
    string name = get_func(F_, ds, args.begin());
    string param = (args.end()-2)->as_string();
    const Token& var = *(args.end()-1);
    F_->mgr.substitute_func_param(name, param, get_vm_from_token(var));
    F_->mgr.use_parameters();
    F_->outdated_plot();
}

void Runner::command_assign_all(const vector<Token>& args, int ds)
{
    // args: (Dataset|Nop) (F|Z) '*' Lname Expr
    assert(args[0].type == kTokenDataset || args[0].type == kTokenNop);
    assert(args[1].type == kTokenUletter);
    assert(args[2].type == kTokenMult);
    assert(args[3].type == kTokenLname);
    assert(args[4].type == kTokenEVar);
    if (args[0].type == kTokenDataset)
        ds = args[0].value.i;
    char c = *args[1].str;
    string param = args[3].as_string();
    VMData* vd = get_vm_from_token(args[4]);
    const FunctionSum& fz = F_->get_model(ds)->get_fz(c);
    v_foreach (string, i, fz.names) {
        if (contains_element(F_->mgr.find_function(*i)->tp()->fargs, param))
            F_->mgr.substitute_func_param(*i, param, vd);
    }
    F_->mgr.use_parameters();
    F_->outdated_plot();
}

void Runner::command_name_var(const vector<Token>& args)
{
    assert(args.size() == 2 || args.size() == 4);
    assert(args[0].type == kTokenVarname);
    string name = Lexer::get_string(args[0]);
    VMData* vd = get_vm_from_token(args[1]);
    int pos = F_->mgr.make_variable(name, vd);
    if (args.size() == 4) {
        RealRange domain = args2range(args[2], args[3]);
        F_->mgr.set_domain(pos, domain);
    }
    F_->mgr.use_parameters();
    F_->outdated_plot(); // TODO: only for replacing old variable
}

static
int get_fz_or_func(const Ftk *F, int ds, vector<Token>::const_iterator a,
                   vector<string>& added)
{
    // $func -> 1
    // (Dataset|Nop) (F|Z) (Expr|Nop) -> 3
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
    v_foreach (string, i, names) {
        int n = F->mgr.find_function_nr(*i);
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
    FunctionSum& sum = F_->get_mutable_model(lhs_ds)->get_fz(*args[1].str);
    bool removed_functions = false;
    if (args[2].type == kTokenAssign && !sum.names.empty()) {
        sum.names.clear();
        sum.idx.clear();
        removed_functions = true;
    }
    vector<string> new_names;
    for (size_t i = 3; i < args.size(); i += 2) {
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
            v_foreach (string, j, v) {
                string name = F_->mgr.next_func_name();
                F_->mgr.assign_func_copy(name, *j);
                new_names.push_back(name);
            }
            i += n_tokens;
        }
        else if (args[i].type == kTokenCname) { // Foo(1,2)
            string name = F_->mgr.next_func_name();
            int n_args = make_func_from_template(name, args, i);
            new_names.push_back(name);
            i += 2 * n_args;
        }
        else
            assert(0);
        assert(i+1 == args.size() || args[i+1].type == kTokenPlus);
    }

    add_functions_to(F_, new_names, sum);

    if (removed_functions)
        F_->mgr.auto_remove_functions();
    F_->mgr.use_parameters();
    F_->mgr.update_indices_in_models();
    F_->outdated_plot();
}

void Runner::command_load(const vector<Token>& args)
{
    int dataset = args[0].value.i;
    string filename = Lexer::get_string(args[1]);
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
    F_->outdated_plot();
}


void Runner::command_point_tr(const vector<Token>& args, int ds)
{
    // This command can be executed thousands of times when running
    // output of "info state". Data::after_transform() is too slow to
    // be called from here. In typical script, complexity of this function
    // should not depend on the number of points (we assume that in typical
    // script, i.e. in a script from "info state", sorting is not needed).
    Data *data = F_->get_data(ds);
    vector<Point>& points = data->get_mutable_points();
    // args: (kTokenUletter kTokenExpr kTokenExpr)+
    bool sorted = true;
    for (size_t n = 0; n < args.size(); n += 3) {
        char c = *args[n].str;
        int idx = iround(args[n+1].value.d);
        double val = args[n+2].value.d;
        if (idx < 0)
            idx += points.size();
        if (idx < 0 || idx > (int) points.size())
            throw ExecuteError("wrong point index: " + S(idx));
        if (idx == (int) points.size()) {
            if (c != 'x' && c != 'X')
                throw ExecuteError("wrong index; to add point assign X first.");
            data->append_point();
        }
        Point& p = points[idx];
        if (c == 'x' || c == 'X') {
            p.x = val;
            if ((idx != 0 && points[idx-1].x > val) ||
                    (idx+1 < (int) points.size() && val > points[idx+1].x))
                sorted = false;
            data->find_step();
        }
        else if (c == 'y' || c == 'Y') {
            p.y = val;
        }
        else if (c == 's' || c == 'S')
            p.sigma = val;
        else if (c == 'a' || c == 'A') {
            bool old_a = p.is_active;
            p.is_active = (fabs(val) >= 0.5);
            if (old_a != p.is_active)
                data->update_active_for_one_point(idx);
        }
    }

    if (!sorted) {
        data->sort_points();
        data->find_step();
        data->update_active_p();
    }
    F_->outdated_plot();
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
    F_->outdated_plot();
}

void Runner::execute_command(Command& c, int ds)
{
    switch (c.type) {
        case kCmdDebug:
            command_debug(F_, ds, c.args[0], c.args[1]);
            break;
        case kCmdDefine:
            F_->get_tpm()->define(c.defined_tp);
            break;
        case kCmdDelete:
            command_delete(c.args);
            break;
        case kCmdDeleteP:
            command_delete_points(c.args, ds);
            break;
        case kCmdExec:
            assert(0);
            // kCmdExec is handled elsewhere
            break;
        case kCmdFit:
            command_fit(c.args, ds);
            break;
        case kCmdGuess:
            command_guess(c.args, ds);
            break;
        case kCmdInfo:
            command_redirectable(F_, ds, kCmdInfo, c.args);
            break;
        case kCmdLua:
            assert(0);
            // kCmdLua is handled elsewhere
            break;
        case kCmdPlot:
            command_plot(c.args, ds);
            break;
        case kCmdPrint:
            command_redirectable(F_, ds, kCmdPrint, c.args);
            break;
        case kCmdReset:
            F_->reset();
            F_->outdated_plot();
            break;
        case kCmdSet:
            command_set(c.args);
            break;
        case kCmdSleep:
            F_->ui()->wait(c.args[0].value.d);
            break;
        case kCmdUndef:
            command_undefine(c.args);
            break;
        case kCmdUse:
            F_->set_default_dm(c.args[0].value.i);
            F_->outdated_plot();
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
            command_name_func(c.args, ds);
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
            F_->get_data(ds)->set_title(Lexer::get_string(c.args[0]));
            break;
        case kCmdAssignParam:
            if (c.args[2].type == kTokenMult)
                command_assign_all(c.args, ds);
            else
                command_assign_param(c.args, ds);
            break;
        //case kCmdAssignAll:
        //    command_assign_all(c.args, ds);
        //    break;
        case kCmdNameVar:
            command_name_var(c.args);
            break;
        case kCmdChangeModel:
            command_change_model(c.args, ds);
            break;
        case kCmdNull:
            // nothing
            break;
    }
}

void Runner::recalculate_command(Command& c, int ds, Statement& st)
{
    // Don't evaluate commands that are parsed in command_*().
    if (c.type == kCmdAllPointsTr || c.type == kCmdDeleteP ||
            c.type == kCmdDatasetTr)
        return;

    const vector<Point>& points = F_->get_data(ds)->points();
    vm_foreach (Token, t, c.args)
        if (t->type == kTokenExpr) {
            Lexer lex(t->str);
            ep_.clear_vm();
            ep_.parse_expr(lex, ds);
            t->value.d = ep_.calculate(/*n=*/0, points);
        }
        else if (t->type == kTokenEVar) {
            Lexer lex(t->str);
            ep_.clear_vm();
            ep_.parse_expr(lex, ds, NULL, NULL, ExpressionParser::kAstMode);
            st.vdlist[t->value.i] = ep_.vm();
        }
}

// Execute the last parsed string.
// Throws ExecuteError, ExitRequestedException.
void Runner::execute_statement(Statement& st)
{
    boost::scoped_ptr<Settings> s_orig;
    vdlist_ = &st.vdlist;
    try {
        if (!st.with_args.empty()) {
            s_orig.reset(new Settings(*F_->get_settings()));
            command_set(st.with_args);
        }
        v_foreach (int, i, st.datasets) {
            vm_foreach (Command, c, st.commands) {
                // The values of expression were calculated when parsing
                // in the context of the first dataset.
                // We need to re-evaluate it for all but the first dataset,
                // and also if it is preceded by other command or by "with"
                // (e.g. epsilon can change the result)
                if (i != st.datasets.begin() || c != st.commands.begin() ||
                        !st.with_args.empty())
                    recalculate_command(*c, *i, st);

                if (c->type == kCmdExec || c->type == kCmdLua) {
                    // this command contains nested commands that use the same
                    // Parser/Runner.
                    assert(c->args.size() == 1 || (c->args.size() == 2 &&
                                             c->args[0].type == kTokenAssign));
                    bool eq = (c->args[0].type == kTokenAssign);
                    const Token& t = c->args.back();
                    TokenType tt = t.type;
                    string str = Lexer::get_string(t);
                    Statement backup;
                    // According to the 0x standard swap() does not invalidate
                    // iterators that refer to elements.
                    st.datasets.swap(backup.datasets);
                    st.with_args.swap(backup.with_args);
                    st.commands.swap(backup.commands);
                    int old_default_dm = F_->default_dm();
                    F_->set_default_dm(*i);
                    if (eq) {
                        if (c->type == kCmdExec)
                            F_->lua_bridge()->exec_lua_output(str);
                        else // if (c->type == kCmdLua)
                            F_->lua_bridge()->exec_lua_string("return " + str);
                    }
                    else {
                        if (c->type == kCmdExec)
                            command_exec(tt, str);
                        else // if (c->type == kCmdLua)
                            F_->lua_bridge()->exec_lua_string(str);
                    }
                    F_->set_default_dm(old_default_dm);
                    st.datasets.swap(backup.datasets);
                    st.with_args.swap(backup.with_args);
                    st.commands.swap(backup.commands);
                }
                else
                    execute_command(*c, *i);
            }
        }
    }
    catch (...) {
        if (!st.with_args.empty())
            F_->settings_mgr()->set_all(*s_orig);
        throw;
    }
    if (!st.with_args.empty())
        F_->settings_mgr()->set_all(*s_orig);
}

} // namespace fityk

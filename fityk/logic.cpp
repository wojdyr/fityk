// This file is part of fityk program. Copyright 2001-2013 Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#define BUILDING_LIBFITYK
#include "logic.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>

#include "common.h"
#include "data.h"
#include "model.h"
#include "ui.h"
#include "fit.h"
#include "settings.h"
#include "mgr.h"
#include "tplate.h"
#include "var.h"
#include "luabridge.h"
#include "lexer.h" // Lexer::kNew
#include "cparser.h"
#include "runner.h"

using namespace std;

namespace fityk {

Full::Full()
    : mgr(this), view(&dk)
{
    // reading numbers won't work with decimal points different than '.'
    setlocale(LC_NUMERIC, "C");
    cmd_executor_ = new CommandExecutor(this);
    ui_ = new UserInterface(this, cmd_executor_);
    initialize();
}

Full::~Full()
{
    destroy();
    delete ui_;
    delete cmd_executor_;
}

// initializations common for ctor and reset()
void Full::initialize()
{
    lua_bridge_ = new LuaBridge(this);
    fit_manager_ = new FitManager(this);
    // Settings ctor is using FitManager
    settings_mgr_ = new SettingsMgr(this);
    tplate_mgr_ = new TplateMgr;
    tplate_mgr_->add_builtin_types(cmd_executor_->parser());
    view = View(&dk);
    ui_->mark_plot_dirty();
    dk.append(new Data(this, mgr.create_model()));
    dk.set_default_idx(0);
    settings_mgr_->do_srand();
}

// cleaning common for dtor and reset()
void Full::destroy()
{
    dk.clear();
    mgr.do_reset();
    delete fit_manager_;
    delete settings_mgr_;
    delete tplate_mgr_;
    delete lua_bridge_;
}

// reset everything but UserInterface (and related settings)
void Full::reset()
{
    int verbosity = get_settings()->verbosity;
    bool autoplot = get_settings()->autoplot;
    destroy();
    initialize();
    if (verbosity != get_settings()->verbosity)
        settings_mgr_->set_as_number("verbosity", verbosity);
    if (autoplot != get_settings()->autoplot)
        settings_mgr_->set_as_number("autoplot", autoplot);
}

void DataKeeper::remove(int d)
{
    index_check(d);
    if (datas_.size() == 1) {
        datas_[0]->model()->clear();
        datas_[0]->clear();
    } else {
        delete datas_[d];
        datas_.erase(datas_.begin() + d);
    }
}

Fit* Full::get_fit() const
{
    string method_name = get_settings()->fitting_method;
    return fit_manager()->get_method(method_name);
}

namespace {

int atoi_all(string const& s)
{
    char *endptr;
    int n = strtol(s.c_str(), &endptr, 10);
    if (*endptr != 0)
        throw ExecuteError("integral number expected, got: " + s);
    return n;
}


// e.g.: "1,3..5,7" -> 1,3,4,5,7
//       "4"        -> 4
//       "2,1"      -> 2,1
vector<int> parse_int_range(string const& s, int maximum)
{
    vector<int> values;
    vector<string> t = split_string(s, ",");
    v_foreach (string, i, t) {
        string::size_type dots = i->find("..");
        if (dots == string::npos) {
            int n = atoi_all(*i);
            values.push_back(n);
        } else {
            int m = atoi_all(i->substr(0, dots));
            string n_ = i->substr(dots+2);
            int n = n_.empty() ? maximum : atoi_all(i->substr(dots+2));
            if (m < 0)
                m += maximum;
            if (n < 0)
                n += maximum;
            if (m < 0 || n < 0)
                throw ExecuteError("Negative number found in range: " + s);
            if (m <= n)
                for (int j = m; j <= n; ++j)
                    values.push_back(j);
            else
                for (int j = m; j >= n; --j)
                    values.push_back(j);
        }
    }
    return values;
}
} //anonymous namespace

void DataKeeper::import_dataset(int slot, string const& filename,
                                string const& format, string const& options,
                                BasicContext* ctx, ModelManager &mgr)
{
    const bool new_dataset = (slot == Lexer::kNew);

    // split "filename" (e.g. "foo.dat:1:2,3::") into real filename
    // and colon-separated indices
    int count_colons = ::count(filename.begin(), filename.end(), ':');
    string fn;
    vector<int> indices[3];
    vector<int> block_range;
    if (count_colons >= 4) {
        // take filename
        string::size_type fn_end = string::npos;
        for (int i = 0; i < 4; ++i)
            fn_end = filename.rfind(':', fn_end - 1);
        fn = filename.substr(0, fn_end);

        // blocks
        string::size_type end_pos = filename.size();
        string::size_type bpos = filename.rfind(':', end_pos - 1);
        string::size_type blen = end_pos - bpos - 1;
        if (blen > 0) {
            int block_count = Data::count_blocks(fn, format, options);
            string range = filename.substr(bpos+1, blen);
            block_range = parse_int_range(range, block_count-1);
        }
        end_pos = bpos;

        int first_block = block_range.empty() ? 0 : block_range[0];
        int col_count = Data::count_columns(fn, format, options, first_block);
        for (int i = 2; i >= 0; --i) {
            string::size_type pos = filename.rfind(':', end_pos - 1);
            string::size_type len = end_pos - pos - 1;
            if (len > 0) {
                string range = filename.substr(pos+1, len);
                indices[i] = parse_int_range(range, col_count);
            }
            end_pos = pos;
        }
        assert(fn_end == end_pos);
    } else {
        fn = filename;
    }

    if (indices[0].size() > 1)
        throw ExecuteError("Only one column x can be specified");
    if (indices[2].size() > 1)
        throw ExecuteError("Only one column sigma can be specified");
    if (indices[1].size() > 1 && !new_dataset)
        throw ExecuteError("Multiple y columns can be specified only with @+");

    int idx_x = indices[0].empty() ?  INT_MAX : indices[0][0];
    if (indices[1].empty())
        indices[1].push_back(INT_MAX);
    int idx_s = indices[2].empty() ? INT_MAX : indices[2][0];

    for (size_t i = 0; i < indices[1].size(); ++i) {
        if (new_dataset && (count() != 1 || !data(0)->completely_empty())) {
            // load data into new slot
            auto_ptr<Data> d(new Data(ctx, mgr.create_model()));
            d->load_file(fn, idx_x, indices[1][i], idx_s,
                         block_range, format, options);
            append(d.release());
        } else {
            // if new_dataset is true, there is only one dataset
            Data *d = data(new_dataset ? 0 : slot);
            d->load_file(fn, idx_x, indices[1][i], idx_s,
                         block_range, format, options);
        }
    }
}

void Full::outdated_plot()
{
    ui_->mark_plot_dirty();
    fit_manager_->outdated_error_cache();
}

bool Full::are_independent(std::vector<Data*> dd) const
{
    for (size_t i = 0; i != mgr.variables().size(); ++i)
        if (mgr.get_variable(i)->is_simple()) {
            bool dep = false;
            v_foreach(Data*, d, dd)
                if ((*d)->model()->is_dependent_on_var(i)) {
                    if (dep)
                        return false;
                    dep = true;
                }
        }
    return true;
}

static
bool is_fityk_script(string filename)
{
    const char *magic = "# Fityk";

    FILE *f = fopen(filename.c_str(), "rb");
    if (!f)
        return false;

    if (endswith(filename, ".fit") || endswith(filename, ".fityk") ||
            endswith(filename, ".fit.gz") || endswith(filename, ".fityk.gz")) {
        fclose(f);
        return true;
    }

    const int magic_len = strlen(magic);
    char buffer[32];
    fgets(buffer, magic_len, f);
    fclose(f);
    return !strncmp(magic, buffer, magic_len);
}

void Full::process_cmd_line_arg(const string& arg)
{
    if (startswith(arg, "=->"))
        ui()->exec_and_log(string(arg, 3));
    else if (endswith(arg, ".lua"))
        lua_bridge()->exec_lua_script(arg);
    else if (is_fityk_script(arg))
        ui()->exec_fityk_script(arg);
    else {
        ui()->exec_and_log("@+ <'" + arg + "'");
    }
}

bool Full::check_syntax(const string& str)
{
    return cmd_executor_->parser()->check_syntax(str);
}

void Full::parse_and_execute_line(const string& str)
{
    return cmd_executor_->raw_execute_line(str);
}


} // namespace fityk

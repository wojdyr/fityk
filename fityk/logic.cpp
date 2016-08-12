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
    lua_bridge_ = new LuaBridge(this);
    initialize();
}

Full::~Full()
{
    destroy();
    delete lua_bridge_;
    delete ui_;
    delete cmd_executor_;
}

// initializations common for ctor and reset()
void Full::initialize()
{
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

void DataKeeper::import_dataset(int slot, const string& data_path,
                                const string& format, const string& options,
                                BasicContext* ctx, ModelManager &mgr)
{
    const bool new_dataset = (slot == Lexer::kNew);
    // split "data_path" (e.g. "foo.dat:1:2,3::") into filename
    // and colon-separated indices
    int count_colons = ::count(data_path.begin(), data_path.end(), ':');
    LoadSpec spec;
    vector<int> indices[3];
    if (count_colons >= 4) {
        // take filename
        string::size_type fn_end = string::npos;
        for (int i = 0; i < 4; ++i)
            fn_end = data_path.rfind(':', fn_end - 1);
        spec.path = data_path.substr(0, fn_end);

        // blocks
        string::size_type end_pos = data_path.size();
        string::size_type bpos = data_path.rfind(':', end_pos - 1);
        string::size_type blen = end_pos - bpos - 1;
        if (blen > 0) {
            int block_count = Data::count_blocks(spec.path, format, options);
            string range = data_path.substr(bpos+1, blen);
            spec.blocks = parse_int_range(range, block_count-1);
        }
        end_pos = bpos;

        int first_block = spec.blocks.empty() ? 0 : spec.blocks[0];
        int col_count = Data::count_columns(spec.path, format, options,
                                            first_block);
        for (int i = 2; i >= 0; --i) {
            string::size_type pos = data_path.rfind(':', end_pos - 1);
            string::size_type len = end_pos - pos - 1;
            if (len > 0) {
                string range = data_path.substr(pos+1, len);
                indices[i] = parse_int_range(range, col_count);
            }
            end_pos = pos;
        }
        assert(fn_end == end_pos);
    } else {
        spec.path = data_path;
    }

    if (indices[0].size() > 1)
        throw ExecuteError("Only one column x can be specified");
    if (indices[2].size() > 1)
        throw ExecuteError("Only one column sigma can be specified");
    if (indices[1].size() > 1 && !new_dataset)
        throw ExecuteError("Multiple y columns can be specified only with @+");

    if (!indices[0].empty())
        spec.x_col = indices[0][0];
    if (!indices[2].empty())
        spec.sig_col = indices[2][0];

    spec.format = format;
    spec.options = options;
    if (indices[1].empty())
        indices[1].push_back(LoadSpec::NN);
    for (size_t i = 0; i < indices[1].size(); ++i) {
        spec.y_col = indices[1][i];
        do_import_dataset(new_dataset, slot, spec, ctx, mgr);
    }
}


void DataKeeper::do_import_dataset(bool new_dataset, int slot,
                                   const LoadSpec& spec,
                                   BasicContext* ctx, ModelManager &mgr)
{
    Data *d;
    auto_ptr<Data> auto_d;
    if (!new_dataset)
        d = data(slot);
    else if (count() == 1 && data(0)->completely_empty()) // reusable slot 0
        d = data(0);
    else { // new slot
        auto_d.reset(new Data(ctx, mgr.create_model()));
        d = auto_d.get();
    }
    d->load_file(spec);
    if (auto_d.get())
        append(auto_d.release());
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
    char *ret = fgets(buffer, magic_len, f);
    fclose(f);
    return ret && !strncmp(magic, buffer, magic_len);
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

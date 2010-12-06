// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
// $Id$

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <locale.h>

#include "common.h"
#include "logic.h"
#include "data.h"
#include "model.h"
#include "ui.h"
#include "fit.h"
#include "guess.h"
#include "settings.h"
#include "mgr.h"
#include "func.h"
#include "tplate.h"
#include "lexer.h" // Lexer::kNew

using namespace std;

DataAndModel::DataAndModel(Ftk *F, Data* data)
    : data_(data ? data : new Data(F)), model_(new Model(F))
{}

bool DataAndModel::has_any_info() const
{
    return data()->has_any_info() ||
           !model()->get_ff().empty() ||
           !model()->get_zz().empty();
}


Ftk::Ftk()
    : VariableManager(this),
      view(this),
      default_relative_domain_width(0.1)
{
    // reading numbers won't work with decimal points different than '.'
    setlocale(LC_NUMERIC, "C");
    ui_ = new UserInterface(this);
    initialize();
}

Ftk::~Ftk()
{
    destroy();
    delete ui_;
}

// initializations common for ctor and reset()
void Ftk::initialize()
{
    fit_container_ = new FitMethodsContainer(this);
    // Settings ctor is using FitMethodsContainer
    settings_ = new Settings(this);
    tplate_mgr_ = new TplateMgr;
    tplate_mgr_->add_builtin_types(ui_->parser());
    view = View(this);
    dirty_plot_ = true;
    append_dm();
    get_settings()->do_srand();
}

// cleaning common for dtor and reset()
void Ftk::destroy()
{
    purge_all_elements(dms_);
    VariableManager::do_reset();
    delete fit_container_;
    delete settings_;
    delete tplate_mgr_;
}

// reset everything but UserInterface (and related settings)
void Ftk::reset()
{
    string verbosity = get_settings()->getp("verbosity");
    string autoplot = get_settings()->getp("autoplot");
    destroy();
    ui_->keep_quiet = true;
    initialize();
    get_settings()->setp("verbosity", verbosity);
    get_settings()->setp("autoplot", autoplot);
    ui_->keep_quiet = false;
}

int Ftk::append_dm(Data *data)
{
    DataAndModel* dm = new DataAndModel(this, data);
    dms_.push_back(dm);
    return dms_.size() - 1;
}

void Ftk::remove_dm(int d)
{
    if (d < 0 || d >= size(dms_))
        throw ExecuteError("there is no such dataset: @" + S(d));
    delete dms_[d];
    dms_.erase(dms_.begin() + d);
    if (dms_.empty())
        append_dm();
}

int Ftk::check_dm_number(int n) const
{
    if (n == -1) {
        if (get_dm_count() == 1)
            return 0;
        else
            throw ExecuteError("Dataset must be specified.");
    }
    if (n < 0 || n >= get_dm_count())
        throw ExecuteError("No such dataset: @" + S(n));
    return n;
}

/// Send warning to UI.
void Ftk::warn(string const &s) const
{
    get_ui()->output_message(UserInterface::kWarning, s);
}

/// Send implicitely requested message to UI.
void Ftk::rmsg(string const &s) const
{
    get_ui()->output_message(UserInterface::kNormal, s);
}

/// Send message to UI.
void Ftk::msg(string const &s) const
{
    if (get_verbosity() >= 0)
         get_ui()->output_message(UserInterface::kNormal, s);
}

/// Send verbose message to UI.
void Ftk::vmsg(string const &s) const
{
    if (get_verbosity() >= 1)
         get_ui()->output_message(UserInterface::kNormal, s);
}

/// execute command(s) from string
Commands::Status Ftk::exec(string const &s)
{
    return get_ui()->exec_and_log(s);
}

Fit* Ftk::get_fit() const
{
    int nr = get_settings()->get_e("fitting_method");
    return get_fit_container()->get_method(nr);
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
        }
        else {
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


void Ftk::import_dataset(int slot, string const& filename,
                         string const& format, string const& options)
{
    const bool new_dataset = (slot == Lexer::kNew);

    // split "filename" (e.g. "foo.dat:1:2,3::") into real filename
    // and colon-separated indices
    int count_colons = count(filename.begin(), filename.end(), ':');
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
    }
    else {
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
        if (new_dataset && (get_dm_count() != 1 || get_dm(0)->has_any_info())) {
            // load data into new slot
            auto_ptr<Data> data(new Data(this));
            data->load_file(fn, idx_x, indices[1][i], idx_s,
                            block_range, format, options);
            append_dm(data.release());
        }
        else {
            // if new_dataset is true, there is only one dataset
            int n = new_dataset ? 0 : slot;
            get_data(n)->load_file(fn, idx_x, indices[1][i], idx_s,
                                   block_range, format, options);
        }
    }

    if (get_dm_count() == 1) {
        RealRange r; // default value: [:]
        view.change_view(r, r, vector1(0));
    }
}

void Ftk::outdated_plot()
{
    dirty_plot_ = true;
    fit_container_->outdated_error_cache();
}

// the use of this global variable in libfityk will be eliminated,
// because it's not thread safe.
//Ftk* AL = 0;



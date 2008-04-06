// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License version 2
// $Id$

#include <stdio.h>
#include <fstream>
#include <time.h>
#include <stdlib.h>
#include <ctype.h>
#include <locale.h>

#include "common.h"
#include "logic.h"
#include "data.h"
#include "sum.h"
#include "ui.h"
#include "fit.h"
#include "guess.h"
#include "settings.h"
#include "mgr.h"
#include "func.h"

using namespace std;

DataWithSum::DataWithSum(Ftk *F, Data* data_)
    : data(data_ ? data_ : new Data(F)), sum(new Sum(F))  
{}

bool DataWithSum::has_any_info() const
{
    return get_data()->has_any_info() || get_sum()->has_any_info(); 
}


Ftk::Ftk()
    : VariableManager(this),
      view(this),
      default_relative_domain_width(0.1)
{
    // reading numbers won't work with decimal points different than '.'
    setlocale(LC_NUMERIC, "C");
    ui = new UserInterface(this);
    initialize();
    AL = this;
}

Ftk::~Ftk() 
{
    destroy();
    delete ui;
}

// initializations common for ctor and reset()
void Ftk::initialize()
{
    fit_container = new FitMethodsContainer(this);
    // Settings ctor is using FitMethodsContainer 
    settings = new Settings(this);
    view = View(this);
    append_ds();
    get_settings()->do_srand();
    UdfContainer::initialize_udfs();
}

// cleaning common for dtor and reset()
void Ftk::destroy()
{
    dsds.clear();
    VariableManager::do_reset();
    delete fit_container;
    delete settings;
}

// reset everything but UserInterface (and related settings)
void Ftk::reset()
{
    string verbosity = get_settings()->getp("verbosity");
    string autoplot = get_settings()->getp("autoplot");
    destroy();
    ui->keep_quiet = true;
    initialize();
    get_settings()->setp("verbosity", verbosity);
    get_settings()->setp("autoplot", autoplot);
    ui->keep_quiet = false;
}

int Ftk::append_ds(Data *data)
{
    DataWithSum* ds = new DataWithSum(this, data);
    dsds.push_back(ds); 
    return dsds.size() - 1; 
}

void Ftk::remove_ds(int d)
{
    if (d < 0 || d >= size(dsds))
        throw ExecuteError("there is no such dataset: @" + S(d));
    delete dsds[d];
    dsds.erase(dsds.begin() + d);
    if (dsds.empty())
        append_ds();
}

const Function* Ftk::find_function_any(string const &fstr) const
{
    if (fstr.empty())
        return 0;
    return VariableManager::find_function(find_function_name(fstr));
}

string Ftk::find_function_name(string const &fstr) const
{
    if (fstr[0] == '%' || islower(fstr[0]))
        return fstr;
    int pos = 0;
    int pref = -1;
    if (fstr[0] == '@') {
        pos = fstr.find(".") + 1;
        pref = strtol(fstr.c_str()+1, 0, 10);
    }
    vector<string> const &names = get_ds(pref)->get_sum()->get_names(fstr[pos]);
    int idx_ = strtol(fstr.c_str()+pos+2, 0, 10);
    int idx = (idx_ >= 0 ? idx_ : idx_ + names.size());
    if (!is_index(idx, names))
        throw ExecuteError("There is no item with index " + S(idx_));
    return names[idx];
}


void Ftk::dump_all_as_script(string const &filename)
{
    ofstream os(filename.c_str(), ios::out);
    if (!os) {
        warn ("Can't open file: " + filename);
        return;
    }
    os << fityk_version_line << endl;
    os << "## dumped at: " << time_now() << endl;
    os << "set verbosity = quiet #the rest of the file is not shown\n"; 
    os << "set autoplot = never\n";
    os << "reset\n";
    os << "# ------------  settings  ------------\n";
    os << get_settings()->set_script() << endl;
    os << "# ------------  variables and functions  ------------\n";
    for (vector<Variable*>::const_iterator i = variables.begin();
            i != variables.end(); ++i)
        os << (*i)->xname << " = " << (*i)->get_formula(parameters) << endl;
    os << endl;
    vector<UdfContainer::UDF> const& udfs = UdfContainer::get_udfs();
    for (vector<UdfContainer::UDF>::const_iterator i = udfs.begin();
            i != udfs.end(); ++i)
        if (!i->is_builtin)
            os << "define " << i->formula << endl;
    os << endl;
    for (vector<Function*>::const_iterator i = functions.begin();
            i != functions.end(); ++i) {
        if ((*i)->has_outdated_type()) {
            string new_formula = Function::get_formula((*i)->type_name);
            if (!new_formula.empty())
                os << "undefine " << (*i)->type_name << endl;
            os << "define " << (*i)->type_formula << endl;
            os << (*i)->get_basic_assignment() << endl;
            os << "undefine " << (*i)->type_name << endl;
            if (!new_formula.empty())
                os << "define " << new_formula << endl;
        }
        else
            os << (*i)->get_basic_assignment() << endl;
    }
    os << endl;
    os << "# ------------  datasets and sums  ------------\n";
    for (int i = 0; i != get_ds_count(); ++i) {
        Data const* data = get_data(i);
        if (i != 0)
            os << "@+\n";
        if (!data->get_title().empty())
            os << "set @" << i << ".title = '" << data->get_title() << "'\n";
        int m = data->points().size();
        os << "M=" << m << " in @" << i << endl;
        os << "X=" << data->get_x_max() << " in @" << i 
            << " # =max(x), prevents sorting." << endl;
        for (int j = 0; j != m; ++j) {
            Point const& p = data->points()[j];
            os << "X[" << j << "]=" << p.x << ", Y[" << j << "]=" << p.y 
                << ", S[" << j << "]=" << p.sigma 
                << ", A[" << j << "]=" << (p.is_active ? 1 : 0) 
                << " in @" << i << endl;
        }
        os << endl;
        Sum const* sum = get_sum(i);
        if (!sum->get_ff_names().empty())
            os << "@" << i << ".F = " 
                << join_vector(concat_pairs("%", sum->get_ff_names()), " + ") 
                << endl;
        if (!sum->get_zz_names().empty())
            os << "@" << i << ".Z = " 
                << join_vector(concat_pairs("%", sum->get_zz_names()), " + ") 
                << endl;
        os << endl;
    }
    os << "plot " << view.str() << " in @" << view.get_datasets()[0] << endl; 
    os << "set autoplot = " << get_settings()->getp("autoplot") << endl;
    os << "set verbosity = " << get_settings()->getp("verbosity") << endl;
}


int Ftk::check_ds_number(int n) const
{
    if (n == -1) {
        if (get_ds_count() == 1)
            return 0;
        else
            throw ExecuteError("Dataset must be specified.");
    }
    if (n < 0 || n >= get_ds_count())
        throw ExecuteError("There is no dataset @" + S(n));
    return n;
}

/// Send warning to UI. 
void Ftk::warn(std::string const &s) const
{ 
    get_ui()->output_message(os_warn, s); 
}

/// Send implicitely requested message to UI. 
void Ftk::rmsg(std::string const &s) const
{ 
    get_ui()->output_message(os_normal, s);
}

/// Send message to UI. 
void Ftk::msg(std::string const &s) const
{ 
    if (get_ui()->get_verbosity() >= 0)
         get_ui()->output_message(os_normal, s); 
}

/// Send verbose message to UI. 
void Ftk::vmsg(std::string const &s) const
{ 
    if (get_ui()->get_verbosity() >= 1)
         get_ui()->output_message(os_normal, s); 
}

int Ftk::get_verbosity() const 
{ 
    return settings->get_e("verbosity"); 
}

/// execute command(s) from string
Commands::Status Ftk::exec(std::string const &s) 
{ 
    return get_ui()->exec_and_log(s); 
}

Fit* Ftk::get_fit() 
{ 
    int nr = get_settings()->get_e("fitting-method");
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
vector<int> parse_int_range(string const& s)
{
    vector<int> values;
    vector<string> t = split_string(s, ",");
    for (vector<string>::const_iterator i = t.begin(); i != t.end(); ++i) {
        string::size_type dots = i->find("..");
        if (dots == string::npos) {
            int n = atoi_all(*i);
            values.push_back(n);
        }
        else {
            int m = atoi_all(i->substr(0, dots));
            int n = atoi_all(i->substr(dots+2));
            if (abs(m-n) > 99) // too much..., take only the first one
                values.push_back(n);
            else
                for (int j = min(m,n); j <= max(m,n); ++j)
                    values.push_back(j);
        }
    }
    return values;
}
} //anonymous namespace


void Ftk::import_dataset(int slot, string const& filename, 
                         vector<string> const& options)
{
    const int new_dataset = -1;

    // split "filename" (e.g. "foo.dat:1:2,3::") into real filename 
    // and colon-separated indices
    int count_colons = count(filename.begin(), filename.end(), ':');
    string fn;
    vector<int> indices[4];
    if (count_colons >= 4) {
        string::size_type old_pos = filename.size();
        for (int i = 3; i >= 0; --i) {
            string::size_type pos = filename.rfind(':', old_pos - 1);
            string::size_type len = old_pos - pos - 1;
            if (len > 0)
                indices[i] = parse_int_range(filename.substr(pos+1, len));
            old_pos = pos;
        }
        fn = filename.substr(0, old_pos);
    }
    else {
        fn = filename;
    }

    if (indices[0].size() > 1)
        throw ExecuteError("Only one column x can be specified");
    if (indices[2].size() > 1)
        throw ExecuteError("Only one column sigma can be specified");
    if (indices[1].size() > 1 && slot != new_dataset)
        throw ExecuteError("Multiple y columns can be specified only with @+");

    int idx_x = indices[0].empty() ?  INT_MAX : indices[0][0];
    if (indices[1].empty())
        indices[1].push_back(INT_MAX);
    int idx_s = indices[2].empty() ? INT_MAX : indices[2][0];

    for (size_t i = 0; i < indices[1].size(); ++i) {
        if (slot == new_dataset
            && (this->get_ds_count() != 1 || this->get_data(0)->has_any_info()
                                        || this->get_sum(0)->has_any_info())) {
            // load data into new slot
            auto_ptr<Data> data(new Data(this));
            data->load_file(fn, idx_x, indices[1][i], idx_s, 
                            indices[3], options);
            append_ds(data.release());
        }
        else {
            // if slot == new_dataset and there is only one dataset, 
            // then get_data(slot) will point to this single slot
            get_data(slot)->load_file(fn, idx_x, indices[1][i], idx_s, 
                                      indices[3], options);
        }
    }

    if (get_ds_count() == 1) 
        view.fit_zoom();
}

// the use of this global variable in libfityk will be eliminated,
// because it's not thread safe. 
Ftk* AL = 0;



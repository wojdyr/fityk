// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

/// implementation of libfityk public API

#define BUILDING_LIBFITYK
#define FITYK_DECLARE_C_API
#include "fityk.h"

#include <cassert>
#include <cctype>
#include <cstring>

#include "common.h"
#include "ui.h"
#include "logic.h"
#include "data.h"
#include "model.h"
#include "fit.h"
#include "func.h"
#include "info.h"
#include "settings.h"

using namespace std;

#define CATCH_EXECUTE_ERROR \
    catch (ExecuteError& e) { \
        last_error_ = string("ExecuteError: ") + e.what(); \
        if (throws_) \
            throw; \
    }

#define CATCH_SYNTAX_ERROR \
    catch (SyntaxError& e) { \
        last_error_ = string("SyntaxError: ") + e.what(); \
        if (throws_) \
            throw; \
    }

// not thread-safe, but let's keep it simple
static FILE* message_sink_ = NULL;

namespace {
using namespace fityk;

void write_message_to_file(UserInterface::Style style, string const& s)
{
    if (message_sink_ && style != UserInterface::kInput)
        fprintf(message_sink_, "%s\n", s.c_str());
}

realt get_wssr_or_ssr(const Full* priv, int dataset, bool weigthed)
{
    if (dataset == ALL_DATASETS) {
        realt result = 0;
        for (int i = 0; i < priv->dk.count(); ++i)
            result += Fit::compute_wssr_for_data(priv->dk.data(i), weigthed);
        return result;
    }
    else {
        return Fit::compute_wssr_for_data(priv->dk.data(dataset), weigthed);
    }
}


vector<Data*> get_datasets_(Full* priv, int dataset)
{
    vector<Data*> dd;
    if (dataset == ALL_DATASETS) {
        for (int i = 0; i < priv->dk.count(); ++i)
            dd.push_back(priv->dk.data(i));
    }
    else {
        dd.push_back(priv->dk.data(dataset));
    }
    return dd;
}

int hd(Full* priv, int dataset)
{
    return dataset == DEFAULT_DATASET ? priv->dk.default_idx() : dataset;
}

} // anonymous namespace

namespace fityk
{

string RealRange::str() const
{
    string s;
    if (!lo_inf() || !hi_inf())
        s = " [" + (lo_inf() ? string() : eS(lo)) + ":"
                 + (hi_inf() ? string() : eS(hi)) + "]";
    return s;
}

Point::Point() : x(0), y(0), sigma(1), is_active(true) {}
Point::Point(realt x_, realt y_) : x(x_), y(y_), sigma(1), is_active(true) {}
Point::Point(realt x_, realt y_, realt sigma_) : x(x_), y(y_), sigma(sigma_),
                                                is_active(true) {}
string Point::str() const { return "(" + S(x) + "; " + S(y) + "; " +
                                 S(sigma) + (is_active ? ")*" : ") "); }

struct FitykInternalData
{
    bool owns;
    UiApi::t_show_message_callback* old_message_callback;
};


Fityk::Fityk()
    : throws_(true), p_(new FitykInternalData)
{
    priv_ = new Full;
    p_->owns = true;
    p_->old_message_callback = NULL;
}

Fityk::Fityk(Full* F)
    : throws_(true), p_(new FitykInternalData)
{
    priv_ = F;
    p_->owns = false;
    p_->old_message_callback = NULL;
}

Fityk::~Fityk()
{
    if (p_->owns)
        delete priv_;
    delete p_;
}

void Fityk::execute(string const& s)  throw(SyntaxError, ExecuteError,
                                            ExitRequestedException)
{
    try {
        priv_->parse_and_execute_line(s);
    }
    CATCH_SYNTAX_ERROR
    CATCH_EXECUTE_ERROR
}

string Fityk::get_info(string const& s, int dataset)  throw(SyntaxError,
                                                            ExecuteError)
{
    try {
        string result;
        parse_and_eval_info(priv_, s, hd(priv_, dataset), result);
        return result;
    }
    CATCH_SYNTAX_ERROR
    CATCH_EXECUTE_ERROR
    return "";
}

realt Fityk::calculate_expr(string const& s, int dataset)  throw(SyntaxError,
                                                                  ExecuteError)
{
    try {
        Lexer lex(s.c_str());
        ExpressionParser ep(priv_);
        int d = hd(priv_, dataset);
        ep.parse_expr(lex, d);
        return ep.calculate(0, priv_->dk.data(d)->points());
    }
    CATCH_SYNTAX_ERROR
    CATCH_EXECUTE_ERROR
    return 0.;
}

int Fityk::get_dataset_count() const
{
    return priv_->dk.count();
}

int Fityk::get_default_dataset() const
{
    return priv_->dk.default_idx();
}

int Fityk::get_parameter_count() const
{
    return priv_->mgr.parameters().size();
}

const vector<realt>& Fityk::all_parameters() const
{
    return priv_->mgr.parameters();
}

vector<Var*> Fityk::all_variables() const
{
    const vector<Variable*>& variables = priv_->mgr.variables();
    return vector<Var*>(variables.begin(), variables.end());
}

vector<Func*> Fityk::all_functions() const
{
    const vector<Function*>& functions = priv_->mgr.functions();
    return vector<Func*>(functions.begin(), functions.end());
}

const Func* Fityk::get_function(const std::string& name) const
{
    if (name.empty())
        return NULL;
    int n = priv_->mgr.find_function_nr(name[0] == '%' ? name.substr(1) : name);
    if (n == -1)
        return NULL;
    return priv_->mgr.functions()[n];
}

vector<Func*> Fityk::get_components(int dataset, char fz)
{
    const vector<int>& indexes = priv_->dk.get_model(dataset)->get_fz(fz).idx;
    const vector<Function*>& functions = priv_->mgr.functions();
    vector<Func*> ret(indexes.size());
    for (size_t i = 0; i != indexes.size(); ++i)
        ret[i] = functions[indexes[i]];
    return ret;
}

realt Fityk::get_model_value(realt x, int dataset)  throw(ExecuteError)
{
    try {
        return priv_->dk.get_model(hd(priv_, dataset))->value(x);
    }
    CATCH_EXECUTE_ERROR
    return 0.;
}

vector<realt> Fityk::get_model_vector(vector<realt> const& x, int dataset)
                                                          throw(ExecuteError)
{
    vector<realt> xx(x);
    vector<realt> yy(x.size(), 0.);
    try {
        priv_->dk.get_model(hd(priv_, dataset))->compute_model(xx, yy);
    }
    CATCH_EXECUTE_ERROR
    return yy;
}

const Var* Fityk::get_variable(string const& name)  throw(ExecuteError)
{
    try {
        string vname;
        if (name.empty())
            throw ExecuteError("get_variable() called with empty name");
        else if (name[0] == '$')
            vname = string(name, 1);
        else if (name[0] == '%' && name.find('.') < name.size() - 1) {
            string::size_type pos = name.find('.');
            Function const* f = priv_->mgr.find_function(string(1, pos-1));
            string pname = name.substr(pos+1);
            vname = f->used_vars().get_name(f->get_param_nr(pname));
        }
        else
            vname = name;
        return priv_->mgr.find_variable(vname);
    }
    CATCH_EXECUTE_ERROR
    return NULL; // avoid compiler warning
}

double Fityk::get_view_boundary(char side)
{
    switch (side) {
        case 'L': return priv_->view.left();
        case 'R': return priv_->view.right();
        case 'T': return priv_->view.top();
        case 'B': return priv_->view.bottom();
        default: return 0.;
    }
}

void Fityk::load_data(int dataset,
                      vector<realt> const& x,
                      vector<realt> const& y,
                      vector<realt> const& sigma,
                      string const& title)     throw(ExecuteError)
{
    try {
        priv_->dk.data(dataset)->load_arrays(x, y, sigma, title);
    }
    CATCH_EXECUTE_ERROR
}

void Fityk::add_point(realt x, realt y, realt sigma, int dataset)
                                                          throw(ExecuteError)
{
    try {
        priv_->dk.data(hd(priv_, dataset))->add_one_point(x, y, sigma);
    }
    CATCH_EXECUTE_ERROR
}

vector<Point> const& Fityk::get_data(int dataset)  throw(ExecuteError)
{
    static const vector<Point> empty;
    try {
        return priv_->dk.data(hd(priv_, dataset))->points();
    }
    CATCH_EXECUTE_ERROR
    return empty;
}


void Fityk::redir_messages(FILE *stream)
{
    if (stream) {
        UiApi::t_show_message_callback* old
          = priv_->ui()->connect_show_message(write_message_to_file);
        if (old != write_message_to_file)
            p_->old_message_callback = old;
    }
    else {
        // note: if redir_messages() is used for the first time,
        // p_->old_message_callback is NULL and the output is just disabled
        p_->old_message_callback =
            priv_->ui()->connect_show_message(p_->old_message_callback);
    }
    message_sink_ = stream;
}

void Fityk::set_option_as_string(const string& opt, const string& val)
                                                            throw(ExecuteError)
{
    priv_->mutable_settings_mgr()->set_as_string(opt, val);
}

void Fityk::set_option_as_number(const string& opt, double val)
                                                            throw(ExecuteError)
{
    priv_->mutable_settings_mgr()->set_as_number(opt, val);
}

string Fityk::get_option_as_string(const string& opt) const  throw(ExecuteError)
{
    return priv_->settings_mgr()->get_as_string(opt, /*quote_str=*/false);
}

double Fityk::get_option_as_number(const string& opt) const  throw(ExecuteError)
{
    return priv_->settings_mgr()->get_as_number(opt);
}

void Fityk::out(string const& s) const
{
    priv_->ui()->output_message(UserInterface::kNormal, s);
}

string Fityk::input(string const& prompt)
{
    return priv_->ui()->get_input_from_user(prompt);
}

realt Fityk::get_wssr(int dataset)  throw(ExecuteError)
{
    try {
        return get_wssr_or_ssr(priv_, dataset, true);
    }
    CATCH_EXECUTE_ERROR
    return 0.;
}

realt Fityk::get_ssr(int dataset)  throw(ExecuteError)
{
    try {
        return get_wssr_or_ssr(priv_, dataset, false);
    }
    CATCH_EXECUTE_ERROR
    return 0.;
}

realt Fityk::get_rsquared(int dataset)  throw(ExecuteError)
{
    try {
        if (dataset == ALL_DATASETS) {
            realt result = 0;
            for (int i = 0; i < priv_->dk.count(); ++i)
                result += Fit::compute_r_squared_for_data(priv_->dk.data(i),
                                                          NULL, NULL);
            return result;
        }
        else {
            return Fit::compute_r_squared_for_data(priv_->dk.data(dataset),
                                                   NULL, NULL);
        }
    }
    CATCH_EXECUTE_ERROR
    return 0.;
}

int Fityk::get_dof(int dataset)  throw(ExecuteError)
{
    try {
        return priv_->get_fit()->get_dof(get_datasets_(priv_, dataset));
    }
    CATCH_EXECUTE_ERROR
    return 0;
}

vector<vector<realt> > Fityk::get_covariance_matrix(int dataset)
                                                           throw(ExecuteError)
{
    try {
        vector<Data*> dss = get_datasets_(priv_, dataset);
        vector<double> c = priv_->get_fit()->get_covariance_matrix(dss);
        //reshape
        size_t na = priv_->mgr.parameters().size();
        assert(c.size() == na * na);
        vector<vector<realt> > r(na);
        for (size_t i = 0; i != na; ++i)
            r[i] = vector<realt>(c.begin() + i*na, c.begin() + i*(na+1));
        return r;
    }
    CATCH_EXECUTE_ERROR
    return vector<vector<realt> >();
}

realt* Fityk::get_covariance_matrix_as_array(int dataset)
{
    try {
        vector<Data*> dss = get_datasets_(priv_, dataset);
        vector<double> c = priv_->get_fit()->get_covariance_matrix(dss);
        realt* array = (realt*) malloc(c.size() * sizeof(realt));
        if (array != NULL)
            for (size_t i = 0; i != c.size(); ++i)
                array[i] = c[i];
        return array;
    }
    CATCH_EXECUTE_ERROR
    return NULL;
}

UiApi* Fityk::get_ui_api()
{
    return priv_->ui();
}

void Fityk::process_cmd_line_arg(const string& arg)
{
    priv_->process_cmd_line_arg(arg);
}

} //namespace fityk


// C API, not recommended for use in C++ and other languages
using fityk::Fityk;

extern "C" {

Fityk* fityk_create()
{
    Fityk *f = new Fityk;
    f->set_throws(false);
    return f;
}

void fityk_delete(Fityk *f)
{
    delete f;
}

bool fityk_execute(Fityk *f, const char* command)
{
    try {
        f->execute(command);
    }
    catch(ExitRequestedException) {
        return false;
    }
    return true;
}

void fityk_load_data(Fityk *f, int dataset,
                     double *x, double *y, double *sigma, int num,
                     const char* title)
{
    f->load_data(dataset, vector<realt>(x, x+num), vector<realt>(y, y+num),
                 vector<realt>(sigma, sigma+num), title);
}

const char* fityk_last_error(const Fityk *f)
{
    if (f->last_error().empty())
        return NULL;
    return f->last_error().c_str();
}

void fityk_clear_last_error(Fityk *f)
{
    f->clear_last_error();
}

char* fityk_get_info(Fityk *f, const char *s, int dataset)
{
    const string info = f->get_info(s, dataset);
    char* ret = (char*) malloc(info.size() + 1);
    strcpy(ret, info.c_str());
    return ret;
}

realt fityk_calculate_expr(Fityk *f, const char* s, int dataset)
{
    return f->calculate_expr(s, dataset);
}

int fityk_get_dataset_count(const Fityk *f)
{
    return f->get_dataset_count();
}

int fityk_get_parameter_count(const Fityk* f)
{
    return f->get_parameter_count();
}

const Point* fityk_get_data_point(Fityk *f, int dataset, int index)
{
    const vector<Point>& data = f->get_data(dataset);
    if (index >= 0 && (size_t) index < data.size())
        return &data[index];
    else
        return NULL;
}

realt fityk_get_model_value(Fityk *f, realt x, int dataset)
{
    return f->get_model_value(x, dataset);
}

realt fityk_get_wssr(Fityk *f, int dataset) { return f->get_wssr(dataset); }
realt fityk_get_ssr(Fityk *f, int dataset) { return f->get_ssr(dataset); }
realt fityk_get_rsquared(Fityk *f, int dataset)
                                        { return f->get_rsquared(dataset); }
int fityk_get_dof(Fityk *f, int dataset) { return f->get_dof(dataset); }

realt* fityk_get_covariance_matrix(Fityk *f, int dataset)
{
    return f->get_covariance_matrix_as_array(dataset);
}

} // extern "C"

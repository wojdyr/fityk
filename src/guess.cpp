// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License version 2
// $Id$

#include <algorithm>
#include <ctype.h>

#include "common.h"
#include "guess.h"
#include "data.h"
#include "sum.h"
#include "logic.h"
#include "ui.h"
#include "func.h"
#include "settings.h"
#include "datatrans.h"

using namespace std;

namespace {

fp my_y (DataWithSum const* ds, int n, EstConditions const* ec=0);
fp data_area (DataWithSum const* ds, int from, int to, 
              EstConditions const* ec=0);
int max_data_y_pos (DataWithSum const* ds, int from, int to, 
                    EstConditions const* ec=0);
fp compute_data_fwhm (DataWithSum const* ds, 
                      int from, int max_pos, int to, fp level,
                      EstConditions const* ec=0);
void parse_range(DataWithSum const* ds, std::vector<std::string> const& range, 
                 fp& range_from, fp& range_to);


fp my_y(DataWithSum const* ds, int n, EstConditions const* ec) 
{
    fp x = ds->get_data()->get_x(n);
    fp y = ds->get_data()->get_y(n);

    if (!ec)
        return y - ds->get_sum()->value(x);

    for (vector<int>::const_iterator i = ec->real_peaks.begin();
                                             i != ec->real_peaks.end(); i++)
        y -= AL->get_functions()[*i]->calculate_value(x); 
    return y;
}

fp data_area(DataWithSum const* ds, int from, int to, 
             EstConditions const* ec) 
{
    fp area = 0;
    fp x_prev = ds->get_data()->get_x(from);
    fp y_prev = my_y(ds, from, ec);
    for (int i = from + 1; i <= to; i++) {
        fp x =  ds->get_data()->get_x(i);
        fp y =  my_y(ds, i, ec);
        area += (x - x_prev) * (y_prev + y) / 2;
        x_prev = x;
        y_prev = y;
    }
    return area;
}

int max_data_y_pos(DataWithSum const* ds, int from, int to, 
                   EstConditions const* ec) 
{
    assert (from < to);
    int pos = from;
    fp maxy = my_y(ds, from, ec);
    for (int i = from + 1; i < to; i++) {
        fp y = my_y(ds, i, ec);
        if (y > maxy) {
            maxy = y;
            pos = i;
        }
    }
    return pos;
}

fp compute_data_fwhm(DataWithSum const* ds, 
                     int from, int max_pos, int to, fp level,
                     EstConditions const* ec) 
{
    assert (from <= max_pos && max_pos <= to);
    const fp hm = my_y(ds, max_pos, ec) * level;
    const int limit = 3; 
    int l = from, r = to, counter = 0;
    for (int i = max_pos; i >= from; i--) { //going down (and left) from maximum
        fp y = my_y(ds, i, ec);
        if (y > hm) {
            if (counter > 0) //previous point had y < hm
                counter--;  // compensating it; perhaps it was only fluctuation
        }
        else {
            counter++;     //this point is below half (if level==0.5) width
            if (counter >= limit) { // but i want `limit' points below to be
                l = min (i + counter, max_pos);// sure that it's not fuctuation
                break;
            }
        }
    }
    for (int i = max_pos; i <= to; i++) { //the same for right half of peak
        fp y = my_y(ds, i, ec);
        if (y > hm) {
            if (counter > 0)
                counter--;
        }
        else {
            counter++;
            if (counter >= limit) {
                r = max (i - counter, max_pos);
                break;
            }
        }
    }
    fp fwhm = ds->get_data()->get_x(r) - ds->get_data()->get_x(l);
    return max (fwhm, epsilon);
}

void parse_range(DataWithSum const* ds, vector<string> const& range,
                 fp& range_from, fp& range_to)
{
    assert (range.size() == 2);
    string le = range[0];
    string ri = range[1];
    if (le.empty())
        range_from = ds->get_data()->get_x_min();
    else if (le == ".") 
        range_from = AL->view.left;
    else
        range_from = strtod(le.c_str(), 0);
    if (ri.empty())
        range_to = ds->get_data()->get_x_max();
    else if (ri == ".") 
        range_to = AL->view.right;
    else
        range_to = strtod(ri.c_str(), 0);
}

void estimate_any_parameters(DataWithSum const* ds, fp range_from, fp range_to,
                             int &l_bor, int &r_bor)
{
    AL->use_parameters();
    if (ds->get_data()->get_n() <= 0) 
        throw ExecuteError("No active data.");

    l_bor = max (ds->get_data()->get_lower_bound_ac (range_from), 0);
    r_bor = min (ds->get_data()->get_upper_bound_ac (range_to), 
                     ds->get_data()->get_n() - 1);
    if (l_bor >= r_bor)
        throw ExecuteError("Searching peak outside of data points range. "
                           "Abandoned. Tried at [" + S(range_from) + " : " 
                           + S(range_to) + "]");
}

} // anonymous namespace


void estimate_peak_parameters(DataWithSum const* ds, fp range_from, fp range_to,
                              fp *center, fp *height, fp *area, fp *fwhm,
                              EstConditions const* ec) 
{
    int l_bor, r_bor;
    estimate_any_parameters(ds, range_from, range_to, l_bor, r_bor);
    int max_y_pos = max_data_y_pos(ds, l_bor, r_bor, ec);
    if (max_y_pos == l_bor || max_y_pos == r_bor - 1) {
        string s = "Estimating peak parameters: peak outside of search scope."
                  " Tried at [" + S(range_from) + " : " + S(range_to) + "]";
        if (getSettings()->get_b("can-cancel-guess")) 
            throw ExecuteError(s + " Canceled.");
        msg (s);
    }
    fp h = my_y(ds, max_y_pos, ec);
    if (height) 
        *height = h * getSettings()->get_f("height-correction");
    fp center_ = ds->get_data()->get_x(max_y_pos);
    if (center)
        *center = center_;
    fp fwhm_ = compute_data_fwhm(ds, l_bor, max_y_pos, r_bor, 0.5, ec) 
                                    * getSettings()->get_f("width-correction");
    if (fwhm)
        *fwhm = fwhm_;
    estimate_any_parameters(ds, center_-fwhm_, center_+fwhm_, l_bor, r_bor);
    if (area) 
        *area = data_area(ds, l_bor, r_bor, ec); 
}

void estimate_linear_parameters(DataWithSum const* ds, 
                                fp range_from, fp range_to,
                                fp *slope, fp *intercept, fp *avgy,
                                EstConditions const* ec) 
{
    int l_bor, r_bor;
    estimate_any_parameters(ds, range_from, range_to, l_bor, r_bor);

    fp sx = 0, sy = 0, sxx = 0, syy = 0, sxy = 0;
    for (int i = l_bor; i < r_bor; i++) {
        fp x = ds->get_data()->get_x(i);
        fp y = my_y(ds, i, ec);
        sx += x;
        sy += y;
        sxx += x*x;
        syy += y*y;
        sxy += x*y;
    }
    int n = r_bor - l_bor;
    *slope = (n * sxy - sx * sy) / (n * sxx - sx * sx);
    *intercept = (sy - (*slope) * sx) / n;
    *avgy = sy / n;
}


string get_guess_info(DataWithSum const* ds, vector<string> const& range) 
{
    string s;
    fp range_from, range_to;
    parse_range(ds, range, range_from, range_to);
    EstConditions estc;
    estc.real_peaks = ds->get_sum()->get_ff_idx();
    
    fp c = 0., h = 0., a = 0., fwhm = 0.;
    estimate_peak_parameters(ds, range_from, range_to, 
                             &c, &h, &a, &fwhm, &estc);
    if (h != 0.) 
        s += "center: " + S(c) + ", height: " + S(h) + ", area: " + S(a) 
            + ", FWHM: " + S(fwhm) + "\n";
    fp slope = 0, intercept = 0, avgy = 0;
    estimate_linear_parameters(ds, range_from, range_to, 
                               &slope, &intercept, &avgy, &estc);
    s += "slope: " + S(slope) + ", intercept: " + S(intercept) 
        + ", avg-y: " + S(avgy);
    return s;
}

namespace {

FunctionKind get_defvalue_kind(std::string const& d)
{
    static vector<string> linear_p(3), peak_p(4); 
    static bool initialized = false;
    if (!initialized) {
        linear_p[0] = "intercept";
        linear_p[1] = "slope";
        linear_p[2] = "avgy";
        peak_p[0] = "center";
        peak_p[1] = "height";
        peak_p[2] = "area";
        peak_p[3] = "fwhm";
        initialized = true;
    }
    if (contains_element(linear_p, d))
        return fk_linear;
    else if (contains_element(peak_p, d))
        return fk_peak;
    else 
        return fk_unknown;
}

FunctionKind get_function_kind_from_varnames(vector<string> const& vars)
{
    for (vector<string>::const_iterator i = vars.begin(); i != vars.end(); ++i){
        FunctionKind k = get_defvalue_kind(*i);
        if (k != fk_unknown)
            return k;
    }
    return fk_unknown;
}

FunctionKind get_function_kind_from_defvalues(vector<string> const& defv)
{
    for (vector<string>::const_iterator i = defv.begin(); i != defv.end(); ++i){
        int start = -1;
        for (size_t j = 0; j < i->size(); ++j) {
            char c = (*i)[j];
            if (start == -1) {
                if (isalpha(c))
                    start = j;
            }
            else {
                if (!isalnum(c) && c != '_') {
                    FunctionKind k 
                        = get_defvalue_kind(string(*i, start, j-start));
                    if (k != fk_unknown)
                        return k;
                    start = -1;
                }
            }
        }
        if (start != -1) {
            FunctionKind k = get_defvalue_kind(string(*i, start));
            if (k != fk_unknown)
                return k;
        }
    }
    return fk_unknown;
}

} // anonymous namespace

FunctionKind get_function_kind(string const& formula)
{
    vector<string> vars = Function::get_varnames_from_formula(formula);
    FunctionKind k = get_function_kind_from_varnames(vars);
    if (k != fk_unknown)
        return k;
    vector<string> defv = Function::get_defvalues_from_formula(formula);
    return get_function_kind_from_defvalues(defv);
}

void guess_and_add(DataWithSum* ds, 
                   string const& name, string const& function,
                   vector<string> const& range, vector<string> vars)
{
    EstConditions estc;
    Sum const* sum = ds->get_sum();
    // prepare a list of considered peaks
    estc.real_peaks = sum->get_ff_idx();
    if (!name.empty()) {
        assert(name[0] == '%');
        vector<string> const& names = sum->get_ff_names();
        vector<string>::const_iterator name_idx = find(names.begin(), 
                                                  names.end(), string(name,1));
        if (name_idx != names.end()) {
            int pos = name_idx - names.begin();
            estc.real_peaks.erase(estc.real_peaks.begin() + pos);
        }
    }
    // variables given explicitely by user (usually none)
    vector<string> vars_lhs(vars.size());
    for (int i = 0; i < size(vars); ++i)
        vars_lhs[i] = string(vars[i], 0, vars[i].find('='));

    // get range
    fp range_from, range_to;

    // handle a special case with implicit range:
    //  %peak = guess Gaussian center=$peak_center
    if (range[0].empty() && range[1].empty() 
            && contains_element(vars_lhs, "center")) {
        int ci = find(vars_lhs.begin(), vars_lhs.end(), "center") 
            - vars_lhs.begin();
        string ctr_str = string(vars[ci], vars[ci].find('=') + 1); 
        replace_all(ctr_str, "~", "");
        fp center = get_transform_expression_value(ctr_str, 0);
        fp delta = getSettings()->get_f("guess-at-center-pm");
        range_from = center - delta;
        range_to = center + delta;
    }
    else
        parse_range(ds, range, range_from, range_to);

    FunctionKind k = get_function_kind(Function::get_formula(function));
    if (k == fk_peak) {
        fp c = 0., h = 0., a = 0., fwhm = 0.;
        estimate_peak_parameters(ds, range_from, range_to, 
                                 &c, &h, &a, &fwhm, &estc);
        if (!contains_element(vars_lhs, "center"))
            vars.push_back("center=~"+S(c));
        if (!contains_element(vars_lhs, "height"))
            vars.push_back("height=~"+S(h));
        if (!contains_element(vars_lhs, "fwhm") 
                && !contains_element(vars_lhs, "hwhm"))
            vars.push_back("fwhm=~"+S(fwhm));
        if (!contains_element(vars_lhs, "area"))
            vars.push_back("area=~"+S(a));
    }
    else if (k == fk_linear) {
        fp slope, intercept, avgy;
        estimate_linear_parameters(ds, range_from, range_to, 
                                   &slope, &intercept, &avgy, &estc);
        if (!contains_element(vars_lhs, "slope"))
            vars.push_back("slope=~"+S(slope));
        if (!contains_element(vars_lhs, "intercept"))
            vars.push_back("intercept=~"+S(intercept));
        if (!contains_element(vars_lhs, "avgy"))
            vars.push_back("avgy=~"+S(avgy));
    }

    string real_name = AL->assign_func(name, function, vars);
    ds->get_sum()->add_function_to(real_name, 'F');
}

bool is_parameter_guessable(string const& name, FunctionKind k)
{
    if (k == fk_linear)
        return name == "slope" || name == "intercept" || name == "avgy";
    else if (k == fk_peak)
        return name == "center" || name == "height" || name == "fwhm"
            || name == "area" || name == "hwhm";
    else 
        return false;
}

bool is_defvalue_guessable(string defvalue, FunctionKind k)
{
    if (k == fk_linear) {
        replace_words(defvalue, "slope", "1");
        replace_words(defvalue, "intercept", "1");
        replace_words(defvalue, "avgy", "1");
    }
    else if (k == fk_peak) {
        replace_words(defvalue, "center", "1");
        replace_words(defvalue, "height", "1");
        replace_words(defvalue, "fwhm", "1");
        replace_words(defvalue, "area", "1");
    }
    try {
        get_transform_expression_value(defvalue, 0);
    } 
    catch (ExecuteError &e) {
        return false;
    } 
    return true;
}

bool is_function_guessable(string const& formula, bool check_defvalue)
{
    int lb = formula.find('(');
    int rb = find_matching_bracket(formula, lb);
    string all_names(formula, lb+1, rb-lb-1);
    vector<string> nd = split_string(all_names, ',');

    FunctionKind k = get_function_kind(formula);
    vector<string> vars, defv;
    for (vector<string>::const_iterator i = nd.begin(); i != nd.end(); ++i) {
        string::size_type eq = i->find('=');
        if (eq == string::npos) { //no defvalue
            if (!is_parameter_guessable(strip_string(*i), k))
                return false;
        }
        else if (check_defvalue 
                 && !is_parameter_guessable(strip_string(string(*i, 0, eq)), k)
                 && !is_defvalue_guessable(string(*i, eq+1), k)) {
                return false;
        }
    }
    return true;
}

bool is_function_guessable(vector<string> const& vars, 
                           vector<string> const& defv,
                           FunctionKind* fk)
{
    FunctionKind k = get_function_kind_from_varnames(vars);
    if (k == fk_unknown)
        k = get_function_kind_from_defvalues(defv);
    for (size_t i = 0; i != vars.size(); ++i) 
        if (!is_parameter_guessable(vars[i], k)
                             && !is_defvalue_guessable(defv[i], k)) 
                return false;
    if (fk)
        *fk = k;
    return true;
}




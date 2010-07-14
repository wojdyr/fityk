// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
// $Id$

#include <algorithm>
#include <ctype.h>

#include "common.h"
#include "guess.h"
#include "data.h"
#include "model.h"
#include "logic.h"
#include "ui.h"
#include "func.h"
#include "settings.h"
#include "datatrans.h"

using namespace std;

Guess::Guess(Ftk const *F_, DataAndModel const* dm)
    : F(F_), data(dm->data())
{
    real_peaks = dm->model()->get_ff_idx();
}

fp Guess::my_y(int n)
{
    fp x = data->get_x(n);
    fp y = data->get_y(n);
    for (vector<int>::const_iterator i = real_peaks.begin();
                                             i != real_peaks.end(); i++)
	y -= F->get_function(*i)->calculate_value(x);
    return y;
}

fp Guess::data_area(int from, int to)
{
    fp area = 0;
    fp x_prev = data->get_x(from);
    fp y_prev = my_y(from);
    for (int i = from + 1; i <= to; i++) {
        fp x =  data->get_x(i);
        fp y =  my_y(i);
        area += (x - x_prev) * (y_prev + y) / 2;
        x_prev = x;
        y_prev = y;
    }
    return area;
}

int Guess::max_data_y_pos(int from, int to)
{
    assert (from < to);
    int pos = from;
    fp maxy = my_y(from);
    for (int i = from + 1; i < to; i++) {
        fp y = my_y(i);
        if (y > maxy) {
            maxy = y;
            pos = i;
        }
    }
    return pos;
}

fp Guess::compute_data_fwhm(int from, int max_pos, int to)
{
    assert (from <= max_pos && max_pos <= to);
    const fp hm = 0.5 * my_y(max_pos);
    const int limit = 3;
    int l = from, r = to;

    // first we search the width of the left side of the peak
    int counter = 0;
    for (int i = max_pos; i >= from; --i) {
        fp y = my_y(i);
        if (y > hm) {
            if (counter > 0) // previous point had y < hm
                counter--;   // compensate it, it was only fluctuation
        }
        else {
            counter++;
            // we found a point below `hm', but we need to find `limit' points
            // below `hm' to be sure that it's not a fluctuation
            if (counter == limit) {
                l = i + counter;
                break;
            }
        }
    }

    // do the same for the right side
    counter = 0;
    for (int i = max_pos; i <= to; i++) {
        fp y = my_y(i);
        if (y > hm) {
            if (counter > 0)
                counter--;
        }
        else {
            counter++;
            if (counter == limit) {
                // +1 here is intentionally not symmetric with the left side
                r = i - counter + 1;
                break;
            }
        }
    }

    fp fwhm = data->get_x(r) - data->get_x(l);
    return max (fwhm, epsilon);
}

void Guess::parse_range(string const& left, string const& right,
                        fp& range_from, fp& range_to)
{
    if (left.empty())
        range_from = data->get_x_min();
    else if (left == ".")
        range_from = F->view.left;
    else
        range_from = strtod(left.c_str(), 0);
    if (right.empty())
        range_to = data->get_x_max();
    else if (right == ".")
        range_to = F->view.right;
    else
        range_to = strtod(right.c_str(), 0);
}

void Guess::get_point_range(fp range_from, fp range_to,
                            int &l_bor, int &r_bor)
{
///    F->use_parameters();
    if (data->get_n() <= 0)
        throw ExecuteError("No active data.");

    l_bor = max (data->get_lower_bound_ac (range_from), 0);
    r_bor = min (data->get_upper_bound_ac (range_to),
                 data->get_n() - 1);
    if (l_bor >= r_bor)
        throw ExecuteError("Searching peak outside of data points range. "
                           "Abandoned. Tried at [" + eS(range_from) + " : "
                           + eS(range_to) + "]");
}


void Guess::estimate_peak_parameters(fp range_from, fp range_to,
                                     fp *center, fp *height, fp *area, fp *fwhm)
{
    int l_bor, r_bor;
    get_point_range(range_from, range_to, l_bor, r_bor);
    int max_y_pos = max_data_y_pos(l_bor, r_bor);
    if (max_y_pos == l_bor || max_y_pos == r_bor - 1) {
        string s = "Estimating peak parameters: peak outside of search scope."
                  " Tried at [" + eS(range_from) + " : " + eS(range_to) + "]";
        if (F->get_settings()->get_b("can_cancel_guess"))
            throw ExecuteError(s + " Canceled.");
        F->msg(s);
    }
    fp h = my_y(max_y_pos);
    if (height)
        *height = h * F->get_settings()->get_f("height_correction");
    fp center_ = data->get_x(max_y_pos);
    if (center)
        *center = center_;
    fp fwhm_ = compute_data_fwhm(l_bor, max_y_pos, r_bor)
                               * F->get_settings()->get_f("width_correction");
    if (fwhm)
        *fwhm = fwhm_;
    get_point_range(center_-fwhm_, center_+fwhm_, l_bor, r_bor);
    if (area)
        *area = data_area(l_bor, r_bor);
}

void Guess::estimate_linear_parameters(fp range_from, fp range_to,
                                       fp *slope, fp *intercept, fp *avgy)
{
    int l_bor, r_bor;
    get_point_range(range_from, range_to, l_bor, r_bor);

    fp sx = 0, sy = 0, sxx = 0, syy = 0, sxy = 0;
    for (int i = l_bor; i <= r_bor; i++) {
        fp x = data->get_x(i);
        fp y = my_y(i);
        sx += x;
        sy += y;
        sxx += x*x;
        syy += y*y;
        sxy += x*y;
    }
    int n = r_bor - l_bor + 1;
    *slope = (n * sxy - sx * sy) / (n * sxx - sx * sx);
    *intercept = (sy - (*slope) * sx) / n;
    *avgy = sy / n;
}


void Guess::remove_peak(string const& name)
{
    if (name.empty())
	return;
    assert(name[0] == '%');
    for (vector<int>::iterator i = real_peaks.begin();
                                             i != real_peaks.end(); i++)
        if (F->get_function(*i)->xname == name) {
            real_peaks.erase(i);
            return;
        }
}

void Guess::get_guess_info(RealRange const& range, string& result)
{
    // TODO: pass RealRange to estimate_*_parameters()
    fp range_from, range_to;
    if (range.from == RealRange::kInf)
        range_from = data->get_x_min();
    else if (range.from == RealRange::kNone)
        range_from = F->view.left;
    else
        range_from = range.from_val;

    if (range.to == RealRange::kInf)
        range_to = data->get_x_max();
    else if (range.to == RealRange::kNone)
        range_to = F->view.right;
    else
        range_to = range.to_val;

    fp c = 0., h = 0., a = 0., fwhm = 0.;
    estimate_peak_parameters(range_from, range_to,
                             &c, &h, &a, &fwhm);
    if (h != 0.)
        result += "center: " + eS(c) + ", height: " + S(h) + ", area: " + S(a)
            + ", FWHM: " + S(fwhm) + "\n";
    fp slope = 0, intercept = 0, avgy = 0;
    estimate_linear_parameters(range_from, range_to,
                               &slope, &intercept, &avgy);
    result += "slope: " + S(slope) + ", intercept: " + S(intercept)
        + ", avg-y: " + S(avgy);
}

/// guessed parameters are appended to vars
void Guess::guess(string const& name, string const& function,
                  string const& from_str, string const& to_str,
                  vector<string>& vars)
{
    remove_peak(name);
    // variables given explicitely by user (usually none)
    vector<string> vars_lhs(vars.size());
    for (int i = 0; i < size(vars); ++i)
        vars_lhs[i] = string(vars[i], 0, vars[i].find('='));

    // get range
    fp range_from, range_to;

    // handle a special case with implicit range:
    //  %peak = guess Gaussian center=$peak_center
    if (from_str.empty() && to_str.empty()
            && contains_element(vars_lhs, "center")) {
        int ci = find(vars_lhs.begin(), vars_lhs.end(), "center")
            - vars_lhs.begin();
        string ctr_str = string(vars[ci], vars[ci].find('=') + 1);
        replace_all(ctr_str, "~", "");
        fp center = get_transform_expression_value(ctr_str, 0);
        fp delta = F->get_settings()->get_f("guess_at_center_pm");
        range_from = center - delta;
        range_to = center + delta;
    }
    else
        parse_range(from_str, to_str, range_from, range_to);

    FunctionKind k = get_function_kind(Function::get_formula(function));
    if (k == fk_peak) {
        fp c = 0., h = 0., a = 0., fwhm = 0.;
        estimate_peak_parameters(range_from, range_to,
                                 &c, &h, &a, &fwhm);
        if (!contains_element(vars_lhs, "center"))
            vars.push_back("center=~"+eS(c));
        if (!contains_element(vars_lhs, "height"))
            vars.push_back("height=~"+eS(h));
        if (!contains_element(vars_lhs, "fwhm")
                && !contains_element(vars_lhs, "hwhm"))
            vars.push_back("fwhm=~"+eS(fwhm));
        if (!contains_element(vars_lhs, "area"))
            vars.push_back("area=~"+eS(a));
    }
    else if (k == fk_linear) {
        fp slope, intercept, avgy;
        estimate_linear_parameters(range_from, range_to,
                                   &slope, &intercept, &avgy);
        if (!contains_element(vars_lhs, "slope"))
            vars.push_back("slope=~"+eS(slope));
        if (!contains_element(vars_lhs, "intercept"))
            vars.push_back("intercept=~"+eS(intercept));
        if (!contains_element(vars_lhs, "avgy"))
            vars.push_back("avgy=~"+eS(avgy));
    }
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
    catch (ExecuteError& /*e*/) {
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




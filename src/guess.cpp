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

Guess::Guess(Settings const *settings) : settings_(settings)
{
}

void Guess::initialize(const DataAndModel* dm, int lb, int rb, int ignore_idx)
{
    xx_.resize(rb - lb);
    for (int j = lb; j != rb; ++j)
        xx_[j] = dm->data()->get_x(j);
    yy_.clear(); // just in case
    yy_.resize(rb - lb, 0.);
    dm->model()->compute_model(xx_, yy_, ignore_idx);
    for (int j = lb; j != rb; ++j)
        yy_[j] = dm->data()->get_y(j) - yy_[j];
}


fp Guess::find_fwhm(int pos, fp* area)
{
    const fp hm = 0.5 * yy_[pos];
    const int n = 3;
    int left_pos = 0;
    int right_pos = yy_.size() - 1;

    // first we search the width of the left side of the peak
    int counter = 0;
    for (int i = pos; i > 0; --i) {
        if (yy_[i] > hm) {
            if (counter > 0) // previous point had y < hm
                --counter;   // compensate it, it was only fluctuation
        }
        else {
            ++counter;
            // we found a point below `hm', but we need to find `n' points
            // below `hm' to be sure that it's not a fluctuation
            if (counter == n) {
                left_pos = i + counter;
                break;
            }
        }
    }

    // do the same for the right side
    counter = 0;
    for (int i = pos; i < right_pos; i++) {
        if (yy_[i] > hm) {
            if (counter > 0)
                counter--;
        }
        else {
            counter++;
            if (counter == n) {
                // +1 here is intentionally asymmetric with the left side
                right_pos = i - counter + 1;
                break;
            }
        }
    }

    if (area) {
        *area = 0;
        for (int i = left_pos; i < right_pos; ++i)
            *area += (xx_[i+1] - xx_[i]) * (yy_[i] + yy_[i+1]) / 2;
    }

    fp fwhm = xx_[right_pos] - xx_[left_pos];
    return max(fwhm, epsilon);
}

void Guess::estimate_peak_parameters(fp *center, fp *height, fp *area, fp *fwhm)
{
    int pos = max_element(yy_.begin(), yy_.end()) - yy_.begin();

    if (pos == 0 || pos == (int) yy_.size() - 1) {
        if (settings_->get_b("can_cancel_guess"))
            throw ExecuteError("Peak outside of the range.");
    }
    if (height)
        *height = yy_[pos] * settings_->get_f("height_correction");
    if (center)
        *center = xx_[pos];
    if (fwhm || area) {
        fp f = find_fwhm(pos, area) * settings_->get_f("width_correction");
        if (fwhm)
            *fwhm = f;
    }
}

void Guess::estimate_linear_parameters(fp *slope, fp *intercept, fp *avgy)
{
    fp sx = 0, sy = 0, sxx = 0, syy = 0, sxy = 0;
    int n = yy_.size();
    for (int i = 0; i != n; ++i) {
        fp x = xx_[i];
        fp y = yy_[i];
        sx += x;
        sy += y;
        sxx += x*x;
        syy += y*y;
        sxy += x*y;
    }
    *slope = (n * sxy - sx * sy) / (n * sxx - sx * sx);
    *intercept = (sy - (*slope) * sx) / n;
    *avgy = sy / n;
}


void Guess::get_guess_info(string& result)
{
    fp c = 0., h = 0., a = 0., fwhm = 0.;
    estimate_peak_parameters(&c, &h, &a, &fwhm);
    if (h != 0.)
        result += "center: " + eS(c) + ", height: " + S(h) + ", area: " + S(a)
            + ", FWHM: " + S(fwhm) + "\n";
    fp slope = 0, intercept = 0, avgy = 0;
    estimate_linear_parameters(&slope, &intercept, &avgy);
    result += "slope: " + S(slope) + ", intercept: " + S(intercept)
        + ", avg-y: " + S(avgy);
}

/// guessed parameters are appended to vars
void Guess::guess(string const& function, vector<string>& par_names,
                  vector<string>& par_values)
{
    if (xx_.empty())
        throw ExecuteError("Guessing in empty range");

    vector<string>::const_iterator ctr =
        find(par_names.begin(), par_names.end(), "center");

    Kind k = get_function_kind(Function::get_formula(function));
    if (k == kPeak) {
        fp c = 0., h = 0., a = 0., fwhm = 0.;
        estimate_peak_parameters(&c, &h, &a, &fwhm);
        if (ctr != par_names.end()) {
            par_names.push_back("center");
            par_values.push_back("~"+eS(c));
        }
        if (!contains_element(par_names, "height")) {
            par_names.push_back("height");
            par_values.push_back("~"+eS(h));
        }
        if (!contains_element(par_names, "fwhm")
                && !contains_element(par_names, "hwhm")) {
            par_names.push_back("fwhm");
            par_values.push_back("~"+eS(fwhm));
        }
        if (!contains_element(par_names, "area")) {
            par_names.push_back("area");
            par_values.push_back("~"+eS(a));
        }
    }
    else if (k == kLinear) {
        fp slope, intercept, avgy;
        estimate_linear_parameters(&slope, &intercept, &avgy);
        if (!contains_element(par_names, "slope")) {
            par_names.push_back("slope");
            par_values.push_back("~"+eS(slope));
        }
        if (!contains_element(par_names, "intercept")) {
            par_names.push_back("intercept");
            par_values.push_back("~"+eS(intercept));
        }
        if (!contains_element(par_names, "avgy")) {
            par_names.push_back("avgy");
            par_values.push_back("~"+eS(avgy));
        }
    }
}

namespace {

Guess::Kind get_defvalue_kind(std::string const& d)
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
        return Guess::kLinear;
    else if (contains_element(peak_p, d))
        return Guess::kPeak;
    else
        return Guess::kUnknown;
}

Guess::Kind get_function_kind_from_varnames(vector<string> const& vars)
{
    for (vector<string>::const_iterator i = vars.begin(); i != vars.end(); ++i){
        Guess::Kind k = get_defvalue_kind(*i);
        if (k != Guess::kUnknown)
            return k;
    }
    return Guess::kUnknown;
}

Guess::Kind get_function_kind_from_defvalues(vector<string> const& defv)
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
                    Guess::Kind k
                        = get_defvalue_kind(string(*i, start, j-start));
                    if (k != Guess::kUnknown)
                        return k;
                    start = -1;
                }
            }
        }
        if (start != -1) {
            Guess::Kind k = get_defvalue_kind(string(*i, start));
            if (k != Guess::kUnknown)
                return k;
        }
    }
    return Guess::kUnknown;
}

} // anonymous namespace

Guess::Kind get_function_kind(string const& formula)
{
    vector<string> vars = Function::get_varnames_from_formula(formula);
    Guess::Kind k = get_function_kind_from_varnames(vars);
    if (k != Guess::kUnknown)
        return k;
    vector<string> defv = Function::get_defvalues_from_formula(formula);
    return get_function_kind_from_defvalues(defv);
}

bool is_parameter_guessable(string const& name, Guess::Kind k)
{
    if (k == Guess::kLinear)
        return name == "slope" || name == "intercept" || name == "avgy";
    else if (k == Guess::kPeak)
        return name == "center" || name == "height" || name == "fwhm"
            || name == "area" || name == "hwhm";
    else
        return false;
}

bool is_defvalue_guessable(string defvalue, Guess::Kind k)
{
    if (k == Guess::kLinear) {
        replace_words(defvalue, "slope", "1");
        replace_words(defvalue, "intercept", "1");
        replace_words(defvalue, "avgy", "1");
    }
    else if (k == Guess::kPeak) {
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

    Guess::Kind k = get_function_kind(formula);
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
                           Guess::Kind* kind)
{
    Guess::Kind k = get_function_kind_from_varnames(vars);
    if (k == Guess::kUnknown)
        k = get_function_kind_from_defvalues(defv);
    for (size_t i = 0; i != vars.size(); ++i)
        if (!is_parameter_guessable(vars[i], k)
                             && !is_defvalue_guessable(defv[i], k))
                return false;
    if (kind)
        *kind = k;
    return true;
}




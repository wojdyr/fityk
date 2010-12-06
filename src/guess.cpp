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
#include "lexer.h"
#include "eparser.h"

using namespace std;
using boost::array;

const array<string, 3> Guess::linear_traits =
                                    {{ "slope", "intercept", "avgy" }};
const array<string, 4> Guess::peak_traits =
                                    {{ "center", "height", "hwhm", "area" }};

Guess::Guess(Settings const *settings) : settings_(settings)
{
}

void Guess::initialize(const DataAndModel* dm, int lb, int rb, int ignore_idx)
{
    int len = rb - lb;
    assert(len >= 0);
    xx_.resize(len);
    for (int j = 0; j != len; ++j)
        xx_[j] = dm->data()->get_x(lb+j);
    yy_.clear(); // just in case
    yy_.resize(len, 0.);
    dm->model()->compute_model(xx_, yy_, ignore_idx);
    for (int j = 0; j != len; ++j)
        yy_[j] = dm->data()->get_y(lb+j) - yy_[j];
}


fp Guess::find_hwhm(int pos, fp* area)
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

    fp hwhm = (xx_[right_pos] - xx_[left_pos]) / 2.;
    return max(hwhm, epsilon);
}

// outputs vector with: center, height, hwhm, area
void Guess::estimate_peak_parameters(fp *center, fp *height, fp *area, fp *hwhm)
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
    if (hwhm || area) {
        fp w = find_hwhm(pos, area) * settings_->get_f("width_correction");
        if (hwhm)
            *hwhm = w;
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
    if (xx_.empty()) {
        result += "empty range";
        return;
    }
    fp c = 0., h = 0., a = 0., hwhm = 0.;
    estimate_peak_parameters(&c, &h, &a, &hwhm);
    if (h != 0.)
        result += "center: " + eS(c) + ", height: " + S(h) + ", area: " + S(a)
            + ", FWHM: " + S(hwhm) + "\n";
    fp slope = 0, intercept = 0, avgy = 0;
    estimate_linear_parameters(&slope, &intercept, &avgy);
    result += "slope: " + S(slope) + ", intercept: " + S(intercept)
        + ", avg-y: " + S(avgy);
}

/// guessed parameters are appended to vars
void Guess::guess(const Tplate* tp,
                  vector<string>& par_names, vector<string>& par_values)
{
    if (xx_.empty())
        throw ExecuteError("guess in empty range");

    if (tp->peak_d) {
        fp c = 0., h = 0., a = 0., hwhm = 0.;
        estimate_peak_parameters(&c, &h, &a, &hwhm);
        if (!contains_element(par_names, "center")) {
            par_names.push_back("center");
            par_values.push_back("~"+eS(c));
        }
        if (!contains_element(par_names, "height")) {
            par_names.push_back("height");
            par_values.push_back("~"+eS(h));
        }
        if (!contains_element(par_names, "hwhm")) {
            par_names.push_back("hwhm");
            par_values.push_back("~"+eS(hwhm));
        }
        if (!contains_element(par_names, "area")) {
            par_names.push_back("area");
            par_values.push_back("~"+eS(a));
        }
    }
    if (tp->linear_d) {
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


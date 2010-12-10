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
    if (len == 0)
        throw ExecuteError("guess: empty range");
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
// returns values corresponding to peak_traits
array<double,4> Guess::estimate_peak_parameters()
{
    int pos = max_element(yy_.begin(), yy_.end()) - yy_.begin();

    if (settings_->can_cancel_guess && (pos == 0 || pos == (int)yy_.size() - 1))
        throw ExecuteError("Peak outside of the range.");

    double height = yy_[pos] * settings_->height_correction;
    double center = xx_[pos];
    double area;
    double hwhm = find_hwhm(pos, &area) * settings_->width_correction;
    array<double,4> r = {{ center, height, hwhm, area }};
    return r;
}

array<double,3> Guess::estimate_linear_parameters()
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
    double slope = (n * sxy - sx * sy) / (n * sxx - sx * sx);
    double intercept = (sy - slope * sx) / n;
    double avgy = sy / n;
    array<double,3> r = {{ slope, intercept, avgy }};
    return r;
}



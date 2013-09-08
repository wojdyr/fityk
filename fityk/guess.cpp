// This file is part of fityk program. Copyright 2001-2013 Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#define BUILDING_LIBFITYK
#include "guess.h"

#include <algorithm>
#include <ctype.h>

#include "common.h"
#include "data.h"
#include "model.h"
#include "logic.h"
#include "ui.h"
#include "func.h"
#include "settings.h"

using namespace std;

namespace fityk {

const vector<string> Guess::linear_traits
                    = vector3(S("slope"), S("intercept"), S("avgy"));
const vector<string> Guess::peak_traits
                    = vector4(S("center"), S("height"), S("hwhm"), S("area"));
const vector<string> Guess::sigmoid_traits
                    = vector4(S("lower"), S("upper"), S("xmid"), S("wsig"));

void Guess::set_data(const Data* data, const RealRange& range, int ignore_idx)
{
    pair<int,int> point_indexes = data->get_index_range(range);
    int len = point_indexes.second - point_indexes.first;
    assert(len >= 0);
    if (len == 0)
        throw ExecuteError("guess: empty range");
    xx_.resize(len);
    for (int j = 0; j != len; ++j)
        xx_[j] = data->get_x(point_indexes.first + j);
    if (settings_->guess_uses_weights) {
        sigma_.resize(len);
        for (int j = 0; j != len; ++j)
            sigma_[j] = data->get_sigma(point_indexes.first + j);
    }
    yy_.clear(); // just in case
    yy_.resize(len, 0.);
    data->model()->compute_model(xx_, yy_, ignore_idx);
    for (int j = 0; j != len; ++j)
        yy_[j] = data->get_y(point_indexes.first + j) - yy_[j];
}


double Guess::find_hwhm(int pos, double* area) const
{
    const double hm = 0.5 * yy_[pos];
    const int n = 3;
    int left_pos = 0;
    int right_pos = yy_.size() - 1;

    // first we search the width of the left side of the peak
    int counter = 0;
    for (int i = pos; i > 0; --i) {
        if (yy_[i] > hm) {
            if (counter > 0) // previous point had y < hm
                --counter;   // compensate it, it was only fluctuation
        } else {
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
        } else {
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

    double hwhm = (xx_[right_pos] - xx_[left_pos]) / 2.;
    return max(hwhm, epsilon);
}

// outputs vector with: center, height, hwhm, area
// returns values corresponding to peak_traits
vector<double> Guess::estimate_peak_parameters() const
{
    // find the highest point, which must be higher than the previous point
    // and not lower than the next one (-> it cannot be the first/last point)
    int pos = -1;
    if (!sigma_.empty()) {
        for (int i = 1; i < (int) yy_.size() - 1; ++i) {
            int t = (pos == -1 ? i-1 : pos);
            if (sigma_[t] * yy_[i] > sigma_[i] * yy_[t] &&
                    sigma_[i+1] * yy_[i] >= sigma_[i] * yy_[i+1])
                pos = i;
        }
    } else {
        for (int i = 1; i < (int) yy_.size() - 1; ++i) {
            int t = (pos == -1 ? i-1 : pos);
            if (yy_[i] > yy_[t] && yy_[i] >= yy_[i+1])
                pos = i;
        }
    }
    if (pos == -1)
        throw ExecuteError("Peak outside of the range.");

    double height = yy_[pos] * settings_->height_correction;
    double center = xx_[pos];
    double area;
    double hwhm = find_hwhm(pos, &area) * settings_->width_correction;
    return vector4(center, height, hwhm, area);
}

vector<double> Guess::estimate_linear_parameters() const
{
    double sx = 0, sy = 0, sxx = 0, /*syy = 0,*/ sxy = 0;
    int n = yy_.size();
    for (int i = 0; i != n; ++i) {
        double x = xx_[i];
        double y = yy_[i];
        sx += x;
        sy += y;
        sxx += x*x;
        //syy += y*y;
        sxy += x*y;
    }
    double slope = (n * sxy - sx * sy) / (n * sxx - sx * sx);
    double intercept = (sy - slope * sx) / n;
    double avgy = sy / n;
    return vector3(slope, intercept, avgy);
}

// ad-hoc arbitrary procedure to guess initial sigmoid parameters,
// with no theoretical justification
vector<double> Guess::estimate_sigmoid_parameters() const
{
    double lower;
    double upper;
    vector<realt> ycopy = yy_;

    // ignoring 20% lowest and 20% highest points
    sort(ycopy.begin(), ycopy.end());
    if (ycopy.size() < 10) {
        lower = ycopy.front();
        upper = ycopy.back();
    } else {
        lower = ycopy[ycopy.size()/5];
        upper = ycopy[ycopy.size()*4/5];
    }

    // fitting ax+b to linearized sigmoid
    double sx = 0, sy = 0, sxx = 0, sxy = 0;
    int n = 0;
    for (size_t i = 0; i != yy_.size(); ++i) {
        if (yy_[i] <= lower || yy_[i] >= upper)
            continue;
        double x = xx_[i];
        // normalizing: y ->  (y - lower) / (upper - lower);
        // and          y -> -log(1/y-1);
        double y =  -log((upper - lower) / (yy_[i] - lower) - 1.);
        sx += x;
        sy += y;
        sxx += x*x;
        sxy += x*y;
        ++n;
    }
    double a = (n * sxy - sx * sy) / (n * sxx - sx * sx);
    double b = (sy - a * sx) / n;

    //double xmid = (xx_.front() + xx_.back()) / 2.;
    double xmid = -b/a;
    //double wsig = (xx_.back() - xx_.front()) / 10.;
    double wsig = 1/a;
    return vector4(lower, upper, xmid, wsig);
}

} // namespace fityk

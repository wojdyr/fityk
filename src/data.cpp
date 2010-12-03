// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
// $Id$

#include "common.h"
#include "data.h"
//#include "ui.h"
#include "numfuncs.h"
#include "settings.h"
#include "logic.h"

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <fstream>
#include <sstream>
#include <algorithm>

#include <xylib/xylib.h>
#include <xylib/cache.h>

using namespace std;

// filename utils
#if defined(_WIN32) || defined(WIN32) || defined(__NT__) || defined(__WIN32__) || defined(__OS2__)
#define FILE_SEP_PATH '\\'
#elif defined(__MAC__) || defined(__APPLE__) || defined(macintosh)
#define FILE_SEP_PATH ':'
#else
#define FILE_SEP_PATH '/'
#endif

string get_file_basename(string const& path)
{
    string::size_type last_sep = path.rfind(FILE_SEP_PATH);
    string::size_type last_dot = path.rfind('.');
    size_t basename_begin = (last_sep == string::npos ? 0 : last_sep + 1);
    if (last_dot != string::npos && last_dot > basename_begin)
        return string(path, basename_begin, last_dot-basename_begin);
    else
        return string(path, basename_begin);
}


string Data::get_info() const
{
    string s;
    if (p_.empty())
        s = "No data points.";
    else
        s = S(p_.size()) + " points, " + S(active_p_.size()) + " active.";
    if (!filename_.empty())
        s += "\nFilename: " + filename_;
    if (given_x_ != INT_MAX || given_y_ != INT_MAX || given_s_ != INT_MAX)
        s += "\nColumns: " + (given_x_ != INT_MAX ? S(given_x_) : S("_"))
                    + ", " + (given_y_ != INT_MAX ? S(given_y_) : S("_"));
    if (given_s_ != INT_MAX)
        s += ", " + S(given_s_);
    if (!title.empty())
        s += "\nData title: " + title;
    if (active_p_.size() != p_.size())
        s += "\nActive data range: " + range_as_string();
    return s;
}

void Data::clear()
{
    filename_ = "";
    title = "";
    given_x_ = given_y_ = given_s_ = INT_MAX;
    given_options_.clear();
    given_blocks_.clear();
    p_.clear();
    x_step_ = 0;
    active_p_.clear();
    has_sigma_ = false;
}

void Data::post_load()
{
    if (p_.empty())
        return;
    string inf = S(p_.size()) + " points.";
    if (!has_sigma_) {
        int dds = F->get_settings()->get_e("data_default_sigma");
        if (dds == 's') {
            for (vector<Point>::iterator i = p_.begin(); i < p_.end(); i++)
                i->sigma = i->y > 1. ? sqrt (i->y) : 1.;
            inf += " No explicit std. dev. Set as sqrt(y)";
        }
        else if (dds == '1') {
            for (vector<Point>::iterator i = p_.begin(); i < p_.end(); i++)
                i->sigma = 1.;
            inf += " No explicit std. dev. Set as equal 1.";
        }
        else
            assert(0);
    }
    F->msg(inf);
    update_active_p();
    recompute_y_bounds();
}

void Data::recompute_y_bounds() {
    bool ini = false;
    for (vector<Point>::iterator i = p_.begin(); i != p_.end(); i++) {
        if (!is_finite(i->y))
            continue;
        if (!ini) {
            y_min_ = y_max_ = i->y;
            ini = true;
        }
        if (i->y < y_min_)
            y_min_ = i->y;
        if (i->y > y_max_)
            y_max_ = i->y;
    }
}

int Data::load_arrays(const vector<fp> &x, const vector<fp> &y,
                      const vector<fp> &sigma, const string &data_title)
{
    assert(x.size() == y.size());
    assert(sigma.empty() || sigma.size() == y.size());
    clear();
    title = data_title;
    p_.resize(y.size());
    if (sigma.empty())
        for (size_t i = 0; i != y.size(); ++i)
            p_[i] = Point(x[i], y[i]);
    else {
        for (size_t i = 0; i != y.size(); ++i)
            p_[i] = Point(x[i], y[i], sigma[i]);
        has_sigma_ = true;
    }
    sort(p_.begin(), p_.end());
    x_step_ = find_step();
    post_load();
    return p_.size();
}

void Data::set_points(const vector<Point> &p)
{
    p_ = p;
    sort(p_.begin(), p_.end());
    after_transform();
}

void Data::revert()
{
    if (filename_.empty())
        throw ExecuteError("Dataset can't be reverted, it was not loaded "
                           "from file");
    string old_title = title;
    string old_filename = filename_;
    // this->filename_ should not be passed by ref to load_file(), because it's
    // cleared before being used
    load_file(old_filename, given_x_, given_y_, given_s_,
              given_blocks_, given_format_, given_options_);
    title = old_title;
}

namespace {

void merge_same_x(vector<Point> &pp, bool avg)
{
    int count_same = 1;
    fp x0 = 0; // 0 is assigned only to avoid compiler warnings
    for (int i = pp.size() - 2; i >= 0; --i) {
        if (count_same == 1)
            x0 = pp[i+1].x;
        if (is_eq(pp[i].x, x0)) {
            pp[i].x += pp[i+1].x;
            pp[i].y += pp[i+1].y;
            pp[i].sigma += pp[i+1].sigma;
            pp[i].is_active = pp[i].is_active || pp[i+1].is_active;
            pp.erase(pp.begin() + i+1);
            count_same++;
            if (i > 0)
                continue;
            else
                i = -1; // to change pp[0]
        }
        if (count_same > 1) {
            pp[i+1].x /= count_same;
            if (avg) {
                pp[i+1].y /= count_same;
                pp[i+1].sigma /= count_same;
            }
            count_same = 1;
        }
    }
}

void shirley_bg(vector<Point> &pp, bool remove)
{
    const int max_iter = 50;
    const double max_rdiff = 1e-6;
    const int n = pp.size();
    double ya = pp[0].y; // lowest bg
    double yb = pp[n-1].y; // highest bg
    double dy = yb - ya;
    vector<double> B(n, ya);
    vector<double> PA(n, 0.);
    double old_A = 0;
    for (int iter = 0; iter < max_iter; ++iter) {
        vector<double> Y(n);
        for (int i = 0; i < n; ++i)
            Y[i] = pp[i].y - B[i];
        for (int i = 1; i < n; ++i)
            PA[i] = PA[i-1] + (Y[i] + Y[i-1]) / 2 * (pp[i].x - pp[i-1].x);
        double rel_diff = old_A != 0. ? fabs(PA[n-1] - old_A) / old_A : 1.;
        if (rel_diff < max_rdiff)
            break;
        old_A = PA[n-1];
        for (int i = 0; i < n; ++i)
            B[i] = ya + dy / PA[n-1] * PA[i];
    }
    if (remove)
        for (int i = 0; i < n; ++i)
            pp[i].y -= B[i];
    else
        for (int i = 0; i < n; ++i)
            pp[i].y = B[i];
}

void apply_operation(vector<Point> &pp, string const& op)
{
    assert (!pp.empty());
    assert (!op.empty());
    if (op == "sum_same_x")
        merge_same_x(pp, false);
    else if (op == "avg_same_x")
        merge_same_x(pp, true);
    else if (op == "shirley_bg")
        shirley_bg(pp, false);
    else if (op == "rm_shirley_bg")
        shirley_bg(pp, true);
    else if (op == "fft") {
        throw ExecuteError("Fourier Transform not implemented yet");
    }
    else if (op == "ifft") {
        throw ExecuteError("Inverse FFT not implemented yet");
    }
    else
        throw ExecuteError("Unknown dataset operation: " + op);
}

} // anonymous namespace

void Data::load_data_sum(vector<Data const*> const& dd, string const& op)
{
    if (dd.empty()) {
        clear();
        return;
    }
    // dd can contain this, we can't change p_ or title in-place.
    string new_filename = dd.size() == 1 ? dd[0]->get_filename() : "";
    vector<Point> new_p;
    string new_title;
    vector_foreach (Data const*, i, dd) {
        new_title += (i == dd.begin() ? "" : " + ") + (*i)->get_title();
        new_p.insert(new_p.end(), (*i)->points().begin(), (*i)->points().end());
    }
    sort(new_p.begin(), new_p.end());
    if (!new_p.empty() && !op.empty())
        apply_operation(new_p, op);
        // data should be sorted after apply_operation()
    clear();
    title = new_title;
    filename_ = new_filename;
    p_ = new_p;
    has_sigma_ = true;
    x_step_ = find_step();
    post_load();
}

void Data::add_one_point(double x, double y, double sigma)
{
    Point pt(x, y, sigma);
    vector<Point>::iterator a = upper_bound(p_.begin(), p_.end(), pt);
    int idx = a - p_.begin();
    p_.insert(a, pt);
    active_p_.insert(upper_bound(active_p_.begin(), active_p_.end(), idx), idx);
    if (pt.y < y_min_)
        y_min_ = pt.y;
    if (pt.y > y_max_)
        y_max_ = pt.y;
    // (fast) x_step_ update
    if (p_.size() < 2)
        x_step_ = 0.;
    else if (p_.size() == 2)
        x_step_ = p_[1].x - p_[0].x;
    else if (x_step_ != 0) {
        //TODO use tiny_relat_diff
        fp max_diff = 1e-4 * fabs(x_step_);
        if (idx == 0 && fabs((p_[1].x - p_[0].x) - x_step_) < max_diff)
            ; //nothing, the same step
        else if (idx == size(p_) - 1
                        && fabs((p_[idx].x - p_[idx-1].x) - x_step_) < max_diff)
            ; //nothing, the same step
        else
            x_step_ = 0.;
    }
}

// the same as replace_all(options, "_", "-")
static string tr_opt(string options)
{
    size_t pos = 0;
    while ((pos = options.find('_', pos)) != string::npos)
    {
        options[pos] = '-';
        ++pos;
    }
    return options;
}

int Data::count_blocks(string const& fn,
                       string const& format, string const& options)
{
    shared_ptr<const xylib::DataSet> xyds(
                        xylib::cached_load_file(fn, format, tr_opt(options)));
    return xyds->get_block_count();
}

int Data::count_columns(string const& fn,
                        string const& format, string const& options,
                        int first_block)
{
    shared_ptr<const xylib::DataSet> xyds(
                        xylib::cached_load_file(fn, format, tr_opt(options)));
    return xyds->get_block(first_block)->get_column_count();
}

// for column indices, INT_MAX is used as not given
void Data::load_file (string const& fn,
                      int idx_x, int idx_y, int idx_s,
                      vector<int> const& blocks,
                      string const& format, string const& options)
{
    if (fn.empty())
        return;

    string block_name;
    try {
        shared_ptr<const xylib::DataSet> xyds(
                        xylib::cached_load_file(fn, format, tr_opt(options)));
        clear(); //removing previous file
        vector<int> bb = blocks.empty() ? vector1(0) : blocks;

        vector_foreach (int, b, bb) {
            assert(xyds);
            xylib::Block const* block = xyds->get_block(*b);
            xylib::Column const& xcol
                = block->get_column(idx_x != INT_MAX ?  idx_x : 1);
            xylib::Column const& ycol
                = block->get_column(idx_y != INT_MAX ?  idx_y : 2);
            int n = block->get_point_count();
            if (n < 5 && bb.size() == 1)
                F->warn("Only " + S(n) + " data points found in file.");

            if (idx_s == INT_MAX) {
                for (int i = 0; i < n; ++i) {
                    p_.push_back(Point(xcol.get_value(i), ycol.get_value(i)));
                }
            }
            else {
                xylib::Column const& scol
                    = block->get_column(idx_s != INT_MAX ?  idx_s : 2);
                for (int i = 0; i < n; ++i) {
                    p_.push_back(Point(xcol.get_value(i), ycol.get_value(i),
                                      scol.get_value(i)));
                }
                has_sigma_ = true;
            }
            if (xcol.get_step() != 0.) { // column has fixed step
                x_step_ = xcol.get_step();
                if (x_step_ < 0) {
                    reverse(p_.begin(), p_.end());
                    x_step_ = -x_step_;
                }
            }
            if (!ycol.get_name().empty()) {
                if (!block_name.empty())
                    block_name += "/";
                block_name += ycol.get_name();
                if (!xcol.get_name().empty())
                    block_name += "(" + xcol.get_name() + ")";
            }
            else if (!block->get_name().empty()) {
                if (!block_name.empty())
                    block_name += "/";
                block_name += block->get_name();
            }
        }
    } catch (runtime_error const& e) {
        throw ExecuteError(e.what());
    }

    if (!block_name.empty())
        title = block_name;
    else {
        title = get_file_basename(fn);
        if (idx_x != INT_MAX && idx_y != INT_MAX)
            title += ":" + S(idx_x) + ":" + S(idx_y);
    }

    if (x_step_ == 0) {
        sort(p_.begin(), p_.end());
        x_step_ = find_step();
    }

    filename_ = fn;
    given_x_ = idx_x;
    given_y_ = idx_y;
    given_s_ = idx_s;
    given_blocks_ = blocks;
    given_options_ = options;

    post_load();
}

fp Data::get_y_at (fp x) const
{
    int n = get_upper_bound_ac (x);
    if (n > size(active_p_) || n <= 0)
        return 0;
    fp y1 = get_y (n - 1);
    fp y2 = get_y (n);
    fp x1 = get_x (n - 1);
    fp x2 = get_x (n);
    return y1 + (y2 - y1) * (x - x1) / (x2 - x1);
}

// std::is_sorted() is added C++0x
template <typename T>
bool is_vector_sorted(vector<T> const& v)
{
    if (v.size() <= 1)
        return true;
    for (typename vector<T>::const_iterator i = v.begin()+1; i != v.end(); ++i)
            if (*i < *(i-1))
                return false;
    return true;
}

void Data::after_transform()
{
    if (!is_vector_sorted(p_))
        sort(p_.begin(), p_.end());
    x_step_ = find_step();
    update_active_p();
    recompute_y_bounds();
}

void Data::update_active_p()
    // pre: p_.x sorted
    // post: active_p_ sorted
{
    active_p_.clear();
    for (int i = 0; i < size(p_); i++)
        if (p_[i].is_active)
            active_p_.push_back(i);
}


//FIXME to remove it or to leave it?
string Data::range_as_string() const
{
    if (active_p_.empty()) {
        F->warn ("File not loaded or all points inactive.");
        return "[]";
    }
    vector<Point>::const_iterator old_p = p_.begin() + active_p_[0];
    fp left =  old_p->x;
    string s = "[" + S (left) + " : ";
    for (vector<int>::const_iterator i = active_p_.begin() + 1;
                                                    i != active_p_.end(); i++) {
        if (p_.begin() + *i != old_p + 1) {
            fp right = old_p->x;
            left = p_[*i].x;
            s += S(right) + "], + [" + S(left) + " : ";
        }
        old_p = p_.begin() + *i;
    }
    fp right = old_p->x;
    s += S(right) + "]";
    return s;
}

///check for fixed step
fp Data::find_step()
{
    const fp tiny_relat_diff = 1e-4;
    if (p_.size() < 2)
        return 0.;
    else if (p_.size() == 2)
        return p_[1].x - p_[0].x;
    fp min_step, max_step, step;
    min_step = max_step = p_[1].x - p_[0].x;
    for (vector<Point>::iterator i = p_.begin() + 2; i < p_.end(); i++) {
        step = i->x - (i-1)->x;
        min_step = min (min_step, step);
        max_step = max (max_step, step);
    }
    fp avg = (max_step + min_step) / 2;
    if ((max_step - min_step) < tiny_relat_diff * fabs(avg))
        return avg;
    else
        return 0.;
}


int Data::get_lower_bound_ac (fp x) const
{
    //pre: p_.x is sorted, active_p_ is sorted
    int pit = lower_bound (p_.begin(), p_.end(), Point(x,0)) - p_.begin();
    return lower_bound (active_p_.begin(), active_p_.end(), pit)
        - active_p_.begin();
}

int Data::get_upper_bound_ac (fp x) const
{
    //pre: p_.x is sorted, active_p_ is sorted
    int pit = upper_bound (p_.begin(), p_.end(), Point(x,0)) - p_.begin();
    return upper_bound (active_p_.begin(), active_p_.end(), pit)
        - active_p_.begin();
}

vector<Point>::const_iterator Data::get_point_at(fp x) const
{
    return lower_bound (p_.begin(), p_.end(), Point(x,0));
}

fp Data::get_x_min() const
{
    vector_foreach (Point, i, p_)
        if (is_finite(i->x))
            return i->x;
    return 0.;
}

fp Data::get_x_max() const
{
    if (p_.empty())
        return 180.;
    for (vector<Point>::const_reverse_iterator i = p_.rbegin();
            i != p_.rend(); ++i)
        if (is_finite(i->x))
            return i->x;
    return 180.;
}


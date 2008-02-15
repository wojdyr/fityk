// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License version 2
// $Id$

#include "common.h"
#include "data.h" 
//#include "ui.h"
#include "numfuncs.h"
#include "datatrans.h" 
#include "settings.h" 
#include "logic.h" 

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <fstream>
#include <sstream>
#include <algorithm>

#include <xylib/xylib.h>

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
    if (p.empty())
        s = "No data points.";
    else
        s = S(p.size()) + " points, " + S(active_p.size()) + " active.";
    if (!filename.empty())
        s += "\nFilename: " + filename;
    if (given_x != INT_MAX || given_y != INT_MAX || given_s != INT_MAX)
        s += "\nColumns: " + (given_x != INT_MAX ? S(given_x) : S("_")) 
                    + ", " + (given_y != INT_MAX ? S(given_y) : S("_"));
    if (given_s != INT_MAX)
        s += ", " + S(given_s);
    if (!title.empty())
        s += "\nData title: " + title;
    if (active_p.size() != p.size())
        s += "\nActive data range: " + range_as_string();
    return s;
}

void Data::clear()
{
    filename = "";   
    title = "";
    given_x = given_y = given_s = INT_MAX;
    given_options.clear();
    given_blocks.clear();
    p.clear();
    x_step = 0;
    active_p.clear();
    has_sigma = false;
}

void Data::post_load()
{
    if (p.empty())
        return;
    string inf = S(p.size()) + " points.";
    if (!has_sigma) {
        int dds = F->get_settings()->get_e("data-default-sigma");
        if (dds == 's') {
            for (vector<Point>::iterator i = p.begin(); i < p.end(); i++) 
                i->sigma = i->y > 1. ? sqrt (i->y) : 1.;
            inf += " No explicit std. dev. Set as sqrt(y)";
        }
        else if (dds == '1') {
            for (vector<Point>::iterator i = p.begin(); i < p.end(); i++) 
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
    for (vector<Point>::iterator i = p.begin(); i != p.end(); i++) {
        if (!is_finite(i->y))
            continue;
        if (!ini) {
            y_min = y_max = i->y;
            ini = true;
        }
        if (i->y < y_min)
            y_min = i->y;
        if (i->y > y_max)
            y_max = i->y;
    }
}

int Data::load_arrays(const vector<fp> &x, const vector<fp> &y, 
                      const vector<fp> &sigma, const string &data_title)
{
    size_t size = y.size();
    assert(y.size() == size);
    assert(sigma.empty() || sigma.size() == size);
    clear();
    title = data_title;
    if (sigma.empty()) 
        for (size_t i = 0; i < size; ++i)
            p.push_back (Point (x[i], y[i]));
    else {
        for (size_t i = 0; i < size; ++i)
            p.push_back (Point (x[i], y[i], sigma[i]));
        has_sigma = true;
    }
    sort(p.begin(), p.end());
    x_step = find_step();
    post_load();
    return p.size();
}

void Data::revert()
{
    if (filename.empty())
        throw ExecuteError("Dataset can't be reverted, it was not loaded "
                           "from file");
    string old_title = title;
    string old_filename = filename; 
    // this->filename should not be passed by ref to load_file(), because it's
    // cleared before being used
    load_file(old_filename, given_x, given_y, given_s,
              given_blocks, given_options);
    title = old_title;
}

namespace {

void merge_same_x(vector<Point> &pp, bool avg)
{
    int count_same = 1;
    fp x0;
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
        double rel_diff = old_A != 0. ? abs(PA[n-1] - old_A) / old_A : 1.;
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
    assert(!dd.empty());
    // dd can contain this, we can't change p or title in-place.
    std::vector<Point> new_p;
    string new_title = dd[0]->get_title();
    string new_filename = dd.size() == 1 ? dd[0]->get_filename() : "";
    for (vector<Data const*>::const_iterator i = dd.begin()+1; 
                                                          i != dd.end(); ++i)
        new_title += " + " + (*i)->get_title();
    for (vector<Data const*>::const_iterator i = dd.begin(); 
                                                          i != dd.end(); ++i) {
        vector<Point> const& points = (*i)->points();
        new_p.insert(new_p.end(), points.begin(), points.end());
    }
    sort(new_p.begin(), new_p.end());
    if (!new_p.empty() && !op.empty())
        apply_operation(new_p, op);
        // data should be sorted after apply_operation()
    clear();
    title = new_title;
    filename = new_filename;
    p = new_p;
    has_sigma = true;
    x_step = find_step();
    post_load();
}

void Data::add_one_point(double x, double y, double sigma)
{
    Point pt(x, y, sigma);
    vector<Point>::iterator a = upper_bound(p.begin(), p.end(), pt);
    int idx = a - p.begin();
    p.insert(a, pt);
    active_p.insert(upper_bound(active_p.begin(), active_p.end(), idx), idx);
    if (pt.y < y_min)
        y_min = pt.y;
    if (pt.y > y_max)
        y_max = pt.y;
    // (fast) x_step update
    if (p.size() < 2)
        x_step = 0.;
    else if (p.size() == 2)
        x_step = p[1].x - p[0].x;
    else if (x_step != 0) {
        //TODO use tiny_relat_diff
        fp max_diff = 1e-4 * fabs(x_step);
        if (idx == 0 && fabs((p[1].x - p[0].x) - x_step) < max_diff)
            ; //nothing, the same step
        else if (idx == size(p) - 1 
                        && fabs((p[idx].x - p[idx-1].x) - x_step) < max_diff)
            ; //nothing, the same step
        else
            x_step = 0.;
    }
}


// for column indices, INT_MAX is used as not given
void Data::load_file (string const& filename_, 
                      int idx_x, int idx_y, int idx_s, 
                      vector<int> const& blocks,
                      vector<string> const& options)
{
    static xylib::DataSet *xyds = NULL;
    static string xyds_path;
    static vector<string> xyds_options;

    if (filename_.empty())
        return;

    try {
        if (filename_ != xyds_path || options != xyds_options) {
            string format_name;
            vector<string> options_tail;
            if (!options.empty()) {
                format_name = options[0];
                options_tail = vector<string>(options.begin()+1, options.end());
            }
            // if xylib::load_file() throws exception, we keep value of xyds
            xylib::DataSet *new_xyds = xylib::load_file(filename_, format_name,
                                                        options_tail);
            assert(new_xyds);
            delete xyds;
            xyds = new_xyds;
            xyds_path = filename_;
            xyds_options = options;
        }

        clear(); //removing previous file

        vector<int> bb = blocks.empty() ? vector1(0) : blocks;

        for (vector<int>::const_iterator b = bb.begin(); b != bb.end(); ++b) {
            assert(xyds);
            xylib::Block const* block = xyds->get_block(*b);
            xylib::Column const& xcol 
                = block->get_column(idx_x != INT_MAX ?  idx_x : 1);
            xylib::Column const& ycol 
                = block->get_column(idx_y != INT_MAX ?  idx_y : 2);
            int n = block->get_point_count();
            if (n < 5 && bb.size() == 1)
                F->warn("Only " + S(p.size()) + " data points found in file.");

            if (idx_s == INT_MAX) {
                for (int i = 0; i < n; ++i) {
                    p.push_back(Point(xcol.get_value(i), ycol.get_value(i)));
                }
            }
            else {
                xylib::Column const& scol 
                    = block->get_column(idx_s != INT_MAX ?  idx_s : 2);
                for (int i = 0; i < n; ++i) {
                    p.push_back(Point(xcol.get_value(i), ycol.get_value(i),
                                      scol.get_value(i)));
                }
                has_sigma = true;
            }
            if (xcol.step != 0.) { // column has fixed step
                x_step = xcol.step;
                if (x_step < 0) {
                    reverse(p.begin(), p.end());
                    x_step = -x_step;
                }
            }
        }

        title = get_file_basename(filename_);
        if (idx_x != INT_MAX && idx_y != INT_MAX)
            title += ":" + S(idx_x) + ":" + S(idx_y);

        if (x_step == 0) {
            sort(p.begin(), p.end());
            x_step = find_step();
        }

        filename = filename_;   
        given_x = idx_x;
        given_y = idx_y;
        given_s = idx_s;
        given_blocks = blocks;
        given_options = options;

        post_load();
    } catch (runtime_error const& e) {
        throw ExecuteError(e.what());
    }
}

fp Data::get_y_at (fp x) const
{
    int n = get_upper_bound_ac (x);
    if (n > size(active_p) || n <= 0)
        return 0;
    fp y1 = get_y (n - 1);
    fp y2 = get_y (n);
    fp x1 = get_x (n - 1);
    fp x2 = get_x (n);
    return y1 + (y2 - y1) * (x - x1) / (x2 - x1);
}

void Data::transform(const string &s) 
{
    p = transform_data(s, p);
    sort(p.begin(), p.end());
    update_active_p();
}

void Data::update_active_p() 
    // pre: p.x sorted
    // post: active_p sorted
{
    active_p.clear();
    for (int i = 0; i < size(p); i++)
        if (p[i].is_active) 
            active_p.push_back(i);
}

// does anyone need it ? 
/*
int Data::auto_range (fp y_level, fp x_margin)
{
    // pre: p.x sorted
    if (p.empty()) {
        F->warn("No points loaded.");
        return -1;
    }
    bool state = (p.begin()->y >= y_level);
    for (vector<Point>::iterator i = p.begin(); i < p.end(); i++) {
        if (state == (i->y >= y_level)) 
            i->is_active = state;
        else if (state && i->y < y_level) {
            i->is_active = false;
            vector<Point>::iterator e = lower_bound (p.begin(), p.end(),
                                                        Point(i->x + x_margin));
            for ( ; i < e; i++)
                i->is_active = true;
            state = false;
        }
        else { // !state && i->y >= y_level
            vector<Point>::iterator b =  lower_bound(p.begin(), p.end(), 
                                                        Point(i->x - x_margin));
            for (vector<Point>::iterator j = b; j <= i; j++)
                j->is_active = true;
            state = true;
        }
    }
    update_active_p();
    return active_p.size();
}
*/

//FIXME to remove it or to leave it?
string Data::range_as_string () const 
{
    if (active_p.empty()) {
        F->warn ("File not loaded or all points inactive.");
        return "[]";
    }
    vector<Point>::const_iterator old_p = p.begin() + active_p[0];
    fp left =  old_p->x;
    string s = "[" + S (left) + " : ";
    for (vector<int>::const_iterator i = active_p.begin() + 1; 
                                                    i != active_p.end(); i++) {
        if (p.begin() + *i != old_p + 1) {
            fp right = old_p->x;
            left = p[*i].x;
            s += S(right) + "], + [" + S(left) + " : ";
        }
        old_p = p.begin() + *i;
    }
    fp right = old_p->x;
    s += S(right) + "]";
    return s;
}

///check for fixed step
fp Data::find_step() 
{
    const fp tiny_relat_diff = 1e-4;
    if (p.size() < 2)
        return 0.;
    else if (p.size() == 2)
        return p[1].x - p[0].x;
    fp min_step, max_step, step;
    min_step = max_step = p[1].x - p[0].x;
    for (vector<Point>::iterator i = p.begin() + 2; i < p.end(); i++) {
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
    //pre: p.x is sorted, active_p is sorted
    int pit = lower_bound (p.begin(), p.end(), Point(x,0)) - p.begin();
    return lower_bound (active_p.begin(), active_p.end(), pit) 
        - active_p.begin();
}

int Data::get_upper_bound_ac (fp x) const
{
    //pre: p.x is sorted, active_p is sorted
    int pit = upper_bound (p.begin(), p.end(), Point(x,0)) - p.begin();
    return upper_bound (active_p.begin(), active_p.end(), pit) 
        - active_p.begin();
}

vector<Point>::const_iterator Data::get_point_at(fp x) const
{
    return lower_bound (p.begin(), p.end(), Point(x,0));
}

void Data::export_to_file(string filename, vector<string> const& vt, 
                          vector<string> const& ff_names,
                          bool append) 
{
    ofstream os(filename.c_str(), ios::out | (append ? ios::app : ios::trunc));
    if (!os) {
        F->warn("Can't open file: " + filename);
        return;
    }
    os << "# " << title << endl;
    vector<string> cols;
    if (vt.empty()) {
        cols.push_back("x");
        cols.push_back("y");
        cols.push_back("s");
    }
    else {
        for (vector<string>::const_iterator i=vt.begin(); i != vt.end(); ++i) 
            if (startswith(*i, "*F")) {
                for (vector<string>::const_iterator j = ff_names.begin(); 
                                                    j != ff_names.end(); ++j)
                    cols.push_back("%" + *j + string(*i, 2));
            }
            else
                cols.push_back(*i);
    }
    os << join_vector(cols, "\t") << "\t#exported by fityk " VERSION << endl;

    vector<vector<fp> > r;
    bool only_active = !contains_element(vt, "a");
    for (vector<string>::const_iterator i = cols.begin(); i != cols.end(); ++i) 
        r.push_back(get_all_point_expressions(*i, this, only_active));

    size_t nc = cols.size();
    for (size_t i = 0; i != r[0].size(); ++i) {
        for (size_t j = 0; j != nc; ++j) {
            os << r[j][i] << (j < nc-1 ? '\t' : '\n'); 
        }
    }
}


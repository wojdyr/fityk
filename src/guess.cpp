// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#include "common.h"
#include "guess.h"
#include <algorithm>
#include "data.h"
#include "sum.h"
#include "logic.h"
#include "ui.h"
#include "func.h"
#include "settings.h"
#include "datatrans.h"

using namespace std;

fp VirtPeak::get_approx_y(fp x) const
{
    if (fabs(x - center) < fwhm) {
        fp dist_in_fwhm = fabs((x - center) / fwhm);
        if (dist_in_fwhm < 0.5)
            return height;
        else // 0.5 < dist_in_fwhm < 1.0
            return height * 2. * (1. - dist_in_fwhm);
    }
    else
        return 0;
}

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

    for (vector<VirtPeak>::const_iterator i = ec->virtual_peaks.begin();
                                             i != ec->virtual_peaks.end(); i++)
        y -= i->get_approx_y(x);
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
    return max (fwhm, EPSILON);
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

} // anonymous namespace


void estimate_peak_parameters(DataWithSum const* ds, fp range_from, fp range_to,
                              fp *center, fp *height, fp *area, fp *fwhm,
                              EstConditions const* ec) 
{
    AL->use_parameters();
    if (ds->get_data()->get_n() <= 0) 
        throw ExecuteError("No active data.");

    int l_bor = max (ds->get_data()->get_lower_bound_ac (range_from), 0);
    int r_bor = min (ds->get_data()->get_upper_bound_ac (range_to), 
                     ds->get_data()->get_n() - 1);
    if (l_bor >= r_bor)
        throw ExecuteError("Searching peak outside of data points range. "
                           "Abandoned. Tried at [" + S(range_from) + " : " 
                           + S(range_to) + "]");
    int max_y_pos = max_data_y_pos(ds, l_bor, r_bor, ec);
    if (max_y_pos == l_bor || max_y_pos == r_bor - 1) {
        string s = "Estimating peak parameters: peak outside of search scope."
                  " Tried at [" + S(range_from) + " : " + S(range_to) + "]";
        if (getSettings()->get_b("can-cancel-guess")) 
            throw ExecuteError(s + " Canceled.");
        info (s);
    }
    fp h = my_y(ds, max_y_pos, ec);
    if (height) 
        *height = h * getSettings()->get_f("height-correction");
    if (center)
        *center = ds->get_data()->get_x(max_y_pos);
    if (fwhm)
        *fwhm = compute_data_fwhm(ds, l_bor, max_y_pos, r_bor, 0.5, ec) 
                                    * getSettings()->get_f("width-correction");
    if (area) 
        *area = data_area(ds, l_bor, r_bor, ec); 
        //FIXME: how to find peak borders?  t * FWHM would be better? t=??
}

string print_simple_estimate(DataWithSum const* ds,
                             fp range_from, fp range_to) 
{
    fp c = 0, h = 0, a = 0, fwhm = 0;
    estimate_peak_parameters(ds, range_from, range_to, &c, &h, &a, &fwhm); 
    return "Peak center: " + S(c) 
            + " (searched in [" + S(range_from) + ":" + S(range_to) + "])" 
            + " height: " + S(h) + ", area: " + S(a) + ", FWHM: " + S(fwhm);
}

string print_multiple_peakfind(DataWithSum const* ds,
                               int n, vector<string> const& range) 
{
    fp range_from, range_to;
    parse_range(ds, range, range_from, range_to);
    string s;
    EstConditions estc;
    estc.real_peaks = ds->get_sum()->get_ff_idx();
    for (int i = 1; i <= n; i++) {
        fp c = 0., h = 0., a = 0., fwhm = 0.;
        estimate_peak_parameters(ds, range_from, range_to, 
                                 &c, &h, &a, &fwhm, &estc);
        estc.virtual_peaks.push_back(VirtPeak(c, h, fwhm));
        if (h == 0.) 
            break;
        s += S(i != 1 ? "\n" : "") + "Peak #" + S(i) + " - center: " + S(c) 
            + ", height: " + S(h) + ", area: " + S(a) + ", FWHM: " + S(fwhm);
    }
    return s;
}


void guess_and_add(DataWithSum* ds, 
                   string const& name, string const& function,
                   vector<string> const& range, vector<string> vars)
{
    fp range_from, range_to;
    parse_range(ds, range, range_from, range_to);
    fp c = 0., h = 0., a = 0., fwhm = 0.;
    EstConditions estc;
    Sum const* sum = ds->get_sum();
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
    estimate_peak_parameters(ds, range_from, range_to, 
                             &c, &h, &a, &fwhm, &estc);
    vector<string> vars_lhs(vars.size());
    for (int i = 0; i < size(vars); ++i)
        vars_lhs[i] = string(vars[i], 0, vars[i].find('='));
    if (!contains_element(vars_lhs, "center"))
        vars.push_back("center=~"+S(c));
    if (!contains_element(vars_lhs, "height"))
        vars.push_back("height=~"+S(h));
    if (!contains_element(vars_lhs, "fwhm") 
            && !contains_element(vars_lhs, "hwhm"))
        vars.push_back("fwhm=~"+S(fwhm));
    if (!contains_element(vars_lhs, "area"))
        vars.push_back("area=~"+S(a));
    string real_name = AL->assign_func(name, function, vars);
    ds->get_sum()->add_function_to(real_name, 'F');
}

bool is_parameter_guessable(string const& name)
{
    return name == "center" || name == "height" || name == "fwhm"
        || name == "area" || name == "hwhm";
}

bool is_defvalue_guessable(string defvalue)
{
    replace_words(defvalue, "center", "1");
    replace_words(defvalue, "height", "1");
    replace_words(defvalue, "fwhm", "1");
    replace_words(defvalue, "area", "1");
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
    
    for (vector<string>::const_iterator i = nd.begin(); i != nd.end(); ++i) {
        string::size_type eq = i->find('=');
        if (eq == string::npos) { //no defvalue
            if (!is_parameter_guessable(strip_string(*i)))
                return false;
        }
        else if (check_defvalue 
                 && !is_parameter_guessable(strip_string(string(*i, 0, eq)))
                 && !is_defvalue_guessable(string(*i, eq+1))) {
                return false;
        }
    }
    return true;
}



// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#include "common.h"
#include "pcore.h"
#include <stdio.h>
#include <fstream>
#include <algorithm>
#include "data.h"
#include "sum.h"
#include "ui.h"
#include "other.h" //temporary
#ifdef USE_XTAL
    #include "crystal.h"
#endif

using namespace std;

PlotCore *my_core;

//========================================================================
//                          class PlotCore
//========================================================================

const fp PlotCore::relative_view_x_margin = 1./20.;
const fp PlotCore::relative_view_y_margin = 1./20.;


PlotCore::PlotCore(Parameters *parameters)
    : view(0, 180, 0, 1e3), ds_was_changed(false), v_was_changed(false)
{
    sum = new Sum(parameters);
#ifdef USE_XTAL
    crystal = new Crystal(sum);
#endif //USE_XTAL
    append_data(); //and set_my_vars() is called from there
}

PlotCore::~PlotCore()
{
#ifdef USE_XTAL
    delete crystal;
#endif //USE_XTAL
    delete sum;
    purge_all_elements(datasets);
}

bool PlotCore::activate_data(int n)
{
    //if n==-1: only call set_my_vars()
    if (n < -1 || n >= size(datasets)) {
        warn("No such datafile in this plot: " + S(n));
        return false;
    }

    if (n != -1 && n != active_data) {
        active_data = n;
        ds_was_changed = true;
    }
    set_my_vars();
    return true;
}

int PlotCore::append_data()
{
    datasets.push_back(new Data);
    active_data = datasets.size() - 1;
    ds_was_changed = true;
    set_my_vars();
    return active_data;
}

void PlotCore::set_my_vars()
{
    my_data = datasets[active_data];
    my_sum = sum;
    my_crystal = crystal;
    my_core = this;
}

void PlotCore::set_view (Rect rt, bool fit)
{
    v_was_changed = true;
    if (my_data->is_empty()) {
        view.left = (rt.left < rt.right && rt.left != -INF ? rt.left : 0);
        view.right = (rt.left < rt.right && rt.right != +INF ? rt.right : 180);
        view.bottom = (rt.bottom < rt.top && rt.bottom != -INF ? rt.bottom : 0);
        view.top = (rt.bottom < rt.top && rt.top != +INF ? rt.top : 1e3);
        return;
    }
    fp x_min = my_data->get_x_min();
    fp x_max = my_data->get_x_max();
    if (x_min == x_max) x_min -= 0.1, x_max += 0.1;
    view = rt;
    // if rt is too wide or too high, 
    // it is narrowed to sensible size (containing all data + margin)
    // the same if rt.left >= rt.right or rt.bottom >= rt.top
    fp x_size = x_max - x_min;
    const fp sens_mult = 10.;
    if (view.left <  x_min - sens_mult * x_size)
        view.left = x_min - x_size * relative_view_x_margin;
    if (view.right >  x_max + sens_mult * x_size)
        view.right = x_max + x_size * relative_view_x_margin; 
    if (view.left >= view.right) {
        view.left = x_min - x_size * relative_view_x_margin;
        view.right = x_max + x_size * relative_view_x_margin; 
    }
    if (fit || view.bottom == -INF && view.top == INF 
            || view.bottom >= view.top)
        set_view_y_fit();
    else {
        fp y_min = my_data->get_y_min();
        fp y_max = my_data->get_y_max();
        if (y_min == y_max) 
            y_min -= 0.1, y_max += 0.1;
        fp y_size = y_max - y_min;
        if (view.bottom < y_min - sens_mult * y_size)
            view.bottom = min (y_min, 0.);
        if (view.top > y_max + y_size)
            view.top = y_max + y_size * relative_view_y_margin;
    }
}

void PlotCore::set_view_y_fit()
{
    if (my_data->is_empty()) {
        warn ("Can't set view when no points are loaded");
        return;
    }
    vector<Point>::const_iterator f = my_data->get_point_at(view.left);
    vector<Point>::const_iterator l = my_data->get_point_at(view.right);
    if (f >= l) { //no points in this range
        view.bottom = 0.;
        view.top = 1.;
        return;
    }
    fp y_max = 0., y_min = 0.;
    //first we are searching for minimal and max. y in active points
    bool min_max_set = false;
    for (vector<Point>::const_iterator i = f; i < l; i++) {
        if (i->is_active) {
            min_max_set = true;
            if (i->y > y_max) 
                y_max = i->y;
            if (i->y < y_min) 
                y_min = i->y;
        }
    }
    if (!min_max_set || y_min == y_max) { //none or 1 active point, so now we  
                                   // search for min. and max. y in all points 
        for (vector<Point>::const_iterator i = f; i < l; i++) { 
            if (i->y > y_max) 
                y_max = i->y;
            if (i->y < y_min) 
                y_min = i->y;
        }
        if (y_min == y_max) { // again not enough points (with different y's)
                y_min -= 0.1; 
                y_min += 0.1;
        }
    }
    // estimated sum maximum
    fp sum_y_max = sum->approx_max(view.left, view.right);
    if (sum_y_max > y_max)
        y_max = sum_y_max;
    view.bottom = y_min;
    view.top = y_max + (y_max - y_min) * relative_view_y_margin;;
}


bool PlotCore::was_changed() const
{ 
    for (vector<Data*>::const_iterator i = datasets.begin(); 
                                                    i != datasets.end(); i++)
        if ((*i)->was_changed())
            return true;
    //if here - no Data changed
    return v_was_changed || ds_was_changed || sum->was_changed();
}

void PlotCore::was_plotted() 
{ 
    ds_was_changed = false; 
    v_was_changed = false;
    for (vector<Data*>::iterator i = datasets.begin(); i != datasets.end(); i++)
        (*i)->d_was_plotted();
    sum->s_was_plotted();
}

vector<string> PlotCore::get_data_titles() const
{
    vector<string> v;
    //v.reserve(datasets.size());
    for (vector<Data*>::const_iterator i = datasets.begin(); 
                                                    i != datasets.end(); i++)
        v.push_back((*i)->get_title());
    return v;
}

const Data *PlotCore::get_data(int n) const
{
    return (n >= 0 && n < size(datasets)) ?  datasets[n] : 0;
}

void PlotCore::remove_data(int n)
{
    if (n < 0 || n >= size(datasets)) {
        warn("No such dataset number: " + S(n));
        return;
    }
    if (n == active_data) 
        active_data = n > 0 ? n-1 : 0;
    purge_element(datasets, n);
    if (datasets.empty()) //it should not be empty
        datasets.push_back(new Data);
    ds_was_changed = true;
    my_data = datasets[active_data];
}

void PlotCore::export_as_script(std::ostream& os) const
{
    for (int i = 0; i != size(datasets); i++) {
        os << "#dataset " << i << endl;
        if (i != 0)
            os << "d.activate ::*" << endl;
        datasets[i]->export_as_script(os);
        os << endl;
    }
    if (active_data != size(datasets) - 1)
        os << "d.activate ::" << active_data << " # set active" << endl; 
    sum->export_as_script(os);
    os << endl;
#ifdef USE_XTAL
    crystal->export_as_script(os);
    os << endl;
#endif //USE_XTAL
}


//========================================================================
//                          class Parameters
//========================================================================

Parameters::Parameters()
    : nA(0), history (vector1(HistoryItem(fp_v0, "new"))), hp(0),
      adomain(), afrozen()
{
}

Pag Parameters::add_a (fp value, Domain d) 
{
    assert (size(values()) == nA && size(adomain)==nA && size(afrozen)==nA);
    vector<fp> a_new = values();
    a_new.push_back (value);
    adomain.push_back (d);
    afrozen.push_back (false);
    int n = nA;
    nA++;
    write_avec (a_new, "add @" + S(n));
    assert (history.size() == 1);
    return Pag (0., n); 
}

void Parameters::write_avec (const vector<fp>& a, string comment, bool no_move) 
{ 
    assert (size(a) == nA && size(adomain) == nA && size(afrozen) == nA);
     //3 special cases:
    if (a == values()) //1. no change
        return;
    p_was_changed = true;
    if (a.size() != values().size()) { //2. added/removed a (change in size)
        history.clear();
        history.push_back (HistoryItem (a, comment));
        hp = 0;
        return;
    }
    if (comment.empty() && !history[hp].saved) { //3. change in place
        history[hp].a = a; 
        return;
    }
    if (!no_move && hp < size(history) - 1) 
        for (vector<HistoryItem>::iterator i = history.end() - 1; 
                                                i > history.begin() + hp; i--)
            if (!i->saved)
                history.erase (i);
    history.push_back (HistoryItem (a, comment));
    if (!no_move) {
        hp = history.size() - 1;
        info ("Parameters changed (by: " + comment + ")");
    }
}

string Parameters::print_history() const
{
    string s = "History contains " + S(history.size()) + " entries. "
        "Current is #" + S(hp + 1);
    for (int i = 0; i < size(history); i++) {
        s += (hp == i ? "\n->" : "\n  ");
        s += (history[i].saved ? " *" : " #") + S(i + 1) 
            + "  changed by: " + history[i].comment;
        //if (...)
        //   s += "   WSSR = " + S(compute_wssr (history[i].a));
        // it would be necessary to include v_fit.h to do this 
        // convenient comparision of WSSRs is done in GUI version 
    }
    return s;
}

string Parameters::history_diff (vector<int> hist_items) const
{
    //hist_items empty -> info about all items
    if (hist_items.empty())
        for (int i = 0; i < size(history); i++)
            hist_items.push_back(i+1);
    //check user input
    if (*max_element(hist_items.begin(), hist_items.end()) > size(history)
            || *min_element (hist_items.begin(), hist_items.end()) <= 0)
        return "Wrong numbers of history entries in query";
    //draw "table"
    string s = "   ";
    for (vector<int>::iterator j = hist_items.begin(); j!=hist_items.end(); j++)
        s += "   #" + S(*j) + "    "; 
    for (int i = 0; i < nA; i++) {
        s += "\n@" + S(i) + ": ";
        for (vector<int>::iterator j = hist_items.begin(); 
                                                j != hist_items.end(); j++)
            s += S(history[(*j) - 1].a[i]) + " | ";
    }
    return s;
}

void Parameters::move_in_history (int k, bool relative)
{
    if (relative)
        hp += k;
    else
        hp = k - 1;
    if (hp < 0) {
        warn ("Beginning of history reached.");
        hp = 0;
    }
    else if (hp >= size(history)) {
        warn ("End of history reached.");
        hp = history.size() - 1;
    }
    p_was_changed = true;
    verbose ("Current history position is #" + S(hp + 1)
            + " of " + S(history.size()));
}

void Parameters::toggle_history_item_saved(int k)
{
    if (k - 1 < 0) k = hp + 1;
    else if (k  - 1 >= size(history)) {
        warn ("There is no item #" + S(k) + " in history");
        return;  
    }  
    history[k - 1].saved = ! history[k - 1].saved;  
}

void Parameters::change_domain (int nr, Domain d) 
{ 
    if (nr < 0 || nr >= size(adomain)) {
        warn ("No such A: @" + S(nr));
        return;
    }
    adomain[nr] = d; 
}

fp Parameters::variation_of_a (int n, fp variat) const
{
    assert (0 <= n && n < nA);
    const Domain& dom = get_domain (n);
    fp ctr = dom.is_ctr_set() ? dom.Ctr() : get_a(n);
    fp sgm = dom.is_set() ? dom.Sigma() 
                          : my_sum->get_def_rel_domain_width() * ctr;
    return ctr + sgm * variat;
}

bool Parameters::do_rm_a(int nr)
{
    assert (nr >= 0 && nr < nA);
    if (AL->refs_to_a (Pag(0., nr)) != 0) 
        return false;
    vector<fp> a_new = values();
    a_new.erase (a_new.begin() + nr);
    adomain.erase (adomain.begin() + nr);
    afrozen.erase (afrozen.begin() + nr);
    nA--;
    AL->synch_after_rm_a (Pag(0., nr));
    write_avec (a_new, "rm @" + S(nr));
    assert (history.size() == 1);
    return true;
}

int Parameters::rm_a (int n, bool silent)
{
    if (n == -1) {
        int counter = 0;
        for (int i = nA - 1; i >= 0; i--)
            if (do_rm_a(i))
                counter ++;
        if (!silent) info (S(counter) + " parameters removed.");
        return counter;
    }

    if (n < 0 || n >= nA) {
        warn ("No such A: @" + S(n));
        return 0;
    }
    else if (do_rm_a(n)) {
        if (!silent) info ("Parameter @" + S(n) + " removed.");
        return 1;
    }
    else {
        warn ("Parameter @" + S(n) + " NOT removed. " 
                + S(AL->refs_to_a (Pag(0., n))) + " links. First remove: " 
                + AL->descr_refs_to_a (Pag(0., n)));
        return 0;
    }
}

string Parameters::info_a (int nr) const
{
    if (nr == -1) { // info about all A's
        string s;
        for (int i = 0; i < nA; i++)
            s += info_a(i) + (i != nA - 1 ? "\n" : "");
        return s;
    }
    else if (nr < 0 || nr >= nA ) 
        return "No such A: @"  + S(nr);
    else {
        ostringstream os;
        os << "@" << nr << ": " << get_a(nr); 
        os << (afrozen[nr] ? " F " : "  ") << adomain[nr].str();
        if (AL->refs_to_a (Pag(0., nr)))
            os << " (used by: " << AL->descr_refs_to_a (Pag(0., nr)) + ")"; 
        return os.str();
    }
}

fp Parameters::change_a (int nr, fp value, char c /*='='*/, bool add_to_history)
{
    if (nr == -1) {
        for (int i = 0; i < nA; i++)
            change_a (i, value, c, i == nA - 1 ? add_to_history : false);
        info (S(nA) + " parameters changed.");
        return 0.;
    }
    if (nr < 0 || nr >= nA) {
        warn ("No such A: @" + S(nr));
        return 0.;
    }
    fp before = get_a (nr); 
    vector<fp> a = values();
    switch (c) {
        case '=': a[nr] = value; break;
        case '+': a[nr] += value; break;
        case '-': a[nr] -= value; break;
        case '*': a[nr] *= value; break;
        case '/':
            if (value)
                a[nr] /= value;
            else 
                warn ("Trying to divide by zero in sum::change_a");
            break;
        case '%': if (value != 100.) a[nr] *= (value/100); break;
        default: assert(0); break;
    }
    if (before != a[nr])
        p_was_changed = true;
    if (add_to_history)
        write_avec (a, "manual");
    else
        write_avec (a, "");
    return before;
}

int Parameters::freeze(int nr, bool frozen)
{
    if (nr == -1) {
        fill (afrozen.begin(), afrozen.end(), frozen);
        return 0;
    }
    if (nr >= nA || nr < 0) {
        warn("There is no @" + S(nr) + ".");
        return -1;
    }
    if (frozen == afrozen[nr]) {
        warn ("@" + S(nr) + " is already " + S(frozen ? "" : "not ") 
               + "frozen.");
        return -1;
    }
    else {
        afrozen[nr] = frozen;
        verbose ("@" + S(nr) + "set " + S(frozen ? "" : "not ") + "frozen");
        return 0;
    }
}

int Parameters::count_frozen() const
{
    return count (afrozen.begin(), afrozen.end(), true);
}

string Parameters::frozen_info () const
{
    string s;
    for (int i = 0; i < size(afrozen); i++)
        if (afrozen[i])
            s += (s.empty() ? "@" : ", @") + S(i);
    return s;
}

void Parameters::export_as_script(std::ostream& os) const
{
    os << "# exporting parameters..." << endl;
    for (int i = 0; i < count_a(); i++) {  // @
        os << "s.add ~"<< get_a(i) 
           << "  " << get_domain(i).str();
        os << "  # @" << i <<  "  ("<< AL->refs_to_a(Pag(0., i)) << " link(s))";
        string refs = AL->descr_refs_to_a(Pag(0., i));
        if (!refs.empty())
            os << " (used by: " << refs << ")"; 
        os << endl;
    }
    if (count_frozen())
        os << "s.freeze " << frozen_info() << endl;
    os << "# parameters exported..." << endl;
}


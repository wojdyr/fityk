// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#ifndef pcore__H__
#define pcore__H__

#include <string>
#include "pag.h"

//small utility - for vector<T*> - delete & erase
template<typename T>
void purge_element(std::vector<T*> &vec, int n)
{
    assert(n >= 0 && n < static_cast<int>(vec.size()));
    T* tmp = vec[n];
    delete tmp;
    vec.erase(vec.begin() + n);
}

// and similar utility - delete & clear
template<typename T>
void purge_all_elements(std::vector<T*> &vec)
{
    while (!vec.empty()) {
        T* tmp = vec.back();
        delete tmp;
        vec.pop_back();
    }
}

class Parameters;

struct Rect 
{
    fp left, right, bottom, top;
    Rect (fp l, fp r, fp b, fp u) : left(l), right(r), bottom(b), top(u) {}
    Rect (fp l, fp r) : left(l), right(r), bottom(0), top(0) {}
    Rect () : left(0), right(0), bottom(0), top(0) {}
    fp width() const { return right - left; }
    fp height() const { return top - bottom; }
    std::string str() const
    { 
        return "[" + (left!=right ? S(left) + ":" + S(right) : std::string(" "))
            + "] [" + (bottom!=top ? S(bottom) + ":" + S(top) 
                                               : std::string (" ")) + "]";
    }
};


class Domain 
{ 
    bool set, ctr_set;
    fp ctr, sigma; 

public:
    Domain () : set(false), ctr_set(false) {}
    Domain (fp sigm) : set(true), ctr_set(false), sigma(sigm) {}
    Domain (fp c, fp sigm) : set(true), ctr_set(true), ctr(c), sigma(sigm) {}
    Domain (pre_Domain &p) : set(p.set), ctr_set(p.ctr_set), //used in parser.y
                             ctr(p.ctr), sigma(p.sigma) {}
    bool is_set() const { return set; }
    bool is_ctr_set() const { return ctr_set; }
    fp Ctr() const { assert(set && ctr_set); return ctr; }
    fp Sigma() const { assert(set); return sigma; }
    std::string str() const 
        { return set ? "[" + (ctr_set ? S(ctr) : S()) 
                                         + " +- " + S(sigma) + "]" : S(""); }
};


struct HistoryItem 
{ 
    std::vector<fp> a; 
    std::string comment; 
    bool saved;
    HistoryItem (std::vector<fp> a_, std::string c) : a(a_), comment(c), 
                                                       saved(false) {}
};



// PlotCore manages multiple datasets, Sum, (and Crystal - it can be changed
// in future) and view
class PlotCore
{
    friend class ApplicationLogic; //will be removed later
public:
    static const fp relative_view_x_margin, relative_view_y_margin;
    Rect view;
    bool plus_background;

    PlotCore(Parameters *parameters);
    ~PlotCore();

    bool activate_data(int n); // for n=-1 create new dataset
    int append_data();
    std::vector<std::string> get_data_titles() const;
    int get_data_count() const { return datasets.size(); }
    const Data *get_data(int n) const; 
    int get_active_data_position() const { return active_data; }
    const Data *get_active_data() const { return get_data(active_data);}
    void set_my_vars();
    void remove_data(int n);
    void export_as_script (std::ostream& os) const;
    const Sum *get_sum() const { return sum; }

    std::string view_info() const;
    void set_view (Rect rect, bool fit = false);
    void set_view_h (fp l, fp r) {set_view (Rect(l, r, view.bottom, view.top));}
    void set_view_v (fp b, fp t) {set_view (Rect(view.left, view.right, b, t));}
    void set_view_y_fit();
    void set_plus_background(bool b) {plus_background=b; v_was_changed = true;}

    bool was_changed() const;  // return true when plot should be redrawed
    void was_plotted(); // called on redrawing 

private:
    ///each dataset (class Data) usually comes from one datafile
    std::vector<Data*> datasets;
    Sum *sum;
#ifdef USE_XTAL
    Crystal *crystal;
#endif //USE_XTAL
    int active_data; //position of selected dataset in vector<Data*> datasets
    bool ds_was_changed; //selection of dataset changed
    bool v_was_changed; //view was changed

    PlotCore (const PlotCore&); //disable
};



// Parameters contains vector of parameters that are subject to fitting,
// previous values of these parameters, domains of parameters etc.
// One instance of this class is shared among PlotCore's contained in one
// ApplicationLogic. Used mainly by Sum class.
class Parameters
{
public:
/*** history of a-parameter ***/
    Parameters();
    std::string print_history() const;
    std::string history_diff (std::vector<int> hist_items) const;
    void move_in_history (int k, bool relative);
    void toggle_history_item_saved(int k);
    int history_size() { return history.size(); }
    int history_position() { return hp; }
    const HistoryItem& history_item (int nr) 
                { assert (nr >= 0 && nr < size(history)); return history[nr]; }
    void write_avec (const std::vector<fp>& a, std::string comment, 
                                                        bool no_move = false);
    const std::vector<fp>& values() const { return history[hp].a; }
/*** a-parameter ***/
    int count_a() const { return nA; }
    fp get_a(int n) const { assert(0 <= n && n < nA); return values()[n]; }
    Pag add_a (fp value, Domain d);
    std::string info_a (int nr) const;
    int rm_a (int nr, bool silent = false);
    bool do_rm_a(int nr);
    const Domain& get_domain(int n) const 
        { assert (0 <= n && n < nA); return adomain[n]; }
    fp change_a (int nr, fp value, char c = '=', bool add_to_history = true); 
                // returns old value        //c: +,-,*,/,=,% (105 % == 1.05 *)
    void change_domain (int nr, Domain d);
    fp variation_of_a (int n, fp variat) const;
    int freeze(int nr, bool frozen);
    bool is_frozen (int n) const {assert(0 <= n && n < nA); return afrozen[n];}
    std::string frozen_info () const;
    int count_frozen() const;
    void was_plotted() { p_was_changed = false; }
    bool was_changed() const { return p_was_changed; }

    void export_as_script (std::ostream& os) const;

private:
    int nA;
    std::vector <HistoryItem> history;
    int hp; //current position in history
    std::vector<Domain> adomain;
    std::vector<bool> afrozen;
    bool p_was_changed;
};


extern PlotCore *my_core;

#endif 

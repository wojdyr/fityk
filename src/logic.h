// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#ifndef FITYK__LOGIC__H__
#define FITYK__LOGIC__H__

#include <string>
#include <memory>
#include "var.h"
#include "func.h"


class View 
{
public:
    enum {
        change_left = 1,
        change_right = 2,
        change_top = 4,
        change_bottom = 8,
        change_all = change_left|change_right|change_top|change_bottom,
        fit_left = 16,
        fit_right = 32,
        fit_horizontally=fit_left|fit_right,
        fit_top = 64,
        fit_bottom = 128,
        fit_vertically = fit_top|fit_bottom,
        fit_all= fit_horizontally|fit_vertically
    };
    static const fp relative_x_margin, relative_y_margin;
    fp left, right, bottom, top;

    View(fp l, fp r, fp b, fp t) : left(l), right(r), bottom(b), top(t) {}
    View() : left(0), right(0), bottom(0), top(0) {}
    fp width() const { return right - left; }
    fp height() const { return top - bottom; }
    std::string str() const;
    void set(fp l, fp r, fp b, fp t, int flag=change_all) 
                                      { set_h(l,r,flag); set_v(b,t,flag); }
    void set_h(fp l, fp r, int flag=change_all) { // set horizontal range
        if (flag&change_left) left=l;
        if (flag&change_right) right=r;
    }
    void set_v(fp b, fp t, int flag=change_all) { // vertical
        if (flag&change_top) top=t;
        if (flag&change_bottom) bottom=b;
    }
    void set_items(std::vector<Data*> const &dd, std::vector<Sum*> const &ss) 
                                                    { datas = dd; sums = ss; }
    void fit(int flag=fit_all); 
    void parse_and_set(std::vector<std::string> const& lrbt); 
protected:
    std::vector<Data*> datas; 
    std::vector<Sum*> sums; 
    void get_x_range(fp &x_min, fp &x_max);
    void get_y_range(fp &y_min, fp &y_max);
};


class Domain 
{ 
    bool set, ctr_set;
    fp ctr, sigma; 

public:
    Domain () : set(false), ctr_set(false) {}
    Domain (fp sigm) : set(true), ctr_set(false), sigma(sigm) {}
    Domain (fp c, fp sigm) : set(true), ctr_set(true), ctr(c), sigma(sigm) {}
    //Domain (pre_Domain &p) : set(p.set), ctr_set(p.ctr_set),//used in parser.y
    //                         ctr(p.ctr), sigma(p.sigma) {}
    bool is_set() const { return set; }
    bool is_ctr_set() const { return ctr_set; }
    fp Ctr() const { assert(set && ctr_set); return ctr; }
    fp Sigma() const { assert(set); return sigma; }
    std::string str() const 
        { return set ? "[" + (ctr_set ? S(ctr) : S()) 
                                         + " +- " + S(sigma) + "]" : S(""); }
};



class DataWithSum
{
public:
    bool visible_data;

    DataWithSum(VariableManager *mgr, Data* data=0);
    Data *get_data() const { return data.get(); } 
    Sum *get_sum() const { return sum.get(); }
    void export_as_script (std::ostream& os) const;

private:
    std::auto_ptr<Data> data;
    std::auto_ptr<Sum> sum;

    DataWithSum(DataWithSum const&); //disable
};


class ApplicationLogic : public VariableManager
{
public:
    View view;
    /// used for randomly drawing parameter values, in fitting methods like GA
    fp default_relative_domain_width;

    ApplicationLogic() :default_relative_domain_width(0.1)
                        { reset_all(); }

    ~ApplicationLogic() { reset_all(true); }
    void reset_all (bool finish=false); 
    void dump_all_as_script (std::string const &filename);

    void activate_ds(int d);
    int append_ds(Data *data=0)  { dsds.push_back(new DataWithSum(this, data)); 
                                   return dsds.size()-1; }
    void remove_ds(int d);
    void remove_ds(std::vector<int> const &dd) { 
        for (std::vector<int>::const_iterator i=dd.begin(); i != dd.end(); ++i) 
            remove_ds(*i);
    }
    int get_ds_count() const { return dsds.size(); }
    const DataWithSum *get_ds(int n) const { return dsds[n]; } 
    int get_active_ds_position() const { return active_ds; }
    const DataWithSum *get_active_ds() const { return get_ds(active_ds); }
    Data *get_data(int n);

protected:
    std::vector<DataWithSum*> dsds;
    int active_ds;
};


extern ApplicationLogic *AL;

#endif 

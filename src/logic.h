// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License version 2
// $Id$

#ifndef FITYK__LOGIC__H__
#define FITYK__LOGIC__H__

#include <string>
#include <memory>
#include <algorithm>
#include "mgr.h"
#include "ui.h" //Commands::Status


class DataWithSum;
class Settings;
class Ftk;
class UserInterface;
class FitMethodsContainer;
class Fit;

/// manages view, i.e. x and y intervals visible currently to the user 
/// can set view using string like "[20:][-100:1000]" 
/// most difficult part here is finding an automatic view for given data and sum
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
    /// set datasets that are to be used in fit()
    void set_datasets(std::vector<DataWithSum*> const& dd);
    /// fit specified edges to the data range 
    void fit(int flag=fit_all); 
    void parse_and_set(std::vector<std::string> const& lrbt); 
protected:
    std::vector<Data*> datas; 
    std::vector<Sum*> sums; 
    void get_x_range(fp &x_min, fp &x_max);
    void get_y_range(fp &y_min, fp &y_max);
};


/// keeps Data and its Sum
class DataWithSum
{
public:
    DataWithSum(Ftk *F, Data* data_=0);
    Data *get_data() const { return data.get(); } 
    Sum *get_sum() const { return sum.get(); }
    bool has_any_info() const;

private:
    std::auto_ptr<Data> data;
    std::auto_ptr<Sum> sum;

    DataWithSum(DataWithSum const&); //disable
};


/// keeps all functions, variables, parameters, datasets with sums and View
class Ftk : public VariableManager
{
public:
    View view;
    /// used for randomly drawing parameter values, in fitting methods like GA
    fp default_relative_domain_width;

    Ftk();
    ~Ftk();
    /// reset everything but UserInterface (and related settings)
    void reset();
    void dump_all_as_script (std::string const &filename);

    void activate_ds(int d);
    int append_ds(Data *data=0);
    void remove_ds(int d);
    int get_ds_count() const { return dsds.size(); }
    DataWithSum* get_ds(int n) { return dsds[check_ds_number(n)]; }
    DataWithSum const* get_ds(int n) const { return dsds[check_ds_number(n)]; }
    std::vector<DataWithSum*> const& get_dsds() const { return dsds; }
    int get_active_ds_position() const { return active_ds; }
    Data *get_data(int n) { return get_ds(n)->get_data(); }
    Sum *get_sum(int n)   { return get_ds(n)->get_sum(); }
    bool has_ds(DataWithSum const* p) const 
                      { return count(dsds.begin(), dsds.end(), p) > 0; }
    std::string find_function_name(std::string const &fstr) const;
    const Function* find_function_any(std::string const &fstr) const;

    Settings const* get_settings() const { return settings; }
    Settings* get_settings() { return settings; }

    UserInterface const* get_ui() const { return ui; }
    UserInterface* get_ui() { return ui; }

    FitMethodsContainer const* get_fit_container() const {return fit_container;}
    FitMethodsContainer* get_fit_container() { return fit_container; }
    Fit* get_fit(); 

    /// Send warning to UI. 
    void warn(std::string const &s) const;

    /// Send implicitely requested message to UI. 
    void rmsg(std::string const &s) const;

    /// Send message to UI. 
    void msg(std::string const &s) const; 

    /// Send verbose message to UI. 
    void vmsg(std::string const &s) const;

    int get_verbosity() const;

    /// execute command(s) from string
    Commands::Status exec(std::string const &s);

    /// import dataset (or multiple datasets, in special cases)
    void import_dataset(int slot, std::string const& filename, 
                        std::string const& type, std::vector<int> const& cols);

protected:
    std::vector<DataWithSum*> dsds;
    int active_ds;
    Settings* settings;
    UserInterface* ui;
    FitMethodsContainer* fit_container;

    void initialize();
    void destroy();

private:
    /// verify that n is the valid number for get_ds() and return n 
    int check_ds_number(int n) const;
};

extern Ftk* AL;

#endif 

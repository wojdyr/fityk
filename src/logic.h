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
#include "view.h" 


class Settings;
class Ftk;
class UserInterface;
class FitMethodsContainer;
class Fit;


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

    int append_ds(Data *data=0);
    void remove_ds(int d);
    int get_ds_count() const { return dsds.size(); }
    DataWithSum* get_ds(int n) { return dsds[check_ds_number(n)]; }
    DataWithSum const* get_ds(int n) const { return dsds[check_ds_number(n)]; }
    std::vector<DataWithSum*> const& get_dsds() const { return dsds; }
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

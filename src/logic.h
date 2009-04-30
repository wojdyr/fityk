// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License version 2 or (at your option) 3
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
class Model;


/// keeps Data and its Model
class DataAndModel
{
public:
    DataAndModel(Ftk *F, Data* data=NULL);
    Data *data() const { return data_.get(); } 
    Model *model() const { return model_.get(); }
    bool has_any_info() const;

private:
    std::auto_ptr<Data> data_;
    std::auto_ptr<Model> model_;

    DataAndModel(DataAndModel const&); //disable
};


/// keeps all functions, variables, parameters, datasets with models and View
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

    int append_dm(Data *data=0);
    void remove_dm(int d);
    int get_dm_count() const { return dms.size(); }
    DataAndModel* get_dm(int n) { return dms[check_dm_number(n)]; }
    DataAndModel const* get_dm(int n) const { return dms[check_dm_number(n)]; }
    std::vector<DataAndModel*> const& get_dms() const { return dms; }
    Data *get_data(int n) { return get_dm(n)->data(); }
    Model *get_model(int n)   { return get_dm(n)->model(); }
    bool contains_dm(DataAndModel const* p) const 
                      { return count(dms.begin(), dms.end(), p) > 0; }
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
                        std::vector<std::string> const& options);

protected:
    std::vector<DataAndModel*> dms;
    Settings* settings;
    UserInterface* ui;
    FitMethodsContainer* fit_container;

    void initialize();
    void destroy();

private:
    /// verify that n is the valid number for get_dm() and return n 
    int check_dm_number(int n) const;
};

extern Ftk* AL;

#endif 

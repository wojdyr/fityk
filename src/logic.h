// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
// $Id$

#ifndef FITYK__LOGIC__H__
#define FITYK__LOGIC__H__

#include <string>
#include <memory>
#include <algorithm>
#include "mgr.h"
#include "ui.h" //Commands::Status
#include "view.h"
#include "settings.h"


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

    int append_dm(Data *data=0);
    void remove_dm(int d);

    std::vector<DataAndModel*> const& get_dms() const { return dms_; }
    int get_dm_count() const { return dms_.size(); }

    DataAndModel* get_dm(int n) { return dms_[check_dm_number(n)]; }
    DataAndModel const* get_dm(int n) const { return dms_[check_dm_number(n)]; }

    Data const* get_data(int n) const { return get_dm(n)->data(); }
    Data *get_data(int n) { return get_dm(n)->data(); }

    Model const* get_model(int n) const { return get_dm(n)->model(); }
    Model *get_model(int n)   { return get_dm(n)->model(); }

    bool contains_dm(DataAndModel const* p) const
                      { return count(dms_.begin(), dms_.end(), p) > 0; }

    Settings const* get_settings() const { return settings_; }
    Settings* get_settings() { return settings_; }

    UserInterface const* get_ui() const { return ui_; }
    UserInterface* get_ui() { return ui_; }

    FitMethodsContainer const* get_fit_container() const
        { return fit_container_; }
    FitMethodsContainer* get_fit_container() { return fit_container_; }
    Fit* get_fit() const;

    /// Send warning to UI.
    void warn(std::string const &s) const;

    /// Send implicitely requested message to UI.
    void rmsg(std::string const &s) const;

    /// Send message to UI.
    void msg(std::string const &s) const;

    /// Send verbose message to UI.
    void vmsg(std::string const &s) const;

    int get_verbosity() const { return settings_->get_verbosity(); }

    /// execute command(s) from string
    Commands::Status exec(std::string const &s);

    /// import dataset (or multiple datasets, in special cases)
    void import_dataset(int slot, std::string const& filename,
                        std::string const& format, std::string const& options);

    /// called after changes that (possibly) need to be reflected in the plot
    /// (IOW when plot needs to be updated). This function is also used
    /// to mark cache of parameter errors as outdated.
    void outdated_plot();

    /// called after replotting 
    void updated_plot() { dirty_plot_ = false; }

    /// returns true if the plot should be replotted
    bool is_plot_outdated() { return dirty_plot_; }

private:
    std::vector<DataAndModel*> dms_;
    Settings* settings_;
    UserInterface* ui_;
    FitMethodsContainer* fit_container_;
    bool dirty_plot_;

    void initialize();
    void destroy();
    /// verify that n is the valid number for get_dm() and return n
    int check_dm_number(int n) const;
};

//extern Ftk* AL;

#endif

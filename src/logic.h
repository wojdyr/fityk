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
class TplateMgr;


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

    const std::vector<DataAndModel*>& get_dms() const { return dms_; }
    int get_dm_count() const { return dms_.size(); }

    DataAndModel* get_dm(int n) { return dms_[check_dm_number(n)]; }
    const DataAndModel* get_dm(int n) const { return dms_[check_dm_number(n)]; }

    const Data* get_data(int n) const { return get_dm(n)->data(); }
    Data *get_data(int n) { return get_dm(n)->data(); }

    const Model* get_model(int n) const { return get_dm(n)->model(); }
    Model *get_model(int n)   { return get_dm(n)->model(); }

    bool contains_dm(const DataAndModel* p) const
                      { return count(dms_.begin(), dms_.end(), p) > 0; }

    int default_dm() const { return 0; }

    const SettingsMgr* settings_mgr() const { return settings_mgr_; }
    SettingsMgr* settings_mgr() { return settings_mgr_; }

    const Settings* get_settings() const { return &settings_mgr_->m(); }

    const UserInterface* get_ui() const { return ui_; }
    UserInterface* get_ui() { return ui_; }

    const FitMethodsContainer* get_fit_container() const
        { return fit_container_; }
    FitMethodsContainer* get_fit_container() { return fit_container_; }
    Fit* get_fit() const;

    const TplateMgr* get_tpm() const { return tplate_mgr_; }
    TplateMgr* get_tpm() { return tplate_mgr_; }

    /// Send warning to UI.
    void warn(std::string const &s) const;

    /// Send implicitely requested message to UI.
    void rmsg(std::string const &s) const;

    /// Send message to UI.
    void msg(std::string const &s) const;

    /// Send verbose message to UI.
    void vmsg(std::string const &s) const;

    int get_verbosity() const { return get_settings()->verbosity; }

    /// execute command(s) from string
    Commands::Status exec(const std::string &s);

    /// import dataset (or multiple datasets, in special cases)
    void import_dataset(int slot, const std::string& filename,
                        const std::string& format, const std::string& options);

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
    SettingsMgr* settings_mgr_;
    UserInterface* ui_;
    FitMethodsContainer* fit_container_;
    TplateMgr* tplate_mgr_;
    bool dirty_plot_;

    void initialize();
    void destroy();
    /// verify that n is the valid number for get_dm() and return n
    int check_dm_number(int n) const;
};

#endif

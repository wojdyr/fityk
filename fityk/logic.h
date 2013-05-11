// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#ifndef FITYK__LOGIC__H__
#define FITYK__LOGIC__H__

#include <string>
#include <boost/scoped_ptr.hpp>
#include <algorithm>
#include "mgr.h"
#include "ui.h" //UserInterface::Status
#include "view.h"
#include "settings.h"

namespace fityk {

class BasicContext
{
public:
    //BasicContext(UserInterface *ui)
    //    : settings_mgr_(new SettingsMgr(this)), ui_(ui) {}
    //~BasicContext() { delete settings_mgr; }
    const SettingsMgr* settings_mgr() const { return settings_mgr_; }
    SettingsMgr* settings_mgr() { return settings_mgr_; }

    const UserInterface* ui() const { return ui_; }
    UserInterface* ui() { return ui_; }

    // short names for popular calls
    int get_verbosity() const { return get_settings()->verbosity; }
    void msg(const std::string &s) const
                                { if (get_verbosity() >= 0) ui_->mesg(s); }
    const Settings* get_settings() const { return &settings_mgr_->m(); }
protected:
    SettingsMgr* settings_mgr_;
    UserInterface* ui_;
};

class Ftk;
class FitManager;
class Fit;
class Model;
class TplateMgr;
class LuaBridge;


/// keeps Data and its Model
class DataAndModel
{
public:
    DataAndModel(Ftk *F, Data* data=NULL);
    Data *data() const { return data_.get(); }
    Model *model() const { return model_.get(); }
    bool has_any_info() const;

private:
    boost::scoped_ptr<Data> data_;
    boost::scoped_ptr<Model> model_;

    DataAndModel(DataAndModel const&); //disable
};


/// keeps all functions, variables, parameters, datasets with models and View
class FITYK_API Ftk : public BasicContext
{
public:
    ModelManager mgr;
    View view;

    Ftk();
    ~Ftk();
    /// reset everything but UserInterface (and related settings)
    void reset();

    int append_dm(Data *data=0);
    void remove_dm(int d);

    const std::vector<DataAndModel*>& get_dms() const { return dms_; }
    int get_dm_count() const { return dms_.size(); }

    DataAndModel* get_dm(int n) { check_dm_number(n); return dms_[n]; }
    const DataAndModel* get_dm(int n) const {check_dm_number(n);return dms_[n];}

    const Data* get_data(int n) const { return get_dm(n)->data(); }
    Data *get_data(int n) { return get_dm(n)->data(); }

    const Model* get_model(int n) const { return get_dm(n)->model(); }
    Model *get_mutable_model(int n)   { return get_dm(n)->model(); }

    int default_dm() const { return default_dm_; }
    void set_default_dm(int n) { check_dm_number(n); default_dm_ = n; }

    const FitManager* fit_manager() const { return fit_manager_; }
    FitManager* fit_manager() { return fit_manager_; }
    Fit* get_fit() const;

    const TplateMgr* get_tpm() const { return tplate_mgr_; }
    TplateMgr* get_tpm() { return tplate_mgr_; }

    LuaBridge* lua_bridge() { return lua_bridge_; }

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
    bool is_plot_outdated() const { return dirty_plot_; }

    // check if given models share common parameters
    bool are_independent(std::vector<DataAndModel*> dms) const;

    // interprets command-line argument as data or script file or as command
    void process_cmd_line_arg(const std::string& arg);

private:
    int default_dm_;
    std::vector<DataAndModel*> dms_;
    FitManager* fit_manager_;
    TplateMgr* tplate_mgr_;
    LuaBridge* lua_bridge_;
    bool dirty_plot_;

    void initialize();
    void destroy();

    /// verify that n is the valid number for get_dm() and return n
    void check_dm_number(int n) const
    {
        if (!is_index(n, dms_))
            throw ExecuteError("No such dataset: @" + S(n));
    }

    DISALLOW_COPY_AND_ASSIGN(Ftk);
};

} // namespace fityk
#endif

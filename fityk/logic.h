// This file is part of fityk program. Copyright 2001-2013 Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#ifndef FITYK__LOGIC__H__
#define FITYK__LOGIC__H__

#include <string>
#include "mgr.h"
#include "ui.h" //UserInterface
#include "view.h"
#include "settings.h"
#include "data.h" // Data.model()

namespace fityk {

class FitManager;
class Fit;
class TplateMgr;
class LuaBridge;
class CommandExecutor;

class FITYK_API DataKeeper
{
public:
    DataKeeper() : default_idx_(0) {}
    void append(Data *data) { datas_.push_back(data); }
    void remove(int d);
    void clear() { purge_all_elements(datas_); }

    const std::vector<Data*>& datas() const { return datas_; }
    int count() const { return datas_.size(); }

    Data* data(int n) { index_check(n); return datas_[n]; }
    const Data* data(int n) const { index_check(n); return datas_[n]; }

    const Model* get_model(int n) const { return data(n)->model(); }
    Model *get_mutable_model(int n) { return data(n)->model(); }

    int default_idx() const { return default_idx_; }
    void set_default_idx(int n) { index_check(n); default_idx_ = n; }

    /// import dataset (or multiple datasets, in special cases)
    void import_dataset(int slot, const std::string& data_path,
                        const std::string& format, const std::string& options,
                        BasicContext* ctx, ModelManager &mgr);
    void do_import_dataset(bool new_dataset, int slot,
                        const std::string& filename,
                        int idx_x, int idx_y, int idx_s,
                        const std::vector<int>& block_range,
                        const std::string& format, const std::string& options,
                        BasicContext* ctx, ModelManager &mgr);

private:
    int default_idx_;
    std::vector<Data*> datas_;

    /// verify that n is the valid number for get_data() and return n
    void index_check(int n) const
    {
        if (!is_index(n, datas_))
            throw ExecuteError("No such dataset: @" + S(n));
    }

};

/// A restricted interface to class Full. Provides read-only access to
/// settings and access to UserInterface, primarily for printing messages.
class FITYK_API BasicContext
{
public:
    const UserInterface* ui() const { return ui_; }
    UserInterface* ui() { return ui_; }

    // short names for popular calls
    int get_verbosity() const { return get_settings()->verbosity; }
    void msg(const std::string &s) const
                                { if (get_verbosity() >= 0) ui_->mesg(s); }
    const SettingsMgr* settings_mgr() const { return settings_mgr_; }
    const Settings* get_settings() const { return &settings_mgr_->m(); }

protected:
    SettingsMgr* settings_mgr_;
    UserInterface* ui_;
};

/// Full context.
/// Contains everything in libfityk except public API.
class FITYK_API Full : public BasicContext
{
public:
    // for simplicity, these members are public and accessed directly
    ModelManager mgr;
    DataKeeper dk;
    View view;

    Full();
    ~Full();
    /// reset everything but UserInterface (and related settings)
    void reset();

    SettingsMgr* mutable_settings_mgr() { return settings_mgr_; }

    const FitManager* fit_manager() const { return fit_manager_; }
    FitManager* fit_manager() { return fit_manager_; }
    Fit* get_fit() const;

    const TplateMgr* get_tpm() const { return tplate_mgr_; }
    TplateMgr* get_tpm() { return tplate_mgr_; }

    LuaBridge* lua_bridge() { return lua_bridge_; }

    /// called after changes that (possibly) need to be reflected in the plot
    /// (IOW when plot needs to be updated). This function is also used
    /// to mark cache of parameter errors as outdated.
    void outdated_plot();

    // check if given models share common parameters
    bool are_independent(std::vector<Data*> dd) const;

    // interprets command-line argument as data or script file or as command
    void process_cmd_line_arg(const std::string& arg);

    /// return true if the syntax is correct
    bool check_syntax(const std::string& str);

    void parse_and_execute_line(const std::string& str);

private:
    // these members, as well as settings_mgr_ and ui_, are used via getters
    FitManager* fit_manager_;
    TplateMgr* tplate_mgr_;
    LuaBridge* lua_bridge_;
    CommandExecutor* cmd_executor_;

    // these two are used in ctor, dtor and reset()
    void initialize();
    void destroy();

    DISALLOW_COPY_AND_ASSIGN(Full);
};

} // namespace fityk
#endif

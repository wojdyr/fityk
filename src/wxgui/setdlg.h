// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
// $Id$

#ifndef FITYK__WX_SETDLG__H__
#define FITYK__WX_SETDLG__H__

#include <vector>
#include <string>
#include <utility>
#include "cmn.h" //s2wx, RealNumberCtrl

class SpinCtrl;

class SettingsDlg : public wxDialog
{
public:
    typedef std::vector<std::pair<std::string, std::string> > pair_vec;
    SettingsDlg(wxWindow* parent, const wxWindowID id);
    void OnChangeButton(wxCommandEvent& event);
    void OnOK(wxCommandEvent& event);
    pair_vec get_changed_items();
private:
    wxChoice *sigma_ch, *verbosity_ch, *export_f_ch, *nm_distrib;
    wxCheckBox *exit_cb;
    SpinCtrl *delay_sp, *seed_sp, *mwssre_sp;
    RealNumberCtrl *cut_func, *eps_rc, *height_correction, *width_correction,
                   *domain_p, *lm_lambda_ini, *lm_lambda_up, *lm_lambda_down,
                   *lm_stop, *lm_max_lambda,
                   *nm_convergence, *nm_move_factor;
    wxCheckBox *cancel_poos, *nm_move_all, *autoplot_cb;
    wxTextCtrl *dir_ld_tc, *dir_xs_tc, *dir_ex_tc;

    void add_persistence_note(wxWindow *parent, wxSizer *sizer);

    DECLARE_EVENT_TABLE()
};

#endif

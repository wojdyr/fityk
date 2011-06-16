// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#ifndef FITYK__WX_SETDLG__H__
#define FITYK__WX_SETDLG__H__

#include <vector>
#include <string>
#include <utility>
#include "cmn.h" //s2wx, RealNumberCtrl

class SettingsDlg : public wxDialog
{
public:
    SettingsDlg(wxWindow* parent);
    void OnOK(wxCommandEvent& event);
private:
    wxChoice *sigma_ch, *nm_distrib;
    wxCheckBox *exit_cb;
    wxSpinCtrl *delay_sp, *seed_sp, *mwssre_sp, *verbosity_sp;
    RealNumberCtrl *cut_func, *eps_rc, *height_correction, *width_correction,
                   *domain_p, *fit_max_time,
                   *lm_lambda_ini, *lm_lambda_up, *lm_lambda_down,
                   *lm_stop, *lm_max_lambda,
                   *nm_convergence, *nm_move_factor;
    wxCheckBox *nm_move_all, *fit_replot_cb;
    wxTextCtrl *format_tc;

    void exec_set_command();
};

#endif

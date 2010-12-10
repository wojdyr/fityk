// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
// $Id$

/// In this file:
///  Settings Dialog (SettingsDlg)

#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/statline.h>
#include <wx/notebook.h>
#include <wx/config.h>

#include "setdlg.h"
#include "../settings.h"
#include "../ui.h"  //startup_commands_filename
#include "../logic.h"
#include "frame.h"  //ftk

using namespace std;


enum {
    ID_SET_LDBUT           = 26400,
    ID_SET_XSBUT                  ,
    ID_SET_EXBUT                  ,
};


BEGIN_EVENT_TABLE(SettingsDlg, wxDialog)
    EVT_BUTTON (ID_SET_LDBUT, SettingsDlg::OnChangeButton)
    EVT_BUTTON (ID_SET_XSBUT, SettingsDlg::OnChangeButton)
    EVT_BUTTON (ID_SET_EXBUT, SettingsDlg::OnChangeButton)
    EVT_BUTTON (wxID_OK, SettingsDlg::OnOK)
END_EVENT_TABLE()

namespace {

RealNumberCtrl *addRealNumberCtrl(wxWindow *parent, wxString const& label,
                                  string const& value, wxSizer *sizer,
                                  wxString const& label_after=wxString())
{
    wxStaticText *st = new wxStaticText(parent, -1, label);
    RealNumberCtrl *ctrl = new RealNumberCtrl(parent, -1, value);
    wxBoxSizer *hsizer = new wxBoxSizer(wxHORIZONTAL);
    hsizer->Add(st, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    hsizer->Add(ctrl, 0, wxALL, 5);
    if (!label_after.IsEmpty()) {
        wxStaticText *sta = new wxStaticText(parent, -1, label_after);
        hsizer->Add(sta, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    }
    sizer->Add(hsizer, 0, wxEXPAND);
    return ctrl;
}

wxCheckBox *addCheckbox(wxWindow *parent, wxString const& label,
                        bool value, wxSizer *sizer)
{
    wxCheckBox *ctrl = new wxCheckBox(parent, -1, label);
    ctrl->SetValue(value);
    if (sizer)
        sizer->Add(ctrl, 0, wxEXPAND|wxALL, 5);
    return ctrl;
}

wxChoice *addEnumSetting(wxWindow *parent, wxString const& label,
                         string const& option, wxSizer* sizer)
{
    wxBoxSizer *siz = new wxBoxSizer(wxHORIZONTAL);
    wxStaticText *st = new wxStaticText(parent, -1, label);
    siz->Add(st, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    wxChoice *ctrl = new wxChoice(parent, -1,
                                  wxDefaultPosition, wxDefaultSize,
                                  stl2wxArrayString(
                                    ftk->get_settings()->expand_enum(option)));
    ctrl->SetStringSelection(s2wx(ftk->get_settings()->getp(option)));
    siz->Add(ctrl, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    sizer->Add(siz, 0, wxEXPAND);
    return ctrl;
}

} //anonymous namespace

SettingsDlg::SettingsDlg(wxWindow* parent, const wxWindowID id)
    : wxDialog(parent, id, wxT("Settings"),
               wxDefaultPosition, wxDefaultSize,
               wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER)
{
    Settings const* settings = ftk->get_settings();
    wxNotebook *nb = new wxNotebook(this, -1);
    wxPanel *page_general = new wxPanel(nb, -1);
    nb->AddPage(page_general, wxT("general"));
    wxPanel *page_peakfind = new wxPanel(nb, -1);
    nb->AddPage(page_peakfind, wxT("peak-finding"));
    wxNotebook *page_fitting = new wxNotebook(nb, -1);
    nb->AddPage(page_fitting, wxT("fitting"));
    wxPanel *page_dirs = new wxPanel(nb, -1);
    nb->AddPage(page_dirs, wxT("directories"));

    // page general
    wxBoxSizer *sizer_general = new wxBoxSizer(wxVERTICAL);

    sigma_ch = addEnumSetting(page_general, wxT("default std. dev. of data y:"),
                              "data_default_sigma", sizer_general);
    cut_func = addRealNumberCtrl(page_general,
                                 wxT("f(x) can be assumed 0, if |f(x)|<"),
                                 settings->cut_function_level(),
                                 sizer_general);

    verbosity_sp = new SpinCtrl(page_general, -1, settings->verbosity(),
                                wxT("verbosity (in output pane)"), -1, 2,
                                70);
    //TODO
    //
    exit_cb = addCheckbox(page_general,
                          wxT("quit if error or warning was generated"),
                          settings->exit_on_warning(),
                          sizer_general);

    wxStaticText *seed_st = new wxStaticText(page_general, -1,
                        wxT("pseudo-random generator seed (0 = time-based)"));
    seed_sp = new SpinCtrl(page_general, -1,
                           settings->pseudo_random_seed(), 0, 999999,
                           70);
    wxBoxSizer *sizer_general_seed = new wxBoxSizer(wxHORIZONTAL);
    sizer_general_seed->Add(seed_st, 1, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    sizer_general_seed->Add(seed_sp, 0, wxTOP|wxBOTTOM|wxRIGHT, 5);
    sizer_general->Add(sizer_general_seed, 0, wxEXPAND);

    eps_rc = addRealNumberCtrl(page_general,
                               wxT("epsilon for floating-point comparison"),
                               settings->epsilon(),
                               sizer_general);
    add_persistence_note(page_general, sizer_general);
    page_general->SetSizerAndFit(sizer_general);

    // page peak-finding
    wxBoxSizer *sizer_pf = new wxBoxSizer(wxVERTICAL);

    height_correction = addRealNumberCtrl(page_peakfind,
                           wxT("factor used to correct detected peak height"),
                           settings->height_correction(),
                           sizer_pf);
    width_correction = addRealNumberCtrl(page_peakfind,
                           wxT("factor used to correct detected peak width"),
                           settings->width_correction(),
                           sizer_pf);

    cancel_poos = addCheckbox(page_peakfind,
                          wxT("cancel peak guess, if the result is doubtful"),
                          settings->can_cancel_guess(),
                          sizer_pf);
    //sizer_pf->Add(cancel_poos, 0, wxALL, 5);

    add_persistence_note(page_peakfind, sizer_pf);
    page_peakfind->SetSizerAndFit(sizer_pf);

    // page fitting
    wxPanel *page_fit_common = new wxPanel(page_fitting, -1);
    page_fitting->AddPage(page_fit_common, wxT("common"));
    wxPanel *page_fit_LM = new wxPanel(page_fitting, -1);
    page_fitting->AddPage(page_fit_LM, wxT("Lev-Mar"));
    wxPanel *page_fit_NM = new wxPanel(page_fitting, -1);
    page_fitting->AddPage(page_fit_NM, wxT("Nelder-Mead"));
    wxPanel *page_fit_GA = new wxPanel(page_fitting, -1);
    page_fitting->AddPage(page_fit_GA, wxT("GA"));

    // sub-page common
    wxBoxSizer *sizer_fcmn = new wxBoxSizer(wxVERTICAL);
    wxStaticBoxSizer *sizer_fcstop = new wxStaticBoxSizer(wxHORIZONTAL,
                                page_fit_common, wxT("termination criteria"));
    wxStaticText *mwssre_st = new wxStaticText(page_fit_common, -1,
                                               wxT("max. WSSR evaluations"));
    mwssre_sp = new SpinCtrl(page_fit_common, -1,
                             settings->max_wssr_evaluations(), 0, 999999,
                             70);
    sizer_fcstop->Add(mwssre_st, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    sizer_fcstop->Add(mwssre_sp, 0, wxALL, 5);
    sizer_fcmn->Add(sizer_fcstop, 0, wxEXPAND|wxALL, 5);

    domain_p = addRealNumberCtrl(page_fit_common,
                                 wxT("default domain of variable is +/-"),
                                 settings->variable_domain_percent(),
                                 sizer_fcmn,
                                 wxT("%"));

    autoplot_cb = addCheckbox(page_fit_common,
                              wxT("refresh plot after each iteration"),
                              settings->fit_replot(), sizer_fcmn);

    wxStaticText *delay_st = new wxStaticText(page_fit_common, -1,
                            wxT("time (in sec.) between stats/plot updates"));
    delay_sp = new SpinCtrl(page_fit_common, -1,
                            settings->refresh_period(), -1, 9999,
                            70);
    wxBoxSizer *delay_sizer = new wxBoxSizer(wxHORIZONTAL);
    delay_sizer->Add(delay_st, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    delay_sizer->Add(delay_sp, 0, wxALL, 5);
    sizer_fcmn->Add(delay_sizer, 0, wxEXPAND|wxALL, 5);

    add_persistence_note(page_fit_common, sizer_fcmn);
    page_fit_common->SetSizerAndFit(sizer_fcmn);

    // sub-page Lev-Mar
    wxBoxSizer *sizer_flm = new wxBoxSizer(wxVERTICAL);
    wxStaticBoxSizer *sizer_flmlambda = new wxStaticBoxSizer(wxVERTICAL,
                                page_fit_LM, wxT("lambda parameter"));
    lm_lambda_ini = addRealNumberCtrl(page_fit_LM, wxT("initial value"),
                         settings->lm_lambda_start(), sizer_flmlambda);
    lm_lambda_up = addRealNumberCtrl(page_fit_LM, wxT("increasing factor"),
                         settings->lm_lambda_up_factor(), sizer_flmlambda);
    lm_lambda_down = addRealNumberCtrl(page_fit_LM, wxT("decreasing factor"),
                         settings->lm_lambda_down_factor(), sizer_flmlambda);
    sizer_flm->Add(sizer_flmlambda, 0, wxEXPAND|wxALL, 5);
    wxStaticBoxSizer *sizer_flmstop = new wxStaticBoxSizer(wxVERTICAL,
                                page_fit_LM, wxT("termination criteria"));
    lm_stop = addRealNumberCtrl(page_fit_LM, wxT("WSSR relative change <"),
                         settings->lm_stop_rel_change(), sizer_flmstop);
    lm_max_lambda = addRealNumberCtrl(page_fit_LM, wxT("max. value of lambda"),
                         settings->lm_max_lambda(), sizer_flmstop);
    sizer_flm->Add(sizer_flmstop, 0, wxEXPAND|wxALL, 5);
    add_persistence_note(page_fit_LM, sizer_flm);
    page_fit_LM->SetSizerAndFit(sizer_flm);

    // sub-page N-M
    wxBoxSizer *sizer_fnm = new wxBoxSizer(wxVERTICAL);

    wxStaticBoxSizer *sizer_fnmini = new wxStaticBoxSizer(wxVERTICAL,
                              page_fit_NM, wxT("simplex initialization"));

    nm_move_all = addCheckbox(page_fit_NM,
                           wxT("randomize all vertices (otherwise on is left)"),
                           settings->nm_move_all(), sizer_fnmini);
    nm_distrib = addEnumSetting(page_fit_NM, wxT("distribution type"),
                                "nm_distribution", sizer_fnmini);
    nm_move_factor = addRealNumberCtrl(page_fit_NM,
                         wxT("factor by which domain is expanded"),
                         settings->nm_move_factor(), sizer_fnmini);
    sizer_fnm->Add(sizer_fnmini, 0, wxEXPAND|wxALL, 5);

    wxStaticBoxSizer *sizer_fnmstop = new wxStaticBoxSizer(wxVERTICAL,
                                page_fit_NM, wxT("termination criteria"));
    nm_convergence = addRealNumberCtrl(page_fit_NM,
                         wxT("worst and best relative difference"),
                         settings->nm_convergence(), sizer_fnmstop);
    sizer_fnm->Add(sizer_fnmstop, 0, wxEXPAND|wxALL, 5);
    add_persistence_note(page_fit_NM, sizer_fnm);
    page_fit_NM->SetSizerAndFit(sizer_fnm);

    // TODO: sub-page GA

    // page directories
    wxBoxSizer *sizer_dirs = new wxBoxSizer(wxVERTICAL);
    wxStaticBoxSizer *sizer_dirs_data = new wxStaticBoxSizer(wxHORIZONTAL,
                     page_dirs, wxT("default directory for load data dialog"));
    dir_ld_tc = new wxTextCtrl(page_dirs, -1,
                           wxConfig::Get()->Read(wxT("/loadDataDir"), wxT("")));
    sizer_dirs_data->Add(dir_ld_tc, 1, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    sizer_dirs_data->Add(new wxButton(page_dirs, ID_SET_LDBUT, wxT("Change")),
                         0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    sizer_dirs->Add(sizer_dirs_data, 0, wxEXPAND|wxALL, 5);

    wxStaticBoxSizer *sizer_dirs_script = new wxStaticBoxSizer(wxHORIZONTAL,
                page_dirs, wxT("default directory for execute script dialog"));
    dir_xs_tc = new wxTextCtrl(page_dirs, -1,
                         wxConfig::Get()->Read(wxT("/execScriptDir"), wxT("")));
    sizer_dirs_script->Add(dir_xs_tc, 1, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    sizer_dirs_script->Add(new wxButton(page_dirs, ID_SET_XSBUT, wxT("Change")),
                           0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    sizer_dirs->Add(sizer_dirs_script, 0, wxEXPAND|wxALL, 5);

    wxStaticBoxSizer *sizer_dirs_export = new wxStaticBoxSizer(wxHORIZONTAL,
                page_dirs, wxT("default directory for save/export dialogs"));
    dir_ex_tc = new wxTextCtrl(page_dirs, -1,
                         wxConfig::Get()->Read(wxT("/exportDir"), wxT("")));
    sizer_dirs_export->Add(dir_ex_tc, 1, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    sizer_dirs_export->Add(new wxButton(page_dirs, ID_SET_EXBUT, wxT("Change")),
                           0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    sizer_dirs->Add(sizer_dirs_export, 0, wxEXPAND|wxALL, 5);

    sizer_dirs->Add(new wxStaticText(page_dirs, -1,
                 wxT("Directories given above are used when the dialogs\n")
                 wxT("are displayed first time after launching the program.")),
                 0, wxALL|wxEXPAND, 5);
    page_dirs->SetSizerAndFit(sizer_dirs);

    //finish layout
    wxBoxSizer *top_sizer = new wxBoxSizer (wxVERTICAL);
    top_sizer->Add(nb, 1, wxALL|wxEXPAND, 10);
    top_sizer->Add (new wxStaticLine(this, -1), 0, wxEXPAND|wxLEFT|wxRIGHT, 5);
    top_sizer->Add (CreateButtonSizer (wxOK|wxCANCEL),
                    0, wxALL|wxALIGN_CENTER, 5);
    SetSizerAndFit(top_sizer);
}

void SettingsDlg::add_persistence_note(wxWindow *parent, wxSizer *sizer)
{
    wxStaticBoxSizer *persistence = new wxStaticBoxSizer(wxHORIZONTAL,
                                           parent, wxT("persistance note"));
    persistence->Add(new wxStaticText(parent, -1,
                       wxT("To have values above remained after restart, ")
                       wxT("put proper\ncommands into init file: ")
                       + get_conf_file(startup_commands_filename)),
                     0, wxALL|wxALIGN_CENTER, 5);
    sizer->AddStretchSpacer();
    sizer->Add(persistence, 0, wxEXPAND|wxALL, 5);
}

void SettingsDlg::OnChangeButton(wxCommandEvent& event)
{
    wxTextCtrl *tc = 0;
    if (event.GetId() == ID_SET_LDBUT)
        tc = dir_ld_tc;
    else if (event.GetId() == ID_SET_XSBUT)
        tc = dir_xs_tc;
    else if (event.GetId() == ID_SET_EXBUT)
        tc = dir_ex_tc;
    wxString dir = wxDirSelector(wxT("Choose a folder"), tc->GetValue());
    if (!dir.empty())
        tc->SetValue(dir);
}

SettingsDlg::pair_vec SettingsDlg::get_changed_items()
{
    pair_vec result;
    map<string, string> m;
    m["data_default_sigma"] = wx2s(sigma_ch->GetStringSelection());
    m["cut_function_level"] = wx2s(cut_func->GetValue());
    m["verbosity"] = wx2s(verbosity_ch->GetStringSelection());
    m["exit_on_warning"] = exit_cb->GetValue() ? "1" : "0";
    m["pseudo_random_seed"] = S(seed_sp->GetValue());
    m["epsilon"] = wx2s(eps_rc->GetValue());
    m["height_correction"] = wx2s(height_correction->GetValue());
    m["width_correction"] = wx2s(width_correction->GetValue());
    m["can_cancel_guess"] = cancel_poos->GetValue() ? "1" : "0";
    m["max_wssr_evaluations"] = S(mwssre_sp->GetValue());
    m["variable_domain_percent"] = wx2s(domain_p->GetValue());
    m["autoplot"] = autoplot_cb->GetValue() ? "on_fit_iteration"
                                            : "on_plot_change";
    m["refresh_period"] = S(delay_sp->GetValue());
    m["lm_lambda_start"] = wx2s(lm_lambda_ini->GetValue());
    m["lm_lambda_up_factor"] = wx2s(lm_lambda_up->GetValue());
    m["lm_lambda_down_factor"] = wx2s(lm_lambda_down->GetValue());
    m["lm_stop_rel_change"] = wx2s(lm_stop->GetValue());
    m["lm_max_lambda"] = wx2s(lm_max_lambda->GetValue());
    m["nm_move_all"] = nm_move_all->GetValue() ? "1" : "0";
    m["nm_distribution"] = wx2s(nm_distrib->GetStringSelection());
    m["nm_move_factor"] = wx2s(nm_move_factor->GetValue());
    m["nm_convergence"] = wx2s(nm_convergence->GetValue());
    vector<string> kk = ftk->get_settings()->get_key_list();
    for (vector<string>::const_iterator i = kk.begin(); i != kk.end(); ++i)
        if (m.count(*i) && m[*i] != ftk->get_settings()->getp(*i))
            result.push_back(make_pair(*i, m[*i]));
    return result;
}

void SettingsDlg::OnOK(wxCommandEvent&)
{
    vector<pair<string, string> > p = get_changed_items();
    if (!p.empty()) {
        vector<string> eqs;
        for (vector<pair<string, string> >::const_iterator i = p.begin();
                i != p.end(); ++i)
            eqs.push_back(i->first + "=" + i->second);
        ftk->exec("set " + join_vector(eqs, ", "));
    }
    wxConfig::Get()->Write(wxT("/loadDataDir"), dir_ld_tc->GetValue());
    wxConfig::Get()->Write(wxT("/execScriptDir"), dir_xs_tc->GetValue());
    wxConfig::Get()->Write(wxT("/exportDir"), dir_ex_tc->GetValue());
    close_it(this, wxID_OK);
}



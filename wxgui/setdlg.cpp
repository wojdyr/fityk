// This file is part of fityk program. Copyright 2001-2013 Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

///  Settings Dialog (SettingsDlg)

#include <wx/wx.h>
#include <wx/statline.h>
#include <wx/notebook.h>
#include <wx/config.h>
#include <wx/filepicker.h>

#include "setdlg.h"
#include "fityk/settings.h"
#include "fityk/logic.h" // settings_mgr()
#include "frame.h"  //ftk

using namespace std;

namespace {

RealNumberCtrl *addRealNumberCtrl(wxWindow *parent, const wxString& label,
                                  double value, wxSizer *sizer,
                                  int indentation=0,
                                  bool percent=false)
{
    wxStaticText *st = new wxStaticText(parent, -1, label);
    RealNumberCtrl *ctrl = new RealNumberCtrl(parent, -1, value);
    wxBoxSizer *hsizer = new wxBoxSizer(wxHORIZONTAL);
    if (indentation)
        hsizer->AddSpacer(indentation);
    hsizer->Add(st, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    hsizer->Add(ctrl, 0, wxALL, 5);
    if (percent)
        hsizer->Add(new wxStaticText(parent, -1, wxT("%")),
                    0, wxRIGHT|wxALIGN_CENTER_VERTICAL, 5);
    sizer->Add(hsizer, 0, wxEXPAND);
    return ctrl;
}

wxTextCtrl *addTextCtrl(wxWindow *parent, const wxString& label,
                        const string& value, wxSizer *sizer)
{
    wxStaticText *st = new wxStaticText(parent, -1, label);
    wxTextCtrl *ctrl = new wxTextCtrl(parent, -1, s2wx(value));
    wxBoxSizer *hsizer = new wxBoxSizer(wxHORIZONTAL);
    hsizer->Add(st, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    hsizer->Add(ctrl, 0, wxALL, 5);
    sizer->Add(hsizer, 0, wxEXPAND);
    return ctrl;
}

wxDirPickerCtrl *addDirPicker(wxWindow *parent, const wxString& label,
                              const string& path, wxSizer *sizer)
{
    wxStaticText *st = new wxStaticText(parent, -1, label);
    wxDirPickerCtrl *ctrl = new wxDirPickerCtrl(parent, -1, s2wx(path));
    wxBoxSizer *hsizer = new wxBoxSizer(wxHORIZONTAL);
    hsizer->Add(st, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    hsizer->Add(ctrl, 0, wxALL, 5);
    sizer->Add(hsizer, 0, wxEXPAND);
    return ctrl;
}


wxCheckBox *addCheckbox(wxWindow *parent, const wxString& label,
                        bool value, wxSizer *sizer,
                        int indentation=0)
{
    wxCheckBox *ctrl = new wxCheckBox(parent, -1, label);
    ctrl->SetValue(value);
    wxBoxSizer *hsizer = new wxBoxSizer(wxHORIZONTAL);
    if (indentation)
        hsizer->AddSpacer(indentation);
    hsizer->Add(ctrl);
    sizer->Add(hsizer, 0, wxEXPAND|wxALL, 5);
    return ctrl;
}

wxChoice *addEnumSetting(wxWindow *parent, const wxString& label,
                         const string& option, wxSizer* sizer,
                         int indentation=0)
{
    wxBoxSizer *hsizer = new wxBoxSizer(wxHORIZONTAL);
    if (indentation)
        hsizer->AddSpacer(indentation);
    wxStaticText *st = new wxStaticText(parent, -1, label);
    hsizer->Add(st, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    wxArrayString array;
    const char** values = fityk::SettingsMgr::get_allowed_values(option);
    while (*values != NULL) {
        array.Add(pchar2wx(*values));
        ++values;
    }
    wxChoice *ctrl = new wxChoice(parent, -1, wxDefaultPosition, wxDefaultSize,
                                  array);
    ctrl->SetStringSelection(s2wx(ftk->settings_mgr()->get_as_string(option)));
    hsizer->Add(ctrl, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    sizer->Add(hsizer, 0, wxEXPAND);
    return ctrl;
}

SpinCtrl* addSpinCtrl(wxWindow *parent, const wxString& label,
                      int value, int min_v, int max_v, wxSizer *sizer,
                      int indentation=0)
{
    wxStaticText *st = new wxStaticText(parent, -1, label);
    SpinCtrl *spin = new SpinCtrl(parent, -1, value, min_v, max_v, 70);
    wxBoxSizer *box = new wxBoxSizer(wxHORIZONTAL);
    if (indentation)
        box->AddSpacer(indentation);
    box->Add(st, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    box->Add(spin, 0, wxTOP|wxBOTTOM, 5);
    sizer->Add(box, 0, wxEXPAND);
    return spin;
}

wxStaticText* new_bold_text(wxWindow *parent, const wxString& label)
{
    wxStaticText *fcterm = new wxStaticText(parent, -1, label);
    wxFont bold_font = fcterm->GetFont();
    bold_font.SetWeight(wxFONTWEIGHT_BOLD);
    fcterm->SetFont(bold_font);
    return fcterm;
}

} //anonymous namespace

SettingsDlg::SettingsDlg(wxWindow* parent)
    : wxDialog(parent, -1, wxT("Engine Settings"),
               wxDefaultPosition, wxDefaultSize,
               wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER)
{
    const fityk::Settings* settings = ftk->get_settings();
    wxNotebook *nb = new wxNotebook(this, -1);
    wxPanel *page_general = new wxPanel(nb, -1);
    nb->AddPage(page_general, wxT("general"));
    wxPanel *page_peakfind = new wxPanel(nb, -1);
    nb->AddPage(page_peakfind, wxT("peak-finding"));
    wxNotebook *page_fitting = new wxNotebook(nb, -1);
    nb->AddPage(page_fitting, wxT("fitting"));

    // page general
    wxBoxSizer *sizer_general = new wxBoxSizer(wxVERTICAL);

    sigma_ch = addEnumSetting(page_general, wxT("default std. dev. of data y:"),
                              "default_sigma", sizer_general);
    cut_func = addRealNumberCtrl(page_general,
                                 wxT("f(x) is negligible if |f(x)|<"),
                                 settings->function_cutoff,
                                 sizer_general);

    verbosity_sp = addSpinCtrl(page_general, wxT("verbosity level (in output pane)"),
                               settings->verbosity, -1, 2, sizer_general);

    sigma_ch = addEnumSetting(page_general, wxT("default std. dev. of data y:"),
                              "default_sigma", sizer_general);
    onerror_ch = addEnumSetting(page_general, "action on error", "on_error",
                                sizer_general);

    seed_sp = addSpinCtrl(page_general,
                          wxT("pseudo-random generator seed (0 = time-based)"),
                          settings->pseudo_random_seed, 0, 999999,
                          sizer_general);

    eps_rc = addRealNumberCtrl(page_general,
                               wxT("epsilon for floating-point comparison"),
                               settings->epsilon,
                               sizer_general);

    format_tc = addTextCtrl(page_general,
                            wxT("numeric format used by info/print"),
                            settings->numeric_format,
                            sizer_general);

    cwd_dp = addDirPicker(page_general,
                          "working directory",
                          settings->cwd,
                          sizer_general);

    page_general->SetSizerAndFit(sizer_general);

    // page peak-finding
    wxBoxSizer *sizer_pf = new wxBoxSizer(wxVERTICAL);

    height_correction = addRealNumberCtrl(page_peakfind,
                           wxT("factor used to correct detected peak height"),
                           settings->height_correction,
                           sizer_pf);
    width_correction = addRealNumberCtrl(page_peakfind,
                           wxT("factor used to correct detected peak width"),
                           settings->width_correction,
                           sizer_pf);

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
    domain_p = addRealNumberCtrl(page_fit_common,
                                 wxT("default domain of variable is +/-"),
                                 settings->domain_percent,
                                 sizer_fcmn, 0, /*percent=*/true);

    fit_replot_cb = addCheckbox(page_fit_common,
                              wxT("refresh plot after each iteration"),
                              settings->fit_replot, sizer_fcmn);

    delay_sp = addSpinCtrl(page_fit_common,
                           wxT("time (in sec.) between stats/plot updates"),
                           settings->refresh_period, -1, 9999,
                           sizer_fcmn);

    //wxStaticBoxSizer *sizer_fcstop = new wxStaticBoxSizer(wxHORIZONTAL,
    //                            page_fit_common, wxT("termination criteria"));
    sizer_fcmn->AddSpacer(10);
    sizer_fcmn->Add(new_bold_text(page_fit_common, wxT("termination criteria")),
                    wxSizerFlags().Border());
    mwssre_sp = addSpinCtrl(page_fit_common, wxT("max. WSSR evaluations"),
                            settings->max_wssr_evaluations, 0, 999999,
                            sizer_fcmn, 10);
    fit_max_time = addRealNumberCtrl(page_fit_common, wxT("max. fitting time"),
                                   settings->max_fitting_time, sizer_fcmn, 10);
    //sizer_fcmn->Add(sizer_fcstop, 0, wxEXPAND|wxALL, 5);

    page_fit_common->SetSizerAndFit(sizer_fcmn);

    // sub-page Lev-Mar
    wxBoxSizer *sizer_flm = new wxBoxSizer(wxVERTICAL);
    sizer_flm->Add(new_bold_text(page_fit_LM, wxT("lambda parameter")),
                   wxSizerFlags().Border());
    lm_lambda_ini = addRealNumberCtrl(page_fit_LM, wxT("initial value"),
                         settings->lm_lambda_start, sizer_flm, 10);
    lm_lambda_up = addRealNumberCtrl(page_fit_LM, wxT("increasing factor"),
                         settings->lm_lambda_up_factor, sizer_flm, 10);
    lm_lambda_down = addRealNumberCtrl(page_fit_LM, wxT("decreasing factor"),
                         settings->lm_lambda_down_factor, sizer_flm, 10);
    sizer_flm->AddSpacer(10);
    sizer_flm->Add(new_bold_text(page_fit_LM, wxT("termination criteria")),
                   wxSizerFlags().Border());
    lm_stop = addRealNumberCtrl(page_fit_LM, wxT("WSSR relative change <"),
                                settings->lm_stop_rel_change, sizer_flm, 10);
    lm_max_lambda = addRealNumberCtrl(page_fit_LM, wxT("max. value of lambda"),
                                      settings->lm_max_lambda, sizer_flm, 10);
    page_fit_LM->SetSizerAndFit(sizer_flm);

    // sub-page N-M
    wxBoxSizer *sizer_fnm = new wxBoxSizer(wxVERTICAL);

    sizer_fnm->Add(new_bold_text(page_fit_NM, wxT("simplex initialization")),
                   wxSizerFlags().Border());
    nm_move_all = addCheckbox(page_fit_NM,
                       wxT("randomize all vertices (otherwise one is left)"),
                       settings->nm_move_all, sizer_fnm, 10);
    nm_distrib = addEnumSetting(page_fit_NM, wxT("distribution type"),
                                "nm_distribution", sizer_fnm, 10);
    nm_move_factor = addRealNumberCtrl(page_fit_NM,
                         wxT("factor by which domain is expanded"),
                         settings->nm_move_factor, sizer_fnm, 10);

    sizer_fnm->AddSpacer(10);
    sizer_fnm->Add(new_bold_text(page_fit_NM, wxT("termination criteria")),
                   wxSizerFlags().Border());
    nm_convergence = addRealNumberCtrl(page_fit_NM,
                                 wxT("relative difference between vertices"),
                                 settings->nm_convergence, sizer_fnm, 10);
    page_fit_NM->SetSizerAndFit(sizer_fnm);

    // TODO: sub-page GA

    //finish layout
    wxBoxSizer *top_sizer = new wxBoxSizer (wxVERTICAL);
    top_sizer->Add(nb, 1, wxLEFT|wxRIGHT|wxTOP|wxEXPAND, 10);
#ifdef __WXMAC__
    nb->SetMinSize(wxSize(-1, 300));
#endif
    wxStaticText *note = new SmallStaticText(this,
      wxT("These settings can be saved in the init script")
      wxT(" (Session \u2023 Edit Init File).")
      wxT("\nThe interface can be configured in GUI \u2023 Configure."));
    top_sizer->Add(note, 0, wxALL|wxALIGN_CENTER, 10);
    top_sizer->Add (CreateButtonSizer (wxOK|wxCANCEL),
                    0, wxALL|wxALIGN_CENTER, 5);
    SetSizerAndFit(top_sizer);

    Connect(wxID_OK, wxEVT_COMMAND_BUTTON_CLICKED,
            wxCommandEventHandler(SettingsDlg::OnOK));
}

static
string add(const char* name, const string& new_value)
{
    string old_value = ftk->settings_mgr()->get_as_string(name);
    if (old_value == new_value)
        return "";
    return S(name) + " = " + new_value + ", ";
}

void SettingsDlg::exec_set_command()
{
    string assign = "set ";
    assign += add("default_sigma", wx2s(sigma_ch->GetStringSelection()));
    assign += add("function_cutoff", wx2s(cut_func->GetValue()));
    assign += add("verbosity", S(verbosity_sp->GetValue()));
    assign += add("on_error", wx2s(onerror_ch->GetStringSelection()));
    assign += add("pseudo_random_seed", S(seed_sp->GetValue()));
    assign += add("epsilon", wx2s(eps_rc->GetValue()));
    assign += add("numeric_format", "'" + wx2s(format_tc->GetValue()) + "'");
    assign += add("cwd", "'" + wx2s(cwd_dp->GetPath()) + "'");
    assign += add("height_correction", wx2s(height_correction->GetValue()));
    assign += add("width_correction", wx2s(width_correction->GetValue()));
    assign += add("max_wssr_evaluations", S(mwssre_sp->GetValue()));
    assign += add("max_fitting_time", wx2s(fit_max_time->GetValue()));
    assign += add("domain_percent", wx2s(domain_p->GetValue()));
    assign += add("fit_replot", fit_replot_cb->GetValue() ? "1" : "0");
    assign += add("refresh_period", S(delay_sp->GetValue()));
    assign += add("lm_lambda_start", wx2s(lm_lambda_ini->GetValue()));
    assign += add("lm_lambda_up_factor", wx2s(lm_lambda_up->GetValue()));
    assign += add("lm_lambda_down_factor", wx2s(lm_lambda_down->GetValue()));
    assign += add("lm_stop_rel_change", wx2s(lm_stop->GetValue()));
    assign += add("lm_max_lambda", wx2s(lm_max_lambda->GetValue()));
    assign += add("nm_move_all", nm_move_all->GetValue() ? "1" : "0");
    assign += add("nm_distribution", wx2s(nm_distrib->GetStringSelection()));
    assign += add("nm_move_factor", wx2s(nm_move_factor->GetValue()));
    assign += add("nm_convergence", wx2s(nm_convergence->GetValue()));
    if (assign.size() == 4) // no options
        return;
    assign.resize(assign.size() - 2);
    exec(assign);
}

void SettingsDlg::OnOK(wxCommandEvent&)
{
    exec_set_command();
    EndModal(wxID_OK);
}

/*
We used to have Directories tab in the Settings dialog,
but probably the settings there were rarely useful and confusing,
so it was removed.

The settings are still read from .fityk/wxoption, and can be changed
by editing the config file. The keys are:
/loadDataDir - default directory for load-data dialogs
/execScriptDir - default directory for execute-script dialogs
/exportDir - default directory for export dialogs
*/


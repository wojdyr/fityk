// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#ifndef FITYK_WX_FITRUN_H_
#define FITYK_WX_FITRUN_H_

#include <vector>
#include <wx/spinctrl.h>

#include "cmn.h"

class FitRunDlg : public wxDialog
{
public:
    FitRunDlg(wxWindow* parent, wxWindowID id, bool initialize);
    std::string get_cmd() const;
private:
    wxRadioBox* data_rb;
    wxChoice* method_c;
    wxCheckBox* initialize_cb, *autoplot_cb;
    SpinCtrl *maxiter_sc, *maxeval_sc;
    wxStaticText *nomaxeval_st, *nomaxiter_st;
    std::vector<int> sel; // indices of selected datasets

    void OnSpinEvent(wxSpinEvent &) { update_unlimited(); }
    void OnChangeDsOrMethod(wxCommandEvent&) { update_allow_continue(); }
    void update_unlimited();
    void update_allow_continue();
    DECLARE_EVENT_TABLE()
};

#endif

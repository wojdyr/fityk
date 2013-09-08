// This file is part of fityk program. Copyright 2001-2013 Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#ifndef FITYK_WX_FITRUN_H_
#define FITYK_WX_FITRUN_H_

#include <vector>
#include <wx/spinctrl.h>

#include "cmn.h"

class FitRunDlg : public wxDialog
{
public:
    FitRunDlg(wxWindow* parent, wxWindowID id);
    std::string get_cmd() const;
private:
    wxRadioBox* data_rb;
    wxChoice* method_c;
    wxCheckBox *separately_cb, *autoplot_cb;
    SpinCtrl *maxiter_sc, *maxeval_sc;
    wxStaticText *nomaxeval_st, *nomaxiter_st;

    void OnSpinEvent(wxSpinEvent &) { update_unlimited(); }
    void OnChangeDsOrMethod(wxCommandEvent&) {}
    void update_unlimited();
    DECLARE_EVENT_TABLE()
};

#endif

// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License version 2 or (at your option) 3
// $Id$

/// In this file:
///  Status Bar (FStatusBar) and dialog for its configuration (ConfStatBarDlg)

#include <wx/wx.h>
#include <wx/spinctrl.h>
#include <wx/statline.h>

#include "statbar.h"
#include "../common.h" //wx2s, s2wx, GET_BMP
#include "../datatrans.h" //get_dt_code(), get_value_for_point()

// icons
#include "img/mouse16.h"
#include "img/sbprefs.h"

using namespace std;

//TODO:
// - ConfStatBarDlg should have only Close button
// - coordinates of the center of the plot should be shown as example
//   when ConfStatBarDlg is called
// - tooltip for settings button
// - mouse icon should display a tooltip with full information about current
//   mode and all button usage (also with Shift/Alt/Ctrl)

//===============================================================
//                    FStatusBar   
//===============================================================

FStatusBar::FStatusBar(wxWindow *parent)
        : wxPanel(parent, -1)
{
    wxBoxSizer *sizer = new wxBoxSizer(wxHORIZONTAL);
    split = new wxSplitterWindow(this, -1);
    text = new wxStaticText(split, -1, wxT(""));
    coords = new wxStaticText(split, -1, wxT(""), wxDefaultPosition, wxSize(150, 3));
    split->SplitVertically(text, coords);
    split->SetSashGravity(1.0);
    split->SetMinimumPaneSize(50);
    sizer->Add(split, wxSizerFlags(1).Centre().Border(wxLEFT));

    wxBitmapButton *prefbtn = new wxBitmapButton(this, -1, GET_BMP(sbprefs));
    sizer->Add(prefbtn, wxSizerFlags().Expand().Centre());

    wxString long_hint = wxT("add-in-range");
    lmouse_hint = new wxStaticText(this, -1, long_hint,
                                   wxDefaultPosition, wxDefaultSize,
                                   wxALIGN_RIGHT|wxST_NO_AUTORESIZE);
    rmouse_hint = new wxStaticText(this, -1, long_hint,
                                   wxDefaultPosition, wxDefaultSize,
                                   wxST_NO_AUTORESIZE);
    wxBoxSizer *hint_sizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer *vhint_sizer = new wxBoxSizer(wxHORIZONTAL);
    vhint_sizer->Add(lmouse_hint, wxSizerFlags().Centre().FixedMinSize());
    wxStaticBitmap *mousebmp = new wxStaticBitmap(this, -1, GET_BMP(mouse16));
    vhint_sizer->Add(mousebmp, wxSizerFlags().Centre().Border(wxRIGHT, 3));
    vhint_sizer->Add(rmouse_hint, wxSizerFlags().Centre().FixedMinSize());
    hint_sizer->Add(new wxStaticLine(this), wxSizerFlags().Expand());
    hint_sizer->Add(vhint_sizer, wxSizerFlags(1).Expand());
    sizer->Add(hint_sizer, wxSizerFlags().Expand());
    sizer->AddSpacer(16);
    SetSizer(sizer);

    Connect(prefbtn->GetId(), wxEVT_COMMAND_BUTTON_CLICKED,
            (wxObjectEventFunction) &FStatusBar::OnPrefButton);
}

void FStatusBar::save_settings(wxConfigBase *cf) const
{
    cf->SetPath(wxT("/StatusBar"));
    int pos = split->GetClientSize().x - split->GetSashPosition();
    cf->Write(wxT("sash_pos"), pos);
    cf->Write(wxT("show_btn"), show_btn);
    cf->Write(wxT("show_hints"), show_hints);
    cf->Write(wxT("x_prec"), x_prec);
    cf->Write(wxT("y_prec"), y_prec);
    cf->Write(wxT("e_prec"), e_prec);
    cf->Write(wxT("extraValue"), extra_value);
}

void FStatusBar::read_settings(wxConfigBase *cf)
{
    cf->SetPath(wxT("/StatusBar"));
    int pos = cf->Read(wxT("sash_pos"), -200);
    show_btn = cfg_read_bool(cf, wxT("show_btn"), true);
    show_hints = cfg_read_bool(cf, wxT("show_hints"), true);
    x_prec = cf->Read(wxT("x_prec"), 3);
    y_prec = cf->Read(wxT("y_prec"), 1);
    e_prec = cf->Read(wxT("e_prec"), 3);
    wxString ev = cf->Read(wxT("extraValue"), wxT(""));

    set_extra_value(wx2s(ev));
    split->SetSashPosition(pos);
    show_or_hide();
}

void FStatusBar::set_coords_format()
{
    fmt_main.Printf(wxT("%% %d.%df  %% %d.%df"), 
                    int_len+x_prec, x_prec, int_len+y_prec, y_prec);
    fmt_aux.Printf(wxT("%% %d.%df  [%% %d.%df]"), 
                   int_len+x_prec, x_prec, int_len+y_prec, y_prec);
    if (!extra_value.IsEmpty()) {
        wxString extra_fmt 
            = wxString::Format(wxT("  -> %% %d.%df"), int_len+e_prec, e_prec);
        fmt_main += extra_fmt;
        fmt_aux += extra_fmt;
    }
}

void FStatusBar::set_hints(string const& left, string const& right)
{
    lmouse_hint->SetLabel(s2wx(left));
    rmouse_hint->SetLabel(s2wx(right));
}

void FStatusBar::set_coords(double x, double y, PlotTypeEnum pte)
{
    double val = 0;
    if (!extra_value.IsEmpty())
        val = get_value_for_point(e_code, e_numbers, x, y);
    wxString const& fmt = (pte == pte_main ? fmt_main : fmt_aux);
    coords->SetLabel(wxString::Format(fmt, x, y, val));
}

bool FStatusBar::set_extra_value(string const& s)
{
    if (!s.empty() && !get_dt_code(s, e_code, e_numbers))
        return false;
    extra_value = s2wx(s);
    set_coords_format();
    return true;
}

void FStatusBar::show_or_hide()
{
    wxSizer *sizer = GetSizer();
    // this doesn't work: sizer->Show(prefbtn, show_btn);
    sizer->Show(1, show_btn);
    sizer->Show(2, show_hints);
    sizer->Layout();
}

void FStatusBar::OnPrefButton(wxCommandEvent&)
{
    ConfStatBarDlg dlg(NULL, -1, this);
    dlg.ShowModal();
}

//===============================================================
//                     ConfStatBarDlg
//===============================================================


ConfStatBarDlg::ConfStatBarDlg(wxWindow* parent, wxWindowID id, FStatusBar* sb_)
    //explicit conversion of title to wxString() is neccessary
  : wxDialog(parent, id, wxString(wxT("Configure Status Bar")),
             wxDefaultPosition, wxDefaultSize, 
             wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER),
    sb(sb_)
{
    wxBoxSizer *top_sizer = new wxBoxSizer(wxVERTICAL);

    show_btn_cb = new wxCheckBox(this, -1, wxT("show settings button"));
    show_btn_cb->SetValue(sb->show_btn);
    top_sizer->Add(show_btn_cb, wxSizerFlags().Border());

    show_hints_cb = new wxCheckBox(this, -1, wxT("show mouse hints"));
    show_hints_cb->SetValue(sb->show_hints);
    top_sizer->Add(show_hints_cb, wxSizerFlags().Border());

    wxStaticBoxSizer *f_sizer = new wxStaticBoxSizer(wxVERTICAL, this, 
                                   wxT("coordinates format"));

    wxGridSizer *gsizer = new wxGridSizer(2, 5, 5);

    wxSizerFlags cl = wxSizerFlags().Align(wxALIGN_CENTRE_VERTICAL);
    wxSizerFlags cr 
                = wxSizerFlags().Align(wxALIGN_CENTRE_VERTICAL|wxALIGN_RIGHT);

    gsizer->Add(new wxStaticText(this, -1, wxT("precision of x")), cr);
    x_prec_sc = new SpinCtrl(this, -1, sb->x_prec, 0, 9, 40);
    gsizer->Add(x_prec_sc, cl);

    gsizer->Add(new wxStaticText(this, -1, wxT("precision of y")), cr);
    y_prec_sc = new SpinCtrl(this, -1, sb->y_prec, 0, 9, 40);
    gsizer->Add(y_prec_sc, cl);

    gsizer->Add(new wxStaticText(this, -1, wxT("formula of extra value")), cr);
    extra_tc = new wxTextCtrl(this, -1, sb->extra_value);
    gsizer->Add(extra_tc, wxSizerFlags().Expand().Centre());

    gsizer->Add(new wxStaticText(this, -1,wxT("precision of extra value")), cr);
    e_prec_sc = new SpinCtrl(this, -1, sb->e_prec, 0, 9, 40);
    gsizer->Add(e_prec_sc, cl);

    f_sizer->Add(gsizer, wxSizerFlags(1).Expand());
    top_sizer->Add(f_sizer, wxSizerFlags().Expand().Border());

    add_apply_close_buttons(this, top_sizer);
    SetSizerAndFit(top_sizer);

    SetEscapeId(wxID_CLOSE);
    Connect(wxID_APPLY, wxEVT_COMMAND_BUTTON_CLICKED,
            (wxObjectEventFunction) &ConfStatBarDlg::OnApply);
}

void ConfStatBarDlg::OnApply (wxCommandEvent&)
{
    sb->show_btn = show_btn_cb->GetValue();
    sb->show_hints = show_hints_cb->GetValue();
    sb->show_or_hide();
    sb->x_prec = x_prec_sc->GetValue();
    sb->y_prec = y_prec_sc->GetValue();
    sb->e_prec = e_prec_sc->GetValue();
    bool r = sb->set_extra_value(wx2s(extra_tc->GetValue()));
    if (!r)
        extra_tc->SetSelection(-1, -1);
}


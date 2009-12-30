// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
// $Id$

/// In this file:
///  Status Bar (FStatusBar) and dialog for its configuration (ConfStatBarDlg)

#include <wx/wx.h>
#include <wx/spinctrl.h>
#include <wx/statline.h>
#include <wx/tooltip.h>

#include "statbar.h"
#include "cmn.h" //wx2s, s2wx, GET_BMP
#include "../datatrans.h" //get_dt_code(), get_value_for_point()

// icons
#include "img/mouse16.h"
#include "img/sbprefs.h"
#include "img/ok24.h"

using namespace std;

//===============================================================
//                    FStatusBar
//===============================================================

FStatusBar::FStatusBar(wxWindow *parent)
        : wxPanel(parent, -1), last_x(0), last_y(0)
{
    wxBoxSizer *sizer = new wxBoxSizer(wxHORIZONTAL);
    split = new wxSplitterWindow(this, -1, wxDefaultPosition, wxDefaultSize, 0);
    text = new wxStaticText(split, -1, wxT(""));
    coords = new wxStaticText(split, -1, wxT(""),
                              wxDefaultPosition, wxSize(150, -1));
    split->SplitVertically(text, coords);
    split->SetSashGravity(1.0);
    split->SetMinimumPaneSize(50);
    sizer->Add(split, wxSizerFlags(1).Centre().Border(wxLEFT));

    wxBitmapButton *prefbtn = new wxBitmapButton
#ifdef __WXMSW__
        // on wxMSW the default width is too small
        (this, -1, GET_BMP(sbprefs), wxDefaultPosition, wxSize(22, -1));
#else
        (this, -1, GET_BMP(sbprefs));
#endif
    prefbtn->SetToolTip(wxT("configure status bar"));
    sizer->Add(prefbtn, wxSizerFlags().Expand().Centre());

    // wxALIGN_RIGHT flag for wxStaticText doesn't work on wxGTK 2.9
    // (wx bug #10716), so we must align the text in a different way
    wxString long_hint = wxT("add-in-range");
    lmouse_hint = new wxStaticText(this, -1, long_hint);
    rmouse_hint = new wxStaticText(this, -1, long_hint,
                                   wxDefaultPosition, wxDefaultSize,
                                   wxST_NO_AUTORESIZE);
    mousebmp = new wxStaticBitmap(this, -1, GET_BMP(mouse16));
    wxBoxSizer *lmouse_sizer = new wxBoxSizer(wxHORIZONTAL);
    lmouse_sizer->AddStretchSpacer();
    lmouse_sizer->Add(lmouse_hint, wxSizerFlags().Right());
    lmouse_sizer->SetMinSize(rmouse_hint->GetClientSize());
    wxBoxSizer *hint_sizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer *vhint_sizer = new wxBoxSizer(wxHORIZONTAL);
    vhint_sizer->Add(lmouse_sizer, wxSizerFlags(1).Expand().FixedMinSize());
    vhint_sizer->Add(mousebmp, wxSizerFlags().Centre().Border(wxRIGHT, 3));
    vhint_sizer->Add(rmouse_hint, wxSizerFlags().Centre().FixedMinSize());
    hint_sizer->Add(new wxStaticLine(this), wxSizerFlags().Expand());
    hint_sizer->Add(vhint_sizer, wxSizerFlags(1).Expand().FixedMinSize());
    sizer->Add(hint_sizer, wxSizerFlags().Expand());

    sizer->AddSpacer(16);
    SetSizer(sizer);
    Connect(prefbtn->GetId(), wxEVT_COMMAND_BUTTON_CLICKED,
            (wxObjectEventFunction) &FStatusBar::OnPrefButton);
    mousebmp->Connect(wxEVT_LEFT_DOWN,
                      (wxObjectEventFunction) &FStatusBar::OnMouseBmpClicked,
                      NULL, this);
    mousebmp->Connect(wxEVT_RIGHT_DOWN,
                      (wxObjectEventFunction) &FStatusBar::OnMouseBmpClicked,
                      NULL, this);
}

void FStatusBar::save_settings(wxConfigBase *cf) const
{
    cf->SetPath(wxT("/StatusBar"));
    int coord_width = split->GetClientSize().x - split->GetSashPosition();
    cf->Write(wxT("coord_width"), coord_width);
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
    int coord_width = cf->Read(wxT("coord_width"), 200);
    show_btn = cfg_read_bool(cf, wxT("show_btn"), true);
    show_hints = cfg_read_bool(cf, wxT("show_hints"), true);
    x_prec = cf->Read(wxT("x_prec"), 3);
    y_prec = cf->Read(wxT("y_prec"), 1);
    e_prec = cf->Read(wxT("e_prec"), 3);
    wxString ev = cf->Read(wxT("extraValue"), wxT(""));

    set_extra_value(wx2s(ev));
    split->SetSashPosition(-coord_width);
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

void FStatusBar::set_hints(string const& left, string const& right,
                           string const& mode_name,
                           string const& shift_left, string const& shift_right)
{
    lmouse_hint->SetLabel(s2wx(left));
    rmouse_hint->SetLabel(s2wx(right));
    string tip = "In this mode (" + mode_name + "), on main plot:"
                 "\n  left mouse button: " + left
               + "\n  right mouse button: " + right
               + "\n  Shift + left button: " + shift_left
               + "\n  Shift + right button: " + shift_right
               + "\nIn all modes, on main plot:"
                 "\n  middle mouse button: zoom"
                 "\n  Ctrl + left button: zoom"
                 "\n  Ctrl + right button: menu"
               ;
    mousebmp->SetToolTip(s2wx(tip));
    Layout();
}

void FStatusBar::set_coords(double x, double y, PlotTypeEnum pte)
{
    double val = 0;
    if (!extra_value.IsEmpty())
        val = get_value_for_point(e_code, e_numbers, x, y);
    wxString const& fmt = (pte == pte_main ? fmt_main : fmt_aux);
    coords->SetLabel(wxString::Format(fmt, x, y, val));
    last_x = x;
    last_y = y;
    last_pte = pte;
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

void FStatusBar::OnMouseBmpClicked(wxMouseEvent&)
{
    wxToolTip *tip = mousebmp->GetToolTip();
    if (tip == NULL)
        return;
    wxMessageBox(tip->GetTip(), wxT("Mouse usage"));
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
                                                     wxT("coordinates"));

    wxStaticText *evcomment = new wxStaticText(this, -1,
                 wxT("In addition to x and y, extra numeric value ")
                 wxT("(function of x and/or y) can be shown, e.g.:")
                 wxT("\n4*pi*sin(x/2*pi/180)/1.54051"));
    evcomment->SetFont(*wxITALIC_FONT);
    f_sizer->Add(evcomment, wxSizerFlags().Border());

    wxBoxSizer *evsizer = new wxBoxSizer(wxHORIZONTAL);
    evsizer->Add(new wxStaticText(this, -1, wxT("extra value formula")),
                 wxSizerFlags().Center().Border(wxRIGHT));
    extra_tc = new wxTextCtrl(this, -1, sb->extra_value);
    evsizer->Add(extra_tc, wxSizerFlags(1).Center());
    okbmp = new wxStaticBitmap(this, -1, GET_BMP(ok24));
    evsizer->Add(okbmp, wxSizerFlags().Center()
#if wxCHECK_VERSION(2, 8, 8)
            .ReserveSpaceEvenIfHidden()
#endif
                );
    f_sizer->Add(evsizer, wxSizerFlags().Expand().Border());

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

    gsizer->Add(new wxStaticText(this, -1,wxT("precision of extra value")), cr);
    e_prec_sc = new SpinCtrl(this, -1, sb->e_prec, 0, 9, 40);
    gsizer->Add(e_prec_sc, cl);

    f_sizer->Add(gsizer, wxSizerFlags(1).Border());
    top_sizer->Add(f_sizer, wxSizerFlags().Expand().Border());

    top_sizer->Add(persistance_note_sizer(this),
                   wxSizerFlags().Expand().Border());

    top_sizer->Add(new wxButton(this, wxID_CLOSE),
                   wxSizerFlags().Right().Border());
    SetSizerAndFit(top_sizer);

    SetEscapeId(wxID_CLOSE);

    //okbmp->Show(!sb->extra_value.empty());

    Connect(show_btn_cb->GetId(), wxEVT_COMMAND_CHECKBOX_CLICKED,
            (wxObjectEventFunction) &ConfStatBarDlg::OnShowBtnCheckbox);
    Connect(show_hints_cb->GetId(), wxEVT_COMMAND_CHECKBOX_CLICKED,
            (wxObjectEventFunction) &ConfStatBarDlg::OnShowHintsCheckbox);

    Connect(extra_tc->GetId(), wxEVT_COMMAND_TEXT_UPDATED,
            (wxObjectEventFunction) &ConfStatBarDlg::OnExtraValueChange);

    Connect(x_prec_sc->GetId(), wxEVT_COMMAND_SPINCTRL_UPDATED,
            (wxObjectEventFunction) &ConfStatBarDlg::OnPrecisionSpin);
    Connect(y_prec_sc->GetId(), wxEVT_COMMAND_SPINCTRL_UPDATED,
            (wxObjectEventFunction) &ConfStatBarDlg::OnPrecisionSpin);
    Connect(e_prec_sc->GetId(), wxEVT_COMMAND_SPINCTRL_UPDATED,
            (wxObjectEventFunction) &ConfStatBarDlg::OnPrecisionSpin);
}

void ConfStatBarDlg::OnShowBtnCheckbox(wxCommandEvent& event)
{
    sb->show_btn = event.IsChecked();
    sb->show_or_hide();
}

void ConfStatBarDlg::OnShowHintsCheckbox(wxCommandEvent& event)
{
    sb->show_hints = event.IsChecked();
    sb->show_or_hide();
}

void ConfStatBarDlg::OnPrecisionSpin(wxCommandEvent& event)
{
    if (event.GetId() == x_prec_sc->GetId())
        sb->x_prec = x_prec_sc->GetValue();
    else if (event.GetId() == y_prec_sc->GetId())
        sb->y_prec = y_prec_sc->GetValue();
    else if (event.GetId() == e_prec_sc->GetId())
        sb->e_prec = e_prec_sc->GetValue();
    sb->set_coords_format();
    sb->show_last_coordinates();
}

void ConfStatBarDlg::OnExtraValueChange(wxCommandEvent&)
{
    string str = wx2s(extra_tc->GetValue());
    bool ok = sb->set_extra_value(str);
    okbmp->Show(ok);
#if !wxCHECK_VERSION(2, 8, 8)
    GetSizer()->Layout();
#endif
    sb->show_last_coordinates();
}


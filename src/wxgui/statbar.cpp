// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License version 2 or (at your option) 3
// $Id$

/// In this file:
///  Status Bar (FStatusBar) and dialog for its configuration (ConfStatBarDlg)

#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/spinctrl.h>

#include "statbar.h"
#include "../common.h" //wx2s, s2wx
#include "../datatrans.h" //get_dt_code(), get_value_for_point()

//statusbar icons - mouse buttons
#include "img/mouse_l.xpm"
#include "img/mouse_r.xpm"

using namespace std;


//===============================================================
//                    FStatusBar   
//===============================================================
BEGIN_EVENT_TABLE(FStatusBar, wxStatusBar)
    EVT_SIZE(FStatusBar::OnSize)
END_EVENT_TABLE()

FStatusBar::FStatusBar(wxWindow *parent)
        : wxStatusBar(parent, -1), statbmp2(0)
{
    SetMinHeight(15);
    statbmp1 = new wxStaticBitmap(this, -1, wxIcon(mouse_l_xpm));
    statbmp2 = new wxStaticBitmap(this, -1, wxIcon(mouse_r_xpm));
}

void FStatusBar::save_settings(wxConfigBase *cf) const
{
    cf->SetPath(wxT("/StatusBar"));
    cf->Write(wxT("textWidth"), widths[sbf_text]);
    cf->Write(wxT("hint1Width"), widths[sbf_hint1]);
    cf->Write(wxT("hint2Width"), widths[sbf_hint2]);
    cf->Write(wxT("coordWidth"), widths[sbf_coord]);
    cf->Write(wxT("mainFmt"), fmt_main);
    cf->Write(wxT("auxFmt"), fmt_aux);
    cf->Write(wxT("extraValue"), extra_value);
}

void FStatusBar::read_settings(wxConfigBase *cf)
{
    cf->SetPath(wxT("/StatusBar"));
    widths[sbf_text] = cf->Read(wxT("textWidth"), -1);
    widths[sbf_hint1] = cf->Read(wxT("hint1Width"), 100);
    widths[sbf_hint2] = cf->Read(wxT("hint2Width"), 100);
    widths[sbf_coord] = cf->Read(wxT("coordWidth"), 120);
    fmt_main = cf->Read(wxT("mainFmt"), wxT("%.3f  %.0f"));
    fmt_aux = cf->Read(wxT("auxFmt"), wxT("%.3f  [%.0f]"));
    wxString ev = cf->Read(wxT("extraValue"), wxT(""));
    if (!ev.IsEmpty())
        set_extra_value(wx2s(ev));
    SetFieldsCount(sbf_max, widths);
}

void FStatusBar::move_bitmaps()
{
    if (!statbmp2) 
        return; 
    statbmp1->Show(widths[sbf_hint1] > 20);
    statbmp2->Show(widths[sbf_hint2] > 20);
    wxRect rect;
    GetFieldRect(sbf_hint1, rect);
    wxSize size = statbmp1->GetSize();
    int bmp_y = rect.y + (rect.height - size.y) / 2;
    statbmp1->Move(rect.x + 1, bmp_y);
    GetFieldRect(sbf_hint2, rect);
    statbmp2->Move(rect.x + 1, bmp_y);
}

void FStatusBar::set_hint(string const& left, string const& right)
{
    wxString space = (widths[sbf_hint1] > 20 ? wxT("    ") : wxT("")); 
    SetStatusText(space + s2wx(left),  sbf_hint1);
    SetStatusText(space + s2wx(right), sbf_hint2);
}

void FStatusBar::set_widths(int hint, int coord)
{
    widths[sbf_hint1] = hint;
    widths[sbf_hint2] = hint;
    widths[sbf_coord] = coord;
    SetStatusWidths(sbf_max, widths);
    move_bitmaps();
}

void FStatusBar::set_coord_info(double x, double y, bool aux)
{
    wxString str;
    double val = 0;
    if (!extra_value.IsEmpty())
        val = get_value_for_point(e_code, e_numbers, x, y);
    int r = str.Printf((aux ? fmt_aux : fmt_main), x, y, val);
    SetStatusText(r > 0 ? str : wxString(),  sbf_coord);
}

bool FStatusBar::set_extra_value(string const& s)
{
    if (!get_dt_code(s, e_code, e_numbers))
        return false;
    extra_value = s2wx(s);
    return true;
}

int FStatusBar::get_field_width(int field) const
{
    wxRect rect;
    GetFieldRect(field, rect);
    return rect.GetWidth() + 2 * GetBorderX();
}

//===============================================================
//                     ConfStatBarDlg
//===============================================================

namespace {
wxString avoid_proc_n(wxString s)
{
    s.Replace(wxT("%n"), wxT("%N"));
    return s;
}

wxBoxSizer *get_labeled_control_sizer(wxWindow *parent, wxString const& label,
                                      wxWindow *control, bool expand=0)
{
    wxBoxSizer *sizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add(new wxStaticText(parent, -1, label),
               0, wxALL|wxALIGN_CENTRE_VERTICAL, 5);
    sizer->Add(control, expand, wxALL, 5);
    return sizer;
}

} // anonymous namespace


BEGIN_EVENT_TABLE(ConfStatBarDlg, wxDialog)
    EVT_BUTTON(wxID_APPLY, ConfStatBarDlg::OnApply)
    EVT_BUTTON(wxID_CLOSE, ConfStatBarDlg::OnClose)
END_EVENT_TABLE()

ConfStatBarDlg::ConfStatBarDlg(wxWindow* parent, wxWindowID id, FStatusBar* sb_)
    //explicit conversion of title to wxString() is neccessary
  : wxDialog(parent, id, wxString(wxT("Configure Status Bar")),
             wxDefaultPosition, wxDefaultSize, 
             wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER),
    sb(sb_)
{
    wxBoxSizer *top_sizer = new wxBoxSizer(wxVERTICAL);

    wxStaticBoxSizer *w_sizer  = new wxStaticBoxSizer(wxVERTICAL, this, 
                                                      wxT("field widths"));
    whint_sc = new SpinCtrl(this, -1, sb->get_field_width(sbf_hint1), 
                            0, 200, 50);
    w_sizer->Add(get_labeled_control_sizer(this, wxT("mouse hints: "), 
                                           whint_sc));

    width_sc = new SpinCtrl(this, -1, sb->get_field_width(sbf_coord), 
                            0, 400, 50);
    w_sizer->Add(get_labeled_control_sizer(this, wxT("cursor position info: "),
                                           width_sc));
    top_sizer->Add(w_sizer, 0, wxALL|wxEXPAND, 5);

    wxStaticBoxSizer *f_sizer  = new wxStaticBoxSizer(wxVERTICAL, this, 
                                   wxT("cursor position info"));
    fm_tc = new wxTextCtrl(this, -1, sb->fmt_main);
    f_sizer->Add(new wxStaticText(this, -1, 
                      wxT("format of printf function:")), wxALL, 5);
    f_sizer->Add(get_labeled_control_sizer(this, 
                               wxT("at main plot "), fm_tc, 1),
                   0, wxEXPAND);

    fa_tc = new wxTextCtrl(this, -1, sb->fmt_aux);
    f_sizer->Add(get_labeled_control_sizer(this, 
                               wxT("at auxiliary plot "), fa_tc, 1),
                   0, wxEXPAND);
    ff_tc = new wxTextCtrl(this, -1, sb->get_extra_value());
    f_sizer->Add(get_labeled_control_sizer(this, 
                                   wxT("3 values are used: x, y,"), ff_tc, 1),
                 0, wxEXPAND);
    top_sizer->Add(f_sizer, 0, wxALL|wxEXPAND, 5);

    add_apply_close_buttons(this, top_sizer);
    SetSizerAndFit(top_sizer);
}

void ConfStatBarDlg::OnApply (wxCommandEvent&)
{
    sb->set_widths(whint_sc->GetValue(), width_sc->GetValue());
    sb->fmt_main = avoid_proc_n(fm_tc->GetValue());
    sb->fmt_aux = avoid_proc_n(fa_tc->GetValue());
    bool r = sb->set_extra_value(wx2s(ff_tc->GetValue()));
    if (!r)
        ff_tc->SetSelection(-1, -1);
}


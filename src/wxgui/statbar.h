// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License version 2 or (at your option) 3
// $Id$

#ifndef FITYK__WX_STATBAR__H__
#define FITYK__WX_STATBAR__H__

#include <vector>
#include <string>
#include <wx/config.h>

#include "cmn.h" //SpinCtrl, close_it

enum StatusBarField { sbf_text, sbf_hint1, sbf_hint2, sbf_coord, sbf_max };  

/// Status bar in Fityk
class FStatusBar: public wxStatusBar 
{
public:
    wxString fmt_main, fmt_aux;

    FStatusBar(wxWindow *parent);
    void OnSize(wxSizeEvent& event) { move_bitmaps(); event.Skip(); }
    void move_bitmaps();
    void set_hint(std::string const& left, std::string const& right);
    int get_coord_width() const { return m_statusWidths[sbf_coord]; }
    int get_hint_width() const { return m_statusWidths[sbf_hint1]; }
    void set_widths(int hint, int coord);
    void set_hint_width(int w);
    void set_coord_info(double x, double y, bool aux=false);
    bool set_extra_value(std::string const& s);
    wxString const& get_extra_value() const { return extra_value; }
    void save_settings(wxConfigBase *cf) const;
    void read_settings(wxConfigBase *cf);

private:
    int widths[4]; //4==sbf_max
    wxString extra_value;
    std::vector<int> e_code;
    std::vector<double> e_numbers;
    wxStaticBitmap *statbmp1, *statbmp2;
    DECLARE_EVENT_TABLE()
};

/// Status bar configuration dialog
class ConfStatBarDlg: public wxDialog
{
public:
    ConfStatBarDlg(wxWindow* parent, wxWindowID id, FStatusBar* sb_);
    void OnApply (wxCommandEvent& event);
    void OnClose (wxCommandEvent&) { close_it(this); }
private:
    FStatusBar *sb;
    SpinCtrl *width_sc, *whint_sc;
    wxTextCtrl *fm_tc, *fa_tc, *ff_tc;
    DECLARE_EVENT_TABLE()
};

#endif

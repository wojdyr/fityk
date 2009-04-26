// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License version 2 or (at your option) 3
// $Id$

#ifndef FITYK_WX_STATBAR_H_
#define FITYK_WX_STATBAR_H_

#include <vector>
#include <string>
#include <wx/config.h>

#include "cmn.h" //SpinCtrl, close_it


// used by MainPlot to set hints on FStatusBar
class HintReceiver
{
public:
    virtual void set_hints(std::string const& left, std::string const& right)=0;
};

/// Status bar in Fityk
class FStatusBar: public wxPanel, public HintReceiver
{
    friend class ConfStatBarDlg;
public:
    FStatusBar(wxWindow *parent);

    void set_text(wxString const& text_) { text->SetLabel(text_); }
    wxString get_text() const { return text->GetLabel(); }
    void set_hints(std::string const& left, std::string const& right);
    void set_coords(double x, double y, PlotTypeEnum pte);
    void clear_coords() { coords->SetLabel(wxEmptyString); }
    bool set_extra_value(std::string const& s);
    wxString const& get_extra_value() const { return extra_value; }

    void save_settings(wxConfigBase *cf) const;
    void read_settings(wxConfigBase *cf);

    void OnPrefButton(wxCommandEvent&);

private:
    // a number calculated as a function of cursor coordinates x and y and
    // shown at the status bar (as a 3rd number, after x and y)
    wxString extra_value; 
    std::vector<int> e_code; // bytecode for calculating extra value
    std::vector<double> e_numbers; // numbers for the bytecode in e_code

    // The format for numbers is "% a.bf" where a=int_len+x_prec, b=x_prec
    static const int int_len = 4;
    int x_prec; // precision of x coordinate shown at status bar
    int y_prec; // precision of y
    int e_prec; // precision of extra value

    bool show_btn;
    bool show_hints;

    wxString fmt_main, fmt_aux;
    wxStaticText *text;
    wxStaticText *coords;
    wxStaticText *lmouse_hint, *rmouse_hint;
    wxBitmapButton *prefbtn;
    wxStaticBitmap *mousebmp;
    wxSplitterWindow *split;

    void set_coords_format();
    void show_or_hide();
};

/// Status bar configuration dialog
class ConfStatBarDlg: public wxDialog
{
public:
    ConfStatBarDlg(wxWindow* parent, wxWindowID id, FStatusBar* sb_);
    void OnApply (wxCommandEvent&);
    void OnExtraValueChange(wxCommandEvent&) { check_extra_value(); }
    void check_extra_value();
private:
    FStatusBar *sb;
    wxTextCtrl *extra_tc;
    wxCheckBox *show_btn_cb, *show_hints_cb;
    wxSpinCtrl *x_prec_sc, *y_prec_sc, *e_prec_sc;
    wxStaticBitmap *okbmp;
};

#endif // FITYK_WX_STATBAR_H_

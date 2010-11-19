// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
// $Id$

#ifndef FITYK_WX_STATBAR_H_
#define FITYK_WX_STATBAR_H_

#include <vector>
#include <string>
#include <wx/config.h>

#include "cmn.h" //SpinCtrl
#include "../eparser.h" //ExpressionParser


// used by MainPlot to set hints on FStatusBar
class HintReceiver
{
public:
    virtual void set_hints(std::string const& left, std::string const& right,
                           std::string const& mode_name,
                           std::string const& shift_left,
                           std::string const& shift_right)=0;
};

/// Status bar in Fityk
class FStatusBar: public wxPanel, public HintReceiver
{
    friend class ConfStatBarDlg;
public:
    FStatusBar(wxWindow *parent);

    void set_text(wxString const& text_) { text->SetLabel(text_); }
    wxString get_text() const { return text->GetLabel(); }
    void set_hints(std::string const& left, std::string const& right,
               std::string const& mode_name,
               std::string const& shift_left, std::string const& shift_right);
    void set_coords(double x, double y, PlotTypeEnum pte);
    void clear_coords() { coords->SetLabel(wxEmptyString); }
    bool set_extra_value(wxString const& s);
    void update_extra_fmt();
    wxString const& get_extra_value() const { return extra_value; }

    void save_settings(wxConfigBase *cf) const;
    void read_settings(wxConfigBase *cf);

    void OnPrefButton(wxCommandEvent&);
    void OnMouseBmpClicked(wxMouseEvent&);

    // show last coordinates (as example) in the current format
    void show_last_coordinates() { set_coords(last_x, last_y, last_pte); }

private:
    // a number calculated as a function of cursor coordinates x and y and
    // shown at the status bar (as a 3rd number, after x and y)
    wxString extra_value;
    ExpressionParser extra_parser; // parser/vm for calculating extra value
    int e_prec; // precision of extra value shown at status bar

    bool show_btn;
    bool show_hints;

    double last_x, last_y;
    PlotTypeEnum last_pte;

    wxString extra_fmt;
    wxStaticText *text;
    wxStaticText *coords;
    wxStaticText *lmouse_hint, *rmouse_hint;
    wxBitmapButton *prefbtn;
    wxStaticBitmap *mousebmp;
    wxSplitterWindow *split;

    void show_or_hide();
};

/// Status bar configuration dialog
class ConfStatBarDlg: public wxDialog
{
public:
    ConfStatBarDlg(wxWindow* parent, wxWindowID id, FStatusBar* sb_);
    void OnShowBtnCheckbox(wxCommandEvent& event);
    void OnShowHintsCheckbox(wxCommandEvent& event);
    void OnExtraValueChange(wxCommandEvent&);
    void OnPrecisionSpin(wxCommandEvent& event);
private:
    FStatusBar *sb;
    wxTextCtrl *extra_tc;
    wxCheckBox *show_btn_cb, *show_hints_cb;
    wxSpinCtrl *e_prec_sc;
    wxStaticBitmap *okbmp;
};

#endif // FITYK_WX_STATBAR_H_

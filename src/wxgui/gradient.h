// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License version 2 or (at your option) 3
// $Id$

#ifndef FITYK__WX_GRADIENT__H__
#define FITYK__WX_GRADIENT__H__

#include <vector>

class SpinCtrl;

/// displays colors from data member from left to right (one pixel - one color)
template<typename UpdateCalleeT>
class ColorGradientDisplay : public wxPanel
{
public:
    std::vector<wxColour> data;

    ColorGradientDisplay(wxWindow *parent, 
                      UpdateCalleeT *callee, void (UpdateCalleeT::*callback)())
        : wxPanel(parent, -1), updateCallee(callee), updateCallback(callback) 
    { 
        Connect(wxEVT_PAINT, 
                  wxPaintEventHandler(ColorGradientDisplay::OnPaint)); 
    }
    void OnPaint(wxPaintEvent&);
    bool was_resized() { return GetClientSize().GetWidth() != (int)data.size();}
private:
    UpdateCalleeT *updateCallee;
    void (UpdateCalleeT::*updateCallback)();
};


template<typename UpdateCalleeT>
void ColorGradientDisplay<UpdateCalleeT>::OnPaint(wxPaintEvent&)
{
    if (was_resized())
        (updateCallee->*updateCallback)();
    wxPaintDC dc(this);
    int height = GetClientSize().GetHeight();
    wxPen pen;
    for (size_t i = 0; i < data.size(); ++i) {
        pen.SetColour(data[i]);
        dc.SetPen(pen);
        dc.DrawLine(i, 0, i, height);
    }
}


class ColorSpinSelector : public wxPanel
{
public:
    SpinCtrl *r, *g, *b;

    ColorSpinSelector(wxWindow *parent, wxString const& title, 
                      wxColour const& col);
    void OnSelector(wxCommandEvent &);
    DECLARE_EVENT_TABLE()
};


class GradientDlg : public wxDialog
{
public:
    GradientDlg(wxWindow *parent, wxWindowID id, 
                wxColour const& first_col, wxColour const& last_col);
    void OnSpinEvent(wxSpinEvent &) { update_gradient_display(); }
    void OnRadioChanged(wxCommandEvent &) { update_gradient_display(); }
    wxColour get_value(float x);
private:
    wxRadioBox *kind_rb;
    ColorSpinSelector *from, *to;
    ColorGradientDisplay<GradientDlg> *display;

    void update_gradient_display();

    DECLARE_EVENT_TABLE()
};


template<typename calleeT>
class GradientDlgWithApply : public GradientDlg
{
public:
    GradientDlgWithApply(wxWindow *parent, wxWindowID id, 
                wxColour const& first_col, wxColour const& last_col,
                calleeT *callee_, void (calleeT::*callback_)(GradientDlg*))
        : GradientDlg(parent, id, first_col, last_col), 
          callee(callee_), callback(callback_) 
    { 
        Connect(wxID_APPLY, wxEVT_COMMAND_BUTTON_CLICKED,
                  wxCommandEventHandler(GradientDlgWithApply::OnApply)); 
        Connect(wxID_CLOSE, wxEVT_COMMAND_BUTTON_CLICKED,
                  wxCommandEventHandler(GradientDlgWithApply::OnClose)); 
    }
    void OnApply(wxCommandEvent &) { (callee->*callback)(this); }
    void OnClose(wxCommandEvent&) { close_it(this); }
private:
    calleeT *callee;
    void (calleeT::*callback)(GradientDlg *);
};

#endif

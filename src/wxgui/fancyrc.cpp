// Purpose: FancyRealCtrl (numeric wxTextCtrl + wxSlider + "lock" button)
// Copyright: (c) 2007 Marcin Wojdyr 
// Licence: wxWidgets licence 
// $Id$

/// In this file:
///  Fancy Real Control (FancyRealCtrl), which changes the value of 
///  parameter at the sidebar, and helpers

#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include "fancyrc.h"
#include "cmn.h" //KFTextCtrl

#include "img/lock.xpm"
#include "img/lock_open.xpm"

using namespace std;

static 
wxString double2wxstr(double v) { return wxString::Format(wxT("%g"), v); }

//===============================================================
//                      ValueChangingWidget
//===============================================================

/// small widget used to change value of associated wxTextCtrl with real number
class ValueChangingWidget : public wxSlider
{
public:
    ValueChangingWidget(wxWindow* parent, wxWindowID id, FancyRealCtrl* frc_)
        : wxSlider(parent, id, 0, -100, 100, wxDefaultPosition, wxSize(60, -1)),
          frc(frc_), timer(this, -1), button(0) {}

    void OnTimer(wxTimerEvent &event);
    void OnThumbTrack(wxScrollEvent&); 
    void OnMouse(wxMouseEvent &event);

private:
    FancyRealCtrl *frc;
    wxTimer timer;
    char button;
    
    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(ValueChangingWidget, wxSlider)
    EVT_TIMER(-1, ValueChangingWidget::OnTimer)
    EVT_MOUSE_EVENTS(ValueChangingWidget::OnMouse)
    EVT_SCROLL_THUMBTRACK(ValueChangingWidget::OnThumbTrack)
END_EVENT_TABLE()

void ValueChangingWidget::OnTimer(wxTimerEvent&)
{
    if (button == 'l') {
        frc->ChangeValue(GetValue()*0.001);
    }
    else if (button == 'm') {
        frc->ChangeValue(GetValue()*0.0001);
    }
    else if (button == 'r') {
        frc->ChangeValue(GetValue()*0.00001);
    }
    else {
        assert (button == 0);
        timer.Stop();
        SetValue(0);
        frc->OnStopChanging();
    }
}

void ValueChangingWidget::OnThumbTrack(wxScrollEvent&) 
{ 
    if (!timer.IsRunning()) 
        timer.Start(100);
}

void ValueChangingWidget::OnMouse(wxMouseEvent &event)
{
    if (event.LeftIsDown())
        button = 'l';
    else if (event.RightIsDown())
        button = 'r';
    else if (event.MiddleIsDown())
        button = 'm';
    else
        button = 0;
    event.Skip();
}

//===============================================================
//                          FancyRealCtrl
//===============================================================

BEGIN_EVENT_TABLE(FancyRealCtrl, wxPanel)
    EVT_TEXT_ENTER(-1, FancyRealCtrl::OnTextEnter)
    EVT_BUTTON(-1, FancyRealCtrl::OnLockButton)
END_EVENT_TABLE()

FancyRealCtrl::FancyRealCtrl(wxWindow* parent, wxWindowID id, 
                  double value, wxString const& tip, bool locked_, 
                  V1Callback<FancyRealCtrl const*> const& changing_value_cb,
                  V1Callback<FancyRealCtrl const*> const& changed_value_cb,
                  V1Callback<FancyRealCtrl const*> const& toggled_lock_cb)
    : wxPanel(parent, id), initial_value(value), locked(locked_), 
      changing_value_callback(changing_value_cb),
      changed_value_callback(changed_value_cb), 
      toggled_lock_callback(toggled_lock_cb)
{
    wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
    // KFTextCtrl is a wxTextCtrl which sends wxEVT_COMMAND_TEXT_ENTER 
    // when loses the focus
    tc = new KFTextCtrl(this, -1, double2wxstr(value)); 
    tc->SetToolTip(tip);
    tc->SetEditable(!locked);
    sizer->Add(tc, 1, wxALL|wxALIGN_CENTER_VERTICAL|wxEXPAND, 1);
    lock_btn = new wxBitmapButton(this, -1, GetLockBitmap(),
                                  wxDefaultPosition, wxDefaultSize,
                                  wxNO_BORDER);
    sizer->Add(lock_btn, 0, wxALL|wxALIGN_CENTER_VERTICAL, 0);
    vch = new ValueChangingWidget(this, -1, this);
    vch->Enable(!locked);
    sizer->Add(vch, 0, wxALL|wxALIGN_CENTER_VERTICAL, 1);
    SetSizer(sizer);
}

void FancyRealCtrl::SetTemporaryValue(double value) 
{ 
    tc->SetValue(double2wxstr(value)); 
}

void FancyRealCtrl::AddValue(double term)
{
    double t;
    bool ok = tc->GetValue().ToDouble(&t);
    if (!ok)
        return;
    // changing_value_callback() is called for values different than
    // initial_value, before and after change, usually twice for every value
    if (t != initial_value)
        changing_value_callback(this);
    t += term;
    tc->SetValue(double2wxstr(t));
    if (t != initial_value)
        changing_value_callback(this);
}

double FancyRealCtrl::GetValue() const
{
    double t;
    bool ok = tc->GetValue().ToDouble(&t);
    return ok ? t : initial_value;
}

void FancyRealCtrl::OnStopChanging()
{
    if (tc->GetValue() != double2wxstr(initial_value)) {
        double t;
        bool ok = tc->GetValue().ToDouble(&t);
        if (ok) {
            initial_value = t;
            changed_value_callback(this);
        }
        else
            tc->SetValue(double2wxstr(initial_value));
    }
}

void FancyRealCtrl::ToggleLock()
{
    locked = !locked;
    lock_btn->SetBitmapLabel(GetLockBitmap());
    vch->Enable(!locked);
    tc->SetEditable(!locked);
}

void FancyRealCtrl::ConnectToOnKeyDown(wxObjectEventFunction function, 
                                         wxEvtHandler* sink)
{
    lock_btn->Connect(wxID_ANY, wxEVT_KEY_DOWN, function, 0, sink);
    vch->Connect(wxID_ANY, wxEVT_KEY_DOWN, function, 0, sink);
}

wxBitmap FancyRealCtrl::GetLockBitmap() const
{
    return wxBitmap(locked ? lock_xpm : lock_open_xpm);
}



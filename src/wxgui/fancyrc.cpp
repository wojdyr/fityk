// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License version 2
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
#include "sidebar.h" //SideBar::draw_function_draft()
#include "cmn.h" //wx2s, s2wx, KFTextCtrl
#include "../ui.h" //exec_command()

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

    void OnThumbTrack(wxScrollEvent &WXUNUSED(event)) { 
        if (!timer.IsRunning()) {
            timer.Start(100);
        }
    }
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

void ValueChangingWidget::OnTimer(wxTimerEvent &WXUNUSED(event))
{
    if (button == 'l') {
        frc->change_value(GetValue()*0.001);
    }
    else if (button == 'm') {
        frc->change_value(GetValue()*0.0001);
    }
    else if (button == 'r') {
        frc->change_value(GetValue()*0.00001);
    }
    else {
        assert (button == 0);
        timer.Stop();
        SetValue(0);
        frc->on_stop_changing();
    }
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
                             double value, string const& tc_name, bool locked_,
                             SideBar const* draw_handler_)
    : wxPanel(parent, id), initial_value(value), name(tc_name), 
      locked(locked_), draw_handler(draw_handler_)
{
    wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
    tc = new KFTextCtrl(this, -1, double2wxstr(value)); 
    tc->SetToolTip(s2wx(name));
    sizer->Add(tc, 1, wxALL|wxALIGN_CENTER_VERTICAL|wxEXPAND, 1);
    lock_btn = new wxBitmapButton(this, -1, get_lock_bitmap(),
                                  wxDefaultPosition, wxDefaultSize,
                                  wxNO_BORDER);
    sizer->Add(lock_btn, 0, wxALL|wxALIGN_CENTER_VERTICAL, 0);
    vch = new ValueChangingWidget(this, -1, this);
    sizer->Add(vch, 0, wxALL|wxALIGN_CENTER_VERTICAL, 1);
    SetSizer(sizer);
}

void FancyRealCtrl::set_temporary_value(double value) 
{ 
    tc->SetValue(double2wxstr(value)); 
}

void FancyRealCtrl::set(double value, string const& tc_name)
{
    tc->SetValue(double2wxstr(value));
    initial_value = value;
    if (name != tc_name) {
        name = tc_name;
        tc->SetToolTip(s2wx(name));
    }
}

void FancyRealCtrl::change_value(double factor)
{
    double t;
    bool ok = tc->GetValue().ToDouble(&t);
    if (!ok)
        return;
    if (t != initial_value)
        draw_handler->draw_function_draft(this);
    t += fabs(initial_value) * factor;
    tc->SetValue(double2wxstr(t));
    if (t != initial_value)
        draw_handler->draw_function_draft(this);
}

double FancyRealCtrl::get_value() const
{
    double t;
    bool ok = tc->GetValue().ToDouble(&t);
    return ok ? t : initial_value;
}

void FancyRealCtrl::on_stop_changing()
{
    if (tc->GetValue() != double2wxstr(initial_value)) {
        double t;
        bool ok = tc->GetValue().ToDouble(&t);
        if (ok) {
            initial_value = t;
            exec_command(name + " = ~" + wx2s(tc->GetValue()));
        }
        else
            tc->SetValue(double2wxstr(initial_value));
    }
}

void FancyRealCtrl::toggle_lock(bool exec)
{
    locked = !locked;
    lock_btn->SetBitmapLabel(get_lock_bitmap());
    if (exec)
        exec_command(name + " = " + (locked ? "{" : "~{") + name + "}");
}

void FancyRealCtrl::connect_to_onkeydown(wxObjectEventFunction function, 
                                         wxEvtHandler* sink)
{
    lock_btn->Connect(wxID_ANY, wxEVT_KEY_DOWN, function, 0, sink);
    vch->Connect(wxID_ANY, wxEVT_KEY_DOWN, function, 0, sink);
}

wxBitmap FancyRealCtrl::get_lock_bitmap() const
{
    return wxBitmap(locked ? lock_xpm : lock_open_xpm);
}



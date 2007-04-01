// Purpose: FancyRealCtrl (numeric wxTextCtrl + wxSlider + "lock" button)
// Copyright: (c) 2007 Marcin Wojdyr 
// Licence: wxWidgets licence 
// $Id$

#ifndef FITYK__WX_FANCYRC__H__
#define FITYK__WX_FANCYRC__H__

#include <wx/tooltip.h>
#include <math.h>

#include "callback.h"

class SideBar;
class ValueChangingWidget;

class FancyRealCtrl : public wxPanel
{
public:
    FancyRealCtrl(wxWindow* parent, wxWindowID id, 
                  double value, wxString const& tip, bool locked_,
                  V1Callback<FancyRealCtrl const*> const& changing_value_cb,
                  V1Callback<FancyRealCtrl const*> const& changed_value_cb,
                  V1Callback<FancyRealCtrl const*> const& toggled_lock_cb);

    /// calls changing_value_callback
    void AddValue(double term);
    void ChangeValue(double factor) { AddValue(fabs(initial_value)*factor); }
    /// calls changed_value_callback
    void OnStopChanging();

    double GetValue() const;
    wxString GetValueStr() const { return tc->GetValue(); }
    /// changes only value in text-ctrl
    void SetTemporaryValue(double value); 
    void SetValue(double v) { SetTemporaryValue(v); initial_value = v; }


    void SetTip(wxString const& tt) { tc->SetToolTip(tt); }
    wxString GetTip() const { return tc->GetToolTip()->GetTip(); }

    void ToggleLock(); 
    bool IsLocked() const { return locked; }

    /// redirects OnKeyDown events from lock button and slider
    void ConnectToOnKeyDown(wxObjectEventFunction function, 
                              wxEvtHandler* sink);

private:
    double initial_value;
    bool locked;
    wxTextCtrl *tc;
    wxBitmapButton *lock_btn;
    ValueChangingWidget *vch; 

    //callbacks
    V1Callback<FancyRealCtrl const*> changing_value_callback;
    V1Callback<FancyRealCtrl const*> changed_value_callback;
    V1Callback<FancyRealCtrl const*> toggled_lock_callback;

    wxBitmap GetLockBitmap() const;
    void OnLockButton(wxCommandEvent&) 
                                { ToggleLock(); toggled_lock_callback(this); }
    void OnTextEnter(wxCommandEvent &) { OnStopChanging(); }

    DECLARE_EVENT_TABLE()
};

#endif

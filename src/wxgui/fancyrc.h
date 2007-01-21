// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#ifndef FITYK__WX_FANCYRC__H__
#define FITYK__WX_FANCYRC__H__

#include <string>

class SideBar;
class KFTextCtrl;

/// tiny callback class
class ValueChangingHandler 
{
public:
    virtual ~ValueChangingHandler() {}
    virtual void change_value(double factor) = 0;
    virtual void on_stop_changing() = 0;
};


class FancyRealCtrl : public wxPanel, public ValueChangingHandler
{
public:
    FancyRealCtrl(wxWindow* parent, wxWindowID id, 
                  double value, std::string const& tc_name, bool locked_,
                  SideBar const* draw_handler_);
    void change_value(double factor);
    void on_stop_changing();
    void OnTextEnter(wxCommandEvent &) { on_stop_changing(); }
    void OnLockButton(wxCommandEvent&) { toggle_lock(true); }
    void set(double value, std::string const& tc_name);
    double get_value() const;
    std::string get_name() const { return name; }
    bool get_locked() const { return locked; }
    void set_temporary_value(double value); 
    void toggle_lock(bool exec); 

    DECLARE_EVENT_TABLE()
private:
    double initial_value;
    std::string name;
    bool locked;
    SideBar const* draw_handler;
    KFTextCtrl *tc;
    wxBitmapButton *lock_btn;

    wxBitmap get_lock_bitmap() const;
};

#endif

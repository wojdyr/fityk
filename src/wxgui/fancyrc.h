// Purpose: ParameterPanel and LockableRealCtrl (both for input of real numbers)
// Copyright 2007-2010 Marcin Wojdyr
// Licence: wxWidgets licence
// $Id$

#ifndef FITYK_WX_FANCYRC_H_
#define FITYK_WX_FANCYRC_H_

#include <wx/tooltip.h>
#include <math.h>
#include <vector>

class SideBar;
class ValueChangingWidget;

class ValueChangingWidgetObserver
{
public:
    virtual void change_value(ValueChangingWidget *w, double factor) = 0;
    virtual void finalize_changes() = 0;
};


class LockButton;

// LockableRealCtrl - input widget (text + lock-button) for real values
// that can be marked as locked (constant).
class LockableRealCtrl : public wxPanel
{
public:
    LockableRealCtrl(wxWindow* parent, bool percent=false);
    bool is_locked() const;
    void set_lock(bool locked);
    bool is_null() const { return text->IsEmpty(); }
    bool is_nonzero() const; // non-zero value or not locked
    double get_value() const;
    wxString get_string() const { return text->GetValue(); }
    void set_string(wxString const& s) { text->ChangeValue(s); }
    void set_value(double value);
    void Clear() { text->ChangeValue(wxT("")); }
    wxTextCtrl* get_text_ctrl() const { return text; }

private:
    wxTextCtrl *text;
    LockButton *lock;
};


class ParameterPanelObserver
{
public:
    virtual void on_parameter_changing(const std::vector<double>& values) = 0;
    virtual void on_parameter_changed(int n) = 0;
    virtual void on_parameter_lock_toggled(int n, bool locked) = 0;
};

struct ParameterRowData
{
    wxTextCtrl *text;
    wxBoxSizer *rsizer;
    LockButton* lock;
    ValueChangingWidget *arm;
    wxStaticText *label, *label2;
};

/// A list of input widgets, each row contains label, text, lock-button and
/// slider. In alternative "disabled" mode a row contains label, disabled text
/// and a second label in place of the lock-button and slider.
/// The text control is meant for input of real number,
/// the lock button is for marking the number as constant (locked),
/// the slider is (ab)used as a handle for changing smoothly the number.
/// There is a title (a text label) at the top.
class ParameterPanel : public wxPanel, public ValueChangingWidgetObserver
{
public:
    ParameterPanel(wxWindow* parent, wxWindowID id,
                   ParameterPanelObserver *observer);
    void set_normal_parameter(int n, const wxString& label, double value,
                              bool locked, const wxString& label2);
    void set_disabled_parameter(int n, const wxString& label, double value,
                                const wxString& label2);
    void delete_row_range(int begin, int end);
    wxString get_label2(int n) const;
    double get_value(int n) const;
    void set_value(int n, double value);
    int get_count() const { return values_.size(); }
    wxString get_title() const { return title_st_->GetLabel(); }
    void set_title(const wxString& title) { title_st_->SetLabel(title); }
    void set_key_sink(wxEvtHandler* sink, wxObjectEventFunction method);

    // implementation of ValueChangingWidgetObserver
    virtual void change_value(ValueChangingWidget *w, double factor);
    virtual void finalize_changes();

private:
    ParameterPanelObserver* observer_;
    std::vector<ParameterRowData> rows_;
    std::vector<double> values_;
    wxStaticText* title_st_;
    wxFlexGridSizer *grid_sizer_;
    int active_item_;
    double old_value_;
    wxEvtHandler* key_sink_;
    wxObjectEventFunction key_sink_method_;

    void change_mode(int n, bool normal);
    void append_row();
    int find_in_rows(wxObject* w);
    void OnLockButton(wxCommandEvent& event);
    void OnTextEnter(wxCommandEvent &event);
    void OnMouseWheel(wxMouseEvent &event);
};

// access to xpm bitmaps
const char **get_lock_xpm();
const char **get_lock_open_xpm();

#endif

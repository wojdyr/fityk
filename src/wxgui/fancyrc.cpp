// Purpose: ParameterPanel and LockableRealCtrl (both for input of real numbers)
// Copyright 2007-2010 Marcin Wojdyr
// Licence: wxWidgets licence
// $Id$


#include <wx/wx.h>

#include "fancyrc.h"
#include "cmn.h" //KFTextCtrl

#include "img/lock.xpm"
#include "img/lock_open.xpm"

using namespace std;

static
wxString double2wxstr(double v) { return wxString::Format(wxT("%.7g"), v); }

//===============================================================
//                      ValueChangingWidget
//===============================================================

/// small widget used to change value of associated wxTextCtrl with real number
class ValueChangingWidget : public wxSlider
{
public:
    ValueChangingWidget(wxWindow* parent, wxWindowID id,
                        ValueChangingWidgetObserver* observer);
private:
    ValueChangingWidgetObserver *observer_;
    wxTimer timer_;

    void OnTimer(wxTimerEvent &event);
    void OnThumbTrack(wxScrollEvent&);
};


ValueChangingWidget::ValueChangingWidget(wxWindow* parent, wxWindowID id,
                                         ValueChangingWidgetObserver* observer)
    : wxSlider(parent, id, 0, -100, 100, wxDefaultPosition, wxSize(60, -1)),
      observer_(observer),
      timer_(this, -1)
{
    Connect(wxEVT_TIMER, wxTimerEventHandler(ValueChangingWidget::OnTimer));
    Connect(wxEVT_SCROLL_THUMBTRACK,
                      wxScrollEventHandler(ValueChangingWidget::OnThumbTrack));
}

void ValueChangingWidget::OnTimer(wxTimerEvent&)
{
    wxMouseState state = wxGetMouseState();
    int slider_pos = GetValue();
    if (state.LeftIsDown())
        observer_->change_value(this, slider_pos*0.001);
    else if (state.MiddleIsDown())
        observer_->change_value(this, slider_pos*0.0001);
    else if (state.RightIsDown())
        observer_->change_value(this, slider_pos*0.00001);
    else {
        timer_.Stop();
        SetValue(0); // move the slider to the central position
        observer_->finalize_changes();
    }
}

void ValueChangingWidget::OnThumbTrack(wxScrollEvent&)
{
    if (!timer_.IsRunning())
        timer_.Start(100);
}


class LockButton : public wxBitmapButton
{
public:
    LockButton(wxWindow* parent, bool connect_default_handler=true)
        : wxBitmapButton(parent, -1, wxBitmap(lock_xpm),
                         wxDefaultPosition, wxDefaultSize, wxNO_BORDER),
          locked_(true)
    {
        if (connect_default_handler)
            Connect(GetId(), wxEVT_COMMAND_BUTTON_CLICKED,
                    wxCommandEventHandler(LockButton::OnClick));
    }

    void set_lock(bool locked)
    {
        locked_ = locked;
        SetBitmapLabel(wxBitmap(locked_ ? lock_xpm : lock_open_xpm));
    }

    void toggle_lock() { set_lock(!locked_); }
    bool is_locked() const { return locked_; }

private:
    bool locked_;
    void OnClick(wxCommandEvent&) { toggle_lock(); }
};

LockableRealCtrl::LockableRealCtrl(wxWindow* parent, bool percent)
    : wxPanel(parent, -1)
{
    if (percent)
        text = new KFTextCtrl(this, -1, wxT(""), 50, wxTE_RIGHT);
    else
        text = new KFTextCtrl(this, -1, wxT(""));
    lock = new LockButton(this);
    wxSizer *sizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add(text, wxSizerFlags().Center());
    if (percent)
        sizer->Add(new wxStaticText(this, -1, wxT("%")),
                   wxSizerFlags().Center());
    sizer->Add(lock, wxSizerFlags().Center());
    SetSizerAndFit(sizer);
}

double LockableRealCtrl::get_value() const
{
    double d;
    bool ok = text->GetValue().ToDouble(&d);
    return ok ? d : 0.;
}

void LockableRealCtrl::set_value(double value)
{
    if (value != get_value())
        text->ChangeValue(wxString::Format(wxT("%g"), value));
}

bool LockableRealCtrl::is_locked() const
{
    return lock->is_locked();
}

void LockableRealCtrl::set_lock(bool locked)
{
    lock->set_lock(locked);
}


ParameterPanel::ParameterPanel(wxWindow* parent, wxWindowID id,
                               ParameterPanelObserver *observer)
    : wxPanel(parent, id),
      observer_(observer),
      active_item_(-1),
      key_sink_(NULL)
{
    wxBoxSizer *top_sizer = new wxBoxSizer(wxVERTICAL);
    title_st_ = new wxStaticText(this, -1, wxEmptyString);
    top_sizer->Add(title_st_, wxSizerFlags().Center());
    grid_sizer_ = new wxFlexGridSizer(3, 0, 0);
    grid_sizer_->AddGrowableCol(1);
    top_sizer->Add(grid_sizer_, wxSizerFlags(1).Expand());
    //SetSizerAndFit(top_sizer);
    SetSizer(top_sizer);
    SetAutoLayout(true);
}

wxString ParameterPanel::get_label2(int n) const
{
    return rows_[n].text->GetToolTip()->GetTip();
}

void ParameterPanel::change_value(ValueChangingWidget *w, double factor)
{
    int n = 0;
    for ( ; n != get_count(); ++n)
        if (rows_[n].arm == w)
            break;
    assert(n != get_count());

    double term = fabs(values_[n]) * factor;
    if (term == 0)
        return;
    if (active_item_ != n) {
        active_item_ = n;
        old_value_ = values_[n];
    }
    values_[n] += term;
    rows_[n].text->SetValue(double2wxstr(values_[n]));
    observer_->on_parameter_changing(values_);
}

void ParameterPanel::finalize_changes()
{
    if (active_item_ == -1)
        return;
    assert(active_item_ < get_count());
    observer_->on_parameter_changed(active_item_);
    active_item_ = -1;
}

double ParameterPanel::get_value(int n) const
{
    //double d;
    //bool ok = rows_[n].text->GetValue().ToDouble(&d);
    //return ok ? d : 0.;
    return values_[n];
}

void ParameterPanel::set_value(int n, double value)
{
    values_[n] = value;
    rows_[n].text->ChangeValue(double2wxstr(value));
}

void ParameterPanel::set_normal_parameter(int n, const wxString& label,
                                          double value, bool locked,
                                          const wxString& label2)
{
    change_mode(n, true);
    set_value(n, value);
    rows_[n].label->SetLabel(label);
    rows_[n].text->SetToolTip(label2);
    rows_[n].lock->set_lock(locked);
    rows_[n].text->SetEditable(!locked);
    rows_[n].arm->Enable(!locked);
}

void ParameterPanel::set_disabled_parameter(int n, const wxString& label,
                                            double value,
                                            const wxString& label2)
{
    change_mode(n, false);
    set_value(n, value);
    rows_[n].label->SetLabel(label);
    rows_[n].text->SetToolTip(label2);
    rows_[n].label2->SetLabel(label2);
}

void ParameterPanel::change_mode(int n, bool normal)
{
    assert (n <= get_count());
    if (n == get_count())
        append_row();
    ParameterRowData& row = rows_[n];
    if (row.text->IsEnabled() == normal)
        return;
    row.text->Enable(normal);
    row.lock->Show(normal);
    row.arm->Show(normal);
    row.label2->Show(!normal);
}

void ParameterPanel::append_row()
{
#if 1
    // Deleting/destroying controls causes strange problems in wx2.9.
    // To avoid it, we hide controls rather than delete it.
    if (rows_.size() > values_.size()) {
        ParameterRowData& row = rows_[values_.size()];
        row.label->Show(true);
        row.text->Show(true);
        row.lock->Show(true);
        row.arm->Show(true);
        values_.push_back(0.);
    }
#endif
    ParameterRowData data;
    data.label = new wxStaticText(this, -1, wxEmptyString);
    // KFTextCtrl is a wxTextCtrl which sends wxEVT_COMMAND_TEXT_ENTER
    // when un-focused.
    data.text = new KFTextCtrl(this, -1, wxEmptyString);
    data.rsizer = new wxBoxSizer(wxHORIZONTAL);
    data.lock = new LockButton(this, false);
    data.arm = new ValueChangingWidget(this, -1, this);
    data.label2 = new wxStaticText(this, -1, wxEmptyString);

    data.rsizer->Add(data.lock, wxSizerFlags().Center());
    data.rsizer->Add(data.arm, wxSizerFlags().Center());
    data.rsizer->Add(data.label2, wxSizerFlags().Center());

    grid_sizer_->Add(data.label, wxSizerFlags().Center());
    grid_sizer_->Add(data.text, wxSizerFlags().Expand().Center()
                                         .Border(wxLEFT|wxTOP|wxBOTTOM, 2));
    grid_sizer_->Add(data.rsizer, wxSizerFlags().Center());

    data.label2->Show(false);

    data.text->Connect(wxEVT_COMMAND_TEXT_ENTER,
                       wxCommandEventHandler(ParameterPanel::OnTextEnter),
                       NULL, this);
    data.text->Connect(wxEVT_MOUSEWHEEL,
                       wxMouseEventHandler(ParameterPanel::OnMouseWheel),
                       NULL, this);
    data.arm->Connect(wxEVT_MOUSEWHEEL,
                      wxMouseEventHandler(ParameterPanel::OnMouseWheel),
                      NULL, this);
    data.lock->Connect(wxEVT_COMMAND_BUTTON_CLICKED,
                       wxCommandEventHandler(ParameterPanel::OnLockButton),
                       NULL, this);

    rows_.push_back(data);
    values_.push_back(0.);
    if (key_sink_ != NULL) {
        data.lock->Connect(wxEVT_KEY_DOWN, key_sink_method_, NULL, key_sink_);
        data.arm->Connect(wxEVT_KEY_DOWN, key_sink_method_, NULL, key_sink_);
    }
}

void ParameterPanel::set_key_sink(wxEvtHandler* sink,
                                  wxObjectEventFunction method)
{
    key_sink_ = sink;
    key_sink_method_ = method;
}

void ParameterPanel::delete_row_range(int begin, int end)
{
    // Deleting/destroying controls causes strange problems in wx2.9.
    // To avoid it, we hide controls rather than delete it.
#if 0
    for (int i = 3*end - 1; i >= 3*begin; --i)
        grid_sizer_->Detach(i);
    for (int i = begin; i < end; ++i) {
        ParameterRowData& row = rows_[i];
        row.label->Destroy();
        row.text->Destroy();
        row.lock->Destroy();
        row.arm->Destroy();
        row.label2->Destroy();
    }
#else
    for (int i = begin; i < end; ++i) {
        ParameterRowData& row = rows_[i];
        row.label->Show(false);
        row.text->Show(false);
        row.lock->Show(false);
        row.arm->Show(false);
        row.label2->Show(false);
    }
    rows_.erase(rows_.begin() + begin, rows_.begin() + end);
#endif
    values_.erase(values_.begin() + begin, values_.begin() + end);
}

int ParameterPanel::find_in_rows(wxObject* w)
{
    for (int n = 0; n != get_count(); ++n)
        if (w == rows_[n].text || w == rows_[n].lock || w == rows_[n].arm)
            return n;
    assert(0);
    return 0;
}

void ParameterPanel::OnLockButton(wxCommandEvent& event)
{
    int n = find_in_rows(event.GetEventObject());
    rows_[n].lock->toggle_lock();
    bool locked = rows_[n].lock->is_locked();
    rows_[n].text->SetEditable(!locked);
    rows_[n].arm->Enable(!locked);
    observer_->on_parameter_lock_toggled(n, locked);
}

void ParameterPanel::OnTextEnter(wxCommandEvent &event)
{
    int n = find_in_rows(event.GetEventObject());
    wxTextCtrl *tc = rows_[n].text;
    if (tc->GetValue() == double2wxstr(values_[n]))
        return;
    double t;
    bool ok = tc->GetValue().ToDouble(&t);
    if (ok) {
        values_[n] = t;
        observer_->on_parameter_changed(n);
    }
    else
        tc->ChangeValue(double2wxstr(values_[n]));
}

void ParameterPanel::OnMouseWheel(wxMouseEvent &event)
{
    double change = 1e-5 * event.GetWheelRotation();
    if (event.ShiftDown())
        change *= 10;
    if (event.CmdDown())
        change *= 100;
    if (event.AltDown())
        change /= 20;
    int n = find_in_rows(event.GetEventObject());
    double new_value = values_[n] + fabs(values_[n]) * change;
    set_value(n, new_value);
    observer_->on_parameter_changed(n);
}


const char **get_lock_xpm() { return lock_xpm; }
const char **get_lock_open_xpm() { return lock_open_xpm; }


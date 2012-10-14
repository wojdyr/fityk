// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

/// In this file:
///  Auxiliary Plot, for displaying residuals, peak positions, etc. (AuxPlot)

#include <wx/wx.h>
#include <wx/clrpicker.h>
#include <wx/fontpicker.h>

#include "aplot.h"
#include "frame.h"
#include "plotpane.h"
#include "fityk/model.h"
#include "fityk/func.h"
#include "fityk/data.h"
#include "fityk/logic.h"

using namespace std;
using fityk::Data;
using fityk::Model;

enum {
    ID_aux_prefs            = 25310,
    ID_aux_plot0            = 25311,
    ID_aux_mark_pos         = 25340,
    ID_aux_yz_fit                  ,
    ID_aux_yz_auto
};


class AuxPlotConfDlg : public wxDialog
{
public:
    AuxPlotConfDlg(AuxPlot* ap);

private:
    AuxPlot *ap_;
    wxCheckBox* rev_cb_;
    wxSpinCtrl* zoom_sc_;
    wxColourPickerCtrl *bg_cp_, *active_cp_, *inactive_cp_, *axis_cp_;
    wxFontPickerCtrl *tics_fp_;

    void OnPlotKind(wxCommandEvent& event);
    void OnReversedDiff(wxCommandEvent& event);
    void OnMarkPeakPositions(wxCommandEvent& e) {ap_->OnMarkPeakPositions(e);}
    void OnAutoZoom(wxCommandEvent& event);
    void OnZoomSpin(wxSpinEvent& event);
    void OnColor(wxColourPickerEvent& event);
    void OnTicsFont(wxFontPickerEvent& event);
};


//===============================================================
//                           AuxPlot (auxiliary plot)
//===============================================================
BEGIN_EVENT_TABLE (AuxPlot, FPlot)
    EVT_PAINT (           AuxPlot::OnPaint)
    EVT_MOTION (          AuxPlot::OnMouseMove)
    EVT_LEAVE_WINDOW (    AuxPlot::OnLeaveWindow)
    EVT_LEFT_DOWN (       AuxPlot::OnLeftDown)
    EVT_LEFT_UP (         AuxPlot::OnLeftUp)
    EVT_RIGHT_DOWN (      AuxPlot::OnRightDown)
    EVT_MIDDLE_DOWN (     AuxPlot::OnMiddleDown)
    EVT_MENU (ID_aux_prefs, AuxPlot::OnPrefs)
    EVT_MENU_RANGE (ID_aux_plot0, ID_aux_plot0+10, AuxPlot::OnPopupPlot)
    EVT_MENU (ID_aux_mark_pos, AuxPlot::OnMarkPeakPositions)
    EVT_MENU (ID_aux_yz_fit, AuxPlot::OnPopupYZoomFit)
    EVT_MENU (ID_aux_yz_auto, AuxPlot::OnAutoZoom)
END_EVENT_TABLE()

AuxPlot::AuxPlot(wxWindow *parent, FPlot *master, wxString const& name)
    : FPlot(parent), master_(master), name_(name),
      y_zoom_(1.), y_zoom_base_(1.), fit_y_once_(false)
{
    overlay.switch_mode(Overlay::kVLine);
}

void AuxPlot::OnPaint(wxPaintEvent&)
{
    update_buffer_and_blit();
}

namespace {

inline double model_value(vector<Point>::const_iterator pt, Model const* model)
{
    return model->value(pt->x);
}

double diff_of_data_for_draw_data (vector<Point>::const_iterator i,
                                   Model const* model)
{
    return i->y - model_value(i, model);
}

double rdiff_of_data_for_draw_data (vector<Point>::const_iterator i,
                                    Model const* model)
{
    return model_value(i, model) - i->y;
}

double diff_stddev_of_data_for_draw_data (vector<Point>::const_iterator i,
                                          Model const* model)
{
    return (i->y - model_value(i, model)) / i->sigma;
}

double rdiff_stddev_of_data_for_draw_data (vector<Point>::const_iterator i,
                                           Model const* model)
{
    return (model_value(i, model) - i->y) / i->sigma;
}

double diff_chi2_of_data_for_draw_data (vector<Point>::const_iterator i,
                                        Model const* model)
{
    double t = (i->y - model_value(i, model)) / i->sigma;
    return t*t;
}

double diff_y_perc_of_data_for_draw_data (vector<Point>::const_iterator i,
                                          Model const* model)
{
    return i->y ? (i->y - model_value(i, model)) / i->y * 100 : 0;
}

double rdiff_y_perc_of_data_for_draw_data (vector<Point>::const_iterator i,
                                           Model const* model)
{
    return i->y ? (model_value(i, model) - i->y) / i->y * 100 : 0;
}

} // anonymous namespace

void AuxPlot::draw(wxDC &dc, bool monochrome)
{
    //printf("AuxPlot::draw()\n");
    int pos = frame->get_focused_data_index();
    Data const* data = ftk->get_data(pos);
    Model const* model = ftk->get_model(pos);
    if (auto_zoom_y_ || fit_y_once_) {
        fit_y_zoom(data, model);
        fit_y_once_ = false;
    }
    const int pixel_width = get_pixel_width(dc);
    const int pixel_height = get_pixel_height(dc);
    set_scale(pixel_width, pixel_height);
    if (monochrome) {
        dc.SetPen(*wxBLACK_PEN);
        dc.SetBrush(*wxBLACK_BRUSH);
    }
    else
        dc.SetPen(wxPen(xAxisCol, pen_width));

    if (mark_peak_pos_) {
        v_foreach (int, i, model->get_ff().idx) {
            realt x;
            if (ftk->mgr.get_function(*i)->get_center(&x)) {
                int X = xs.px(x - model->zero_shift(x));
                dc.DrawLine(X, 0, X, pixel_height);
            }
        }
    }

    if (kind_ == apk_empty || data->is_empty())
        return;

    if (x_axis_visible) {
        int Y0 = ys.px(0.);
        dc.DrawLine (0, Y0, pixel_width, Y0);
        if (kind_ == apk_diff)
            draw_zoom_text(dc, !monochrome);
    }
    if (y_axis_visible) {
        int X0 = xs.px(0.);
        dc.DrawLine (X0, 0, X0, pixel_height);
    }
    if (ytics_visible) {
        fityk::Rect rect(0, 0, ys.val(pixel_height), ys.val(0));
        draw_ytics(dc, rect, !monochrome);
    }

    double (*f)(vector<Point>::const_iterator, Model const*) = NULL;
    bool cummulative = false;
    if (kind_ == apk_diff)
        f = reversed_diff_ ? rdiff_of_data_for_draw_data
                           : diff_of_data_for_draw_data;
    else if (kind_ == apk_diff_stddev)
        f = reversed_diff_ ? rdiff_stddev_of_data_for_draw_data
                           : diff_stddev_of_data_for_draw_data;
    else if (kind_ == apk_diff_y_perc)
        f = reversed_diff_ ? rdiff_y_perc_of_data_for_draw_data
                           : diff_y_perc_of_data_for_draw_data;
    else if (kind_ == apk_cum_chi2) {
        f = diff_chi2_of_data_for_draw_data;
        cummulative = true;
    }
    wxColour col = monochrome ? dc.GetPen().GetColour() : wxNullColour;
    draw_data (dc, f, data, model, col, col, 0, cummulative);
}

/// print zoom info - how it compares to zoom of the master plot (e.g. "x3"),
/// it makes sense only for apk_diff plot, when master plot is not logarithmic
void AuxPlot::draw_zoom_text(wxDC& dc, bool set_pen)
{
    if (master_->get_y_scale().logarithm)
        return;
    if (set_pen)
        dc.SetTextForeground(xAxisCol);
    set_font(dc, *wxNORMAL_FONT);
#ifdef __WXMSW__
    // U+00D7 is not rendered correctly on Windows XP
    string s = "x" + S(y_zoom_);
#else
    string s = "\u00D7" + S(y_zoom_); // U+00D7 == &times;
#endif
    wxCoord w, h;
    dc.GetTextExtent (s2wx(s), &w, &h);
    dc.DrawText (s2wx(s), get_pixel_width(dc) - w - 2, 2);
}

void AuxPlot::OnMouseMove(wxMouseEvent &event)
{
    int X = event.GetX();
    frame->set_status_coords(xs.valr(X), ys.valr(event.GetY()), pte_aux);
    if (downX == INT_MIN) {
        if (X < move_plot_margin_width) {
            SetCursor(wxCURSOR_POINT_LEFT);
            X = -1; // don't draw lines
        }
        else if (X > GetClientSize().GetWidth() - move_plot_margin_width) {
            SetCursor(wxCURSOR_POINT_RIGHT);
            X = -1; // don't draw lines
        }
        else
            SetCursor(wxCURSOR_CROSS);
    }
    overlay.change_pos(X, 0);
    frame->plot_pane()->draw_vertical_lines(X, downX, this);
}

void AuxPlot::OnLeaveWindow (wxMouseEvent&)
{
    frame->clear_status_coords();
    overlay.change_pos(-1, -1);
    frame->plot_pane()->draw_vertical_lines(-1, -1, this);
}

bool AuxPlot::is_zoomable()
{
    return kind_ == apk_diff || kind_ == apk_diff_stddev
           || kind_ == apk_diff_y_perc || kind_ == apk_cum_chi2;
}

void AuxPlot::set_scale(int pixel_width, int pixel_height)
{
    // This functions depends on the x and y scales in MainPlot.
    // Since the order in which the plots are redrawn is undetermined,
    // we are updating here the MainPlot scale.
    // But don't change MainPlot's scale when printing.
    if (pixel_height == GetClientSize().GetHeight()) // probably not printing
        master_->set_scale(pixel_width, master_->GetClientSize().GetHeight());

    xs = master_->get_x_scale();

    if (kind_ == apk_cum_chi2) {
        ys.scale = -1. * y_zoom_base_ * y_zoom_;
        ys.origin = - pixel_height / ys.scale;
        return;
    }
    switch (kind_) {
        case apk_empty:
            ys.scale = 1.; //y scale doesn't matter
            break;
        case apk_diff:
            if (master_->get_y_scale().logarithm)
                ys.scale = y_zoom_;
            else
                ys.scale = master_->get_y_scale().scale * y_zoom_;
            break;
        case apk_diff_stddev:
        case apk_diff_y_perc:
            ys.scale = -1. * y_zoom_base_ * y_zoom_;
            break;
        default:
            assert(0);
    }
    ys.origin = - pixel_height / 2. / ys.scale;
}

void AuxPlot::read_settings(wxConfigBase *cf)
{
    wxString path = wxT("/AuxPlot_") + name_;
    cf->SetPath(path);
    kind_ = static_cast<AuxPlotKind>(cf->Read (wxT("kind"), apk_diff));
    mark_peak_pos_ = cfg_read_bool (cf, wxT("markCtr"), false);
    reversed_diff_ = cfg_read_bool (cf, wxT("reversedDiff"), false);
    auto_zoom_y_ = cfg_read_bool(cf, wxT("autozoom"), false);
    line_between_points = cfg_read_bool(cf, wxT("line_between_points"), true);
    point_radius = cf->Read (wxT("point_radius"), 1);
    y_max_tics = cf->Read(wxT("yMaxTics"), 5);
    y_tic_size = cf->Read(wxT("yTicSize"), 4);
    cf->SetPath(wxT("Visible"));
    // nothing here now
    cf->SetPath(wxT("../Colors"));
    set_bg_color(cfg_read_color (cf, wxT("bg"), wxColour(34, 34, 34)));
    activeDataCol = cfg_read_color (cf, wxT("active_data"),
                                                       wxColour(wxT("GREEN")));
    inactiveDataCol = cfg_read_color(cf,wxT("inactive_data"),
                                                      wxColour(128, 128, 128));
    cf->SetPath(wxT(".."));
    FPlot::read_settings(cf);
    refresh();
}

void AuxPlot::save_settings(wxConfigBase *cf) const
{
    cf->SetPath(wxT("/AuxPlot_") + name_);
    cf->Write(wxT("kind"), (int) kind_);
    cf->Write(wxT("markCtr"), mark_peak_pos_);
    cf->Write(wxT("reversedDiff"), reversed_diff_);
    cf->Write(wxT("line_between_points"), line_between_points);
    cf->Write(wxT("autozoom"), auto_zoom_y_);
    cf->Write(wxT("point_radius"), point_radius);
    cf->Write(wxT("yMaxTics"), y_max_tics);
    cf->Write(wxT("yTicSize"), y_tic_size);

    cf->SetPath(wxT("Visible"));
    // nothing here now

    cf->SetPath(wxT("../Colors"));
    cfg_write_color(cf, wxT("bg"), get_bg_color());
    cfg_write_color(cf, wxT("active_data"), activeDataCol);
    cfg_write_color(cf, wxT("inactive_data"),inactiveDataCol);

    // We don't call FPlot::save_settings() to not clutter the config,
    // the only configurable settings from FPlot are xAxisCol and ticsFont.
    //cf->SetPath(wxT(".."));
    //FPlot::save_settings(cf);

    cfg_write_color (cf, wxT("xAxis"), xAxisCol);
    cf->SetPath(wxT(".."));
    cfg_write_font (cf, wxT("ticsFont"), ticsFont);
}

void AuxPlot::OnLeftDown (wxMouseEvent &event)
{
    if (downX != INT_MIN)
        cancel_mouse_left_press();
    if (event.ShiftDown()) { // the same as OnMiddleDown()
        frame->GViewAll();
        return;
    }
    int X = event.GetX();
    // if mouse pointer is near to left or right border, move view
    if (X < move_plot_margin_width)
        frame->scroll_view_horizontally(-0.33);  // <--
    else if (X > GetClientSize().GetWidth() - move_plot_margin_width)
        frame->scroll_view_horizontally(+0.33); // -->
    else {
        downX = X;
        overlay.start_mode(Overlay::kVRange, downX, 0);
        overlay.draw_overlay();
        SetCursor(wxCURSOR_SIZEWE);
        frame->set_status_text("Select x range and release button to zoom...");
        CaptureMouse();
        connect_esc_to_cancel(true);
    }
}

void AuxPlot::cancel_mouse_left_press()
{
    if (downX == INT_MIN)
        return;
    if (GetCapture() == this)
        ReleaseMouse();
    connect_esc_to_cancel(false);
    downX = INT_MIN;
    SetCursor(wxCURSOR_CROSS);

    frame->set_status_text("");
    overlay.switch_mode(Overlay::kVLine);
    overlay.draw_overlay();
    frame->plot_pane()->draw_vertical_lines(-1, -1, this);
}

void AuxPlot::OnLeftUp (wxMouseEvent &event)
{
    if (downX == INT_MIN)
        return;
    double x1 = xs.val(event.GetX());
    double x2 = xs.val(downX); // must be called before cancel_mouse...()
    if (GetCapture() == this)
        ReleaseMouse();
    connect_esc_to_cancel(false);
    downX = INT_MIN;
    SetCursor(wxCURSOR_CROSS);
    overlay.switch_mode(Overlay::kVLine);
    if (abs(event.GetX() - downX) >= 5) {
        RealRange all;
        frame->change_zoom(RealRange(min(x1,x2), max(x1,x2)), all);
    }
    else {
        frame->set_status_text("");
        overlay.draw_overlay();
        frame->plot_pane()->draw_vertical_lines(event.GetX(), -1, this);
    }
}

//popup-menu
void AuxPlot::OnRightDown (wxMouseEvent &event)
{
    if (downX != INT_MIN) {
        cancel_mouse_left_press();
        return;
    }

    wxMenu popup_menu;
    popup_menu.Append(ID_aux_prefs, wxT("&Configure..."), wxT(""));
    popup_menu.AppendSeparator();
    popup_menu.Append (ID_aux_yz_fit, wxT("&Fit to window"));
    popup_menu.Enable(ID_aux_yz_fit, is_zoomable());
    popup_menu.AppendCheckItem (ID_aux_yz_auto, wxT("&Auto-fit"));
    popup_menu.Check (ID_aux_yz_auto, auto_zoom_y_);
    popup_menu.Enable(ID_aux_yz_auto, is_zoomable());
    popup_menu.AppendSeparator();
    popup_menu.AppendRadioItem(ID_aux_plot0+0, wxT("&empty"), wxT("nothing"));
    popup_menu.AppendRadioItem(ID_aux_plot0+1, wxT("&diff"), wxT("y_d - y_s"));
    popup_menu.AppendRadioItem(ID_aux_plot0+2, wxT("&weighted diff"),
                               wxT("(y_d - y_s) / sigma"));
    popup_menu.AppendRadioItem(ID_aux_plot0+3, wxT("&diff/y [%]"),
                               wxT("(y_d - y_s) / y_d [%]"));
    popup_menu.AppendRadioItem(ID_aux_plot0+4, wxT("c&umulative \u03c7\u00b2"),
                           wxT("cumulative weighted sum of squared residuals"));
    popup_menu.Check(ID_aux_plot0+kind_, true);
    popup_menu.AppendSeparator();
    popup_menu.AppendCheckItem(ID_aux_mark_pos, wxT("Show &Peak Positions"),
                               wxT("mark centers of peaks"));
    popup_menu.Check(ID_aux_mark_pos, mark_peak_pos_);
    PopupMenu (&popup_menu, event.GetX(), event.GetY());
}

void AuxPlot::OnMiddleDown (wxMouseEvent&)
{
    if (downX == INT_MIN)
        frame->GViewAll();
    else
        cancel_mouse_left_press();
}

void AuxPlot::show_pref_dialog()
{
    AuxPlotConfDlg dlg(this);
    dlg.ShowModal();
}

void AuxPlot::OnPopupPlot (wxCommandEvent& event)
{
    change_plot_kind(static_cast<AuxPlotKind>(event.GetId()-ID_aux_plot0));
    Refresh(false); // needed on Windows (i don't know why)
}

void AuxPlot::change_plot_kind(AuxPlotKind kind)
{
    kind_ = kind;
    //fit_y_zoom();
    fit_y_once_ = true;
    refresh();
}

void AuxPlot::OnMarkPeakPositions(wxCommandEvent& event)
{
    mark_peak_pos_ = event.IsChecked();
    refresh();
    Refresh(false); // needed on Windows (i don't know why)
}

void AuxPlot::OnPopupYZoomFit (wxCommandEvent&)
{
    //fit_y_zoom();
    fit_y_once_ = true;
    refresh();
    Refresh(false); // needed on Windows (i don't know why)
}

void AuxPlot::fit_y_zoom(Data const* data, Model const* model)
{
    if (!is_zoomable())
        return;
    double y = 0.;
    vector<Point>::const_iterator first = data->get_point_at(ftk->view.left()),
                                  last = data->get_point_at(ftk->view.right());
    if (data->is_empty() || last==first)
        return;
    int pixel_height = GetClientSize().GetHeight();
    switch (kind_) {
        case apk_diff:
            {
            y = get_max_abs_y(diff_of_data_for_draw_data, first, last, model);
            Scale const& mys = master_->get_y_scale();
            y_zoom_ = fabs (pixel_height / (2 * y
                                           * (mys.logarithm ? 1 : mys.scale)));
            double order = pow (10, floor (log10(y_zoom_)));
            y_zoom_ = floor(y_zoom_ / order) * order;
            }
            break;
        case apk_diff_stddev:
            y = get_max_abs_y(diff_stddev_of_data_for_draw_data,
                              first, last, model);
            y_zoom_base_ = pixel_height / (2. * y);
            y_zoom_ = 0.9;
            break;
        case apk_diff_y_perc:
            y = get_max_abs_y(diff_y_perc_of_data_for_draw_data,
                              first, last, model);
            y_zoom_base_ = pixel_height / (2. * y);
            y_zoom_ = 0.9;
            break;
        case apk_cum_chi2:
            y = 0.;
            for (vector<Point>::const_iterator i = first; i < last; i++)
                y += diff_chi2_of_data_for_draw_data(i, model);
            y_zoom_base_ = pixel_height / y;
            y_zoom_ = 0.9;
            break;
        default:
            assert(0);
    }
}

void AuxPlot::OnAutoZoom(wxCommandEvent& event)
{
    auto_zoom_y_ = event.IsChecked();
    if (auto_zoom_y_) {
        //fit_y_zoom() is called from draw
        refresh();
        Refresh(false); // needed on Windows (i don't know why)
    }
}

// ========================================================================

AuxPlotConfDlg::AuxPlotConfDlg(AuxPlot* ap)
  : wxDialog(NULL, -1, wxString(wxT("Configure Auxiliary Plot")),
             wxDefaultPosition, wxDefaultSize,
             wxDEFAULT_DIALOG_STYLE),
    ap_(ap)
{
    wxBoxSizer *top_sizer = new wxBoxSizer(wxVERTICAL);

    wxBoxSizer *hor_sizer = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer *left_sizer = new wxBoxSizer(wxVERTICAL);
    wxArrayString plot_choices;
    plot_choices.Add(wxT("&Empty"));
    plot_choices.Add(wxT("&Diff"));
    plot_choices.Add(wxT("&Weighted diff"));
    plot_choices.Add(wxT("Diff as % of &y"));
    plot_choices.Add(wxT("&Cumulative \u03c7\u00b2")); // chi2
    wxRadioBox* plot_rb = new wxRadioBox(this, -1, wxT("plot"),
                                         wxDefaultPosition, wxDefaultSize,
                                         plot_choices, 1, wxRA_SPECIFY_COLS);
    plot_rb->SetSelection(ap_->kind_);
    left_sizer->Add(plot_rb, 0, wxALL, 5);
    rev_cb_ = new wxCheckBox(this, -1, wxT("&Reversed"));
    rev_cb_->SetValue(ap_->reversed_diff_);
    rev_cb_->Enable(ap_->kind_ != apk_empty && ap_->kind_ != apk_cum_chi2);
    left_sizer->Add(rev_cb_, 0, wxALL, 5);
    wxCheckBox* pos_cb = new wxCheckBox(this, -1, wxT("&Show peak positions"));
    pos_cb->SetValue(ap_->mark_peak_pos_);
    left_sizer->Add(pos_cb, 0, wxALL, 5);
    wxCheckBox* auto_cb = new wxCheckBox(this, -1, wxT("&Auto-zoom y scale"));
    auto_cb->SetValue(ap_->auto_zoom_y_);
    left_sizer->Add(auto_cb, 0, wxALL, 5);
    wxBoxSizer *zoom_sizer = new wxBoxSizer(wxHORIZONTAL);
    zoom_sizer->AddSpacer(10);
    zoom_sizer->Add(new wxStaticText(this, -1, wxT("y zoom")),
                    0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    zoom_sc_ = new SpinCtrl(this, -1, iround(ap_->y_zoom_ * 100),
                            0, 999999, 70);
    zoom_sc_->Enable(!ap_->auto_zoom_y_);
    zoom_sizer->Add(zoom_sc_, 0, wxALL, 5);
    zoom_sizer->Add(new wxStaticText(this, -1, wxT("%")),
                    0, wxRIGHT|wxALIGN_CENTER_VERTICAL, 5);
    left_sizer->Add(zoom_sizer, 0);

    wxGridSizer *gsizer = new wxGridSizer(2, 5, 5);
    wxSizerFlags cl = wxSizerFlags().Align(wxALIGN_CENTRE_VERTICAL),
             cr = wxSizerFlags().Align(wxALIGN_CENTRE_VERTICAL|wxALIGN_RIGHT);
    gsizer->Add(new wxStaticText(this, -1, wxT("background color")), cr);
    bg_cp_ = new wxColourPickerCtrl(this, -1, ap_->get_bg_color());
    gsizer->Add(bg_cp_, cl);
    gsizer->Add(new wxStaticText(this, -1, wxT("active data color")), cr);
    active_cp_ = new wxColourPickerCtrl(this, -1, ap_->activeDataCol);
    gsizer->Add(active_cp_, cl);
    gsizer->Add(new wxStaticText(this, -1, wxT("inactive data color")), cr);
    inactive_cp_ = new wxColourPickerCtrl(this, -1, ap_->inactiveDataCol);
    gsizer->Add(inactive_cp_, cl);
    gsizer->Add(new wxStaticText(this, -1, wxT("axis & tics color")), cr);
    axis_cp_ = new wxColourPickerCtrl(this, -1, ap_->xAxisCol);
    gsizer->Add(axis_cp_, cl);
    gsizer->Add(new wxStaticText(this, -1, wxT("tic label font")), cr);
    tics_fp_ = new wxFontPickerCtrl(this, -1, ap_->ticsFont);
    gsizer->Add(tics_fp_, cl);

    hor_sizer->Add(left_sizer, wxSizerFlags().Border());
    hor_sizer->Add(gsizer, wxSizerFlags().Border());

    top_sizer->Add(hor_sizer, 0);
    top_sizer->Add(persistance_note(this), wxSizerFlags().Center().Border());
    top_sizer->Add(new wxButton(this, wxID_CLOSE),
                   wxSizerFlags().Right().Border());
    SetSizerAndFit(top_sizer);
    SetEscapeId(wxID_CLOSE);

    Connect(plot_rb->GetId(), wxEVT_COMMAND_RADIOBOX_SELECTED,
            wxCommandEventHandler(AuxPlotConfDlg::OnPlotKind));
    Connect(rev_cb_->GetId(), wxEVT_COMMAND_CHECKBOX_CLICKED,
            wxCommandEventHandler(AuxPlotConfDlg::OnReversedDiff));
    Connect(pos_cb->GetId(), wxEVT_COMMAND_CHECKBOX_CLICKED,
            wxCommandEventHandler(AuxPlotConfDlg::OnMarkPeakPositions));
    Connect(auto_cb->GetId(), wxEVT_COMMAND_CHECKBOX_CLICKED,
            wxCommandEventHandler(AuxPlotConfDlg::OnAutoZoom));
    Connect(zoom_sc_->GetId(), wxEVT_COMMAND_SPINCTRL_UPDATED,
            wxSpinEventHandler(AuxPlotConfDlg::OnZoomSpin));
    Connect(bg_cp_->GetId(), wxEVT_COMMAND_COLOURPICKER_CHANGED,
            wxColourPickerEventHandler(AuxPlotConfDlg::OnColor));
    Connect(active_cp_->GetId(), wxEVT_COMMAND_COLOURPICKER_CHANGED,
            wxColourPickerEventHandler(AuxPlotConfDlg::OnColor));
    Connect(inactive_cp_->GetId(), wxEVT_COMMAND_COLOURPICKER_CHANGED,
            wxColourPickerEventHandler(AuxPlotConfDlg::OnColor));
    Connect(axis_cp_->GetId(), wxEVT_COMMAND_COLOURPICKER_CHANGED,
            wxColourPickerEventHandler(AuxPlotConfDlg::OnColor));
    Connect(tics_fp_->GetId(), wxEVT_COMMAND_FONTPICKER_CHANGED,
            wxFontPickerEventHandler(AuxPlotConfDlg::OnTicsFont));
}

void AuxPlotConfDlg::OnPlotKind(wxCommandEvent& event)
{
    AuxPlotKind k = static_cast<AuxPlotKind>(event.GetSelection());
    ap_->change_plot_kind(k);
    rev_cb_->Enable(k != apk_empty && k != apk_cum_chi2);
}

void AuxPlotConfDlg::OnReversedDiff(wxCommandEvent& event)
{
    ap_->reversed_diff_ = event.IsChecked();
    ap_->refresh();
}

void AuxPlotConfDlg::OnAutoZoom(wxCommandEvent& event)
{
    ap_->OnAutoZoom(event);
    zoom_sc_->Enable(!ap_->auto_zoom_y_);
}

void AuxPlotConfDlg::OnZoomSpin(wxSpinEvent& event)
{
    ap_->y_zoom_ = event.GetPosition() / 100.;
    ap_->refresh();
}

void AuxPlotConfDlg::OnTicsFont(wxFontPickerEvent& event)
{
    ap_->ticsFont = event.GetFont();
    ap_->refresh();
}

void AuxPlotConfDlg::OnColor(wxColourPickerEvent& event)
{
    int id = event.GetId();
    if (id == bg_cp_->GetId())
        ap_->set_bg_color(event.GetColour());
    else if (id == active_cp_->GetId())
        ap_->activeDataCol = event.GetColour();
    else if (id == inactive_cp_->GetId())
        ap_->inactiveDataCol = event.GetColour();
    else if (id == axis_cp_->GetId())
        ap_->xAxisCol = event.GetColour();
    ap_->refresh();
}


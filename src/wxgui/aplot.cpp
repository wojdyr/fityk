// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

/// In this file:
///  Auxiliary Plot, for displaying residuals, peak positions, etc. (AuxPlot)

#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/numdlg.h>

#include "aplot.h"
#include "frame.h"
#include "../model.h"
#include "../func.h"
#include "../data.h"
#include "../logic.h"

using namespace std;

enum {
    ID_aux_plot0            = 25310,
    ID_aux_plot_ctr         = 25340,
    ID_aux_revd                    ,
    ID_aux_c_background            ,
    ID_aux_c_active_data           ,
    ID_aux_c_inactive_data         ,
    ID_aux_c_axis                  ,
    ID_aux_color                   ,
    ID_aux_m_tfont                 ,
    ID_aux_yz_fit                  ,
    ID_aux_yz_change               ,
    ID_aux_yz_auto
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
    EVT_MENU_RANGE (ID_aux_plot0, ID_aux_plot0+10, AuxPlot::OnPopupPlot)
    EVT_MENU (ID_aux_plot_ctr, AuxPlot::OnPopupPlotCtr)
    EVT_MENU (ID_aux_revd, AuxPlot::OnPopupReversedDiff)
    EVT_MENU_RANGE (ID_aux_c_background, ID_aux_color-1, AuxPlot::OnPopupColor)
    EVT_MENU (ID_aux_m_tfont, AuxPlot::OnTicsFont)
    EVT_MENU (ID_aux_yz_change, AuxPlot::OnPopupYZoom)
    EVT_MENU (ID_aux_yz_fit, AuxPlot::OnPopupYZoomFit)
    EVT_MENU (ID_aux_yz_auto, AuxPlot::OnPopupYZoomAuto)
END_EVENT_TABLE()

void AuxPlot::OnPaint(wxPaintEvent&)
{
    frame->update_crosshair(-1, -1);
    update_buffer_and_blit();
    //draw, if necessary, vertical or horizontal lines
    line_following_cursor(mat_redraw);
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
    if (auto_zoom_y || fit_y_once) {
        fit_y_zoom(data, model);
        fit_y_once = false;
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

    if (mark_peak_ctrs) {
        v_foreach (int, i, model->get_ff().idx) {
            fp x;
            if (ftk->get_function(*i)->get_center(&x)) {
                int X = xs.px(x - model->zero_shift(x));
                dc.DrawLine(X, 0, X, pixel_height);
            }
        }
    }

    if (kind == apk_empty || data->is_empty())
        return;

    if (x_axis_visible) {
        int Y0 = ys.px(0.);
        dc.DrawLine (0, Y0, pixel_width, Y0);
        if (kind == apk_diff)
            draw_zoom_text(dc, !monochrome);
    }
    if (y_axis_visible) {
        int X0 = xs.px(0.);
        dc.DrawLine (X0, 0, X0, pixel_height);
    }
    if (ytics_visible) {
        Rect rect(0, 0, ys.val(pixel_height), ys.val(0));
        draw_ytics(dc, rect, !monochrome);
    }

    fp (*f)(vector<Point>::const_iterator, Model const*) = 0;
    bool cummulative = false;
    if (kind == apk_diff)
        f = reversed_diff ? rdiff_of_data_for_draw_data
                          : diff_of_data_for_draw_data;
    else if (kind == apk_diff_stddev)
        f = reversed_diff ? rdiff_stddev_of_data_for_draw_data
                          : diff_stddev_of_data_for_draw_data;
    else if (kind == apk_diff_y_perc)
        f = reversed_diff ? rdiff_y_perc_of_data_for_draw_data
                          : diff_y_perc_of_data_for_draw_data;
    else if (kind == apk_cum_chi2) {
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
    if (master->get_y_scale().logarithm)
        return;
    if (set_pen)
        dc.SetTextForeground(xAxisCol);
    set_font(dc, *wxNORMAL_FONT);
    string s = "x" + S(y_zoom);
    wxCoord w, h;
    dc.GetTextExtent (s2wx(s), &w, &h);
    dc.DrawText (s2wx(s), get_pixel_width(dc) - w - 2, 2);
}

void AuxPlot::OnMouseMove(wxMouseEvent &event)
{
    int X = event.GetX();
    line_following_cursor(mat_move, X);
    frame->set_status_coords(xs.valr(X), ys.valr(event.GetY()), pte_aux);
    if (X < move_plot_margin_width)
        SetCursor(wxCURSOR_POINT_LEFT);
    else if (X > GetClientSize().GetWidth() - move_plot_margin_width)
        SetCursor(wxCURSOR_POINT_RIGHT);
    else {
        frame->update_crosshair(X, -1);
        SetCursor(wxCURSOR_CROSS);
    }
}

void AuxPlot::OnLeaveWindow (wxMouseEvent&)
{
    frame->clear_status_coords();
    frame->update_crosshair(-1, -1);
}

bool AuxPlot::is_zoomable()
{
    return kind == apk_diff || kind == apk_diff_stddev
           || kind == apk_diff_y_perc || kind == apk_cum_chi2;
}

void AuxPlot::set_scale(int pixel_width, int pixel_height)
{
    // This functions depends on the x and y scales in MainPlot.
    // Since the order in which the plots are redrawn is undetermined,
    // we are updating here the MainPlot scale.
    master->set_scale(pixel_width, master->GetClientSize().GetHeight());

    xs = master->get_x_scale();

    if (kind == apk_cum_chi2) {
        ys.scale = -1. * y_zoom_base * y_zoom;
        ys.origin = - pixel_height / ys.scale;
        return;
    }
    switch (kind) {
        case apk_empty:
            ys.scale = 1.; //y scale doesn't matter
            break;
        case apk_diff:
            if (master->get_y_scale().logarithm)
                ys.scale = y_zoom;
            else
                ys.scale = master->get_y_scale().scale * y_zoom;
            break;
        case apk_diff_stddev:
        case apk_diff_y_perc:
            ys.scale = -1. * y_zoom_base * y_zoom;
            break;
        default:
            assert(0);
    }
    ys.origin = - pixel_height / 2. / ys.scale;
}

void AuxPlot::read_settings(wxConfigBase *cf)
{
    wxString path = wxT("/AuxPlot_") + name;
    cf->SetPath(path);
    kind = static_cast<AuxPlotKind>(cf->Read (wxT("kind"), apk_diff));
    mark_peak_ctrs = cfg_read_bool (cf, wxT("markCtr"), false);
    reversed_diff = cfg_read_bool (cf, wxT("reversedDiff"), false);
    auto_zoom_y = false;
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
    cf->SetPath(wxT("/AuxPlot_") + name);
    cf->Write (wxT("kind"), (int) kind);
    cf->Write (wxT("markCtr"), mark_peak_ctrs);
    cf->Write (wxT("reversedDiff"), reversed_diff);
    cf->Write (wxT("line_between_points"), line_between_points);
    cf->Write (wxT("point_radius"), point_radius);
    cf->Write(wxT("yMaxTics"), y_max_tics);
    cf->Write(wxT("yTicSize"), y_tic_size);

    cf->SetPath(wxT("Visible"));
    // nothing here now

    cf->SetPath(wxT("../Colors"));
    cfg_write_color(cf, wxT("bg"), get_bg_color());
    cfg_write_color(cf, wxT("active_data"), activeDataCol);
    cfg_write_color(cf, wxT("inactive_data"),inactiveDataCol);
    cf->SetPath(wxT(".."));
    FPlot::save_settings(cf);
}

void AuxPlot::OnLeftDown (wxMouseEvent &event)
{
    cancel_mouse_left_press();
    if (event.ShiftDown()) { // the same as OnMiddleDown()
        frame->GViewAll();
        return;
    }
    int X = event.GetPosition().x;
    // if mouse pointer is near to left or right border, move view
    if (X < move_plot_margin_width)
        frame->scroll_view_horizontally(-0.33);  // <--
    else if (X > GetClientSize().GetWidth() - move_plot_margin_width)
        frame->scroll_view_horizontally(+0.33); // -->
    else {
        mouse_press_X = X;
        start_line_following_cursor(mouse_press_X, kVerticalLine);
        SetCursor(wxCURSOR_SIZEWE);
        frame->set_status_text("Select x range and release button to zoom...");
        CaptureMouse();
    }
}

bool AuxPlot::cancel_mouse_left_press()
{
    if (mouse_press_X != INT_MIN) {
        line_following_cursor(mat_stop);
        ReleaseMouse();
        mouse_press_X = INT_MIN;
        SetCursor(wxCURSOR_CROSS);
        frame->set_status_text("");
        return true;
    }
    else
        return false;
}

void AuxPlot::OnLeftUp (wxMouseEvent &event)
{
    if (mouse_press_X == INT_MIN)
        return;
    if (abs(event.GetX() - mouse_press_X) < 5) { //cancel
        cancel_mouse_left_press();
        return;
    }
    fp x1 = xs.val(event.GetX());
    fp x2 = xs.val(mouse_press_X);
    cancel_mouse_left_press();
    RealRange all;
    frame->change_zoom(RealRange(min(x1,x2), max(x1,x2)), all);
}

//popup-menu
void AuxPlot::OnRightDown (wxMouseEvent &event)
{
    if (cancel_mouse_left_press())
        return;

    wxMenu popup_menu (wxT("aux. plot menu"));
    //wxMenu *kind_menu = new wxMenu;
    popup_menu.AppendRadioItem(ID_aux_plot0+0, wxT("&empty"), wxT("nothing"));
    popup_menu.AppendRadioItem(ID_aux_plot0+1, wxT("&diff"), wxT("y_d - y_s"));
    popup_menu.AppendRadioItem(ID_aux_plot0+2, wxT("&weighted diff"),
                               wxT("(y_d - y_s) / sigma"));
    popup_menu.AppendRadioItem(ID_aux_plot0+3, wxT("&proc diff"),
                               wxT("(y_d - y_s) / y_d [%]"));
    popup_menu.AppendRadioItem(ID_aux_plot0+4, wxT("cumul. &chi2"),
                               wxT("cumulative chi square"));
    popup_menu.Check(ID_aux_plot0+kind, true);
    popup_menu.AppendSeparator();
    popup_menu.AppendCheckItem(ID_aux_revd, wxT("reversed diff"),
                               wxT(""));
    popup_menu.Check(ID_aux_revd, reversed_diff);
    popup_menu.AppendSeparator();
    popup_menu.AppendCheckItem(ID_aux_plot_ctr, wxT("show peak po&sitions"),
                               wxT("mark centers of peaks"));
    popup_menu.Check(ID_aux_plot_ctr, mark_peak_ctrs);
    popup_menu.AppendSeparator();
    popup_menu.Append (ID_aux_yz_fit, wxT("&Fit to window"));
    popup_menu.Enable(ID_aux_yz_fit, is_zoomable());
    popup_menu.Append (ID_aux_yz_change, wxT("Change &y scale"));
    popup_menu.Enable(ID_aux_yz_change, is_zoomable());
    popup_menu.AppendCheckItem (ID_aux_yz_auto, wxT("&Auto-fit"));
    popup_menu.Check (ID_aux_yz_auto, auto_zoom_y);
    popup_menu.Enable(ID_aux_yz_auto, is_zoomable());
    popup_menu.AppendSeparator();
    wxMenu *color_menu = new wxMenu;
    color_menu->Append (ID_aux_c_background, wxT("&Background"));
    color_menu->Append (ID_aux_c_active_data, wxT("&Active Data"));
    color_menu->Append (ID_aux_c_inactive_data, wxT("&Inactive Data"));
    color_menu->Append (ID_aux_c_axis, wxT("&X Axis"));
    popup_menu.Append (ID_aux_color, wxT("&Color"), color_menu);
    wxMenu *misc_menu = new wxMenu;
    misc_menu->Append (ID_aux_m_tfont, wxT("&Tics font"));
    popup_menu.Append (wxNewId(), wxT("&Miscellaneous"), misc_menu);
    PopupMenu (&popup_menu, event.GetX(), event.GetY());
}

void AuxPlot::OnMiddleDown (wxMouseEvent&)
{
    if (cancel_mouse_left_press())
        return;
    frame->GViewAll();
}

void AuxPlot::OnPopupPlot (wxCommandEvent& event)
{
    kind = static_cast<AuxPlotKind>(event.GetId()-ID_aux_plot0);
    //fit_y_zoom();
    fit_y_once = true;
    refresh();
    Refresh(false); // needed on Windows (i don't know why)
}

void AuxPlot::OnPopupPlotCtr (wxCommandEvent& event)
{
    mark_peak_ctrs = event.IsChecked();
    refresh();
    Refresh(false); // needed on Windows (i don't know why)
}

void AuxPlot::OnPopupReversedDiff (wxCommandEvent& event)
{
    reversed_diff = event.IsChecked();
    refresh();
    Refresh(false); // needed on Windows (i don't know why)
}

void AuxPlot::OnPopupColor (wxCommandEvent& event)
{
    wxColour *color = 0;
    int n = event.GetId();
    wxColour bg_color = get_bg_color();
    if (n == ID_aux_c_background)
        color = &bg_color;
    else if (n == ID_aux_c_active_data) {
        color = &activeDataCol;
    }
    else if (n == ID_aux_c_inactive_data) {
        color = &inactiveDataCol;
    }
    else if (n == ID_aux_c_axis)
        color = &xAxisCol;
    else
        return;
    if (change_color_dlg(*color)) {
        if (n == ID_aux_c_background)
            set_bg_color(bg_color);
        refresh();
    }
}

void AuxPlot::OnPopupYZoom (wxCommandEvent&)
{
    int r = wxGetNumberFromUser(wxT("Set zoom in y direction [%]"),
                                wxT(""), wxT(""),
                                static_cast<int>(y_zoom * 100 + 0.5),
                                1, 10000000);
    if (r > 0)
        y_zoom = r / 100.;
    refresh();
}

void AuxPlot::OnPopupYZoomFit (wxCommandEvent&)
{
    //fit_y_zoom();
    fit_y_once = true;
    refresh();
    Refresh(false); // needed on Windows (i don't know why)
}

void AuxPlot::fit_y_zoom(Data const* data, Model const* model)
{
    if (!is_zoomable())
        return;
    fp y = 0.;
    vector<Point>::const_iterator first = data->get_point_at(ftk->view.left()),
                                  last = data->get_point_at(ftk->view.right());
    if (data->is_empty() || last==first)
        return;
    int pixel_height = GetClientSize().GetHeight();
    switch (kind) { // setting y_zoom
        case apk_diff:
            {
            y = get_max_abs_y(diff_of_data_for_draw_data, first, last, model);
            Scale const& mys = master->get_y_scale();
            y_zoom = fabs (pixel_height / (2 * y
                                           * (mys.logarithm ? 1 : mys.scale)));
            fp order = pow (10, floor (log10(y_zoom)));
            y_zoom = floor(y_zoom / order) * order;
            }
            break;
        case apk_diff_stddev:
            y = get_max_abs_y(diff_stddev_of_data_for_draw_data,
                              first, last, model);
            y_zoom_base = pixel_height / (2. * y);
            y_zoom = 0.9;
            break;
        case apk_diff_y_perc:
            y = get_max_abs_y(diff_y_perc_of_data_for_draw_data,
                              first, last, model);
            y_zoom_base = pixel_height / (2. * y);
            y_zoom = 0.9;
            break;
        case apk_cum_chi2:
            y = 0.;
            for (vector<Point>::const_iterator i = first; i < last; i++)
                y += diff_chi2_of_data_for_draw_data(i, model);
            y_zoom_base = pixel_height / y;
            y_zoom = 0.9;
            break;
        default:
            assert(0);
    }
}

void AuxPlot::OnPopupYZoomAuto (wxCommandEvent&)
{
    auto_zoom_y = !auto_zoom_y;
    if (auto_zoom_y) {
        //fit_y_zoom() is called from draw
        refresh();
        Refresh(false); // needed on Windows (i don't know why)
    }
}


// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
// $Id$

#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/fontdlg.h>
#include <algorithm>

#include "mplot.h"
#include "frame.h"
#include "sidebar.h"
#include "statbar.h" // HintReceiver
#include "../data.h"
#include "../logic.h"
#include "../model.h"
#include "../var.h"
#include "../func.h"
#include "../ui.h"
#include "../settings.h"

using namespace std;


enum {
    ID_plot_popup_za                = 25001,
    ID_plot_popup_model               = 25011,
    ID_plot_popup_groups                   ,
    ID_plot_popup_peak                     ,

    ID_plot_popup_c_background             ,
    ID_plot_popup_c_inactive_data          ,
    ID_plot_popup_c_model                    ,
    ID_plot_popup_c_inv                    ,

    ID_plot_popup_axes                     ,
    ID_plot_popup_plabels                  ,

    ID_plot_popup_pt_size           = 25210,// and next 10 ,
    ID_peak_popup_info              = 25250,
    ID_peak_popup_del                      ,
    ID_peak_popup_guess                    ,

    ID_CAD_COLOR,
    ID_CAD_FONT,

    ID_CPL_FONT,
    ID_CPL_SHOW,
    ID_CPL_RADIO,
    ID_CPL_TEXT,
};


//---------------------- FunctionMouseDrag --------------------------------

void FunctionMouseDrag::Drag::change_value(fp x, fp dx, int dX)
{
    if (how == no_drag || dx == 0. || dX == 0)
        return;
    if (how == relative_value) {
        if (ini_x == 0.) {
            ini_x = x - dx;
            if (is_zero(ini_x))
                ini_x += dx;
        }
        //value += dx * fabs(value / x) * multiplier;
        value = x / ini_x * ini_value;
    }
    else if (how == absolute_value)
        value += dx * multiplier;
    else if (how == absolute_pixels)
        value += dX * multiplier;
    else
        assert(0);
}

string FunctionMouseDrag::Drag::get_cmd() const
{
    if (how != no_drag && value != ini_value)
        return "$" + variable_name + " = ~" + eS(value) + "; ";
    else
        return "";
}

void FunctionMouseDrag::Drag::set(Function const* p, int idx,
                                  drag_type how_, fp multiplier_)
{
    Variable const* var = ftk->get_variable(p->get_var_idx(idx));
    if (!var->is_simple()) {
        how = no_drag;
        return;
    }
    how = how_;
    parameter_idx = idx;
    parameter_name = p->type_var_names[idx];
    variable_name = p->get_var_name(idx);
    value = ini_value = p->get_var_value(idx);
    multiplier = multiplier_;
    ini_x = 0.;
}


void FunctionMouseDrag::start(Function const* p, int X, int Y, fp x, fp y)
{
    drag_x.parameter_name = drag_y.parameter_name
        = drag_shift_x.parameter_name = drag_shift_y.parameter_name = "-";
    set_defined_drags();
    if (drag_x.how == no_drag)
        bind_parameter_to_drag(drag_x, "center", p, absolute_value);
    if (drag_y.how == no_drag)
        bind_parameter_to_drag(drag_y, "height", p, absolute_value)
        || bind_parameter_to_drag(drag_y, "area", p, relative_value)
        || bind_parameter_to_drag(drag_y, "avgy", p, absolute_value)
        || bind_parameter_to_drag(drag_y, "intercept", p, relative_value);
    if (drag_shift_x.how == no_drag)
        bind_parameter_to_drag(drag_shift_x, "hwhm", p, absolute_value, 0.5)
        || bind_parameter_to_drag(drag_shift_x, "fwhm", p, absolute_value, 0.5);

    values = p->get_var_values();
    status = "Move to change: " + drag_x.parameter_name + "/"
        + drag_y.parameter_name + ", with [Shift]: "
        + drag_shift_x.parameter_name + "/" + drag_shift_y.parameter_name;

    pX = X;
    pY = Y;
    px = x;
    py = y;
}

void FunctionMouseDrag::set_defined_drags()
{
    drag_x.how = no_drag;
    drag_y.how = no_drag;
    drag_shift_x.how = no_drag;
    drag_shift_y.how = no_drag;
}

bool FunctionMouseDrag::bind_parameter_to_drag(Drag &drag, string const& name,
                                              Function const* p, drag_type how,
                                              fp multiplier)
{
    // search for Function(..., height, ...)
    int idx = index_of_element(p->type_var_names, name);
    if (idx != -1) {
        drag.set(p, idx, how, multiplier);
        return true;
    }

    vector<string> defvalues
        = Function::get_defvalues_from_formula(p->type_formula);

    // search for Function(..., foo=height, ...)
    idx = index_of_element(defvalues, name);
    if (idx != -1) {
        drag.set(p, idx, how, multiplier);
        return true;
    }

    // search for Function(..., foo=height*..., ...)
    for (vector<string>::const_iterator i = defvalues.begin();
                                                i != defvalues.end(); ++i)
        if (startswith(*i, name+"*")) {
            drag.set(p, i - defvalues.begin(), how, multiplier);
            return true;
        }
    return false;
}

void FunctionMouseDrag::move(bool shift, int X, int Y, fp x, fp y)
{
    SideBar *sib = frame->get_sidebar();

    Drag &hor = shift ? drag_shift_x : drag_x;
    hor.change_value(x, x - px, X - pX);
    if (hor.how != no_drag) {
        values[hor.parameter_idx] = hor.value;
        sib->change_parameter_value(hor.parameter_idx, hor.value);
        sidebar_dirty = true;
    }
    pX = X;
    px = x;

    Drag &vert = shift ? drag_shift_y : drag_y;
    vert.change_value(y, y - py, Y - pY);
    if (vert.how != no_drag) {
        values[vert.parameter_idx] = vert.value;
        sib->change_parameter_value(vert.parameter_idx, vert.value);
        sidebar_dirty = true;
    }
    pY = Y;
    py = y;
}

void FunctionMouseDrag::stop()
{
    if (sidebar_dirty) {
        frame->get_sidebar()->update_param_panel();
        sidebar_dirty = false;
    }
}

string FunctionMouseDrag::get_cmd() const
{
    return drag_x.get_cmd() + drag_y.get_cmd() + drag_shift_x.get_cmd()
        + drag_shift_y.get_cmd();
}

//===============================================================
//                MainPlot (plot with data and fitted curves)
//===============================================================
BEGIN_EVENT_TABLE(MainPlot, FPlot)
    EVT_PAINT (           MainPlot::OnPaint)
    EVT_MOTION (          MainPlot::OnMouseMove)
    EVT_LEAVE_WINDOW (    MainPlot::OnLeaveWindow)
    EVT_LEFT_DOWN (       MainPlot::OnButtonDown)
    EVT_RIGHT_DOWN (      MainPlot::OnButtonDown)
    EVT_MIDDLE_DOWN (     MainPlot::OnButtonDown)
    EVT_LEFT_DCLICK (     MainPlot::OnLeftDClick)
    EVT_LEFT_UP   (       MainPlot::OnButtonUp)
    EVT_RIGHT_UP (        MainPlot::OnButtonUp)
    EVT_MIDDLE_UP (       MainPlot::OnButtonUp)
    EVT_MENU (ID_plot_popup_za,     MainPlot::OnZoomAll)
    EVT_MENU_RANGE (ID_plot_popup_model, ID_plot_popup_peak,
                                    MainPlot::OnPopupShowXX)
    EVT_MENU_RANGE (ID_plot_popup_c_background, ID_plot_popup_c_model,
                                    MainPlot::OnPopupColor)
    EVT_MENU_RANGE (ID_plot_popup_pt_size, ID_plot_popup_pt_size + max_radius,
                                    MainPlot::OnPopupRadius)
    EVT_MENU (ID_plot_popup_c_inv,  MainPlot::OnInvertColors)
    EVT_MENU (ID_plot_popup_axes,   MainPlot::OnConfigureAxes)
    EVT_MENU (ID_plot_popup_plabels,MainPlot::OnConfigurePLabels)
    EVT_MENU (ID_peak_popup_info,   MainPlot::OnPeakInfo)
    EVT_MENU (ID_peak_popup_del,    MainPlot::OnPeakDelete)
    EVT_MENU (ID_peak_popup_guess,  MainPlot::OnPeakGuess)
END_EVENT_TABLE()

MainPlot::MainPlot (wxWindow *parent)
    : FPlot(parent), bgm(xs),
      basic_mode(mmd_zoom), mode(mmd_zoom),
      pressed_mouse_button(0),
      over_peak(-1), limit1(INT_MIN), limit2(INT_MIN),
      hint_receiver(NULL),
      draw_xor_peak_n(0),
      draw_xor_peak_points(NULL)
{
    set_cursor();
}

void MainPlot::OnPaint(wxPaintEvent&)
{
    frame->update_crosshair(-1, -1);
    limit1 = limit2 = INT_MIN;
    buffered_draw();
    // if necessary, redraw inverted lines
    line_following_cursor(mat_redraw);
    peak_draft(mat_redraw);
    draw_moving_func(mat_redraw);
}

fp y_of_data_for_draw_data(vector<Point>::const_iterator i,
                           Model const* /*model*/)
{
    return i->y;
}

void MainPlot::draw_dataset(wxDC& dc, int n, bool set_pen)
{
    bool shadowed;
    int offset;
    bool r = frame->get_sidebar()->howto_plot_dataset(n, shadowed, offset);
    if (!r)
        return;
    wxColour col = get_data_color(n);
    if (shadowed) {
        wxColour const& bg_col = get_bg_color();
        col.Set((col.Red() + bg_col.Red())/2,
                (col.Green() + bg_col.Green())/2,
                (col.Blue() + bg_col.Blue())/2);
    }
    if (set_pen)
        draw_data(dc, y_of_data_for_draw_data, ftk->get_data(n), 0,
                  col, wxNullColour, offset);
    else
        draw_data(dc, y_of_data_for_draw_data, ftk->get_data(n), 0,
                  dc.GetPen().GetColour(), dc.GetPen().GetColour(), offset);
}

void MainPlot::draw(wxDC &dc, bool monochrome)
{
    //cout << "MainPlot::draw()" << endl;
    int focused_data = frame->get_sidebar()->get_focused_data();
    Model const* model = ftk->get_model(focused_data);

    set_scale(get_pixel_width(dc), get_pixel_height(dc));

    frame->update_crosshair(-1, -1); //erase crosshair before redrawing plot

    int Ymax = get_pixel_height(dc);
    prepare_peaktops(model, Ymax);

    if (monochrome) {
        dc.SetPen(*wxBLACK_PEN);
        dc.SetBrush(*wxBLACK_BRUSH);
    }
    //draw datasets (selected and focused at the end)
    vector<int> ord = frame->get_sidebar()->get_ordered_dataset_numbers();
    for (vector<int>::const_iterator i = ord.begin(); i != ord.end(); ++i)
        draw_dataset(dc, *i, !monochrome);

    if (xtics_visible)
        draw_xtics(dc, ftk->view, !monochrome);
    if (ytics_visible)
        draw_ytics(dc, ftk->view, !monochrome);

    if (peaks_visible)
        draw_peaks(dc, model, !monochrome);
    if (groups_visible)
        draw_groups(dc, model, !monochrome);
    if (model_visible)
        draw_model(dc, model, !monochrome);
    if (x_axis_visible)
        draw_x_axis(dc, !monochrome);
    if (y_axis_visible)
        draw_y_axis(dc, !monochrome);

    if (visible_peaktops(mode) && !monochrome)
        draw_peaktops(dc, model);
    if (mode == mmd_bg) {
        draw_background(dc);
    }
    else {
        if (plabels_visible)
            draw_plabels(dc, model, !monochrome);
    }
}


bool MainPlot::visible_peaktops(MouseModeEnum mode)
{
    return (mode == mmd_zoom || mode == mmd_add || mode == mmd_peak
            || mode == mmd_activate);
}

void MainPlot::draw_x_axis (wxDC& dc, bool set_pen)
{
    if (set_pen)
        dc.SetPen(wxPen(xAxisCol, pen_width));
    int Y0 = ys.px(0.);
    dc.DrawLine (0, Y0, get_pixel_width(dc), Y0);
}

void MainPlot::draw_y_axis (wxDC& dc, bool set_pen)
{
    if (set_pen)
        dc.SetPen(wxPen(xAxisCol, pen_width));
    int X0 = xs.px(0.);
    dc.DrawLine (X0, 0, X0, get_pixel_height(dc));
}

void MainPlot::draw_model(wxDC& dc, Model const* model, bool set_pen)
{
    if (set_pen)
        dc.SetPen(wxPen(modelCol, pen_width));
    int n = get_pixel_width(dc);
    vector<fp> xx(n), yy(n);
    vector<int> YY(n);
    for (int i = 0; i < n; ++i)
        xx[i] = xs.val(i);
    model->compute_model(xx, yy);
    for (int i = 0; i < n; ++i)
        YY[i] = ys.px(yy[i]);
    for (int i = 1; i < n; i++)
        dc.DrawLine (i-1, YY[i-1], i, YY[i]);
    // perhaps wxDC::DrawLines() would be faster?
}


//TODO draw groups
void MainPlot::draw_groups (wxDC& /*dc*/, Model const*, bool)
{
}

void MainPlot::draw_peaks(wxDC& dc, Model const* model, bool set_pen)
{
    fp level = 0;
    vector<int> const& idx = model->get_ff_idx();
    int n = get_pixel_width(dc);
    vector<fp> xx(n), yy(n);
    vector<int> YY(n);
    for (int i = 0; i < n; ++i)
        xx[i] = xs.val(i);
    for (int k = 0; k < size(idx); k++) {
        fill(yy.begin(), yy.end(), 0.);
        Function const* f = ftk->get_function(idx[k]);
        int from=0, to=n-1;
        fp left, right;
        if (f->get_nonzero_range(level, left, right)) {
            from = max(from, xs.px(left));
            to = min(to, xs.px(right));
        }
        if (set_pen)
            dc.SetPen(wxPen(peakCol[k % max_peak_cols], pen_width));
        f->calculate_value(xx, yy);
        for (int i = from; i <= to; ++i)
            YY[i] = ys.px(yy[i]);
        for (int i = from+1; i <= to; i++)
            dc.DrawLine (i-1, YY[i-1], i, YY[i]);
    }
}

void MainPlot::draw_peaktops (wxDC& dc, Model const* model)
{
    dc.SetPen(wxPen(xAxisCol, pen_width));
    dc.SetBrush (*wxTRANSPARENT_BRUSH);
    for (vector<wxPoint>::const_iterator i = special_points.begin();
                                           i != special_points.end(); i++) {
        dc.DrawRectangle (i->x - 1, i->y - 1, 3, 3);
    }
    draw_peaktop_selection(dc, model);
}

void MainPlot::draw_peaktop_selection (wxDC& dc, Model const* model)
{
    int n = frame->get_sidebar()->get_active_function();
    if (n == -1)
        return;
    vector<int> const& idx = model->get_ff_idx();
    vector<int>::const_iterator t = find(idx.begin(), idx.end(), n);
    if (t != idx.end()) {
        wxPoint const&p = special_points[t-idx.begin()];
        dc.SetLogicalFunction (wxINVERT);
        dc.SetPen(*wxBLACK_PEN);
        dc.DrawCircle(p.x, p.y, 4);
    }
}

void MainPlot::draw_plabels (wxDC& dc, Model const* model, bool set_pen)
{
    prepare_peak_labels(model); //TODO re-prepare only when peaks where changed
    set_font(dc, plabelFont);
    vector<wxRect> previous;
    vector<int> const& idx = model->get_ff_idx();
    for (int k = 0; k < size(idx); k++) {
        const wxPoint &peaktop = special_points[k];
        if (set_pen)
            dc.SetTextForeground(peakCol[k % max_peak_cols]);

        wxString label = s2wx(plabels[k]);
        wxCoord w, h;
        if (vertical_plabels) {
            dc.GetMultiLineTextExtent(label, &h, &w); // w and h swapped
            h = 0; // Y correction is not needed
        }
        else
            dc.GetMultiLineTextExtent(label, &w, &h);
        int X = peaktop.x - w/2;
        int Y = peaktop.y - h - 2;
        wxRect rect(X, Y, w, h);

        // eliminate labels overlap
        // perhaps more sophisticated algorithm for automatic label placement
        // should be used
        const int mrg = 0; //margin around label, can be negative
        int counter = 0; // the number of different placements checked
        vector<wxRect>::const_iterator i = previous.begin();
        while (i != previous.end() && counter < 10) {
            if (i->x > rect.GetRight()+mrg || rect.x > i->GetRight()+mrg
                || i->y > rect.GetBottom()+mrg || rect.y > i->GetBottom()+mrg)
                //there is no intersection
                ++i;
            else { // intersection -- try upper rectangle
                rect.SetY(i->y - h - 2);
                i = previous.begin(); //and check for intersections with all...
                ++counter;
            }
        }
        previous.push_back(rect);
        if (vertical_plabels)
            dc.DrawRotatedText(label, rect.x, rect.y, 90);
        else
            dc.DrawLabel(label, rect, wxALIGN_CENTER|wxALIGN_BOTTOM);
    }
}


/*
static bool operator< (const wxPoint& a, const wxPoint& b)
{
    return a.x != b.x ? a.x < b.x : a.y < b.y;
}
*/

void MainPlot::prepare_peaktops(Model const* model, int Ymax)
{
    int Y0 = ys.px(0);
    vector<int> const& idx = model->get_ff_idx();
    int n = idx.size();
    special_points.resize(n);
    for (int k = 0; k < n; k++) {
        Function const *f = ftk->get_function(idx[k]);
        fp x;
        int X, Y;
        if (f->has_center()) {
            x = f->center();
            X = xs.px (x - model->zero_shift(x));
        }
        else {
            X = k * 10;
            x = xs.val(X);
            x += model->zero_shift(x);
        }
        //FIXME: check if these zero_shift()'s above are needed
        Y = ys.px(f->calculate_value(x));
        if (Y < 0 || Y > Ymax)
            Y = Y0;
        special_points[k] = wxPoint(X, Y);
    }
}


void MainPlot::prepare_peak_labels(Model const* model)
{
    vector<int> const& idx = model->get_ff_idx();
    plabels.resize(idx.size());
    for (int k = 0; k < size(idx); k++) {
        Function const *f = ftk->get_function(idx[k]);
        string label = plabel_format;
        string::size_type pos = 0;
        while ((pos = label.find("<", pos)) != string::npos) {
            string::size_type right = label.find(">", pos+1);
            if (right == string::npos)
                break;
            string tag(label, pos+1, right-pos-1);
            if (tag == "area")
                label.replace(pos, right-pos+1, S(f->area()));
            else if (tag == "height")
                label.replace(pos, right-pos+1, S(f->height()));
            else if (tag == "center")
                label.replace(pos, right-pos+1, S(f->center()));
            else if (tag == "fwhm")
                label.replace(pos, right-pos+1, S(f->fwhm()));
            else if (tag == "ib")
                label.replace(pos, right-pos+1, S(f->area()/f->height()));
            else if (tag == "name")
                label.replace(pos, right-pos+1, f->name);
            else if (tag == "br")
                label.replace(pos, right-pos+1, "\n");
            else
                ++pos;
        }
        plabels[k] = label;
    }
}


void MainPlot::draw_background(wxDC& dc, bool set_pen)
{
    if (set_pen)
        dc.SetPen(wxPen(bg_pointsCol, pen_width));
    dc.SetBrush (*wxTRANSPARENT_BRUSH);

    // bg line
    int X = -1, Y = -1;
    int width = get_pixel_width(dc);
    vector<int> bgline = bgm.calculate_bgline(width, ys);
    for (int i = 0; i < width; ++i) {
        int X_ = X, Y_ = Y;
        X = i;
        Y = bgline[i];
        if (X_ >= 0 && (X != X_ || Y != Y_))
            dc.DrawLine (X_, Y_, X, Y);
    }

    // bg points (circles)
    for (BgManager::bg_const_iterator i = bgm.begin(); i != bgm.end(); i++) {
        dc.DrawCircle(xs.px(i->x), ys.px(i->y), 3);
        dc.DrawCircle(xs.px(i->x), ys.px(i->y), 4);
    }
}

void MainPlot::read_settings(wxConfigBase *cf)
{
    cf->SetPath(wxT("/MainPlot/Colors"));
    set_bg_color(cfg_read_color(cf, wxT("bg"), wxColour(48, 48, 48)));
    for (int i = 0; i < max_data_cols; i++)
        dataColour[i] = cfg_read_color(cf, wxString::Format(wxT("data/%i"), i),
                                               wxColour(0, 255, 0));
    inactiveDataCol = cfg_read_color(cf,wxT("inactive_data"),
                                                      wxColour (128, 128, 128));
    modelCol = cfg_read_color (cf, wxT("model"), wxColour(wxT("YELLOW")));
    bg_pointsCol = cfg_read_color(cf, wxT("BgPoints"), wxColour(wxT("RED")));
    for (int i = 0; i < max_group_cols; i++)
        groupCol[i] = cfg_read_color(cf, wxString::Format(wxT("group/%i"), i),
                                     wxColour(173, 216, 230));
    for (int i = 0; i < max_peak_cols; i++)
        peakCol[i] = cfg_read_color(cf, wxString::Format(wxT("peak/%i"), i),
                                    wxColour(255, 0, 0));

    cf->SetPath(wxT("/MainPlot/Visible"));
    peaks_visible = cfg_read_bool(cf, wxT("peaks"), true);
    plabels_visible = cfg_read_bool(cf, wxT("plabels"), false);
    groups_visible = cfg_read_bool(cf, wxT("groups"), false);
    model_visible = cfg_read_bool(cf, wxT("model"), true);
    cf->SetPath(wxT("/MainPlot"));
    point_radius = cf->Read (wxT("point_radius"), 1);
    line_between_points = cfg_read_bool(cf,wxT("line_between_points"), false);
    draw_sigma = cfg_read_bool(cf,wxT("draw_sigma"), false);
    wxFont default_plabel_font(10, wxFONTFAMILY_DEFAULT,
                               wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    plabelFont = cfg_read_font(cf, wxT("plabelFont"), default_plabel_font);
    plabel_format = wx2s(cf->Read(wxT("plabel_format"), wxT("<area>")));
    vertical_plabels = cfg_read_bool(cf, wxT("vertical_plabels"), false);
    x_max_tics = cf->Read(wxT("xMaxTics"), 7);
    y_max_tics = cf->Read(wxT("yMaxTics"), 7);
    x_tic_size = cf->Read(wxT("xTicSize"), 4);
    y_tic_size = cf->Read(wxT("yTicSize"), 4);
    xs.reversed = cfg_read_bool (cf, wxT("xReversed"), false);
    ys.reversed = cfg_read_bool (cf, wxT("yReversed"), false);
    xs.logarithm = cfg_read_bool (cf, wxT("xLogarithm"), false);
    ys.logarithm = cfg_read_bool (cf, wxT("yLogarithm"), false);
    ftk->view.set_log_scale(xs.logarithm, ys.logarithm);
    FPlot::read_settings(cf);
    refresh();
}

void MainPlot::save_settings(wxConfigBase *cf) const
{
    cf->SetPath(wxT("/MainPlot"));
    cf->Write (wxT("point_radius"), point_radius);
    cf->Write (wxT("line_between_points"), line_between_points);
    cf->Write (wxT("draw_sigma"), draw_sigma);
    cfg_write_font (cf, wxT("plabelFont"), plabelFont);
    cf->Write(wxT("plabel_format"), s2wx(plabel_format));
    cf->Write (wxT("vertical_plabels"), vertical_plabels);
    cf->Write(wxT("xMaxTics"), x_max_tics);
    cf->Write(wxT("yMaxTics"), y_max_tics);
    cf->Write(wxT("xTicSize"), x_tic_size);
    cf->Write(wxT("yTicSize"), y_tic_size);
    cf->Write(wxT("xReversed"), xs.reversed);
    cf->Write(wxT("yReversed"), ys.reversed);
    cf->Write(wxT("xLogarithm"), xs.logarithm);
    cf->Write(wxT("yLogarithm"), ys.logarithm);

    cf->SetPath(wxT("/MainPlot/Colors"));
    cfg_write_color(cf, wxT("bg"), get_bg_color());
    for (int i = 0; i < max_data_cols; i++)
        cfg_write_color(cf, wxString::Format(wxT("data/%i"), i), dataColour[i]);
    cfg_write_color (cf, wxT("inactive_data"), inactiveDataCol);
    cfg_write_color (cf, wxT("model"), modelCol);
    cfg_write_color (cf, wxT("BgPoints"), bg_pointsCol);
    for (int i = 0; i < max_group_cols; i++)
        cfg_write_color(cf, wxString::Format(wxT("group/%i"), i), groupCol[i]);
    for (int i = 0; i < max_peak_cols; i++)
        cfg_write_color(cf, wxString::Format(wxT("peak/%i"), i), peakCol[i]);

    cf->SetPath(wxT("/MainPlot/Visible"));
    cf->Write (wxT("peaks"), peaks_visible);
    cf->Write (wxT("plabels"), plabels_visible);
    cf->Write (wxT("groups"), groups_visible);
    cf->Write (wxT("model"), model_visible);
    cf->SetPath(wxT("/MainPlot"));
    FPlot::save_settings(cf);
}

void MainPlot::OnLeaveWindow (wxMouseEvent&)
{
    frame->clear_status_coords();
    frame->update_crosshair(-1, -1);
}

void MainPlot::show_popup_menu (wxMouseEvent &event)
{
    wxMenu popup_menu; //("main plot menu");

    popup_menu.Append(ID_plot_popup_za, wxT("Zoom &All"));
    popup_menu.AppendSeparator();

    wxMenu *show_menu = new wxMenu;
    show_menu->AppendCheckItem (ID_plot_popup_model, wxT("&Model"), wxT(""));
    show_menu->Check (ID_plot_popup_model, model_visible);
    //show_menu->AppendCheckItem (ID_plot_popup_groups,
    //                            wxT("Grouped peaks"), wxT(""));
    //show_menu->Check (ID_plot_popup_groups, groups_visible);
    show_menu->AppendCheckItem (ID_plot_popup_peak, wxT("&Peaks"), wxT(""));
    show_menu->Check (ID_plot_popup_peak, peaks_visible);
    popup_menu.Append (wxNewId(), wxT("&Show"), show_menu);

    wxMenu *color_menu = new wxMenu;
    color_menu->Append (ID_plot_popup_c_background, wxT("&Background"));
    color_menu->Append (ID_plot_popup_c_inactive_data, wxT("&Inactive Data"));
    color_menu->Append (ID_plot_popup_c_model, wxT("&Model"));
    color_menu->AppendSeparator();
    color_menu->Append (ID_plot_popup_c_inv, wxT("&Invert colors"));
    popup_menu.Append (wxNewId(), wxT("&Color"), color_menu);

    wxMenu *size_menu = new wxMenu;
    size_menu->AppendCheckItem (ID_plot_popup_pt_size, wxT("&Line"), wxT(""));
    size_menu->Check (ID_plot_popup_pt_size, line_between_points);
    size_menu->AppendSeparator();
    for (int i = 1; i <= max_radius; i++)
        size_menu->AppendRadioItem (ID_plot_popup_pt_size + i,
                                    wxString::Format (wxT("&%d"), i), wxT(""));
    size_menu->Check (ID_plot_popup_pt_size + point_radius, true);
    popup_menu.Append (wxNewId(), wxT("Data point si&ze"), size_menu);

    popup_menu.Append (ID_plot_popup_axes, wxT("Configure &Axes..."));
    popup_menu.Append (ID_plot_popup_plabels, wxT("Configure Peak &Labels..."));

    PopupMenu (&popup_menu, event.GetX(), event.GetY());
}

void MainPlot::show_peak_menu (wxMouseEvent &event)
{
    if (over_peak == -1) return;
    wxMenu peak_menu;
    peak_menu.Append(ID_peak_popup_info, wxT("Show &Info"));
    peak_menu.Append(ID_peak_popup_del, wxT("&Delete"));
    peak_menu.Append(ID_peak_popup_guess, wxT("&Guess parameters"));
    peak_menu.Enable(ID_peak_popup_guess,
                     ftk->get_function(over_peak)->has_center());
    PopupMenu (&peak_menu, event.GetX(), event.GetY());
}

void MainPlot::PeakInfo()
{
    if (over_peak >= 0)
        ftk->exec("info+ " + ftk->get_function(over_peak)->xname);
}

void MainPlot::OnPeakDelete(wxCommandEvent&)
{
    if (over_peak >= 0)
        ftk->exec("delete " + ftk->get_function(over_peak)->xname);
}

void MainPlot::OnPeakGuess(wxCommandEvent&)
{
    if (over_peak < 0)
        return;
    Function const* p = ftk->get_function(over_peak);
    if (p->has_center()) {
        fp ctr = p->center();
        fp plusmin = max(fabs(p->fwhm()), p->iwidth());
        char buffer[64];
        sprintf(buffer, " [%.12g:%.12g]", ctr-plusmin, ctr+plusmin);
        ftk->exec(p->xname + " = guess " + p->type_name + buffer
                  + frame->get_global_parameters() + frame->get_in_datasets());
    }
}


// mouse usage
//
//  Simple Rules:
//   1. When one button is down, pressing other button cancels action
//   2. Releasing / keeping down / pressing Ctrl/Alt keys, when you keep
//     mouse button down makes no difference.
//   3. Ctrl and Alt buttons are equivalent.
//  ----------------------------------
//  Usage:
//   Left/Right Button (no Ctrl)   -- mode dependent
//   Ctrl + Left/Right Button      -- the same as Left/Right in normal mode
//   Middle Button                 -- rectangle zoom
//   Shift sometimes makes a difference

void MainPlot::set_mouse_mode(MouseModeEnum m)
{
    if (pressed_mouse_button) cancel_mouse_press();
    MouseModeEnum old = mode;
    if (m != mmd_peak)
        basic_mode = m;
    mode = m;
    update_mouse_hints();
    set_cursor();
    if (old != mode && (old == mmd_bg || mode == mmd_bg
                        || visible_peaktops(old) != visible_peaktops(mode)))
        refresh();
}

// update mouse hint on status bar
void MainPlot::update_mouse_hints()
{
    if (!hint_receiver)
        return;
    const char *left="";
    const char *right="";
    const char *mode_name="";
    const char *shift_left = "";
    const char *shift_right = "";
    if (pressed_mouse_button) {
        if (pressed_mouse_button != 1)
            left = "cancel";
        if (pressed_mouse_button != 3)
            right = "cancel";
    }
    else { //button not pressed
        switch (mode) {
            case mmd_peak:
                left = "move peak"; right = "peak menu";
                break;
            case mmd_zoom:
                left = "zoom"; right = "menu";
                shift_left = "vertical zoom";
                shift_right = "horizontal zoom";
                mode_name = "normal";
                break;
            case mmd_bg:
                left = "add point"; right = "del point";
                mode_name = "baseline";
                break;
            case mmd_add:
                left = "manual add";  right = "add in range";
                mode_name = "add-peak";
                break;
            case mmd_activate:
                left = "activate";  right = "disactivate";
                mode_name = "data range";
                shift_left = "activate rectangle";
                shift_right = "disactivate rectangle";
                break;
            default:
                assert(0);
        }
    }
    hint_receiver->set_hints(left, right, mode_name, shift_left, shift_right);
}

void MainPlot::set_cursor()
{
    SetCursor(mode == mmd_peak ? wxCURSOR_CROSS : wxCURSOR_ARROW);
}

void MainPlot::OnMouseMove(wxMouseEvent &event)
{
    //display coords in status bar
    int X = event.GetX();
    int Y = event.GetY();
    frame->set_status_coords(xs.val(X), ys.val(Y), pte_main);

    if (pressed_mouse_button != 0) {
        line_following_cursor(mat_move, lfc_orient == kVerticalLine ? X : Y);
        draw_moving_func(mat_move, X, Y, event.ShiftDown());
        peak_draft(mat_move, X, Y);
        draw_temporary_rect(mat_move, X, Y);
    }
    else { // no button pressed
        if (visible_peaktops(mode))
            look_for_peaktop(event);
        int cY = Y; // cross-hair Y. If negative, only vertical line is drawn.
        // In mmd_activate span mode, draw line following cursor
        if (mode == mmd_activate) {
            if (!(event.AltDown() || event.CmdDown()))
                cY = -1;
        }
        frame->update_crosshair(X, cY);
    }
}

void MainPlot::look_for_peaktop (wxMouseEvent& event)
{
    int focused_data = frame->get_sidebar()->get_focused_data();
    Model const* model = ftk->get_model(focused_data);
    vector<int> const& idx = model->get_ff_idx();
    if (special_points.size() != idx.size())
        refresh();
    int n = get_special_point_at_pointer(event);
    int nearest = n == -1 ? -1 : idx[n];

    if (over_peak == nearest)
        return;

    // if we are here, over_peak != nearest; changing cursor and statusbar text
    // and show limits
    over_peak = nearest;
    if (nearest != -1) {
        Function const* f = ftk->get_function(over_peak);
        string s = f->xname + " " + f->type_name + " ";
        vector<string> const& vn = f->type_var_names;
        for (int i = 0; i < size(vn); ++i)
            s += " " + vn[i] + "=" + S(f->get_var_value(i));
        frame->set_status_text(s);
        set_mouse_mode(mmd_peak);
        fp x1=0., x2=0.;
        bool r = f->get_nonzero_range(ftk->get_settings()->get_cut_level(),
                                      x1, x2);
        if (r) {
            limit1 = xs.px(x1);
            limit2 = xs.px(x2);
            draw_inverted_line(limit1, wxPENSTYLE_DOT_DASH, kVerticalLine);
            draw_inverted_line(limit2, wxPENSTYLE_DOT_DASH, kVerticalLine);
        }
        else
            limit1 = limit2 = INT_MIN;
    }
    else { //was over peak, but now is not
        frame->set_status_text("");
        set_mouse_mode(basic_mode);
        draw_inverted_line(limit1, wxPENSTYLE_DOT_DASH, kVerticalLine);
        draw_inverted_line(limit2, wxPENSTYLE_DOT_DASH, kVerticalLine);
        limit1 = limit2 = INT_MIN;
    }
}

void MainPlot::cancel_mouse_press()
{
    if (pressed_mouse_button) {
        draw_temporary_rect(mat_stop);
        draw_moving_func(mat_stop);
        peak_draft(mat_stop);
        line_following_cursor(mat_stop);
        mouse_press_X = mouse_press_Y = INT_MIN;
        pressed_mouse_button = 0;
        frame->set_status_text("");
        update_mouse_hints();
        set_cursor();
    }
}

MouseOperation MainPlot::what_mouse_operation(wxMouseEvent const& event)
{
    bool ctrl = (event.AltDown() || event.CmdDown());
    bool shift = event.ShiftDown();
    int button = event.GetButton();
    if (button == 2 || // middle button always zooms
        (button == 1 && (ctrl || (mode == mmd_zoom && !shift))))
        return kRectangularZoom;
    else if (button == 3 && (ctrl || (mode == mmd_zoom && !shift)))
        return kShowPlotMenu;
    else if (button == 1 && mode == mmd_zoom && shift)
        return kVerticalZoom;
    else if (button == 3 && mode == mmd_zoom && shift)
        return kHorizontalZoom;
    else if (button == 1 && mode == mmd_peak)
        return kDragPeak;
    else if (button == 3 && mode == mmd_peak)
        return kShowPeakMenu;
    else if (button == 1 && mode == mmd_bg)
        return kAddBgPoint;
    else if (button == 3 && mode == mmd_bg)
        return kDeleteBgPoint;
    else if (button == 1 && mode == mmd_add)
        return kAddPeakTriangle;
    else if (button == 3 && mode == mmd_add)
        return kAddPeakInRange;
    else if (button == 1 && mode == mmd_activate)
        return shift ? kActivateRect : kActivateSpan;
    else if (button == 3 && mode == mmd_activate)
        return shift ? kDisactivateRect : kDisactivateSpan;
    else
        return kNoMouseOp;
}

void MainPlot::OnButtonDown (wxMouseEvent &event)
{
    if (pressed_mouse_button) {
        // if one button is already down, pressing other button cancels action
        cancel_mouse_press();
        return;
    }

    frame->update_crosshair(-1, -1);
    pressed_mouse_button = event.GetButton();
    mouse_press_X = event.GetX();
    mouse_press_Y = event.GetY();
    fp x = xs.val(event.GetX());
    fp y = ys.val(event.GetY());
    mouse_op = what_mouse_operation(event);
    if (mouse_op == kRectangularZoom) {
        draw_temporary_rect(mat_start, event.GetX(), event.GetY());
        SetCursor(wxCURSOR_MAGNIFIER);
        frame->set_status_text("Select second corner to zoom...");
    }
    else if (mouse_op == kShowPlotMenu) {
        show_popup_menu(event);
        cancel_mouse_press();
    }
    else if (mouse_op == kShowPeakMenu) {
        show_peak_menu(event);
        cancel_mouse_press();
    }
    else if (mouse_op == kVerticalZoom) {
        SetCursor(wxCURSOR_SIZENS);
        start_line_following_cursor(mouse_press_Y, kHorizontalLine);
        frame->set_status_text("Select vertical span...");
    }
    else if (mouse_op == kHorizontalZoom) {
        SetCursor(wxCURSOR_SIZEWE);
        start_line_following_cursor(mouse_press_X, kVerticalLine);
        frame->set_status_text("Select horizontal span...");
    }
    else if (mouse_op == kDragPeak) {
        frame->get_sidebar()->activate_function(over_peak);
        draw_moving_func(mat_start, event.GetX(), event.GetY());
        frame->set_status_text("Moving " + ftk->get_function(over_peak)->xname
                                + "...");
    }
    else if (mouse_op == kAddBgPoint) {
        bgm.add_background_point(x, y);
        refresh();
    }
    else if (mouse_op == kDeleteBgPoint) {
        bgm.rm_background_point(x);
        refresh();
    }
    else if (mouse_op == kAddPeakTriangle) {
        func_draft_kind
            = get_function_kind(Function::get_formula(frame->get_peak_type()));
        peak_draft (mat_start, event.GetX(), event.GetY());
        SetCursor(wxCURSOR_SIZING);
        frame->set_status_text("Add drawed peak...");
    }
    else if (mouse_op == kAddPeakInRange) {
        start_line_following_cursor(mouse_press_X, kVerticalLine);
        SetCursor(wxCURSOR_SIZEWE);
        frame->set_status_text("Select range to add a peak in it...");
    }
    else if (mouse_op == kActivateSpan || mouse_op == kDisactivateSpan ||
             mouse_op == kActivateRect || mouse_op == kDisactivateRect) {
        string act_str;
        if (mouse_op == kActivateSpan || mouse_op == kActivateRect) {
            if (!can_activate()) {
                cancel_mouse_press();
                wxMessageBox(
                 wxT("You pressed the left mouse button in data-range mode,")
                 wxT("\nbut all data points are already active.")
                 wxT("\n\nYou can see mouse hints at the status bar: the left")
                 wxT("\nbutton activates, and the right disactivates points.")
                 wxT("\n\nExtra hint: to activate/disactivate points in")
                 wxT(" a rectangle,\npress Shift"),
                             wxT("How to use mouse..."),
                             wxOK|wxICON_INFORMATION);
                return;
            }
            act_str = "activate";
        }
        else
            act_str = "disactivate";
        string status_beginning;
        if (mouse_op == kActivateRect || mouse_op == kDisactivateRect) {
            SetCursor(wxCURSOR_SIZENWSE);
            draw_temporary_rect(mat_start, event.GetX(), event.GetY());
            status_beginning = "Select data in rectangle to ";
        }
        else {
            SetCursor(wxCURSOR_SIZEWE);
            start_line_following_cursor(mouse_press_X, kVerticalLine);
            status_beginning = "Select data range to ";
        }
        frame->set_status_text(status_beginning + act_str + "...");
    }
    update_mouse_hints();
}

bool MainPlot::can_activate()
{
    vector<int> sel = frame->get_sidebar()->get_selected_data_indices();
    for (vector<int>::const_iterator i = sel.begin(); i != sel.end(); ++i) {
        Data const* data = ftk->get_data(*i);
        // if data->is_empty() we allow to try disactivate data to let user
        // experiment with mouse right after launching the program
        if (data->is_empty() || data->get_n() != size(data->points()))
            return true;
    }
    return false;
}

void MainPlot::OnButtonUp (wxMouseEvent &event)
{
    int button = event.GetButton();
    if (button != pressed_mouse_button) {
        pressed_mouse_button = 0;
        return;
    }
    int dist_X = abs(event.GetX() - mouse_press_X);
    int dist_Y = abs(event.GetY() - mouse_press_Y);
    // if Down and Up events are at the same position -> cancel

    // zoom
    if (mouse_op == kRectangularZoom) {
        draw_temporary_rect(mat_stop);
        if (dist_X + dist_Y >= 10) {
            fp x1 = xs.val(mouse_press_X);
            fp x2 = xs.val(event.GetX());
            fp y1 = ys.val(mouse_press_Y);
            fp y2 = ys.val(event.GetY());
            char buffer[128];
            sprintf(buffer, "[%.12g:%.12g] [%.12g:%.12g]",
                    min(x1,x2), max(x1,x2), min(y1,y2), max(y1,y2));
            frame->change_zoom(buffer);
        }
        else
            frame->set_status_text("");
    }
    else if (mouse_op == kVerticalZoom) {
        line_following_cursor(mat_stop);
        if (dist_Y >= 5) {
            fp y1 = ys.val(mouse_press_Y);
            fp y2 = ys.val(event.GetY());
            char buffer[64];
            sprintf(buffer, ". [%.12g:%.12g]", min(y1,y2), max(y1,y2));
            frame->change_zoom(buffer);
        }
        else
            frame->set_status_text("");
    }
    else if (mouse_op == kHorizontalZoom) {
        line_following_cursor(mat_stop);
        if (dist_X >= 5) {
            fp x1 = xs.val(mouse_press_X);
            fp x2 = xs.val(event.GetX());
            char buffer[64];
            sprintf(buffer, "[%.12g:%.12g] .", min(x1,x2), max(x1,x2));
            frame->change_zoom(buffer);
        }
        else
            frame->set_status_text("");
    }
    // drag peak
    else if (mouse_op == kDragPeak) {
        if (dist_X + dist_Y >= 2) {
            string cmd = fmd.get_cmd();
            if (!cmd.empty())
                ftk->exec(cmd);
        }
        draw_moving_func(mat_stop);
        frame->set_status_text("");
    }
    // activate or disactivate data
    else if (mouse_op == kActivateSpan || mouse_op == kDisactivateSpan ||
             mouse_op == kActivateRect || mouse_op == kDisactivateRect) {
        line_following_cursor(mat_stop);
        draw_temporary_rect(mat_stop);
        bool rect = (mouse_op == kActivateRect || mouse_op == kDisactivateRect);
        bool ok = (!rect && dist_X >= 5) ||
                  (rect && dist_X + dist_Y >= 10);
        if (ok) {
            string c = (mouse_op == kActivateSpan || mouse_op == kActivateRect
                        ? "A = a or" : "A = a and not");
            fp x1 = xs.val(mouse_press_X);
            fp x2 = xs.val(event.GetX());
            string cond = eS(min(x1,x2)) + " < x < " + eS(max(x1,x2));
            if (rect) {
                fp y1 = ys.val(mouse_press_Y);
                fp y2 = ys.val(event.GetY());
                cond += " and " + eS(min(y1,y2)) + " < y < " + eS(max(y1,y2));
            }
            ftk->exec(c + " (" + cond + ")" + frame->get_in_datasets());
        }
        frame->set_status_text("");
    }
    // add peak (left button)
    else if (mouse_op == kAddPeakTriangle) {
        frame->set_status_text("");
        peak_draft(mat_stop, event.GetX(), event.GetY());
        if (func_draft_kind == fk_linear || dist_X + dist_Y >= 5)
            add_peak_from_draft(event.GetX(), event.GetY());
    }
    // add peak (in range)
    else if (mouse_op == kAddPeakInRange) {
        frame->set_status_text("");
        if (dist_X >= 5) {
            fp x1 = xs.val(mouse_press_X);
            fp x2 = xs.val(event.GetX());
            ftk->exec("guess " + frame->get_peak_type()
                          + " [" + S(min(x1,x2)) + " : " + S(max(x1,x2)) + "]"
                          + frame->get_global_parameters()
                          + frame->get_in_datasets());
        }
        line_following_cursor(mat_stop);
    }
    else {
        ;// nothing - action done in OnButtonDown()
    }
    pressed_mouse_button = 0;
    update_mouse_hints();
    set_cursor();
}

void MainPlot::add_peak_from_draft(int X, int Y)
{
    string args;
    if (func_draft_kind == fk_linear) {
        fp y = ys.val(Y);
        args = "slope=~" + S(0) + ", intercept=~" + S(y) + ", avgy=~" + S(y);
    }
    else {
        fp height = ys.val(Y);
        fp center = xs.val(mouse_press_X);
        fp fwhm = fabs(center - xs.val(X));
        fp area = height * fwhm;
        args = "height=~" + S(height) + ", center=~" + S(center)
                 + ", fwhm=~" + S(fwhm) + ", area=~" + S(area);
        string global = frame->get_global_parameters();
        if (!global.empty())
            args += "," + global;
    }
    string tail = "F += " + frame->get_peak_type() + "(" + args + ")";
    string cmd;
    if (ftk->get_dm_count() == 1)
        cmd = tail;
    else {
        vector<int> sel = frame->get_sidebar()->get_selected_data_indices();
        cmd = "@" + join_vector(sel, "." + tail + "; @") + "." + tail;
    }
    ftk->exec(cmd);
}


bool MainPlot::draw_moving_func(MouseActEnum ma, int X, int Y, bool shift)
{
    static int prevX=INT_MIN, prevY=INT_MIN;
    static wxCursor old_cursor = wxNullCursor;
    static int func_nr = -1;

    if (over_peak == -1)
        return false;

    Function const* p = ftk->get_function(over_peak);

    if (ma != mat_start && func_nr != over_peak)
            return false;


    if (ma == mat_redraw)
        redraw_xor_peak();
    else if (ma == mat_start) {
        func_nr = over_peak;
        fmd.start(p, X, Y, xs.val(X), ys.val(Y));
        bool erase_previous = false;
        draw_xor_peak(p, fmd.get_values(), erase_previous);
        prevX = X;
        prevY = Y;
        old_cursor = GetCursor();
        SetCursor(wxCURSOR_SIZENWSE);
        connect_esc_to_cancel(true);
    }
    else if (ma == mat_move) {
        fmd.move(shift, X, Y, xs.val(X), ys.val(Y));
        frame->set_status_text(fmd.get_status());
        bool erase_previous = true;
        draw_xor_peak(p, fmd.get_values(), erase_previous);
        prevX = X;
        prevY = Y;
    }
    else if (ma == mat_stop) {
        redraw_xor_peak();
        func_nr = -1;
        if (old_cursor.Ok()) {
            SetCursor(old_cursor);
            old_cursor = wxNullCursor;
        }
        fmd.stop();
        connect_esc_to_cancel(false);
    }
    return true;
}


void MainPlot::draw_xor_peak(Function const* func, vector<fp> const& p_values,
                             bool erase_previous)
{
    wxClientDC dc(this);
    dc.SetLogicalFunction (wxINVERT);
    dc.SetPen(*wxBLACK_DASHED_PEN);

    int n = get_pixel_width(dc);
    if (n <= 0)
        return;
    vector<fp> xx(n), yy(n, 0);
    for (int i = 0; i < n; ++i)
        xx[i] = xs.val(i);
    func->calculate_values_with_params(xx, yy, p_values);

    if (erase_previous && draw_xor_peak_points != NULL)
        dc.DrawLines(draw_xor_peak_n, draw_xor_peak_points);
    if (n != draw_xor_peak_n) {
        draw_xor_peak_n = n;
        delete draw_xor_peak_points;
        draw_xor_peak_points = new wxPoint[n];
        for (int i = 0; i < n; ++i)
            draw_xor_peak_points[i].x = i;
    }

    for (int i = 0; i < n; ++i)
        draw_xor_peak_points[i].y = ys.px(yy[i]);
    dc.DrawLines(n, draw_xor_peak_points);
}

void MainPlot::redraw_xor_peak(bool clear)
{
    if (draw_xor_peak_n == 0 || draw_xor_peak_points == NULL)
        return;
    wxClientDC dc(this);
    dc.SetLogicalFunction (wxINVERT);
    dc.SetPen(*wxBLACK_DASHED_PEN);
    dc.DrawLines(draw_xor_peak_n, draw_xor_peak_points);
    if (clear) {
        delete draw_xor_peak_points;
        draw_xor_peak_points = NULL;
        draw_xor_peak_n = 0;
    }
}


void MainPlot::peak_draft(MouseActEnum ma, int X_, int Y_)
{
    static wxPoint prev(INT_MIN, INT_MIN);
    if (ma != mat_start) {
        if (prev.x == INT_MIN)
            return;
        //clear/redraw old peak-draft
        draw_peak_draft(mouse_press_X, abs(mouse_press_X - prev.x), prev.y);
    }
    switch (ma) {
        case mat_start:
            connect_esc_to_cancel(true);
            // no break
        case mat_move:
            prev.x = X_, prev.y = Y_;
            draw_peak_draft(mouse_press_X, abs(mouse_press_X-prev.x), prev.y);
            break;
        case mat_stop:
            prev.x = prev.y = INT_MIN;
            connect_esc_to_cancel(false);
            break;
        case mat_redraw: //already redrawn
            break;
        default: assert(0);
    }
}

void MainPlot::draw_peak_draft(int Ctr, int Hwhm, int Y)
{
    if (Ctr == INT_MIN || Hwhm == INT_MIN || Y == INT_MIN)
        return;
    wxClientDC dc(this);
    dc.SetLogicalFunction (wxINVERT);
    dc.SetPen(*wxBLACK_DASHED_PEN);
    int Y0 = ys.px(0);
    if (func_draft_kind == fk_linear) {
        dc.DrawLine (0, Y, get_pixel_width(dc), Y);
    }
    else {
        dc.DrawLine (Ctr, Y0, Ctr, Y); //vertical line
        dc.DrawLine (Ctr - Hwhm, (Y+Y0)/2, Ctr + Hwhm, (Y+Y0)/2); //horizontal
        dc.DrawLine (Ctr, Y, Ctr - 2 * Hwhm, Y0); //left slope
        dc.DrawLine (Ctr, Y, Ctr + 2 * Hwhm, Y0); //right slope
    }
}

void MainPlot::draw_temporary_rect(MouseActEnum ma, int X_, int Y_)
{
    static int X1 = INT_MIN, Y1 = INT_MIN, X2 = INT_MIN, Y2 = INT_MIN;

    if (ma != mat_start && X1 == INT_MIN)
        return;
    switch (ma) {
        case mat_start:
            X1 = X2 = X_;
            Y1 = Y2 = Y_;
            draw_rect (X1, Y1, X2, Y2);
            CaptureMouse();
            connect_esc_to_cancel(true);
            break;
        case mat_move:
            draw_rect (X1, Y1, X2, Y2); //clear old rectangle
            X2 = X_;
            Y2 = Y_;
            draw_rect (X1, Y1, X2, Y2);
            break;
        case mat_stop:
            draw_rect (X1, Y1, X2, Y2); //clear old rectangle
            X1 = INT_MIN;
            ReleaseMouse();
            connect_esc_to_cancel(false);
            break;
        case mat_redraw: //unused
            break;
    }
}

void MainPlot::draw_rect (int X1, int Y1, int X2, int Y2)
{
    if (X1 == INT_MIN || Y1 == INT_MIN || X2 == INT_MIN || Y2 == INT_MIN)
        return;
    wxClientDC dc(this);
    dc.SetLogicalFunction (wxINVERT);
    dc.SetPen(*wxBLACK_DASHED_PEN);
    dc.SetBrush (*wxTRANSPARENT_BRUSH);
    int xmin = min (X1, X2);
    int width = max (X1, X2) - xmin;
    int ymin = min (Y1, Y2);
    int height = max (Y1, Y2) - ymin;
    dc.DrawRectangle (xmin, ymin, width, height);
}

void MainPlot::OnPopupShowXX (wxCommandEvent& event)
{
    switch (event.GetId()) {
        case ID_plot_popup_model:  model_visible = !model_visible;   break;
        case ID_plot_popup_groups: groups_visible = !groups_visible; break;
        case ID_plot_popup_peak:   peaks_visible = !peaks_visible;   break;
        default: assert(0);
    }
    refresh();
}

void MainPlot::OnPopupColor(wxCommandEvent& event)
{
    int n = event.GetId();
    wxColour *color = 0;
    wxColour bg_color = get_bg_color();
    if (n == ID_plot_popup_c_background)
        color = &bg_color;
    else if (n == ID_plot_popup_c_inactive_data) {
        color = &inactiveDataCol;
    }
    else if (n == ID_plot_popup_c_model)
        color = &modelCol;
    else
        return;
    if (change_color_dlg(*color)) {
        if (n == ID_plot_popup_c_background) {
            frame->update_data_pane();
            set_bg_color(bg_color);
        }
        refresh();
    }
}

void MainPlot::OnInvertColors (wxCommandEvent&)
{
    set_bg_color(invert_colour(get_bg_color()));
    for (int i = 0; i < max_data_cols; i++)
        dataColour[i] = invert_colour(dataColour[i]);
    inactiveDataCol = invert_colour(inactiveDataCol);
    modelCol = invert_colour(modelCol);
    xAxisCol = invert_colour(xAxisCol);
    for (int i = 0; i < max_group_cols; i++)
        groupCol[i] = invert_colour(groupCol[i]);
    for (int i = 0; i < max_peak_cols; i++)
        peakCol[i] = invert_colour(peakCol[i]);
    frame->update_data_pane();
    refresh();
}

void MainPlot::OnPopupRadius (wxCommandEvent& event)
{
    int nr = event.GetId() - ID_plot_popup_pt_size;
    if (nr == 0)
        line_between_points = !line_between_points;
    else
        point_radius = nr;
    refresh();
}

void MainPlot::OnConfigureAxes (wxCommandEvent&)
{
    ConfigureAxesDlg dialog(NULL, -1, this);
    dialog.ShowModal();
}

void MainPlot::OnConfigurePLabels (wxCommandEvent&)
{
    ConfigurePLabelsDlg dialog(NULL, -1, this);
    dialog.ShowModal();
}

void MainPlot::OnZoomAll (wxCommandEvent&)
{
    frame->OnGViewAll(dummy_cmd_event);
}

//===============================================================
//                     ConfigureAxesDlg
//===============================================================

BEGIN_EVENT_TABLE(ConfigureAxesDlg, wxDialog)
    EVT_BUTTON(wxID_APPLY, ConfigureAxesDlg::OnApply)
    EVT_BUTTON(wxID_CLOSE, ConfigureAxesDlg::OnClose)
    EVT_BUTTON(ID_CAD_COLOR, ConfigureAxesDlg::OnChangeColor)
    EVT_BUTTON(ID_CAD_FONT, ConfigureAxesDlg::OnChangeFont)
END_EVENT_TABLE()

ConfigureAxesDlg::ConfigureAxesDlg(wxWindow* parent, wxWindowID id,
                                   MainPlot* plot_)
    //explicit conversion of title to wxString() is neccessary
  : wxDialog(parent, id, wxString(wxT("Configure Axes"))), plot(plot_),
    axis_color(plot_->xAxisCol)
{
    wxBoxSizer *top_sizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer *sizer1 = new wxBoxSizer(wxHORIZONTAL);
    wxStaticBoxSizer *xsizer = new wxStaticBoxSizer(wxVERTICAL, this,
                                                    wxT("X axis"));
    x_show_axis = new wxCheckBox(this, -1, wxT("show axis"));
    x_show_axis->SetValue(plot->x_axis_visible);
    xsizer->Add(x_show_axis, 0, wxALL, 5);
    x_show_tics = new wxCheckBox(this, -1, wxT("show tics"));
    x_show_tics->SetValue(plot->xtics_visible);
    xsizer->Add(x_show_tics, 0, wxALL, 5);
    wxBoxSizer* xsizer_t = new wxBoxSizer(wxVERTICAL);
    x_show_minor_tics = new wxCheckBox(this, -1, wxT("show minor tics"));
    x_show_minor_tics->SetValue(plot->xminor_tics_visible);
    xsizer_t->Add(x_show_minor_tics, 0, wxALL, 5);
    x_show_grid = new wxCheckBox(this, -1, wxT("show grid"));
    x_show_grid->SetValue(plot->x_grid);
    xsizer_t->Add(x_show_grid, 0, wxALL, 5);
    wxBoxSizer *xmt_sizer = new wxBoxSizer(wxHORIZONTAL);
    xmt_sizer->Add(new wxStaticText(this, -1, wxT("max. number of tics")),
                  0, wxALL|wxALIGN_CENTRE_VERTICAL, 5);
    x_max_tics = new wxSpinCtrl (this, -1, wxT("7"),
                                 wxDefaultPosition, wxSize(50, -1),
                                 wxSP_ARROW_KEYS, 1, 30, 7);
    x_max_tics->SetValue(plot->x_max_tics);
    xmt_sizer->Add(x_max_tics, 0, wxALL, 5);
    xsizer_t->Add(xmt_sizer);
    wxBoxSizer *xts_sizer = new wxBoxSizer(wxHORIZONTAL);
    xts_sizer->Add(new wxStaticText(this, -1, wxT("length of tics")),
                  0, wxALL|wxALIGN_CENTRE_VERTICAL, 5);
    x_tics_size = new wxSpinCtrl (this, -1, wxT("4"),
                                  wxDefaultPosition, wxSize(50, -1),
                                  wxSP_ARROW_KEYS, -10, 20, 4);
    x_tics_size->SetValue(plot->x_tic_size);
    xts_sizer->Add(x_tics_size, 0, wxALL, 5);
    xsizer_t->Add(xts_sizer);
    xsizer->Add(xsizer_t, 0, wxLEFT, 15);
    x_reversed_cb = new wxCheckBox(this, -1, wxT("reversed axis"));
    x_reversed_cb->SetValue(plot->xs.reversed);
    xsizer->Add(x_reversed_cb, 0, wxALL, 5);
    x_logarithm_cb = new wxCheckBox(this, -1, wxT("logarithmic scale"));
    x_logarithm_cb->SetValue(plot->xs.logarithm);
    xsizer->Add(x_logarithm_cb, 0, wxALL, 5);
    sizer1->Add(xsizer, 0, wxALL, 5);

    wxStaticBoxSizer *ysizer = new wxStaticBoxSizer(wxVERTICAL, this,
                                                    wxT("Y axis"));
    y_show_axis = new wxCheckBox(this, -1, wxT("show axis"));
    y_show_axis->SetValue(plot->y_axis_visible);
    ysizer->Add(y_show_axis, 0, wxALL, 5);
    y_show_tics = new wxCheckBox(this, -1, wxT("show tics"));
    y_show_tics->SetValue(plot->ytics_visible);
    ysizer->Add(y_show_tics, 0, wxALL, 5);
    wxBoxSizer* ysizer_t = new wxBoxSizer(wxVERTICAL);
    y_show_minor_tics = new wxCheckBox(this, -1, wxT("show minor tics"));
    y_show_minor_tics->SetValue(plot->yminor_tics_visible);
    ysizer_t->Add(y_show_minor_tics, 0, wxALL, 5);
    y_show_grid = new wxCheckBox(this, -1, wxT("show grid"));
    y_show_grid->SetValue(plot->y_grid);
    ysizer_t->Add(y_show_grid, 0, wxALL, 5);
    wxBoxSizer *ymt_sizer = new wxBoxSizer(wxHORIZONTAL);
    ymt_sizer->Add(new wxStaticText(this, -1, wxT("max. number of tics")),
                  0, wxALL|wxALIGN_CENTRE_VERTICAL, 5);
    y_max_tics = new wxSpinCtrl (this, -1, wxT("7"),
                                 wxDefaultPosition, wxSize(50, -1),
                                 wxSP_ARROW_KEYS, 1, 30, 7);
    y_max_tics->SetValue(plot->y_max_tics);
    ymt_sizer->Add(y_max_tics, 0, wxALL, 5);
    ysizer_t->Add(ymt_sizer);
    wxBoxSizer *yts_sizer = new wxBoxSizer(wxHORIZONTAL);
    yts_sizer->Add(new wxStaticText(this, -1, wxT("length of tics")),
                  0, wxALL|wxALIGN_CENTRE_VERTICAL, 5);
    y_tics_size = new wxSpinCtrl (this, -1, wxT("4"),
                                  wxDefaultPosition, wxSize(50, -1),
                                  wxSP_ARROW_KEYS, 1, 20, 4);
    y_tics_size->SetValue(plot->y_tic_size);
    yts_sizer->Add(y_tics_size, 0, wxALL, 5);
    ysizer_t->Add(yts_sizer);
    ysizer->Add(ysizer_t, 0, wxLEFT, 15);
    y_reversed_cb = new wxCheckBox(this, -1, wxT("reversed axis"));
    y_reversed_cb->SetValue(plot->ys.reversed);
    ysizer->Add(y_reversed_cb, 0, wxALL, 5);
    y_logarithm_cb = new wxCheckBox(this, -1, wxT("logarithmic scale"));
    y_logarithm_cb->SetValue(plot->ys.logarithm);
    ysizer->Add(y_logarithm_cb, 0, wxALL, 5);
    sizer1->Add(ysizer, 0, wxALL, 5);

    top_sizer->Add(sizer1, 0);
    wxBoxSizer *common_sizer = new wxBoxSizer(wxHORIZONTAL);
    common_sizer->Add(new wxButton(this, ID_CAD_COLOR,
                                   wxT("Change axes color...")),
                      0, wxALL, 5);
    common_sizer->Add(new wxButton(this, ID_CAD_FONT,
                                   wxT("Change tics font...")),
                      0, wxALL, 5);
    top_sizer->Add(common_sizer, 0, wxALIGN_CENTER);
    add_apply_close_buttons(this, top_sizer);
    SetSizerAndFit(top_sizer);
}

void ConfigureAxesDlg::OnApply (wxCommandEvent&)
{
    bool scale_changed = false;
    plot->xAxisCol = axis_color;
    plot->x_axis_visible = x_show_axis->GetValue();
    plot->xtics_visible = x_show_tics->GetValue();
    plot->xminor_tics_visible = x_show_minor_tics->GetValue();
    plot->x_grid = x_show_grid->GetValue();
    plot->x_max_tics = x_max_tics->GetValue();
    plot->x_tic_size = x_tics_size->GetValue();
    if (plot->xs.reversed != x_reversed_cb->GetValue()) {
        plot->xs.reversed = x_reversed_cb->GetValue();
        scale_changed = true;
    }
    if (plot->xs.logarithm != x_logarithm_cb->GetValue()) {
        plot->xs.logarithm = x_logarithm_cb->GetValue();
        scale_changed = true;
    }
    plot->y_axis_visible = y_show_axis->GetValue();
    plot->ytics_visible = y_show_tics->GetValue();
    plot->yminor_tics_visible = y_show_minor_tics->GetValue();
    plot->y_grid = y_show_grid->GetValue();
    plot->y_max_tics = y_max_tics->GetValue();
    plot->y_tic_size = y_tics_size->GetValue();
    plot->ys.reversed = y_reversed_cb->GetValue();
    plot->ys.logarithm = y_logarithm_cb->GetValue();
    ftk->view.set_log_scale(plot->xs.logarithm, plot->ys.logarithm);
    frame->refresh_plots(false, scale_changed ? kAllPlots : kMainPlot);
}

void ConfigureAxesDlg::OnChangeFont (wxCommandEvent&)
{
    plot->change_tics_font();
}

//===============================================================
//                     ConfigurePLabelsDlg
//===============================================================

BEGIN_EVENT_TABLE(ConfigurePLabelsDlg, wxDialog)
    EVT_BUTTON(wxID_APPLY, ConfigurePLabelsDlg::OnApply)
    EVT_BUTTON(wxID_CLOSE, ConfigurePLabelsDlg::OnClose)
    EVT_BUTTON(ID_CPL_FONT, ConfigurePLabelsDlg::OnChangeLabelFont)
    EVT_CHECKBOX(ID_CPL_SHOW, ConfigurePLabelsDlg::OnCheckShowLabel)
    EVT_TEXT(ID_CPL_TEXT, ConfigurePLabelsDlg::OnChangeLabelText)
    EVT_RADIOBOX(ID_CPL_RADIO, ConfigurePLabelsDlg::OnRadioLabel)
END_EVENT_TABLE()

ConfigurePLabelsDlg::ConfigurePLabelsDlg(wxWindow* parent, wxWindowID id,
                                         MainPlot* plot_)
    //explicit conversion of title to wxString() is neccessary
  : wxDialog(parent, id, wxString(wxT("Configure Peak Labels"))), plot(plot_),
    in_onradiolabel(false)
{
    wxBoxSizer *top_sizer = new wxBoxSizer(wxVERTICAL);
    show_plabels = new wxCheckBox(this, ID_CPL_SHOW, wxT("show peak labels"));
    top_sizer->Add(show_plabels, 0, wxALL, 5);
    wxBoxSizer *sizer1 = new wxBoxSizer(wxHORIZONTAL);
    vector<string> label_radio_choice;
    label_radio_choice.push_back("area");
    label_radio_choice.push_back("height");
    label_radio_choice.push_back("center");
    label_radio_choice.push_back("fwhm");
    label_radio_choice.push_back("name");
    label_radio_choice.push_back("custom");
    label_radio = new wxRadioBox(this, ID_CPL_RADIO, wxT("labels with:"),
                                 wxDefaultPosition, wxDefaultSize,
                                 stl2wxArrayString(label_radio_choice),
                                 1, wxRA_SPECIFY_COLS);
    sizer1->Add(label_radio, 0, wxALL|wxEXPAND, 5);
    wxStaticBoxSizer *xsizer = new wxStaticBoxSizer(wxVERTICAL, this,
                                            wxT("list of replaceable tokens"));
    xsizer->Add(new wxStaticText(this, -1, wxT("<area>   area of peak\n")
                                           wxT("<height> height of peak\n")
                                           wxT("<center> center of peak\n")
                                           wxT("<fwhm>   FWHM of peak\n")
                                       wxT("<ib>    integral breadth of peak\n")
                                           wxT("<name>   name of function\n")
                                           wxT("<br>     line break\n")),
                0, wxALL|wxEXPAND, 5);
    sizer1->Add(xsizer, 0, wxALL, 5);
    top_sizer->Add(sizer1, 0);
    label_text = new wxTextCtrl(this, ID_CPL_TEXT, wxT(""));
    top_sizer->Add(label_text, 0, wxALL|wxEXPAND, 5);

    vertical_rb = new wxRadioBox(this, ID_CPL_RADIO, wxT("label direction"),
                                 wxDefaultPosition, wxDefaultSize,
                                 stl2wxArrayString(vector2(string("horizontal"),
                                                           string("vertical"))),
                                 2);
    top_sizer->Add(vertical_rb, 0, wxALL|wxEXPAND, 5);

    top_sizer->Add(new wxButton(this, ID_CPL_FONT, wxT("Change label font...")),
                   0, wxALL|wxALIGN_CENTER, 5);
    top_sizer->Add(new wxStaticText(this, -1,
                                   wxT("Labels have the same colors as peaks")),
                   0, wxALL, 5);

    add_apply_close_buttons(this, top_sizer);
    SetSizerAndFit(top_sizer);

    //set initial values
    show_plabels->SetValue(plot->plabels_visible);
    label_text->SetValue(s2wx(plot->plabel_format));
    vertical_rb->SetSelection(plot->vertical_plabels ? 1 : 0);
    label_radio->SetStringSelection(wxT("custom"));
    label_text->Enable(plot->plabels_visible);
    label_radio->Enable(plot->plabels_visible);
    vertical_rb->Enable(plot->plabels_visible);
}

void ConfigurePLabelsDlg::OnChangeLabelText (wxCommandEvent&)
{
    // don't change radio if this event is triggered by label_text->SetValue()
    // from radiobox event handler
    if (!in_onradiolabel)
        label_radio->SetStringSelection(wxT("custom"));
}

void ConfigurePLabelsDlg::OnCheckShowLabel (wxCommandEvent& event)
{
    bool checked = event.IsChecked();
    label_radio->Enable(checked);
    label_text->Enable(checked);
    vertical_rb->Enable(checked);
}

void ConfigurePLabelsDlg::OnRadioLabel (wxCommandEvent&)
{
    in_onradiolabel = true;
    wxString s = label_radio->GetStringSelection();
    if (s != wxT("custom"))
        label_text->SetValue(wxT("<") + s + wxT(">"));
    in_onradiolabel = false;
}

void ConfigurePLabelsDlg::OnApply (wxCommandEvent&)
{
    plot->plabels_visible = show_plabels->GetValue();
    plot->plabel_format = wx2s(label_text->GetValue());
    plot->vertical_plabels = vertical_rb->GetSelection() != 0;
    plot->refresh();
}

void ConfigurePLabelsDlg::OnChangeLabelFont (wxCommandEvent&)
{
    wxFontData data;
    data.SetInitialFont(plot->plabelFont);
    wxFontDialog dialog(NULL, data);
    if (dialog.ShowModal() == wxID_OK)
    {
        wxFontData retData = dialog.GetFontData();
        plot->plabelFont = retData.GetChosenFont();
        plot->refresh();
    }
}



//===============================================================
//           BgManager (for interactive background setting)
//===============================================================

/*
// outdated
void auto_background (int n, fp p1, bool is_perc1, fp p2, bool is_perc2)
{
    //FIXME: Do you know any good algorithm, that can extract background
    //       from data?
    if (n <= 0 || n >= size(p) / 2)
        return;
    int ps = p.size();
    for (int i = 0; i < n; i++) {
        int l = ps * i / n;
        int u = ps * (i + 1) / n;
        vector<fp> v (u - l);
        for (int k = l; k < u; k++)
            v[k - l] = p[k].orig_y;
        sort (v.begin(), v.end());
        int y_avg_beg = 0, y_avg_end = v.size();
        if (is_perc1) {
            p1 = min (max (p1, 0.), 100.);
            y_avg_beg = static_cast<int>(v.size() * p1 / 100);
        }
        else {
            y_avg_beg = upper_bound (v.begin(), v.end(), v[0] + p1) - v.begin();
            if (y_avg_beg == size(v))
                y_avg_beg--;
        }
        if (is_perc2) {
            p2 = min (max (p2, 0.), 100.);
            y_avg_end = y_avg_beg + static_cast<int>(v.size() * p2 / 100);
        }
        else {
            fp end_val = v[y_avg_beg] + p2;
            y_avg_end = upper_bound (v.begin(), v.end(), end_val) - v.begin();
        }
        if (y_avg_beg < 0)
            y_avg_beg = 0;
        if (y_avg_end > size(v))
            y_avg_end = v.size();
        if (y_avg_beg >= y_avg_end) {
            if (y_avg_beg >= size(v))
                y_avg_beg = v.size() - 1;
            y_avg_end = y_avg_beg + 1;
        }
        int counter = 0;
        fp y = 0;
        for (int j = y_avg_beg; j < y_avg_end; j++){
            counter++;
            y += v[j];
        }
        y /= counter;
        add_background_point ((p[l].x + p[u - 1].x) / 2, y, bgc_bg);
    }
}
*/


void BgManager::add_background_point(fp x, fp y)
{
    if (!bg_backup.empty()) {
        int r = wxMessageBox(wxT("The old background will be removed\n")
                             wxT("and the new one will be created.\n")
                             wxT("It will make impossible to undo\n")
                             wxT("removing the old background.\n\n")
                             wxT("Continue?"),
                             wxT("Do you want to start a new background?"),
                             wxICON_QUESTION|wxYES_NO);
        if (r == wxYES) {
            bg_backup.clear();
            frame->update_toolbar();
        }
        else
            return;
    }
    rm_background_point(x);
    PointQ t(x, y);
    bg_iterator l = lower_bound(bg.begin(), bg.end(), t);
    bg.insert (l, t);
}

void BgManager::rm_background_point (fp x)
{
    int X = x_scale.px(x);
    fp lower = x_scale.val(X - min_dist);
    fp upper = x_scale.val(X + min_dist);
    if (lower > upper)
        swap(lower, upper);
    bg_iterator l = lower_bound(bg.begin(), bg.end(), PointQ(lower, 0));
    bg_iterator u = upper_bound (bg.begin(), bg.end(), PointQ(upper, 0));
    if (u > l) {
        bg.erase(l, u);
        ftk->vmsg (S(u - l) + " background points removed.");
    }
}

void BgManager::clear_background()
{
    int n = bg.size();
    bg.clear();
    if (n != 0)
        ftk->vmsg (S(n) + " background points deleted.");
}

void BgManager::strip_background()
{
    if (bg.empty())
        return;
    vector<fp> pars;
    for (bg_const_iterator i = bg.begin(); i != bg.end(); i++) {
        pars.push_back(i->x);
        pars.push_back(i->y);
    }
    bg_backup = bg;
    cmd_tail = (spline_bg ? "spline[" : "interpolate[")
               + join_vector(pars, ", ") + "](x)"
               + frame->get_in_datasets();
    clear_background();
    ftk->exec("Y = y - " + cmd_tail);
    ftk->vmsg("Background stripped.");
}

void BgManager::undo_strip_background()
{
    if (bg_backup.empty())
        return;
    bg = bg_backup;
    bg_backup.clear();
    ftk->exec("Y = y + " + cmd_tail);
}

vector<int> BgManager::calculate_bgline(int window_width, Scale const& y_scale)
{
    vector<int> bgline(window_width);
    if (spline_bg)
        prepare_spline_interpolation(bg);
    for (int i = 0; i < window_width; i++) {
        fp x = x_scale.val(i);
        fp y = spline_bg ? get_spline_interpolation(bg, x)
                         : get_linear_interpolation(bg, x);
        bgline[i] = y_scale.px(y);
    }
    return bgline;
}

void BgManager::set_as_convex_hull()
{
    SimplePolylineConvex convex;
    vector<int> sel = frame->get_selected_data_indices();
    if (sel.size() != 1)
        return;
    const Data* data = ftk->get_data(sel[0]);
    for (int i = 0; i < data->get_n(); ++i)
        convex.push_point(PointQ(data->get_x(i), data->get_y(i)));

    bg = convex.get_vertices();
    bg_backup.clear();
}



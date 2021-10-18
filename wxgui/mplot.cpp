// This file is part of fityk program. Copyright 2001-2013 Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#include <wx/wx.h>

#include <wx/clrpicker.h>
#include <wx/fontpicker.h>
#include <wx/dcgraph.h>
#include <algorithm>
#include <boost/scoped_ptr.hpp>

#include "mplot.h"
#include "plotpane.h"
#include "frame.h"
#include "sidebar.h"
#include "statbar.h" // HintReceiver
#include "bgm.h"
#include "gradient.h"
#include "drag.h"
#include "fityk/data.h"
#include "fityk/logic.h"
#include "fityk/model.h"
#include "fityk/var.h"
#include "fityk/func.h"
#include "fityk/settings.h"
#include "fityk/ast.h"
#include "fityk/info.h"

using namespace std;
using fityk::is_zero;
using fityk::Variable;
using fityk::Function;
using fityk::Tplate;
using fityk::Model;

enum {
    ID_plot_popup_za         = 25001,
    ID_plot_popup_prefs             ,

    ID_peak_popup_info              ,
    ID_peak_popup_del               ,
    ID_peak_popup_guess             ,
    ID_peak_popup_edit              ,
    ID_peak_popup_share //+15
};


class MainPlotConfDlg: public wxDialog
{
public:
    MainPlotConfDlg(MainPlot* mp);

private:
    MainPlot *mp_;
    wxCheckBox *model_cb_, *func_cb_, *labels_cb_, *vertical_labels_cb_,
               *desc_cb_;
    wxComboBox *label_combo_, *desc_combo_;
    wxColourPickerCtrl *bg_cp_, *inactive_cp_, *axis_cp_, *model_cp_, *func_cp_;
    wxSpinCtrl *data_colors_sc_, *model_width_sc_;
    MultiColorCombo *data_color_combo_;
    wxFontPickerCtrl *tics_fp_, *label_fp_;
    wxCheckBox *x_show_axis_cb_, *x_show_tics_cb_, *x_show_minor_tics_cb_,
               *x_show_grid_cb_, *x_reversed_cb_, *x_logarithm_cb_;
    wxCheckBox *y_show_axis_cb_, *y_show_tics_cb_, *y_show_minor_tics_cb_,
               *y_show_grid_cb_, *y_reversed_cb_, *y_logarithm_cb_;
    wxSpinCtrl *x_max_tics_sc_, *x_tic_size_sc_;
    wxSpinCtrl *y_max_tics_sc_, *y_tic_size_sc_;

    void OnModelCheckbox(wxCommandEvent& event)
        { mp_->model_visible_ = event.IsChecked(); mp_->refresh(); }

    void OnFuncCheckbox(wxCommandEvent& event)
        { mp_->peaks_visible_ = event.IsChecked(); mp_->refresh(); }

    void OnLabelsCheckbox(wxCommandEvent& event)
        { mp_->plabels_visible_ = event.IsChecked(); mp_->refresh(); }

    void OnDescCheckbox(wxCommandEvent& event)
        { mp_->desc_visible_ = event.IsChecked(); mp_->refresh(); }

    void OnModelWidthSpin(wxSpinEvent& event)
        { mp_->model_line_width_ = event.GetPosition(); mp_->refresh(); }

    void OnVerticalCheckbox(wxCommandEvent& event)
    {
        mp_->vertical_plabels_ = event.IsChecked();
        if (mp_->plabels_visible_)
            mp_->refresh();
    }

    void OnColor(wxColourPickerEvent& event);

    void OnDataColorsSpin(wxSpinEvent& event)
    {
        mp_->data_colors_.resize(event.GetPosition(), mp_->data_colors_[0]);
        data_color_combo_->Refresh();
        frame->update_data_pane();
        mp_->refresh();
    }

    void OnDataColorSelection(wxCommandEvent&)
    {
        frame->update_data_pane();
        mp_->refresh();
    }

    void OnTicsFont(wxFontPickerEvent& event)
        { mp_->ticsFont = event.GetFont(); mp_->refresh(); }

    void OnLabelFont(wxFontPickerEvent& event)
        { mp_->plabelFont = event.GetFont(); mp_->refresh(); }

    void OnLabelTextChanged(wxCommandEvent&)
    {
        mp_->plabel_format_ = wx2s(label_combo_->GetValue());
        if (mp_->plabels_visible_)
            mp_->refresh();
    }

    void OnDescTextChanged(wxCommandEvent&)
    {
        mp_->desc_format_ = wx2s(desc_combo_->GetValue());
        if (mp_->desc_visible_)
            mp_->refresh();
    }


    void OnShowXAxis(wxCommandEvent& event)
        { mp_->x_axis_visible = event.IsChecked(); mp_->refresh(); }
    void OnShowYAxis(wxCommandEvent& event)
        { mp_->y_axis_visible = event.IsChecked(); mp_->refresh(); }

    void OnShowXTics(wxCommandEvent& event)
        { mp_->xtics_visible = event.IsChecked(); mp_->refresh(); }
    void OnShowYTics(wxCommandEvent& event)
        { mp_->ytics_visible = event.IsChecked(); mp_->refresh(); }

    void OnShowXMinorTics(wxCommandEvent& event)
        { mp_->xminor_tics_visible = event.IsChecked(); mp_->refresh(); }
    void OnShowYMinorTics(wxCommandEvent& event)
        { mp_->yminor_tics_visible = event.IsChecked(); mp_->refresh(); }

    void OnShowXGrid(wxCommandEvent& event)
        { mp_->x_grid = event.IsChecked(); mp_->refresh(); }
    void OnShowYGrid(wxCommandEvent& event)
        { mp_->y_grid = event.IsChecked(); mp_->refresh(); }

    void OnReversedX(wxCommandEvent& event)
    {
        mp_->xs.reversed = event.IsChecked();
        frame->plot_pane()->refresh_plots(false, kAllPlots);
    }
    void OnReversedY(wxCommandEvent& event)
        { mp_->ys.reversed = event.IsChecked(); mp_->refresh(); }

    void OnLogX(wxCommandEvent& event)
    {
        mp_->xs.logarithm = event.IsChecked();
        ftk->view.set_log_scale(mp_->xs.logarithm, mp_->ys.logarithm);
        frame->plot_pane()->refresh_plots(false, kAllPlots);
    }
    void OnLogY(wxCommandEvent& event)
    {
        mp_->ys.logarithm = event.IsChecked();
        ftk->view.set_log_scale(mp_->xs.logarithm, mp_->ys.logarithm);
        mp_->refresh();
    }

    void OnMaxXTicsSpin(wxSpinEvent& event)
        { mp_->x_max_tics = event.GetPosition(); mp_->refresh(); }
    void OnMaxYTicsSpin(wxSpinEvent& event)
        { mp_->y_max_tics = event.GetPosition(); mp_->refresh(); }

    void OnXTicSize(wxSpinEvent& event)
        { mp_->x_tic_size = event.GetPosition(); mp_->refresh(); }
    void OnYTicSize(wxSpinEvent& event)
        { mp_->y_tic_size = event.GetPosition(); mp_->refresh(); }
};

// horizontal pixel range (from - to) is used for X values
static
void stroke_line(wxDC& dc, const vector<double>& YY, int from=0, int to=-1)
{
    if (to == -1)
        to = YY.size() - 1;
    wxGCDC* gdc = wxDynamicCast(&dc, wxGCDC);
    if (gdc) {
        wxGraphicsContext *gc = gdc->GetGraphicsContext();
        wxGraphicsPath path = gc->CreatePath();
        path.MoveToPoint(from, YY[from]);
        for (int i = from+1; i <= to; ++i)
            path.AddLineToPoint(i, YY[i]);
        gc->StrokePath(path);
    } else {
        int n = to - from + 1;
        wxPoint *points = new wxPoint[n];
        for (int i = 0; i < n; ++i) {
            points[i].x = from + i;
            points[i].y = iround(YY[from + i]);
        }
        dc.DrawLines(n, points);
        delete [] points;
    }
}

static
void stroke_line(wxDC& dc, const vector<double>& XX, const vector<double>& YY)
{
    assert(XX.size() == YY.size());
    int n = XX.size();
    if (n == 0)
        return;
    wxGCDC* gdc = wxDynamicCast(&dc, wxGCDC);
    if (gdc) {
        wxGraphicsContext *gc = gdc->GetGraphicsContext();
        wxGraphicsPath path = gc->CreatePath();
        path.MoveToPoint(XX[0], YY[0]);
        for (int i = 1; i < n; ++i)
            path.AddLineToPoint(XX[i], YY[i]);
        gc->StrokePath(path);
    } else {
        wxPoint *points = new wxPoint[n];
        for (int i = 0; i < n; ++i) {
            points[i].x = iround(XX[i]);
            points[i].y = iround(YY[i]);
        }
        dc.DrawLines(n, points);
        delete [] points;
    }
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
    EVT_MOUSE_AUX1_DOWN ( MainPlot::OnAuxDown)
    EVT_MOUSE_AUX2_DOWN ( MainPlot::OnAuxDown)
    EVT_MOUSEWHEEL (      MainPlot::OnMouseWheel)
    EVT_MENU (ID_plot_popup_za,     MainPlot::OnZoomAll)
    EVT_MENU (ID_plot_popup_prefs,  MainPlot::OnConfigure)
    EVT_MENU (ID_peak_popup_info,   MainPlot::OnPeakInfo)
    EVT_MENU (ID_peak_popup_del,    MainPlot::OnPeakDelete)
    EVT_MENU (ID_peak_popup_guess,  MainPlot::OnPeakGuess)
    EVT_MENU (ID_peak_popup_edit,   MainPlot::OnPeakEdit)
    EVT_MENU_RANGE (ID_peak_popup_share, ID_peak_popup_share+15,
                    MainPlot::OnPeakShare)
END_EVENT_TABLE()

MainPlot::MainPlot (wxWindow *parent)
    : FPlot(parent),
      bgm_(new BgManager(xs)),
      dragged_func_(new DraggedFunc(ftk->mgr)),
      basic_mode_(mmd_zoom), mode_(mmd_zoom),
      crosshair_cursor_(false),
      pressed_mouse_button_(0),
      over_peak_(-1),
      hint_receiver_(NULL),
      auto_freeze_(false)
{
    set_cursor();
    SetMinSize(wxSize(200, 200));
}

MainPlot::~MainPlot()
{
    delete bgm_;
    delete dragged_func_;
}

void MainPlot::OnPaint(wxPaintEvent&)
{
    update_buffer_and_blit();
}

static
double y_of_data_for_draw_data(vector<Point>::const_iterator i,
                           const Model* /*model*/)
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
        const wxColour& bg_col = get_bg_color();
        col.Set((col.Red() + bg_col.Red())/2,
                (col.Green() + bg_col.Green())/2,
                (col.Blue() + bg_col.Blue())/2);
    }
    if (set_pen)
        draw_data(dc, y_of_data_for_draw_data, ftk->dk.data(n), 0,
                  col, wxNullColour, offset);
    else
        draw_data(dc, y_of_data_for_draw_data, ftk->dk.data(n), 0,
                  dc.GetPen().GetColour(), dc.GetPen().GetColour(), offset);
}

void MainPlot::draw(wxDC &dc, bool monochrome)
{
    //printf("MainPlot::draw()\n");
    int focused_data = frame->get_focused_data_index();
    const Model* model = ftk->dk.get_model(focused_data);

    set_scale(get_pixel_width(dc), get_pixel_height(dc));

    int Ymax = get_pixel_height(dc);
    prepare_peaktops(model, Ymax);

    if (monochrome) {
        dc.SetPen(*wxBLACK_PEN);
        dc.SetBrush(*wxBLACK_BRUSH);
    }

    if (desc_visible_)
        draw_desc(dc, focused_data, !monochrome);

    //draw datasets (selected and focused at the end)
    vector<int> ord = frame->get_sidebar()->get_ordered_dataset_numbers();
    v_foreach (int, i, ord)
        draw_dataset(dc, *i, !monochrome);

    if (xtics_visible)
        draw_xtics(dc, ftk->view, !monochrome);
    if (ytics_visible)
        draw_ytics(dc, ftk->view, !monochrome);

    if (peaks_visible_)
        draw_peaks(dc, model, !monochrome);
    //if (groups_visible_)
    //    draw_groups(dc, model, !monochrome);
    if (model_visible_)
        draw_model(dc, model, !monochrome);
    if (x_axis_visible)
        draw_x_axis(dc, !monochrome);
    if (y_axis_visible)
        draw_y_axis(dc, !monochrome);

    if (visible_peaktops(mode_) && !monochrome)
        draw_peaktops(dc, model);
    if (mode_ == mmd_bg) {
        bgm_->update_focused_data(focused_data);
        draw_baseline(dc);
    } else {
        if (plabels_visible_)
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

// one point for every pixel and extra points for function centers
// The latter is to avoid plots like here:
// https://groups.google.com/d/msg/fityk-users/9tHeKQ37rbg/4H6VUD9iTu8J
static
vector<realt> get_x_points_for_model_line(const Model *model, const Scale& xs,
                                          int width)
{
    vector<realt> centers;
    v_foreach(int, k, model->get_ff().idx) {
        realt ctr;
        if (ftk->mgr.get_function(*k)->get_center(&ctr)) {
            realt X = xs.px_d(ctr);
            if (X > 0 && X < width-1)
                centers.push_back(ctr);
        }
    }
    sort(centers.begin(), centers.end());

    vector<realt> xx(width + centers.size());
    if (xs.scale >= 0) {
        reverse(centers.begin(), centers.end());
        for (int i = 0, pos = 0; i < width; ++i) {
            realt x = xs.val(i);
            while (!centers.empty() && centers.back() < x) {
                xx[pos++] = centers.back();
                centers.pop_back();
            }
            xx[pos++] = x;
        }
    } else {
        for (int i = 0, pos = 0; i < width; ++i) {
            realt x = xs.val(i);
            while (!centers.empty() && centers.back() > x) {
                xx[pos++] = centers.back();
                centers.pop_back();
            }
            xx[pos++] = x;
        }
    }

    return xx; // counting on RVO
}

void MainPlot::draw_model(wxDC& dc, const Model* model, bool set_pen)
{
    if (set_pen)
        dc.SetPen(wxPen(modelCol, model_line_width_ * pen_width));
    int width = get_pixel_width(dc);
    vector<realt> xx = get_x_points_for_model_line(model, xs, width);

    vector<realt> yy(xx.size());
    vector<double> YY(xx.size());
    model->compute_model(xx, yy);
    for (size_t i = 0; i != yy.size(); ++i)
        YY[i] = ys.px_d(yy[i]);
    vm_foreach(realt, x, xx) // in-place conversion to screen coords
        *x = xs.px_d(*x);
    stroke_line(dc, xx, YY);
}


//TODO draw groups
//void MainPlot::draw_groups (wxDC& /*dc*/, const Model*, bool)
//{
//}

void MainPlot::draw_peaks(wxDC& dc, const Model* model, bool set_pen)
{
    double level = 0;
    const vector<int>& idx = model->get_ff().idx;
    int n = get_pixel_width(dc);
    vector<realt> xx(n), yy(n);
    vector<double> YY(n);
    for (int i = 0; i < n; ++i) {
        xx[i] = xs.val(i);
        xx[i] += model->zero_shift(xx[i]);
    }
    for (int k = 0; k < (int) idx.size(); k++) {
        fill(yy.begin(), yy.end(), 0.);
        const Function* f = ftk->mgr.get_function(idx[k]);
        int from=0, to=n-1;
        realt left, right;
        if (f->get_nonzero_range(level, left, right)) {
            from = max(from, xs.px(left));
            to = min(to, xs.px(right));
        }
        if (set_pen)
            dc.SetPen(wxPen(peakCol[k % max_peak_cols], pen_width));
        f->calculate_value(xx, yy);
        for (int i = from; i <= to; ++i)
            YY[i] = ys.px_d(yy[i]);
        stroke_line(dc, YY, from, to);
    }
}

void MainPlot::draw_peaktops (wxDC& dc, const Model* model)
{
    dc.SetPen(wxPen(xAxisCol, pen_width));
    dc.SetBrush (*wxTRANSPARENT_BRUSH);
    v_foreach (wxPoint, i, special_points) {
        dc.DrawRectangle (i->x - 1, i->y - 1, 3, 3);
    }
    draw_peaktop_selection(dc, model);
}

void MainPlot::draw_peaktop_selection (wxDC& dc, const Model* model)
{
    // using the same pen
    int n = frame->get_sidebar()->get_active_function();
    if (n == -1)
        return;
    const vector<int>& idx = model->get_ff().idx;
    int t = index_of_element(idx, n);
    if (t != -1)
        dc.DrawCircle(special_points[t].x, special_points[t].y, 4);
}

void MainPlot::draw_plabels (wxDC& dc, const Model* model, bool set_pen)
{
    prepare_peak_labels(model); //TODO re-prepare only when peaks where changed
    set_font(dc, plabelFont);
    vector<wxRect> previous;
    const vector<int>& idx = model->get_ff().idx;
    for (int k = 0; k < (int) idx.size(); k++) {
        const wxPoint &peaktop = special_points[k];
        if (set_pen)
            dc.SetTextForeground(peakCol[k % max_peak_cols]);

        wxString label = s2wx(plabels_[k]);
        wxCoord w, h;
        if (vertical_plabels_) {
            dc.GetMultiLineTextExtent(label, &h, &w); // w and h swapped
            h = 0; // Y correction is not needed
        } else
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
        if (vertical_plabels_)
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

void MainPlot::prepare_peaktops(const Model* model, int Ymax)
{
    int Y0 = ys.px(0);
    const vector<int>& idx = model->get_ff().idx;
    int n = idx.size();
    special_points.resize(n);
    int no_ctr_idx = 0;
    for (int k = 0; k < n; k++) {
        const Function *f = ftk->mgr.get_function(idx[k]);
        double x;
        realt ctr;
        int X, Y;
        if (f->get_center(&ctr)) {
            X = xs.px(ctr - model->zero_shift(ctr));
            // instead of these two lines we could simply do x=ctr,
            // but it would be slightly inaccurate
            x = xs.val(X);
            x += model->zero_shift(x);
        } else {
            X = no_ctr_idx * 10 + 5;
            ++no_ctr_idx;
            x = xs.val(X);
            x += model->zero_shift(x);
        }
        Y = ys.px(f->calculate_value(x));
        if (Y < 0 || Y > Ymax)
            Y = Y0;
        special_points[k] = wxPoint(X, Y);
    }
}

void MainPlot::prepare_peak_labels(const Model* model)
{
    const vector<int>& idx = model->get_ff().idx;
    plabels_.resize(idx.size());
    for (int k = 0; k < (int) idx.size(); k++) {
        const Function *f = ftk->mgr.get_function(idx[k]);
        string label = plabel_format_;
        string::size_type pos = 0;
        while ((pos = label.find("<", pos)) != string::npos) {
            string::size_type right = label.find(">", pos+1);
            if (right == string::npos)
                break;
            string tag(label, pos+1, right-pos-1);
            realt a;
            if (tag == "area")
                label.replace(pos, right-pos+1, f->get_area(&a) ? S(a) : " ");
            else if (tag == "height")
                label.replace(pos, right-pos+1, f->get_height(&a) ? S(a) : " ");
            else if (tag == "center")
                label.replace(pos, right-pos+1, f->get_center(&a) ? S(a) : " ");
            else if (tag == "fwhm")
                label.replace(pos, right-pos+1, f->get_fwhm(&a) ? S(a) : " ");
            else if (tag == "ib")
                label.replace(pos, right-pos+1, f->get_ibreadth(&a) ? S(a):" ");
            else if (tag == "name")
                label.replace(pos, right-pos+1, f->name);
            else if (tag == "br")
                label.replace(pos, right-pos+1, "\n");
            else
                ++pos;
        }
        plabels_[k] = label;
    }
}

void MainPlot::draw_desc(wxDC& dc, int dataset, bool set_pen)
{
    string result;
    try {
        parse_and_eval_info(ftk, desc_format_, dataset, result);
    }
    catch (const fityk::SyntaxError& /*e*/) {
        result = "syntax error!";
    }
    catch (const fityk::ExecuteError& /*e*/) {
        result = "(---)";
    }
    wxString label = s2wx(result);
    set_font(dc, plabelFont);
    if (set_pen)
        dc.SetTextForeground(xAxisCol);
    wxCoord w, h;
    dc.GetMultiLineTextExtent(label, &w, &h);
    wxSize size = dc.GetSize();
    wxRect rect(size.x - w - 5, 5, w, h);
    dc.DrawLabel(label, rect, wxALIGN_RIGHT);
}

void MainPlot::draw_baseline(wxDC& dc, bool set_pen)
{
    if (set_pen)
        dc.SetPen(wxPen(bg_pointsCol, pen_width));
    dc.SetBrush (*wxTRANSPARENT_BRUSH);

    // bg line
    int width = get_pixel_width(dc);
    vector<double> YY = bgm_->calculate_bgline(width, ys);
    stroke_line(dc, YY);

    // bg points (circles)
    v_foreach (fityk::PointQ, i, bgm_->get_bg()) {
        dc.DrawCircle(xs.px(i->x), ys.px(i->y), 3);
        dc.DrawCircle(xs.px(i->x), ys.px(i->y), 4);
    }
}

void MainPlot::read_settings(wxConfigBase *cf)
{
    cf->SetPath(wxT("/MainPlot"));
    int data_colors_count = cf->Read(wxT("data_colors_count"), 16);
    if (data_colors_count < 2)
        data_colors_count = 2;
    else if (data_colors_count > kMaxDataColors)
        data_colors_count = kMaxDataColors;
    data_colors_.resize(data_colors_count);
    cf->SetPath(wxT("/MainPlot/Colors"));
    set_bg_color(cfg_read_color(cf, wxT("bg"), wxColour(48, 48, 48)));
    inactiveDataCol = cfg_read_color(cf, wxT("inactive_data"),
                                                      wxColour(128, 128, 128));
    modelCol = cfg_read_color(cf, wxT("model"), wxColour(255, 255, 0));
    bg_pointsCol = cfg_read_color(cf, wxT("BgPoints"), wxColour(255, 0, 0));
    cf->SetPath(wxT("data"));
    data_colors_[0] = cfg_read_color(cf, wxT("0"), wxColour(0, 255, 0));
    for (int i = 1; i < (int) data_colors_.size(); i++)
        data_colors_[i] = cfg_read_color(cf, s2wx(S(i)), data_colors_[0]);
    cf->SetPath(wxT("../peak"));
    peakCol[0] = cfg_read_color(cf, wxT("0"), wxColour(255, 0, 0));
    for (int i = 0; i < max_peak_cols; i++)
        peakCol[i] = cfg_read_color(cf, s2wx(S(i)), peakCol[0]);
    //for (int i = 0; i < max_group_cols; i++)
    //    groupCol[i] = cfg_read_color(cf, wxString::Format(wxT("group/%i"), i),
    //                                 wxColour(173, 216, 230));

    cf->SetPath(wxT("/MainPlot/Visible"));
    peaks_visible_ = cfg_read_bool(cf, wxT("peaks"), true);
    plabels_visible_ = cfg_read_bool(cf, wxT("plabels"), false);
    desc_visible_ = cfg_read_bool(cf, wxT("desc"), false);
    //groups_visible_ = cfg_read_bool(cf, wxT("groups"), false);
    model_visible_ = cfg_read_bool(cf, wxT("model"), true);
    cf->SetPath(wxT("/MainPlot"));
    point_radius = cf->Read (wxT("point_radius"), 2);
    line_between_points = cfg_read_bool(cf,wxT("line_between_points"), false);
    draw_sigma = cfg_read_bool(cf,wxT("draw_sigma"), false);
    model_line_width_ = cf->Read(wxT("model_line_width"), 1);
    wxFont default_plabel_font(10, wxFONTFAMILY_DEFAULT,
                               wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    plabelFont = cfg_read_font(cf, wxT("plabelFont"), default_plabel_font);
    plabel_format_ = wx2s(cf->Read(wxT("plabel_format"), wxT("<area>")));
    desc_format_ = wx2s(cf->Read(wxT("desc_format"), wxT("filename")));
    vertical_plabels_ = cfg_read_bool(cf, wxT("vertical_plabels"), false);
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
    cf->Write(wxT("model_line_width"), model_line_width_);
    cfg_write_font (cf, wxT("plabelFont"), plabelFont);
    cf->Write(wxT("plabel_format"), s2wx(plabel_format_));
    cf->Write(wxT("desc_format"), s2wx(desc_format_));
    cf->Write (wxT("vertical_plabels"), vertical_plabels_);
    cf->Write(wxT("xMaxTics"), x_max_tics);
    cf->Write(wxT("yMaxTics"), y_max_tics);
    cf->Write(wxT("xTicSize"), x_tic_size);
    cf->Write(wxT("yTicSize"), y_tic_size);
    cf->Write(wxT("xReversed"), xs.reversed);
    cf->Write(wxT("yReversed"), ys.reversed);
    cf->Write(wxT("xLogarithm"), xs.logarithm);
    cf->Write(wxT("yLogarithm"), ys.logarithm);

    cf->Write(wxT("data_colors_count"), (long) data_colors_.size());
    cf->SetPath(wxT("/MainPlot/Colors"));
    cfg_write_color(cf, wxT("bg"), get_bg_color());
    cfg_write_color (cf, wxT("inactive_data"), inactiveDataCol);
    cfg_write_color (cf, wxT("model"), modelCol);
    cfg_write_color (cf, wxT("BgPoints"), bg_pointsCol);
    cf->SetPath(wxT("data"));
    cfg_write_color(cf, wxT("0"), data_colors_[0]);
    for (size_t i = 1; i < data_colors_.size(); i++)
        if (data_colors_[i] != data_colors_[0])
            cfg_write_color(cf, s2wx(S(i)), data_colors_[i]);
    cf->SetPath(wxT("../peak"));
    cfg_write_color(cf, wxT("0"), peakCol[0]);
    for (int i = 1; i < max_peak_cols; i++)
        if (peakCol[i] != peakCol[0])
            cfg_write_color(cf, s2wx(S(i)), peakCol[i]);
    //cf->SetPath(wxT("../group"));
    //for (int i = 0; i < max_group_cols; i++)
    //    cfg_write_color(cf, s2wx(S(i)), groupCol[i]);

    cf->SetPath(wxT("/MainPlot/Visible"));
    cf->Write (wxT("peaks"), peaks_visible_);
    cf->Write (wxT("plabels"), plabels_visible_);
    cf->Write (wxT("desc"), desc_visible_);
    //cf->Write (wxT("groups"), groups_visible_);
    cf->Write (wxT("model"), model_visible_);
    cf->SetPath(wxT("/MainPlot"));
    FPlot::save_settings(cf);
}

void MainPlot::OnLeaveWindow (wxMouseEvent&)
{
    frame->clear_status_coords();
    overlay.change_pos(-1, -1);
    frame->plot_pane()->draw_vertical_lines(-1, -1, this);
}

void MainPlot::show_popup_menu (wxMouseEvent &event)
{
    wxMenu popup_menu;
    popup_menu.Append(ID_plot_popup_za, wxT("Zoom &All"));
    popup_menu.AppendSeparator();
    popup_menu.Append(ID_plot_popup_prefs, wxT("Configure..."));

    PopupMenu(&popup_menu, event.GetX(), event.GetY());
}

void MainPlot::show_peak_menu (wxMouseEvent &event)
{
    if (over_peak_ == -1) return;
    wxMenu peak_menu;
    peak_menu.Append(ID_peak_popup_info, wxT("Show &Info"));
    peak_menu.Append(ID_peak_popup_del, wxT("&Delete"));
    peak_menu.Append(ID_peak_popup_guess, wxT("&Guess parameters"));
    peak_menu.Append(ID_peak_popup_edit, wxT("&Edit function"));
    realt dummy;
    const Function* p = ftk->mgr.get_function(over_peak_);
    peak_menu.Enable(ID_peak_popup_guess, p->get_center(&dummy));
    int active = frame->get_sidebar()->get_active_function();
    if (active >= 0 && active != over_peak_) {
        const Function* a = ftk->mgr.get_function(active);
        wxMenu *share_menu = new wxMenu;
        for (int i = 0; i != max(a->nv(), 16); ++i) {
            const string param = a->get_param(i);
            if (contains_element(p->tp()->fargs, param)) {
                share_menu->AppendCheckItem(ID_peak_popup_share+i,
                                            "&" + s2wx(param));
                if (a->var_name(param) == p->var_name(param))
                    share_menu->Check(ID_peak_popup_share+i, true);
            }
        }
        peak_menu.Append(-1, "&Share with %" + s2wx(a->name), share_menu);
    }
    PopupMenu (&peak_menu, event.GetX(), event.GetY());
}

void MainPlot::PeakInfo()
{
    if (over_peak_ >= 0)
        exec("info prop %" + ftk->mgr.get_function(over_peak_)->name);
}

void MainPlot::OnPeakDelete(wxCommandEvent&)
{
    if (over_peak_ >= 0)
        exec("delete %" + ftk->mgr.get_function(over_peak_)->name);
}

void MainPlot::OnPeakGuess(wxCommandEvent&)
{
    if (over_peak_ < 0)
        return;
    const Function* p = ftk->mgr.get_function(over_peak_);
    realt ctr;
    if (p->get_center(&ctr)) {
        double plusmin = 0;
        realt fwhm, ib;
        if (p->get_fwhm(&fwhm))
            plusmin = fabs(fwhm);
        if (p->get_ibreadth(&ib) && fabs(ib) > plusmin)
            plusmin = fabs(ib);
        plusmin = max(plusmin, 1.);
        exec(frame->get_datasets() + "guess %" + p->name + " = "
                  + frame->get_guess_string(p->tp()->name)
                  + " [" + eS(ctr-plusmin) + ":" + eS(ctr+plusmin) + "]");
    }
}

void MainPlot::OnPeakEdit(wxCommandEvent&)
{
    if (over_peak_ < 0)
        return;
    const Function* p = ftk->mgr.get_function(over_peak_);
    string t = p->get_current_assignment(ftk->mgr.variables(),
                                         ftk->mgr.parameters());
    frame->edit_in_input(t);
}

void MainPlot::OnPeakShare(wxCommandEvent& event)
{
    int active = frame->get_sidebar()->get_active_function();
    if (over_peak_ < 0 || active < 0)
        return;
    const Function* a = ftk->mgr.get_function(active);
    const Function* p = ftk->mgr.get_function(over_peak_);
    string param = a->get_param(event.GetId() - ID_peak_popup_share);
    string lhs = "%" + p->name + "." + param;
    if (event.IsChecked())
        exec(lhs + " = %" + a->name + "." + param);
    else
        exec(lhs + " = ~{" + lhs + "}");
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

void MainPlot::switch_to_mode(MouseModeEnum m)
{
    if (pressed_mouse_button_) {
        if (dragged_func_->has_changed())
            frame->get_sidebar()->update_param_panel(); // reverts the values
        dragged_func_->stop();
        downX = downY = INT_MIN;
        pressed_mouse_button_ = 0;
        frame->set_status_text("");
    }
    MouseModeEnum old = mode_;
    if (old == m)
        return;
    if (m != mmd_peak)
        basic_mode_ = m;
    mode_ = m;
    update_mouse_hints();
    set_cursor();
    overlay.switch_mode(mode_ == mmd_activate ? Overlay::kVLine
                                              : get_normal_ovmode());
    if (old == mmd_bg)
        bgm_->define_bg_func();
    if (old == mmd_bg || mode_ == mmd_bg
                        || visible_peaktops(old) != visible_peaktops(mode_))
        refresh();
    else
        overlay.draw_overlay();
}

void MainPlot::set_data_color(int n, const wxColour& col)
{
    if (n >= kMaxDataColors)
        return;
    if (n >= (int) data_colors_.size())
        data_colors_.resize(n+1, data_colors_[0]);
    data_colors_[n] = col;
}

// update mouse hint on status bar
void MainPlot::update_mouse_hints()
{
    if (!hint_receiver_)
        return;
    const char *left="";
    const char *right="";
    const char *mode_name="";
    const char *shift_left = "";
    const char *shift_right = "";
    if (pressed_mouse_button_) {
        if (pressed_mouse_button_ != 1)
            left = "cancel";
        if (pressed_mouse_button_ != 3)
            right = "cancel";
    } else { //button not pressed
        switch (mode_) {
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
            case mmd_readonly:
                assert(0);
        }
    }
    hint_receiver_->set_hints(left, right, mode_name, shift_left, shift_right);
}

void MainPlot::set_cursor()
{
    SetCursor(mode_ == mmd_peak ? wxCURSOR_CROSS : wxCURSOR_ARROW);
}

void MainPlot::OnMouseMove(wxMouseEvent &event)
{
    //display coords in status bar
    int X = event.GetX();
    int Y = event.GetY();
    frame->set_status_coords(xs.valr(X), ys.valr(Y), pte_main);

    if (pressed_mouse_button_ == 0) { // no button pressed
        if (visible_peaktops(mode_))
            look_for_peaktop(event);
        if (mode_ == mmd_activate) {
            if (event.AltDown() || event.CmdDown()) {
                if (overlay.mode() == Overlay::kVLine) {
                    overlay.switch_mode(get_normal_ovmode());
                    overlay.draw_overlay();
                }
            } else
                overlay.switch_mode(Overlay::kVLine);
        }
    } else if (pressed_mouse_button_ == 1 && mouse_op_ == kDragPeak
             && over_peak_ >= 0) {
        dragged_func_->move(event.ShiftDown(), X, Y, xs.valr(X), ys.valr(Y));
        frame->set_status_text(dragged_func_->status());
        draw_overlay_func(ftk->mgr.get_function(over_peak_),
                          frame->get_sidebar()->parpan_values());
    }

    overlay.change_pos(X, Y);
    if (overlay.mode() == Overlay::kVLine
            || overlay.mode() == Overlay::kCrossHair)
        frame->plot_pane()->draw_vertical_lines(X, -1, this);
    else if (overlay.mode() == Overlay::kVRange)
        frame->plot_pane()->draw_vertical_lines(X, downX, this);
}

// peak description shown on the status bar
static string get_peak_description(int n)
{
    if (n < 0)
        return "";
    const Function* f = ftk->mgr.get_function(n);
    string s = "%" + f->name + " " + f->tp()->name + " ";
    for (int i = 0; i < f->nv(); ++i)
        s += " " + f->get_param(i) + "=" + S(f->av()[i]);
    return s;
}

void MainPlot::look_for_peaktop(wxMouseEvent& event)
{
    int focused_data = frame->get_sidebar()->get_focused_data();
    const Model* model = ftk->dk.get_model(focused_data);
    const vector<int>& idx = model->get_ff().idx;
    if (special_points.size() != idx.size())
        refresh();
    int n = get_special_point_at_pointer(event);
    int nearest = n == -1 ? -1 : idx[n];

    if (over_peak_ != nearest) {
        // change cursor, statusbar text and draw limits
        over_peak_ = nearest;
        frame->set_status_text(get_peak_description(nearest));
        switch_to_mode(nearest == -1 ? basic_mode_ : mmd_peak);
    }
}

void MainPlot::cancel_mouse_press()
{
    if (pressed_mouse_button_) {
        overlay.switch_mode(get_normal_ovmode());
        if (dragged_func_->has_changed())
            frame->get_sidebar()->update_param_panel(); // reverts the values
        dragged_func_->stop();
        if (GetCapture() == this)
            ReleaseMouse();
        connect_esc_to_cancel(false);
        downX = downY = INT_MIN;
        pressed_mouse_button_ = 0;
        frame->set_status_text("");
        update_mouse_hints();
        set_cursor();
    }
}

MouseOperation MainPlot::what_mouse_operation(const wxMouseEvent& event)
{
    bool ctrl = (event.AltDown() || event.CmdDown());
    bool shift = event.ShiftDown();
    int button = event.GetButton();
    if (button == 2 || // middle button always zooms
        (button == 1 && (ctrl || (mode_ == mmd_zoom && !shift))))
        return kRectangularZoom;
    else if (button == 3 && (ctrl || (mode_ == mmd_zoom && !shift)))
        return kShowPlotMenu;
    else if (button == 1 && mode_ == mmd_zoom && shift)
        return kVerticalZoom;
    else if (button == 3 && mode_ == mmd_zoom && shift)
        return kHorizontalZoom;
    else if (button == 1 && mode_ == mmd_peak)
        return kDragPeak;
    else if (button == 3 && mode_ == mmd_peak)
        return kShowPeakMenu;
    else if (button == 1 && mode_ == mmd_bg)
        return kAddBgPoint;
    else if (button == 3 && mode_ == mmd_bg)
        return kDeleteBgPoint;
    else if (button == 1 && mode_ == mmd_add)
        return kAddPeakTriangle;
    else if (button == 3 && mode_ == mmd_add)
        return kAddPeakInRange;
    else if (button == 1 && mode_ == mmd_activate)
        return shift ? kActivateRect : kActivateSpan;
    else if (button == 3 && mode_ == mmd_activate)
        return shift ? kDisactivateRect : kDisactivateSpan;
    else
        return kNoMouseOp;
}

void MainPlot::OnButtonDown (wxMouseEvent &event)
{
    if (pressed_mouse_button_) {
        // if one button is already down, pressing other button cancels action
        cancel_mouse_press();
        overlay.change_pos(event.GetX(), event.GetY());
        overlay.draw_overlay();
        frame->plot_pane()->draw_vertical_lines(-1, -1, this);
        return;
    }

    pressed_mouse_button_ = event.GetButton();
    downX = event.GetX();
    downY = event.GetY();
    double x = xs.valr(event.GetX());
    double y = ys.valr(event.GetY());
    mouse_op_ = what_mouse_operation(event);
    if (mouse_op_ == kRectangularZoom) {
        SetCursor(wxCURSOR_MAGNIFIER);
        CaptureMouse();
        connect_esc_to_cancel(true);
        overlay.start_mode(Overlay::kRect, downX, downY);
        frame->set_status_text("Select second corner to zoom...");
    } else if (mouse_op_ == kShowPlotMenu) {
        show_popup_menu(event);
        pressed_mouse_button_ = 0;
    } else if (mouse_op_ == kShowPeakMenu) {
        show_peak_menu(event);
        pressed_mouse_button_ = 0;
    } else if (mouse_op_ == kVerticalZoom) {
        SetCursor(wxCURSOR_SIZENS);
        CaptureMouse();
        connect_esc_to_cancel(true);
        overlay.start_mode(Overlay::kHRange, downX, downY);
        frame->set_status_text("Select vertical span...");
    } else if (mouse_op_ == kHorizontalZoom) {
        SetCursor(wxCURSOR_SIZEWE);
        CaptureMouse();
        connect_esc_to_cancel(true);
        overlay.start_mode(Overlay::kVRange, downX, downY);
        frame->set_status_text("Select horizontal span...");
    } else if (mouse_op_ == kDragPeak) {
        frame->get_sidebar()->activate_function(over_peak_);
        SetCursor(wxCURSOR_SIZENWSE);
        connect_esc_to_cancel(true);
        const Function* p = ftk->mgr.get_function(over_peak_);
        dragged_func_->start(p, downX, downY, x, y,
                             frame->get_sidebar()->dragged_func_callback());
        overlay.start_mode(Overlay::kFunction, downX, downY);
        draw_overlay_limits(p);
        frame->set_status_text("Moving %" + p->name + "...");
    } else if (mouse_op_ == kAddBgPoint) {
        bgm_->add_background_point(x, y);
        refresh();
    } else if (mouse_op_ == kDeleteBgPoint) {
        bgm_->rm_background_point(x);
        refresh();
    } else if (mouse_op_ == kAddPeakTriangle) {
        const Tplate* tp = ftk->get_tpm()->get_tp(frame->get_peak_type());
        if (tp == NULL)
            return;
        if (tp->traits & Tplate::kPeak) {
            func_draft_kind_ = Tplate::kPeak;
            overlay.start_mode(Overlay::kPeakDraft, downX, ys.px(0));
        } else if (tp->traits & Tplate::kSigmoid) {
            func_draft_kind_ = Tplate::kSigmoid;
            overlay.start_mode(Overlay::kSigmoidDraft, downX, downY);
        } else {
            func_draft_kind_ = Tplate::kLinear;
            overlay.start_mode(Overlay::kLinearDraft, downX, downY);
        }
        SetCursor(wxCURSOR_SIZING);
        connect_esc_to_cancel(true);
        frame->set_status_text("Add function from wireframe...");
        // see also: add_peak_from_draft()
    } else if (mouse_op_ == kAddPeakInRange) {
        SetCursor(wxCURSOR_SIZEWE);
        connect_esc_to_cancel(true);
        overlay.start_mode(Overlay::kVRange, downX, downY);
        frame->set_status_text("Select range to add a peak in it...");
    } else if (mouse_op_ == kActivateSpan || mouse_op_ == kDisactivateSpan ||
             mouse_op_ == kActivateRect || mouse_op_ == kDisactivateRect) {
        string act_str;
        if (mouse_op_ == kActivateSpan || mouse_op_ == kActivateRect) {
            if (!can_activate()) {
                pressed_mouse_button_ = 0;
                wxMessageBox(
                 wxT("You pressed the left mouse button in data-range mode,")
                 wxT("\nbut all data points are already active.")
                 wxT("\n\nThe left button activates,")
                 wxT("\nand the right disactivates points."),
                             wxT("How to use mouse..."),
                             wxOK|wxICON_INFORMATION);
                return;
            }
            act_str = "activate";
        } else
            act_str = "disactivate";
        string status_beginning;
        CaptureMouse();
        connect_esc_to_cancel(true);
        if (mouse_op_ == kActivateRect || mouse_op_ == kDisactivateRect) {
            SetCursor(wxCURSOR_SIZENWSE);
            overlay.start_mode(Overlay::kRect, downX, downY);
            status_beginning = "Select data in rectangle to ";
        } else {
            SetCursor(wxCURSOR_SIZEWE);
            overlay.start_mode(Overlay::kVRange, downX, downY);
            status_beginning = "Select data range to ";
        }
        frame->set_status_text(status_beginning + act_str + "...");
    }
    update_mouse_hints();
}

void MainPlot::OnAuxDown(wxMouseEvent &event)
{
    cancel_action();
    int step = (event.GetEventType() == wxEVT_AUX1_DOWN ? -1 : +1);
    frame->zoom_hist().move(step);
}

void MainPlot::OnMouseWheel(wxMouseEvent &event)
{
    cancel_action();
    // wheel rotation is typically +/-120
    double scale = -0.0025 * event.GetWheelRotation();
    int X = event.GetX();
    int Y = event.GetY();
    int W = GetClientSize().GetWidth();
    int H = GetClientSize().GetHeight();
    double x0 = xs.valr(iround(-scale * X));
    double x1 = xs.valr(iround(W + scale * (W - X)));
    if (x1 < x0)
        swap(x0, x1);
    double y0 = ys.valr(iround(-scale * Y));
    double y1 = ys.valr(iround(H + scale * (H - Y)));
    if (y1 < y0)
        swap(y0, y1);
    frame->change_zoom(RealRange(x0, x1), RealRange(y0, y1));
}

bool MainPlot::can_activate()
{
    vector<int> sel = frame->get_sidebar()->get_selected_data_indices();
    v_foreach (int, i, sel) {
        const fityk::Data* data = ftk->dk.data(*i);
        // if data->is_empty() we allow to try disactivate data to let user
        // experiment with mouse right after launching the program
        if (data->is_empty() || data->get_n() != (int) data->points().size())
            return true;
    }
    return false;
}

static
void freeze_functions_in_range(double x1, double x2, bool freeze)
{
    string cmd;
    v_foreach (Function*, i, ftk->mgr.functions()) {
        realt ctr;
        if (!(*i)->get_center(&ctr))
            continue;
        if (!(x1 < ctr && ctr < x2))
            continue;
        for (int j = 0; j != (*i)->used_vars().get_count(); ++j) {
            const Variable* var =
                ftk->mgr.get_variable((*i)->used_vars().get_idx(j));
            if (freeze && var->is_simple()) {
                cmd += "$" + var->name + "=" + eS(var->value()) + "; ";
            } else if (!freeze && var->is_constant()) {
                cmd += "$" + var->name + "=~" + eS(var->value()) + "; ";
            }
        }
    }
    if (!cmd.empty())
        exec(cmd);
}

void MainPlot::OnButtonUp (wxMouseEvent &event)
{
    int button = event.GetButton();
    if (button != pressed_mouse_button_) {
        pressed_mouse_button_ = 0;
        return;
    }

    overlay.switch_mode(get_normal_ovmode());
    if (GetCapture() == this)
        ReleaseMouse();
    connect_esc_to_cancel(false);
    pressed_mouse_button_ = 0;
    update_mouse_hints();
    set_cursor();

    // some actions are cancelled when Down and Up events are in the same place
    if (abs(event.GetX() - downX) + abs(event.GetY() - downY) < 5 &&
            (mouse_op_ == kRectangularZoom ||
             mouse_op_ == kVerticalZoom || mouse_op_ == kHorizontalZoom ||
             mouse_op_ == kActivateSpan || mouse_op_ == kDisactivateSpan ||
             mouse_op_ == kActivateRect || mouse_op_ == kDisactivateRect ||
             mouse_op_ == kAddPeakTriangle || mouse_op_ == kAddPeakInRange)) {
        frame->set_status_text("");
        overlay.draw_overlay();
        return;
    }

    // zoom
    if (mouse_op_ == kRectangularZoom) {
        double x1 = xs.valr(downX);
        double x2 = xs.valr(event.GetX());
        double y1 = ys.valr(downY);
        double y2 = ys.valr(event.GetY());
        frame->change_zoom(RealRange(min(x1,x2), max(x1,x2)),
                           RealRange(min(y1,y2), max(y1,y2)));
    } else if (mouse_op_ == kVerticalZoom) {
        double y1 = ys.valr(downY);
        double y2 = ys.valr(event.GetY());
        frame->change_zoom(ftk->view.hor,
                           RealRange(min(y1,y2), max(y1,y2)));
    } else if (mouse_op_ == kHorizontalZoom) {
        double x1 = xs.valr(downX);
        double x2 = xs.valr(event.GetX());
        frame->change_zoom(RealRange(min(x1,x2), max(x1,x2)),
                           ftk->view.ver);
    }
    // drag peak
    else if (mouse_op_ == kDragPeak) {
        string cmd = dragged_func_->get_cmd();
        dragged_func_->stop();
        if (!cmd.empty())
            exec(cmd);
        else {
            overlay.draw_overlay();
            frame->set_status_text("");
        }
    }
    // activate or disactivate data
    else if (mouse_op_ == kActivateSpan || mouse_op_ == kDisactivateSpan ||
             mouse_op_ == kActivateRect || mouse_op_ == kDisactivateRect) {
        bool rect = (mouse_op_ == kActivateRect ||
                     mouse_op_ == kDisactivateRect);
        bool activate = (mouse_op_== kActivateSpan ||
                         mouse_op_ == kActivateRect);
        string c = (activate ? "A = a or" : "A = a and not");
        double x1 = xs.valr(downX);
        double x2 = xs.valr(event.GetX());
        if (x1 > x2)
            swap(x1, x2);
        string cond = eS(x1) + " < x and x < " + eS(x2);
        if (rect) {
            double y1 = ys.valr(downY);
            double y2 = ys.valr(event.GetY());
            cond += " and "
                    + eS(min(y1,y2)) + " < y and y < " + eS(max(y1,y2));
        }
        exec(frame->get_datasets() + c + " (" + cond + ")");

        if (auto_freeze_ && !rect)
            freeze_functions_in_range(x1, x2, !activate);
    }
    // add peak (left button)
    else if (mouse_op_ == kAddPeakTriangle) {
        add_peak_from_draft(event.GetX(), event.GetY());
    }
    // add peak (in range)
    else if (mouse_op_ == kAddPeakInRange) {
        double x1 = xs.valr(downX);
        double x2 = xs.valr(event.GetX());
        exec(frame->get_datasets() + "guess "
                  + frame->get_guess_string(frame->get_peak_type())
                  + " [" + eS(min(x1,x2)) + " : " + eS(max(x1,x2)) + "]");
    } else {
        // nothing - action done in OnButtonDown()
    }
}

void MainPlot::add_peak_from_draft(int X, int Y)
{
    string args;
    double x1 = xs.valr(downX);
    double y1 = ys.valr(downY);
    double x2 = xs.valr(X);
    double y2 = ys.valr(Y);
    if (func_draft_kind_ == Tplate::kLinear) {
        if (downX == X)
            return;
        double m = (y2 - y1) / (x2 - x1);
        args = "slope=~" + eS(m) + ", intercept=~" + eS(y1-m*x1)
               + ", avgy=~" + eS((y1+y2)/2);
    } else if (func_draft_kind_ == Tplate::kPeak) {
        double height = y2;
        double center = x1;
        double hwhm = fabs(center - x2);
        double area = 2 * height * hwhm;
        args = "height=~" + eS(height) + ", center=~" + eS(center)
                 + ", area=~" + eS(area);
        if (ftk->mgr.find_variable_nr("_hwhm") == -1)
            args += ", hwhm=~" + eS(hwhm);
    } else if (func_draft_kind_ == Tplate::kSigmoid) {
        double lower = y1 - fabs(y2-y1);
        double upper = y1 + fabs(y2-y1);
        double xmid = x1;
        // this corresponds to the drawing in Overlay::draw_overlay()
        double wsig = (y2 > y1 ? 1 : -1) * 0.5 * fabs(x2-x1);
        args = "lower=~" + eS(lower) + ", upper=~" + eS(upper)
                 + ", xmid=~" + eS(xmid) + ", wsig=~" + eS(wsig);
    }
    string tail = "F += " + frame->get_guess_string(frame->get_peak_type());
    if (*(tail.end() - 1) == ')') {
        tail.resize(tail.size() - 1);
        tail += ", " + args + ")";
    } else
        tail += "(" + args + ")";
    string cmd;
    if (ftk->dk.count() == 1)
        cmd = tail;
    else {
        vector<int> sel = frame->get_sidebar()->get_selected_data_indices();
        cmd = "@" + join_vector(sel, "." + tail + "; @") + "." + tail;
    }
    exec(cmd);
}

static
void calculate_values(const vector<realt>& xx, vector<realt>& yy,
                      const Tplate::Ptr& tp, const vector<realt>& p_values)
{
    int len = p_values.size();
    vector<string> varnames(len);
    for (int i = 0; i != len; ++i)
        varnames[i] = "v" + S(i);
    boost::scoped_ptr<Function> f(
                (*tp->create)(ftk->get_settings(), "tmp", tp, varnames));
    f->init();

    // create variables with the same values as p_values
    vector<Variable*> variables(len);
    const vector<Variable*> dummy_vars;
    for (int i = 0; i != len; ++i) {
        fityk::OpTree* tree = new fityk::OpTree(p_values[i]);
        variables[i] = new Variable("v" + S(i), vector<string>(),
                                    vector1<fityk::OpTree*>(tree));
        variables[i]->set_var_idx(dummy_vars);
        variables[i]->recalculate(dummy_vars, vector<realt>());
    }

    f->update_var_indices(variables);
    f->do_precomputations(variables);
    f->calculate_value(xx, yy);
    purge_all_elements(variables);
}

void MainPlot::draw_overlay_func(const Function* f,
                                 const vector<realt>& p_values)
{
    int n = GetClientSize().GetWidth();
    if (n <= 0)
        return;
    vector<realt> xx(n), yy(n, 0);
    for (int i = 0; i < n; ++i)
        xx[i] = xs.val(i);
    calculate_values(xx, yy, f->tp(), p_values);
    wxPoint *points = new wxPoint[n];
    for (int i = 0; i < n; ++i) {
        points[i].x = i;
        points[i].y = ys.px(yy[i]);
    }
    overlay.draw_lines(n, points);
    delete [] points;
}

void MainPlot::draw_overlay_limits(const Function* f)
{
    double cutoff = ftk->get_settings()->function_cutoff;
    if (cutoff == 0)
        return;
    realt x1, x2;
    bool r = f->get_nonzero_range(cutoff, x1, x2);
    if (r) {
        int xleft = xs.px(x1);
        int xright = xs.px(x2);
        wxPoint lim[4] = {
            wxPoint(xleft, 0),
            wxPoint(xleft, ys.px(cutoff)),
            wxPoint(xright, ys.px(cutoff)),
            wxPoint(xright, 0)
        };
        overlay.draw_lines(4, lim);
    }
}

void MainPlot::OnConfigure(wxCommandEvent&)
{
    MainPlotConfDlg dialog(this);
    dialog.ShowModal();
}

void MainPlot::OnZoomAll(wxCommandEvent&)
{
    frame->GViewAll();
}


//===============================================================

MainPlotConfDlg::MainPlotConfDlg(MainPlot* mp)
  : wxDialog(NULL, -1, wxString(wxT("Configure Main Plot")),
             wxDefaultPosition, wxDefaultSize,
             wxDEFAULT_DIALOG_STYLE),
    mp_(mp)
{
    wxBoxSizer *top_sizer = new wxBoxSizer(wxVERTICAL);

    wxBoxSizer *hor_sizer = new wxBoxSizer(wxHORIZONTAL);
    wxFlexGridSizer *gsizer = new wxFlexGridSizer(2, 5, 5);
    wxSizerFlags cl = wxSizerFlags().Align(wxALIGN_CENTRE_VERTICAL),
             cr = wxSizerFlags().Align(wxALIGN_CENTRE_VERTICAL|wxALIGN_RIGHT);

    gsizer->Add(new wxStaticText(this, -1, wxT("background")), cr);
    bg_cp_ = new wxColourPickerCtrl(this, -1, mp_->get_bg_color());
    gsizer->Add(bg_cp_, cl);

    gsizer->Add(new wxStaticText(this, -1, wxT("colors for datasets")), cr);
    wxBoxSizer *data_col_sizer = new wxBoxSizer(wxHORIZONTAL);
    data_colors_sc_ = make_wxspinctrl(this, -1, mp_->data_colors_.size(),
                                      2, MainPlot::kMaxDataColors, /*width*/60);
    data_col_sizer->Add(data_colors_sc_, cl);
    data_color_combo_ = new MultiColorCombo(this, &mp_->get_bg_color(),
                                            mp_->data_colors_);
    data_color_combo_->SetSelection(0);
    data_col_sizer->Add(data_color_combo_, cl);
    gsizer->Add(data_col_sizer, cl);

    gsizer->Add(new wxStaticText(this, -1, wxT("inactive data")), cr);
    inactive_cp_ = new wxColourPickerCtrl(this, -1, mp_->inactiveDataCol);
    gsizer->Add(inactive_cp_, cl);

    model_cb_ = new wxCheckBox(this, -1, wxT("model (sum)"));
    model_cb_->SetValue(mp_->model_visible_);
    gsizer->Add(model_cb_, cr);
    wxBoxSizer *model_sizer = new wxBoxSizer(wxHORIZONTAL);
    model_cp_ = new wxColourPickerCtrl(this, -1, mp_->modelCol);
    model_sizer->Add(model_cp_, cl.Border(wxRIGHT));
    model_sizer->Add(new wxStaticText(this, -1, wxT("width:")), cl);
    model_width_sc_ = make_wxspinctrl(this, -1, mp_->model_line_width_, 1, 5);
    model_sizer->Add(model_width_sc_, cl);
    gsizer->Add(model_sizer, cl);

    func_cb_ = new wxCheckBox(this, -1, wxT("functions"));
    func_cb_->SetValue(mp_->peaks_visible_);
    gsizer->Add(func_cb_, cr);
    func_cp_ = new wxColourPickerCtrl(this, -1, mp_->peakCol[0]);
    gsizer->Add(func_cp_, cl);

    gsizer->Add(new wxStaticText(this, -1, wxT("axes and tics")), cr);
    wxBoxSizer *tics_sizer = new wxBoxSizer(wxHORIZONTAL);
    axis_cp_ = new wxColourPickerCtrl(this, -1, mp_->xAxisCol);
    tics_fp_ = new wxFontPickerCtrl(this, -1, mp_->ticsFont);
    tics_sizer->Add(axis_cp_, cl);
    tics_sizer->Add(tics_fp_, cl);
    gsizer->Add(tics_sizer, cl);


    desc_cb_ = new wxCheckBox(this, -1, wxT("plot description"));
    desc_cb_->SetValue(mp_->desc_visible_);
    gsizer->Add(desc_cb_, cr);

    wxArrayString desc_choices;
    desc_choices.Add("title");
    desc_choices.Add("filename");
    desc_choices.Add("fit");
    desc_choices.Add("filename,fit");
    desc_combo_ = new wxComboBox(this, -1, s2wx(mp_->desc_format_),
                                 wxDefaultPosition, wxDefaultSize,
                                 desc_choices);
    desc_combo_->SetToolTip("Description (in the right top corner)\n"
                            "is an output of the info command.\n"
                            "You can give any info arguments here.");
    gsizer->Add(desc_combo_, cl);

    gsizer->AddSpacer(10);
    gsizer->AddSpacer(10);
    labels_cb_ = new wxCheckBox(this, -1, wxT("function labels"));
    labels_cb_->SetValue(mp_->plabels_visible_);
    label_fp_ = new wxFontPickerCtrl(this, -1, mp_->plabelFont);
    vertical_labels_cb_ = new wxCheckBox(this, -1, wxT("vertical"));
    vertical_labels_cb_->SetValue(mp_->vertical_plabels_);
    wxArrayString label_choices;
    label_choices.Add(wxT("<area>"));
    label_choices.Add(wxT("<height>"));
    label_choices.Add(wxT("<center>"));
    label_choices.Add(wxT("<fwhm>"));
    label_choices.Add(wxT("<ib>"));
    label_choices.Add(wxT("<name>"));
    label_choices.Add(wxT("<name>  <area>"));
    label_choices.Add(wxT("<name><br><area>"));
    label_combo_ = new wxComboBox(this, -1, s2wx(mp_->plabel_format_),
                                  wxDefaultPosition, wxDefaultSize,
                                  label_choices);
    label_combo_->SetToolTip(wxT("Labels can show the following properties:\n")
                             wxT("<area>   area\n")
                             wxT("<height> height\n")
                             wxT("<center> center\n")
                             wxT("<fwhm>   peak FWHM\n")
                             wxT("<ib>     integral breadth (area/FWHM)\n")
                             wxT("<name>   function's name\n")
                             wxT("<br>     line break\n"));

    gsizer->Add(labels_cb_, cr);
    gsizer->Add(label_combo_, cl);
    gsizer->AddSpacer(0);
    wxBoxSizer* flabels_sizer = new wxBoxSizer(wxHORIZONTAL);
    flabels_sizer->Add(label_fp_, cl);
    flabels_sizer->Add(vertical_labels_cb_, cl);
    gsizer->Add(flabels_sizer, cl);

    hor_sizer->Add(gsizer, wxSizerFlags().Border());

    wxStaticBoxSizer *xsizer = new wxStaticBoxSizer(wxVERTICAL, this,
                                                    wxT("X axis"));
    x_show_axis_cb_ = new wxCheckBox(this, -1, wxT("show axis"));
    x_show_axis_cb_->SetValue(mp_->x_axis_visible);
    xsizer->Add(x_show_axis_cb_, 0, wxALL, 5);
    x_show_tics_cb_ = new wxCheckBox(this, -1, wxT("show tics"));
    x_show_tics_cb_->SetValue(mp_->xtics_visible);
    xsizer->Add(x_show_tics_cb_, 0, wxALL, 5);
    wxBoxSizer* xsizer_t = new wxBoxSizer(wxVERTICAL);
    x_show_minor_tics_cb_ = new wxCheckBox(this, -1, wxT("show minor tics"));
    x_show_minor_tics_cb_->SetValue(mp_->xminor_tics_visible);
    xsizer_t->Add(x_show_minor_tics_cb_, 0, wxALL, 5);
    x_show_grid_cb_ = new wxCheckBox(this, -1, wxT("show grid"));
    x_show_grid_cb_->SetValue(mp_->x_grid);
    xsizer_t->Add(x_show_grid_cb_, 0, wxALL, 5);
    wxBoxSizer *xmt_sizer = new wxBoxSizer(wxHORIZONTAL);
    xmt_sizer->Add(new wxStaticText(this, -1, wxT("max. number of tics")),
                  0, wxALL|wxALIGN_CENTRE_VERTICAL, 5);
    x_max_tics_sc_ = make_wxspinctrl(this, -1, mp_->x_max_tics, 1, 30);
    xmt_sizer->Add(x_max_tics_sc_, 0, wxALL, 5);
    xsizer_t->Add(xmt_sizer);
    wxBoxSizer *xts_sizer = new wxBoxSizer(wxHORIZONTAL);
    xts_sizer->Add(new wxStaticText(this, -1, wxT("length of tics")),
                  0, wxALL|wxALIGN_CENTRE_VERTICAL, 5);
    x_tic_size_sc_ = make_wxspinctrl(this, -1, mp_->x_tic_size, -10, 20);
    xts_sizer->Add(x_tic_size_sc_, 0, wxALL, 5);
    xsizer_t->Add(xts_sizer);
    xsizer->Add(xsizer_t, 0, wxLEFT, 15);
    x_reversed_cb_ = new wxCheckBox(this, -1, wxT("reversed axis"));
    x_reversed_cb_->SetValue(mp_->xs.reversed);
    xsizer->Add(x_reversed_cb_, 0, wxALL, 5);
    x_logarithm_cb_ = new wxCheckBox(this, -1, wxT("logarithmic scale"));
    x_logarithm_cb_->SetValue(mp_->xs.logarithm);
    xsizer->Add(x_logarithm_cb_, 0, wxALL, 5);
    hor_sizer->Add(xsizer, 0, wxALL, 5);

    wxStaticBoxSizer *ysizer = new wxStaticBoxSizer(wxVERTICAL, this,
                                                    wxT("Y axis"));
    y_show_axis_cb_ = new wxCheckBox(this, -1, wxT("show axis"));
    y_show_axis_cb_->SetValue(mp_->y_axis_visible);
    ysizer->Add(y_show_axis_cb_, 0, wxALL, 5);
    y_show_tics_cb_ = new wxCheckBox(this, -1, wxT("show tics"));
    y_show_tics_cb_->SetValue(mp_->ytics_visible);
    ysizer->Add(y_show_tics_cb_, 0, wxALL, 5);
    wxBoxSizer* ysizer_t = new wxBoxSizer(wxVERTICAL);
    y_show_minor_tics_cb_ = new wxCheckBox(this, -1, wxT("show minor tics"));
    y_show_minor_tics_cb_->SetValue(mp_->yminor_tics_visible);
    ysizer_t->Add(y_show_minor_tics_cb_, 0, wxALL, 5);
    y_show_grid_cb_ = new wxCheckBox(this, -1, wxT("show grid"));
    y_show_grid_cb_->SetValue(mp_->y_grid);
    ysizer_t->Add(y_show_grid_cb_, 0, wxALL, 5);
    wxBoxSizer *ymt_sizer = new wxBoxSizer(wxHORIZONTAL);
    ymt_sizer->Add(new wxStaticText(this, -1, wxT("max. number of tics")),
                  0, wxALL|wxALIGN_CENTRE_VERTICAL, 5);
    y_max_tics_sc_ = make_wxspinctrl(this, -1, mp_->y_max_tics, 1, 30);
    ymt_sizer->Add(y_max_tics_sc_, 0, wxALL, 5);
    ysizer_t->Add(ymt_sizer);
    wxBoxSizer *yts_sizer = new wxBoxSizer(wxHORIZONTAL);
    yts_sizer->Add(new wxStaticText(this, -1, wxT("length of tics")),
                  0, wxALL|wxALIGN_CENTRE_VERTICAL, 5);
    y_tic_size_sc_ = make_wxspinctrl(this, -1, mp_->y_tic_size, -10, 20);
    yts_sizer->Add(y_tic_size_sc_, 0, wxALL, 5);
    ysizer_t->Add(yts_sizer);
    ysizer->Add(ysizer_t, 0, wxLEFT, 15);
    y_reversed_cb_ = new wxCheckBox(this, -1, wxT("reversed axis"));
    y_reversed_cb_->SetValue(mp_->ys.reversed);
    ysizer->Add(y_reversed_cb_, 0, wxALL, 5);
    y_logarithm_cb_ = new wxCheckBox(this, -1, wxT("logarithmic scale"));
    y_logarithm_cb_->SetValue(mp_->ys.logarithm);
    ysizer->Add(y_logarithm_cb_, 0, wxALL, 5);
    hor_sizer->Add(ysizer, 0, wxALL, 5);

    top_sizer->Add(hor_sizer, 0);
    top_sizer->Add(persistance_note(this), wxSizerFlags().Center().Border());
    top_sizer->Add(new wxButton(this, wxID_CLOSE),
                   wxSizerFlags().Right().Border());
    SetSizerAndFit(top_sizer);
    SetEscapeId(wxID_CLOSE);

    Connect(model_cb_->GetId(), wxEVT_COMMAND_CHECKBOX_CLICKED,
            wxCommandEventHandler(MainPlotConfDlg::OnModelCheckbox));
    Connect(model_width_sc_->GetId(), wxEVT_COMMAND_SPINCTRL_UPDATED,
            wxSpinEventHandler(MainPlotConfDlg::OnModelWidthSpin));
    Connect(func_cb_->GetId(), wxEVT_COMMAND_CHECKBOX_CLICKED,
            wxCommandEventHandler(MainPlotConfDlg::OnFuncCheckbox));
    Connect(labels_cb_->GetId(), wxEVT_COMMAND_CHECKBOX_CLICKED,
            wxCommandEventHandler(MainPlotConfDlg::OnLabelsCheckbox));
    Connect(desc_cb_->GetId(), wxEVT_COMMAND_CHECKBOX_CLICKED,
            wxCommandEventHandler(MainPlotConfDlg::OnDescCheckbox));
    Connect(vertical_labels_cb_->GetId(), wxEVT_COMMAND_CHECKBOX_CLICKED,
            wxCommandEventHandler(MainPlotConfDlg::OnVerticalCheckbox));
    Connect(-1, wxEVT_COMMAND_COLOURPICKER_CHANGED,
            wxColourPickerEventHandler(MainPlotConfDlg::OnColor));
    Connect(tics_fp_->GetId(), wxEVT_COMMAND_FONTPICKER_CHANGED,
            wxFontPickerEventHandler(MainPlotConfDlg::OnTicsFont));
    Connect(label_fp_->GetId(), wxEVT_COMMAND_FONTPICKER_CHANGED,
            wxFontPickerEventHandler(MainPlotConfDlg::OnLabelFont));
    Connect(label_combo_->GetId(), wxEVT_COMMAND_COMBOBOX_SELECTED,
            wxCommandEventHandler(MainPlotConfDlg::OnLabelTextChanged));
    Connect(label_combo_->GetId(), wxEVT_COMMAND_TEXT_UPDATED,
            wxCommandEventHandler(MainPlotConfDlg::OnLabelTextChanged));
    Connect(desc_combo_->GetId(), wxEVT_COMMAND_COMBOBOX_SELECTED,
            wxCommandEventHandler(MainPlotConfDlg::OnDescTextChanged));
    Connect(desc_combo_->GetId(), wxEVT_COMMAND_TEXT_UPDATED,
            wxCommandEventHandler(MainPlotConfDlg::OnDescTextChanged));
    Connect(data_colors_sc_->GetId(), wxEVT_COMMAND_SPINCTRL_UPDATED,
            wxSpinEventHandler(MainPlotConfDlg::OnDataColorsSpin));
    Connect(data_color_combo_->GetId(), wxEVT_COMMAND_COMBOBOX_SELECTED,
            wxCommandEventHandler(MainPlotConfDlg::OnDataColorSelection));

    Connect(x_show_axis_cb_->GetId(), wxEVT_COMMAND_CHECKBOX_CLICKED,
            wxCommandEventHandler(MainPlotConfDlg::OnShowXAxis));
    Connect(y_show_axis_cb_->GetId(), wxEVT_COMMAND_CHECKBOX_CLICKED,
            wxCommandEventHandler(MainPlotConfDlg::OnShowYAxis));
    Connect(x_show_tics_cb_->GetId(), wxEVT_COMMAND_CHECKBOX_CLICKED,
            wxCommandEventHandler(MainPlotConfDlg::OnShowXTics));
    Connect(y_show_tics_cb_->GetId(), wxEVT_COMMAND_CHECKBOX_CLICKED,
            wxCommandEventHandler(MainPlotConfDlg::OnShowYTics));
    Connect(x_show_minor_tics_cb_->GetId(), wxEVT_COMMAND_CHECKBOX_CLICKED,
            wxCommandEventHandler(MainPlotConfDlg::OnShowXMinorTics));
    Connect(y_show_minor_tics_cb_->GetId(), wxEVT_COMMAND_CHECKBOX_CLICKED,
            wxCommandEventHandler(MainPlotConfDlg::OnShowYMinorTics));
    Connect(x_show_grid_cb_->GetId(), wxEVT_COMMAND_CHECKBOX_CLICKED,
            wxCommandEventHandler(MainPlotConfDlg::OnShowXGrid));
    Connect(y_show_grid_cb_->GetId(), wxEVT_COMMAND_CHECKBOX_CLICKED,
            wxCommandEventHandler(MainPlotConfDlg::OnShowYGrid));
    Connect(x_reversed_cb_->GetId(), wxEVT_COMMAND_CHECKBOX_CLICKED,
            wxCommandEventHandler(MainPlotConfDlg::OnReversedX));
    Connect(y_reversed_cb_->GetId(), wxEVT_COMMAND_CHECKBOX_CLICKED,
            wxCommandEventHandler(MainPlotConfDlg::OnReversedY));
    Connect(x_logarithm_cb_->GetId(), wxEVT_COMMAND_CHECKBOX_CLICKED,
            wxCommandEventHandler(MainPlotConfDlg::OnLogX));
    Connect(y_logarithm_cb_->GetId(), wxEVT_COMMAND_CHECKBOX_CLICKED,
            wxCommandEventHandler(MainPlotConfDlg::OnLogY));
    Connect(x_max_tics_sc_->GetId(), wxEVT_COMMAND_SPINCTRL_UPDATED,
            wxSpinEventHandler(MainPlotConfDlg::OnMaxXTicsSpin));
    Connect(y_max_tics_sc_->GetId(), wxEVT_COMMAND_SPINCTRL_UPDATED,
            wxSpinEventHandler(MainPlotConfDlg::OnMaxYTicsSpin));
    Connect(x_tic_size_sc_->GetId(), wxEVT_COMMAND_SPINCTRL_UPDATED,
            wxSpinEventHandler(MainPlotConfDlg::OnXTicSize));
    Connect(y_tic_size_sc_->GetId(), wxEVT_COMMAND_SPINCTRL_UPDATED,
            wxSpinEventHandler(MainPlotConfDlg::OnYTicSize));
}

void MainPlotConfDlg::OnColor(wxColourPickerEvent& event)
{
    int id = event.GetId();
    if (id == bg_cp_->GetId()) {
        mp_->set_bg_color(event.GetColour());
        frame->update_data_pane();
        data_color_combo_->Refresh();
    } else if (id == inactive_cp_->GetId())
        mp_->inactiveDataCol = event.GetColour();
    else if (id == axis_cp_->GetId())
        mp_->xAxisCol = event.GetColour();
    else if (id == model_cp_->GetId())
        mp_->modelCol = event.GetColour();
    else if (id == func_cp_->GetId()) {
        for (int i = 0; i < MainPlot::max_peak_cols; ++i)
            mp_->peakCol[i] = event.GetColour();
    }
    mp_->refresh();
}


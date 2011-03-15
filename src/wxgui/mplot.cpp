// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#include <wx/wx.h>

#include <wx/clrpicker.h>
#include <wx/fontpicker.h>
#include <algorithm>
#include <boost/scoped_ptr.hpp>

#include "mplot.h"
#include "pplot.h"
#include "frame.h"
#include "sidebar.h"
#include "statbar.h" // HintReceiver
#include "bgm.h"
#include "gradient.h"
#include "../data.h"
#include "../logic.h"
#include "../model.h"
#include "../var.h"
#include "../func.h"
#include "../ui.h"
#include "../settings.h"
#include "../ast.h"

using namespace std;


enum {
    ID_plot_popup_za                = 25001,
    ID_plot_popup_prefs                    ,

    ID_peak_popup_info                     ,
    ID_peak_popup_del                      ,
    ID_peak_popup_guess                    ,
};


class MainPlotConfDlg: public wxDialog
{
public:
    MainPlotConfDlg(MainPlot* mp);

private:
    MainPlot *mp_;
    wxCheckBox *model_cb_, *func_cb_, *labels_cb_, *vertical_labels_cb_;
    wxComboBox *label_combo_;
    wxColourPickerCtrl *bg_cp_, *inactive_cp_, *axis_cp_, *model_cp_, *func_cp_;
    wxSpinCtrl *data_colors_sc_;
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

//---------------------- FunctionMouseDrag --------------------------------

/// utility used in MainPlot for dragging function
class FunctionMouseDrag
{
public:
    enum drag_type
    {
        no_drag,
        relative_value, //eg. for area
        absolute_value,  //eg. for width
        absolute_pixels
    };

    class Drag
    {
    public:
        drag_type how;
        int parameter_idx;
        std::string parameter_name;
        std::string variable_name; /// name of variable that are to be changed
        fp value; /// current value of parameter
        fp ini_value; /// initial value of parameter
        fp multiplier; /// increases or decreases changing rate
        fp ini_x;

        Drag() : how(no_drag) {}
        void set(const Function* p, int idx, drag_type how_, fp multiplier_);
        void change_value(fp x, fp dx, int dX);
        std::string get_cmd() const;
    };

    FunctionMouseDrag() : sidebar_dirty(false) {}
    void start(const Function* p, int X, int Y, fp x, fp y);
    void move(bool shift, int X, int Y, fp x, fp y);
    void stop();
    const std::vector<fp>& get_values() const { return values; }
    const std::string& get_status() const { return status; }
    std::string get_cmd() const;

private:
    Drag drag_x; ///for horizontal dragging (x axis)
    Drag drag_y; /// y axis
    Drag drag_shift_x; ///x with [shift]
    Drag drag_shift_y; ///y with [shift]
    fp px, py;
    int pX, pY;
    std::vector<fp> values;
    std::string status;
    bool sidebar_dirty;

    void set_defined_drags();
    bool bind_parameter_to_drag(Drag &drag, const std::string& name,
                          const Function* p, drag_type how, fp multiplier=1.);
    void set_drag(Drag &drag, const Function* p, int idx,
                  drag_type how, fp multiplier=1.);
};


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

void FunctionMouseDrag::Drag::set(const Function* p, int idx,
                                  drag_type how_, fp multiplier_)
{
    const Variable* var = ftk->get_variable(p->get_var_idx(idx));
    if (!var->is_simple()) {
        how = no_drag;
        return;
    }
    how = how_;
    parameter_idx = idx;
    parameter_name = p->get_param(idx);
    variable_name = p->get_var_name(idx);
    value = ini_value = p->av()[idx];
    multiplier = multiplier_;
    ini_x = 0.;
}


void FunctionMouseDrag::start(const Function* p, int X, int Y, fp x, fp y)
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

    values = p->av();
    size_t nv = p->nv();
    if (nv < values.size()) // av() may contain additional numbers
        values.resize(nv);

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

bool FunctionMouseDrag::bind_parameter_to_drag(Drag &drag, const string& name,
                                              const Function* p, drag_type how,
                                              fp multiplier)
{
    // search for Function(..., height, ...)
    int idx = index_of_element(p->tp()->fargs, name);
    if (idx != -1) {
        drag.set(p, idx, how, multiplier);
        return true;
    }

    const vector<string>& defvals = p->tp()->defvals;
    // search for Function(..., foo=height, ...)
    idx = index_of_element(defvals, name);
    // search for Function(..., foo=height*..., ...)
    if (idx != -1)
        v_foreach (string, i, defvals)
            if (startswith(*i, name+"*")) {
                idx = i - defvals.begin();
                break;
            }
    if (idx != -1) {
        drag.set(p, idx, how, multiplier);
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
    EVT_MENU (ID_plot_popup_prefs,  MainPlot::OnConfigure)
    EVT_MENU (ID_peak_popup_info,   MainPlot::OnPeakInfo)
    EVT_MENU (ID_peak_popup_del,    MainPlot::OnPeakDelete)
    EVT_MENU (ID_peak_popup_guess,  MainPlot::OnPeakGuess)
END_EVENT_TABLE()

MainPlot::MainPlot (wxWindow *parent)
    : FPlot(parent),
      bgm_(new BgManager(xs)),
      fmd_(new FunctionMouseDrag),
      basic_mode_(mmd_zoom), mode_(mmd_zoom),
      crosshair_cursor_(false),
      pressed_mouse_button_(0),
      over_peak_(-1),
      hint_receiver_(NULL),
      auto_freeze_(false)
{
    set_cursor();
}

MainPlot::~MainPlot()
{
    delete bgm_;
    delete fmd_;
}

void MainPlot::OnPaint(wxPaintEvent&)
{
    update_buffer_and_blit();
    // if necessary, redraw inverted lines
    // TODO
}

fp y_of_data_for_draw_data(vector<Point>::const_iterator i,
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
        draw_data(dc, y_of_data_for_draw_data, ftk->get_data(n), 0,
                  col, wxNullColour, offset);
    else
        draw_data(dc, y_of_data_for_draw_data, ftk->get_data(n), 0,
                  dc.GetPen().GetColour(), dc.GetPen().GetColour(), offset);
}

void MainPlot::draw(wxDC &dc, bool monochrome)
{
    //printf("MainPlot::draw()\n");
    int focused_data = frame->get_focused_data_index();
    const Model* model = ftk->get_model(focused_data);

    set_scale(get_pixel_width(dc), get_pixel_height(dc));

    overlay.reset();

    int Ymax = get_pixel_height(dc);
    prepare_peaktops(model, Ymax);

    if (monochrome) {
        dc.SetPen(*wxBLACK_PEN);
        dc.SetBrush(*wxBLACK_BRUSH);
    }
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
        draw_background(dc);
    }
    else {
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

void MainPlot::draw_model(wxDC& dc, const Model* model, bool set_pen)
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
//void MainPlot::draw_groups (wxDC& /*dc*/, const Model*, bool)
//{
//}

void MainPlot::draw_peaks(wxDC& dc, const Model* model, bool set_pen)
{
    fp level = 0;
    const vector<int>& idx = model->get_ff().idx;
    int n = get_pixel_width(dc);
    vector<fp> xx(n), yy(n);
    vector<int> YY(n);
    for (int i = 0; i < n; ++i) {
        xx[i] = xs.val(i);
        xx[i] += model->zero_shift(xx[i]);
    }
    for (int k = 0; k < size(idx); k++) {
        fill(yy.begin(), yy.end(), 0.);
        const Function* f = ftk->get_function(idx[k]);
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
    for (int k = 0; k < size(idx); k++) {
        const wxPoint &peaktop = special_points[k];
        if (set_pen)
            dc.SetTextForeground(peakCol[k % max_peak_cols]);

        wxString label = s2wx(plabels_[k]);
        wxCoord w, h;
        if (vertical_plabels_) {
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
        const Function *f = ftk->get_function(idx[k]);
        fp x, ctr;
        int X, Y;
        if (f->get_center(&ctr)) {
            X = xs.px (ctr - model->zero_shift(ctr));
            // instead of these two lines we could simply do x=ctr,
            // but it would be slightly inaccurate
            x = xs.val(X);
            x += model->zero_shift(x);
        }
        else {
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
    for (int k = 0; k < size(idx); k++) {
        const Function *f = ftk->get_function(idx[k]);
        string label = plabel_format_;
        string::size_type pos = 0;
        while ((pos = label.find("<", pos)) != string::npos) {
            string::size_type right = label.find(">", pos+1);
            if (right == string::npos)
                break;
            string tag(label, pos+1, right-pos-1);
            fp a;
            if (tag == "area")
                label.replace(pos, right-pos+1, f->get_area(&a) ? S(a) : " ");
            else if (tag == "height")
                label.replace(pos, right-pos+1, f->get_height(&a) ? S(a) : " ");
            else if (tag == "center")
                label.replace(pos, right-pos+1, f->get_center(&a) ? S(a) : " ");
            else if (tag == "fwhm")
                label.replace(pos, right-pos+1, f->get_fwhm(&a) ? S(a) : " ");
            else if (tag == "ib")
                label.replace(pos, right-pos+1, f->get_iwidth(&a) ? S(a) : " ");
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


void MainPlot::draw_background(wxDC& dc, bool set_pen)
{
    if (set_pen)
        dc.SetPen(wxPen(bg_pointsCol, pen_width));
    dc.SetBrush (*wxTRANSPARENT_BRUSH);

    // bg line
    int X = -1, Y = -1;
    int width = get_pixel_width(dc);
    vector<int> bgline = bgm_->calculate_bgline(width, ys);
    for (int i = 0; i < width; ++i) {
        int X_ = X, Y_ = Y;
        X = i;
        Y = bgline[i];
        if (X_ >= 0 && (X != X_ || Y != Y_))
            dc.DrawLine (X_, Y_, X, Y);
    }

    // bg points (circles)
    v_foreach (PointQ, i, bgm_->get_bg()) {
        dc.DrawCircle(xs.px(i->x), ys.px(i->y), 3);
        dc.DrawCircle(xs.px(i->x), ys.px(i->y), 4);
    }
}

void MainPlot::read_settings(wxConfigBase *cf)
{
    cf->SetPath(wxT("/MainPlot"));
    int data_colors_count = cf->Read(wxT("data_colors_count"), 16);
    data_colors_count = min(max(data_colors_count, 2), 64);
    data_colors_.resize(data_colors_count);
    cf->SetPath(wxT("/MainPlot/Colors"));
    set_bg_color(cfg_read_color(cf, wxT("bg"), wxColour(48, 48, 48)));
    data_colors_[0] = cfg_read_color(cf, wxT("data/0"), wxColour(0, 255, 0));
    for (int i = 1; i < (int) data_colors_.size(); i++)
        data_colors_[i] = cfg_read_color(cf, wxT("data/") + s2wx(S(i)),
                                         data_colors_[0]);
    inactiveDataCol = cfg_read_color(cf, wxT("inactive_data"),
                                                      wxColour(128, 128, 128));
    modelCol = cfg_read_color (cf, wxT("model"), wxColour(wxT("YELLOW")));
    bg_pointsCol = cfg_read_color(cf, wxT("BgPoints"), wxColour(wxT("RED")));
    //for (int i = 0; i < max_group_cols; i++)
    //    groupCol[i] = cfg_read_color(cf, wxString::Format(wxT("group/%i"), i),
    //                                 wxColour(173, 216, 230));
    peakCol[0] = cfg_read_color(cf, wxT("peak/0"), wxColour(255, 0, 0));
    for (int i = 0; i < max_peak_cols; i++)
        peakCol[i] = cfg_read_color(cf, wxString::Format(wxT("peak/%i"), i),
                                    peakCol[0]);

    cf->SetPath(wxT("/MainPlot/Visible"));
    peaks_visible_ = cfg_read_bool(cf, wxT("peaks"), true);
    plabels_visible_ = cfg_read_bool(cf, wxT("plabels"), false);
    //groups_visible_ = cfg_read_bool(cf, wxT("groups"), false);
    model_visible_ = cfg_read_bool(cf, wxT("model"), true);
    cf->SetPath(wxT("/MainPlot"));
    point_radius = cf->Read (wxT("point_radius"), 1);
    line_between_points = cfg_read_bool(cf,wxT("line_between_points"), false);
    draw_sigma = cfg_read_bool(cf,wxT("draw_sigma"), false);
    wxFont default_plabel_font(10, wxFONTFAMILY_DEFAULT,
                               wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    plabelFont = cfg_read_font(cf, wxT("plabelFont"), default_plabel_font);
    plabel_format_ = wx2s(cf->Read(wxT("plabel_format"), wxT("<area>")));
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
    cfg_write_font (cf, wxT("plabelFont"), plabelFont);
    cf->Write(wxT("plabel_format"), s2wx(plabel_format_));
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
    cfg_write_color(cf, wxT("data/0"), data_colors_[0]);
    for (size_t i = 1; i < data_colors_.size(); i++)
        if (data_colors_[i] != data_colors_[0])
            cfg_write_color(cf, wxT("data/") + s2wx(S(i)), data_colors_[i]);
    cfg_write_color (cf, wxT("inactive_data"), inactiveDataCol);
    cfg_write_color (cf, wxT("model"), modelCol);
    cfg_write_color (cf, wxT("BgPoints"), bg_pointsCol);
    //for (int i = 0; i < max_group_cols; i++)
    //    cfg_write_color(cf, wxString::Format(wxT("group/%i"), i), groupCol[i]);
    cfg_write_color(cf, wxT("peak/0"), peakCol[0]);
    for (int i = 1; i < max_peak_cols; i++)
        if (peakCol[i] != peakCol[0])
            cfg_write_color(cf, wxString::Format(wxT("peak/%i"),i), peakCol[i]);

    cf->SetPath(wxT("/MainPlot/Visible"));
    cf->Write (wxT("peaks"), peaks_visible_);
    cf->Write (wxT("plabels"), plabels_visible_);
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
    fp dummy;
    peak_menu.Enable(ID_peak_popup_guess,
                     ftk->get_function(over_peak_)->get_center(&dummy));
    PopupMenu (&peak_menu, event.GetX(), event.GetY());
}

void MainPlot::PeakInfo()
{
    if (over_peak_ >= 0)
        ftk->exec("info prop %" + ftk->get_function(over_peak_)->name);
}

void MainPlot::OnPeakDelete(wxCommandEvent&)
{
    if (over_peak_ >= 0)
        ftk->exec("delete %" + ftk->get_function(over_peak_)->name);
}

void MainPlot::OnPeakGuess(wxCommandEvent&)
{
    if (over_peak_ < 0)
        return;
    const Function* p = ftk->get_function(over_peak_);
    fp ctr;
    if (p->get_center(&ctr)) {
        fp plusmin = 0, fwhm, iw;
        if (p->get_fwhm(&fwhm))
            plusmin = fabs(fwhm);
        if (p->get_iwidth(&iw) && fabs(iw) > plusmin)
            plusmin = fabs(iw);
        plusmin = max(plusmin, 1.);
        char buffer[64];
        sprintf(buffer, " [%.12g:%.12g]", ctr-plusmin, ctr+plusmin);
        ftk->exec(frame->get_datasets() + "guess %" + p->name + " = "
                  + frame->get_guess_string(p->tp()->name)
                  + " [" + eS(ctr-plusmin) + ":" + eS(ctr+plusmin) + "]");
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
    if (pressed_mouse_button_) {
        fmd_->stop();
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
        overlay.draw();
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
    }
    else { //button not pressed
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
            default:
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
                    overlay.draw();
                }
            }
            else
                overlay.switch_mode(Overlay::kVLine);
        }
    }
    else if (pressed_mouse_button_ == 1 && mouse_op_ == kDragPeak
             && over_peak_ >= 0) {
        fmd_->move(event.ShiftDown(), X, Y, xs.valr(X), ys.valr(Y));
        frame->set_status_text(fmd_->get_status());
        draw_overlay_func(ftk->get_function(over_peak_), fmd_->get_values());
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
    const Function* f = ftk->get_function(n);
    string s = "%" + f->name + " " + f->tp()->name + " ";
    for (int i = 0; i < f->nv(); ++i)
        s += " " + f->get_param(i) + "=" + S(f->av()[i]);
    return s;
}

void MainPlot::look_for_peaktop(wxMouseEvent& event)
{
    int focused_data = frame->get_sidebar()->get_focused_data();
    const Model* model = ftk->get_model(focused_data);
    const vector<int>& idx = model->get_ff().idx;
    if (special_points.size() != idx.size())
        refresh();
    int n = get_special_point_at_pointer(event);
    int nearest = n == -1 ? -1 : idx[n];

    if (over_peak_ != nearest) {
        // change cursor, statusbar text and draw limits
        over_peak_ = nearest;
        frame->set_status_text(get_peak_description(nearest));
        set_mouse_mode(nearest == -1 ? basic_mode_ : mmd_peak);
    }
}

void MainPlot::cancel_mouse_press()
{
    if (pressed_mouse_button_) {
        overlay.switch_mode(get_normal_ovmode());
        fmd_->stop();
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
        overlay.draw();
        frame->plot_pane()->draw_vertical_lines(-1, -1, this);
        return;
    }

    pressed_mouse_button_ = event.GetButton();
    downX = event.GetX();
    downY = event.GetY();
    fp x = xs.valr(event.GetX());
    fp y = ys.valr(event.GetY());
    mouse_op_ = what_mouse_operation(event);
    if (mouse_op_ == kRectangularZoom) {
        SetCursor(wxCURSOR_MAGNIFIER);
        CaptureMouse();
        connect_esc_to_cancel(true);
        overlay.start_mode(Overlay::kRect, downX, downY);
        frame->set_status_text("Select second corner to zoom...");
    }
    else if (mouse_op_ == kShowPlotMenu) {
        show_popup_menu(event);
        pressed_mouse_button_ = 0;
    }
    else if (mouse_op_ == kShowPeakMenu) {
        show_peak_menu(event);
        pressed_mouse_button_ = 0;
    }
    else if (mouse_op_ == kVerticalZoom) {
        SetCursor(wxCURSOR_SIZENS);
        CaptureMouse();
        connect_esc_to_cancel(true);
        overlay.start_mode(Overlay::kHRange, downX, downY);
        frame->set_status_text("Select vertical span...");
    }
    else if (mouse_op_ == kHorizontalZoom) {
        SetCursor(wxCURSOR_SIZEWE);
        CaptureMouse();
        connect_esc_to_cancel(true);
        overlay.start_mode(Overlay::kVRange, downX, downY);
        frame->set_status_text("Select horizontal span...");
    }
    else if (mouse_op_ == kDragPeak) {
        frame->get_sidebar()->activate_function(over_peak_);
        SetCursor(wxCURSOR_SIZENWSE);
        connect_esc_to_cancel(true);
        const Function* p = ftk->get_function(over_peak_);
        fmd_->start(p, downX, downY, x, y);
        overlay.start_mode(Overlay::kFunction, downX, downY);
        draw_overlay_limits(p);
        frame->set_status_text("Moving %" + ftk->get_function(over_peak_)->name
                                + "...");
    }
    else if (mouse_op_ == kAddBgPoint) {
        bgm_->add_background_point(x, y);
        refresh();
    }
    else if (mouse_op_ == kDeleteBgPoint) {
        bgm_->rm_background_point(x);
        refresh();
    }
    else if (mouse_op_ == kAddPeakTriangle) {
        const Tplate* tp = ftk->get_tpm()->get_tp(frame->get_peak_type());
        if (tp == NULL)
            return;
        if (tp->peak_d) {
            func_draft_kind_ = kPeak;
            overlay.start_mode(Overlay::kPeakDraft, downX, ys.px(0));
        }
        else {
            func_draft_kind_ = kLinear;
            overlay.start_mode(Overlay::kLinearDraft, downX, downY);
        }
        SetCursor(wxCURSOR_SIZING);
        connect_esc_to_cancel(true);
        frame->set_status_text("Add drawed peak...");
    }
    else if (mouse_op_ == kAddPeakInRange) {
        SetCursor(wxCURSOR_SIZEWE);
        connect_esc_to_cancel(true);
        overlay.start_mode(Overlay::kVRange, downX, downY);
        frame->set_status_text("Select range to add a peak in it...");
    }
    else if (mouse_op_ == kActivateSpan || mouse_op_ == kDisactivateSpan ||
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
        }
        else
            act_str = "disactivate";
        string status_beginning;
        CaptureMouse();
        connect_esc_to_cancel(true);
        if (mouse_op_ == kActivateRect || mouse_op_ == kDisactivateRect) {
            SetCursor(wxCURSOR_SIZENWSE);
            overlay.start_mode(Overlay::kRect, downX, downY);
            status_beginning = "Select data in rectangle to ";
        }
        else {
            SetCursor(wxCURSOR_SIZEWE);
            overlay.start_mode(Overlay::kVRange, downX, downY);
            status_beginning = "Select data range to ";
        }
        frame->set_status_text(status_beginning + act_str + "...");
    }
    update_mouse_hints();
}

bool MainPlot::can_activate()
{
    vector<int> sel = frame->get_sidebar()->get_selected_data_indices();
    v_foreach (int, i, sel) {
        const Data* data = ftk->get_data(*i);
        // if data->is_empty() we allow to try disactivate data to let user
        // experiment with mouse right after launching the program
        if (data->is_empty() || data->get_n() != size(data->points()))
            return true;
    }
    return false;
}

static
void freeze_functions_in_range(double x1, double x2, bool freeze)
{
    string cmd;
    v_foreach (Function*, i, ftk->functions()) {
        fp ctr;
        if (!(*i)->get_center(&ctr))
            continue;
        if (!(x1 < ctr && ctr < x2))
            continue;
        for (int j = 0; j != (*i)->get_vars_count(); ++j) {
            const Variable* var = ftk->get_variable((*i)->get_var_idx(j));
            if (freeze && var->is_simple()) {
                cmd += "$" + var->name + "=" + eS(var->get_value()) + "; ";
            }
            else if (!freeze && var->is_constant()) {
                cmd += "$" + var->name + "=~" + eS(var->get_value()) + "; ";
            }
        }
    }
    if (!cmd.empty())
        ftk->exec(cmd);
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
        overlay.draw();
        return;
    }

    // zoom
    if (mouse_op_ == kRectangularZoom) {
        fp x1 = xs.valr(downX);
        fp x2 = xs.valr(event.GetX());
        fp y1 = ys.valr(downY);
        fp y2 = ys.valr(event.GetY());
        frame->change_zoom(RealRange(min(x1,x2), max(x1,x2)),
                           RealRange(min(y1,y2), max(y1,y2)));
    }
    else if (mouse_op_ == kVerticalZoom) {
        fp y1 = ys.valr(downY);
        fp y2 = ys.valr(event.GetY());
        frame->change_zoom(ftk->view.hor,
                           RealRange(min(y1,y2), max(y1,y2)));
    }
    else if (mouse_op_ == kHorizontalZoom) {
        fp x1 = xs.valr(downX);
        fp x2 = xs.valr(event.GetX());
        frame->change_zoom(RealRange(min(x1,x2), max(x1,x2)),
                           ftk->view.ver);
    }
    // drag peak
    else if (mouse_op_ == kDragPeak) {
        fmd_->stop();
        string cmd = fmd_->get_cmd();
        if (!cmd.empty())
            ftk->exec(cmd);
        else {
            overlay.draw();
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
        fp x1 = xs.valr(downX);
        fp x2 = xs.valr(event.GetX());
        if (x1 > x2)
            swap(x1, x2);
        string cond = eS(x1) + " < x and x < " + eS(x2);
        if (rect) {
            fp y1 = ys.valr(downY);
            fp y2 = ys.valr(event.GetY());
            cond += " and "
                    + eS(min(y1,y2)) + " < y and y < " + eS(max(y1,y2));
        }
        ftk->exec(frame->get_datasets() + c + " (" + cond + ")");

        if (auto_freeze_ && !rect)
            freeze_functions_in_range(x1, x2, !activate);
    }
    // add peak (left button)
    else if (mouse_op_ == kAddPeakTriangle) {
        add_peak_from_draft(event.GetX(), event.GetY());
    }
    // add peak (in range)
    else if (mouse_op_ == kAddPeakInRange) {
        fp x1 = xs.valr(downX);
        fp x2 = xs.valr(event.GetX());
        ftk->exec(frame->get_datasets() + "guess "
                  + frame->get_guess_string(frame->get_peak_type())
                  + " [" + eS(min(x1,x2)) + " : " + eS(max(x1,x2)) + "]");
    }
    else {
        ;// nothing - action done in OnButtonDown()
    }
}

void MainPlot::add_peak_from_draft(int X, int Y)
{
    string args;
    if (func_draft_kind_ == kLinear && downY != Y) {
        fp x1 = xs.valr(downX);
        fp y1 = ys.valr(downY);
        fp x2 = xs.valr(X);
        fp y2 = ys.valr(Y);
        fp m = (y2 - y1) / (x2 - x1);
        args = "slope=~" + eS(m) + ", intercept=~" + eS(y1-m*x1)
               + ", avgy=~" + eS((y1+y2)/2);
    }
    else {
        fp height = ys.valr(Y);
        fp center = xs.valr(downX);
        fp hwhm = fabs(center - xs.valr(X));
        fp area = 2 * height * hwhm;
        args = "height=~" + eS(height) + ", center=~" + eS(center)
                 + ", area=~" + eS(area);
        if (ftk->find_variable_nr("_hwhm") == -1)
            args += ", hwhm=~" + eS(hwhm);
    }
    string tail = "F += " + frame->get_guess_string(frame->get_peak_type());
    if (*(tail.end() - 1) == ')') {
        tail.resize(tail.size() - 1);
        tail += ", " + args + ")";
    }
    else
        tail += "(" + args + ")";
    string cmd;
    if (ftk->get_dm_count() == 1)
        cmd = tail;
    else {
        vector<int> sel = frame->get_sidebar()->get_selected_data_indices();
        cmd = "@" + join_vector(sel, "." + tail + "; @") + "." + tail;
    }
    ftk->exec(cmd);
}

static
void calculate_values(const vector<fp>& xx, vector<fp>& yy,
                      const Tplate::Ptr& tp, const vector<fp>& p_values)
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
        OpTree* tree = new OpTree(p_values[i]);
        variables[i] = new Variable("v" + S(i), vector<string>(),
                                    vector1<OpTree*>(tree));
        variables[i]->set_var_idx(dummy_vars);
        variables[i]->recalculate(dummy_vars, vector<fp>());
    }

    f->set_var_idx(variables);
    f->do_precomputations(variables);
    f->calculate_value(xx, yy);
    purge_all_elements(variables);
}

void MainPlot::draw_overlay_func(const Function* f, const vector<fp>& p_values)
{
    int n = GetClientSize().GetWidth();
    if (n <= 0)
        return;
    vector<fp> xx(n), yy(n, 0);
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
    fp x1, x2;
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
    data_colors_sc_ = new SpinCtrl(this, -1, mp_->data_colors_.size(), 2, 64);
    data_col_sizer->Add(data_colors_sc_, cl);
    data_color_combo_ = new MultiColorCombo(this, &mp_->get_bg_color(),
                                            mp_->data_colors_);
    data_color_combo_->SetSelection(0);
    data_col_sizer->Add(data_color_combo_, cl);
    gsizer->Add(data_col_sizer, cr);

    gsizer->Add(new wxStaticText(this, -1, wxT("inactive data")), cr);
    inactive_cp_ = new wxColourPickerCtrl(this, -1, mp_->inactiveDataCol);
    gsizer->Add(inactive_cp_, cl);

    model_cb_ = new wxCheckBox(this, -1, wxT("model (sum)"));
    model_cb_->SetValue(mp_->model_visible_);
    gsizer->Add(model_cb_, cr);
    model_cp_ = new wxColourPickerCtrl(this, -1, mp_->modelCol);
    gsizer->Add(model_cp_, cl);

    func_cb_ = new wxCheckBox(this, -1, wxT("functions"));
    func_cb_->SetValue(mp_->peaks_visible_);
    gsizer->Add(func_cb_, cr);
    func_cp_ = new wxColourPickerCtrl(this, -1, mp_->peakCol[0]);
    gsizer->Add(func_cp_, cl);

    labels_cb_ = new wxCheckBox(this, -1, wxT("function labels"));
    labels_cb_->SetValue(mp_->plabels_visible_);
    gsizer->Add(labels_cb_, cr);
    label_fp_ = new wxFontPickerCtrl(this, -1, mp_->plabelFont);
    gsizer->Add(label_fp_, cl);

    gsizer->Add(new wxStaticText(this, -1, wxEmptyString), cr);
    vertical_labels_cb_ = new wxCheckBox(this, -1, wxT("vertical"));
    vertical_labels_cb_->SetValue(mp_->vertical_plabels_);
    gsizer->Add(vertical_labels_cb_, cl);

    gsizer->Add(new wxStaticText(this, -1, wxEmptyString), cr);
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
    vertical_labels_cb_->SetValue(mp_->vertical_plabels_);
    gsizer->Add(label_combo_, cl);

    gsizer->Add(new wxStaticText(this, -1, wxT("axis & tics color")), cr);
    axis_cp_ = new wxColourPickerCtrl(this, -1, mp_->xAxisCol);
    gsizer->Add(axis_cp_, cl);

    gsizer->Add(new wxStaticText(this, -1, wxT("tic label font")), cr);
    tics_fp_ = new wxFontPickerCtrl(this, -1, mp_->ticsFont);
    gsizer->Add(tics_fp_, cl);

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
    x_max_tics_sc_ = new wxSpinCtrl(this, -1, wxT("7"),
                                    wxDefaultPosition, wxSize(50, -1),
                                    wxSP_ARROW_KEYS, 1, 30, 7);
    x_max_tics_sc_->SetValue(mp_->x_max_tics);
    xmt_sizer->Add(x_max_tics_sc_, 0, wxALL, 5);
    xsizer_t->Add(xmt_sizer);
    wxBoxSizer *xts_sizer = new wxBoxSizer(wxHORIZONTAL);
    xts_sizer->Add(new wxStaticText(this, -1, wxT("length of tics")),
                  0, wxALL|wxALIGN_CENTRE_VERTICAL, 5);
    x_tic_size_sc_ = new wxSpinCtrl(this, -1, wxT("4"),
                                    wxDefaultPosition, wxSize(50, -1),
                                    wxSP_ARROW_KEYS, -10, 20, 4);
    x_tic_size_sc_->SetValue(mp_->x_tic_size);
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
    y_max_tics_sc_ = new wxSpinCtrl(this, -1, wxT("7"),
                                    wxDefaultPosition, wxSize(50, -1),
                                    wxSP_ARROW_KEYS, 1, 30, 7);
    y_max_tics_sc_->SetValue(mp_->y_max_tics);
    ymt_sizer->Add(y_max_tics_sc_, 0, wxALL, 5);
    ysizer_t->Add(ymt_sizer);
    wxBoxSizer *yts_sizer = new wxBoxSizer(wxHORIZONTAL);
    yts_sizer->Add(new wxStaticText(this, -1, wxT("length of tics")),
                  0, wxALL|wxALIGN_CENTRE_VERTICAL, 5);
    y_tic_size_sc_ = new wxSpinCtrl(this, -1, wxT("4"),
                                     wxDefaultPosition, wxSize(50, -1),
                                     wxSP_ARROW_KEYS, 1, 20, 4);
    y_tic_size_sc_->SetValue(mp_->y_tic_size);
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
    Connect(func_cb_->GetId(), wxEVT_COMMAND_CHECKBOX_CLICKED,
            wxCommandEventHandler(MainPlotConfDlg::OnFuncCheckbox));
    Connect(labels_cb_->GetId(), wxEVT_COMMAND_CHECKBOX_CLICKED,
            wxCommandEventHandler(MainPlotConfDlg::OnLabelsCheckbox));
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
    }
    else if (id == inactive_cp_->GetId())
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


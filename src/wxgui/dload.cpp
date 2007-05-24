// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License version 2
// $Id$

/// In this file:
///  Custom Data Load Dialog (DLoadDlg) and helpers

#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <fstream>
#include <vector>
#include <wx/statline.h>
#include <wx/file.h>
#include <wx/filename.h>

#include "dload.h" 
#include "gui.h"  // frame->add_recent_data_file(get_filename()
#include "plot.h" // scale_tics_step()
#include "../data.h"
#include "../logic.h" 
#include "../settings.h"
#include "../common.h" //iround

using namespace std;

enum {
    ID_DXLOAD_STDDEV_CB     =28000,
    ID_DXLOAD_COLX                ,
    ID_DXLOAD_COLY                ,
    ID_DXLOAD_SDS                 ,
    ID_DXLOAD_HTITLE              ,
    ID_DXLOAD_AUTO_TEXT           ,
    ID_DXLOAD_AUTO_PLOT           ,
    ID_DXLOAD_OPENHERE            ,
    ID_DXLOAD_OPENNEW             ,
    ID_DXLOAD_FN                  
};


class PreviewPlot : public BufferedPanel
{
public:
    auto_ptr<Data> data;

    PreviewPlot(wxWindow* parent)
        : BufferedPanel(parent), data(new Data(ftk)) 
                  { backgroundCol = *wxBLACK; }
    void OnPaint(wxPaintEvent &event);
    void draw(wxDC &dc, bool);

private:
    double xScale, yScale;
    int H;
    int getX(double x) { return iround(x * xScale); }
    int getY(double y) { return H - iround(y * yScale); }
    void prepare_scaling(wxDC &dc);
    void draw_scale(wxDC &dc);

    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE (PreviewPlot, wxPanel)
    EVT_PAINT (PreviewPlot::OnPaint)
END_EVENT_TABLE()

void PreviewPlot::OnPaint(wxPaintEvent&)
{
    buffered_draw();
}

void PreviewPlot::draw(wxDC &dc, bool)
{
    if (data->is_empty())
        return;
    prepare_scaling(dc);
    draw_scale(dc);
    vector<Point> const& pp = data->points();
    dc.SetPen(*wxGREEN_PEN);
    for (vector<Point>::const_iterator i = pp.begin(); i != pp.end(); ++i)
        dc.DrawPoint(int(i->x * xScale), H - int(i->y * yScale));
}

void PreviewPlot::prepare_scaling(wxDC &dc)
{
    double const margin = 0.1;
    double dx = data->get_x_max() - data->get_x_min();
    double dy = data->get_y_max() - data->get_y_min();
    int W = GetClientSize().GetWidth();
    H = GetClientSize().GetHeight();
    xScale = (1 - 2 * margin) *  W / dx;
    yScale = (1 - 2 * margin) * H / dy;
    dc.SetDeviceOrigin(-iround(data->get_x_min() * xScale - margin * W),  
                       iround(data->get_y_min() * yScale - margin * H));
}

void PreviewPlot::draw_scale(wxDC &dc)
{
    dc.SetPen(*wxWHITE_PEN);
    dc.SetTextForeground(*wxWHITE);
    dc.SetFont(*wxSMALL_FONT);  
    vector<double> minors;
    vector<double> tics 
        = scale_tics_step(data->get_x_min(), data->get_x_max(), 4, minors);
    for (vector<double>::const_iterator i = tics.begin(); i != tics.end(); ++i){
        int X = getX(*i);
        wxString label = s2wx(S(*i));
        if (label == wxT("-0")) 
            label = wxT("0");
        wxCoord tw, th;
        dc.GetTextExtent (label, &tw, &th);
        int Y = dc.DeviceToLogicalY(H - th - 2);
        dc.DrawText (label, X - tw/2, Y + 1);
        dc.DrawLine (X, Y, X, Y - 4);
    }

    tics = scale_tics_step(data->get_y_min(), data->get_y_max(), 4, minors);
    for (vector<double>::const_iterator i = tics.begin(); i != tics.end(); ++i){
        int Y = getY(*i);
        wxString label = s2wx(S(*i));
        if (label == wxT("-0")) 
            label = wxT("0");
        wxCoord tw, th;
        dc.GetTextExtent (label, &tw, &th);
        dc.DrawText (label, dc.DeviceToLogicalX(5), Y - th/2);
        dc.DrawLine (dc.DeviceToLogicalX(0), Y, dc.DeviceToLogicalX(4), Y);
    }
}


BEGIN_EVENT_TABLE(DLoadDlg, wxDialog)
    EVT_CHECKBOX    (ID_DXLOAD_STDDEV_CB, DLoadDlg::OnStdDevCheckBox)
    EVT_CHECKBOX    (ID_DXLOAD_HTITLE,    DLoadDlg::OnHTitleCheckBox)
    EVT_CHECKBOX    (ID_DXLOAD_AUTO_TEXT, DLoadDlg::OnAutoTextCheckBox)
    EVT_CHECKBOX    (ID_DXLOAD_AUTO_PLOT, DLoadDlg::OnAutoPlotCheckBox)
    EVT_SPINCTRL    (ID_DXLOAD_COLX,      DLoadDlg::OnColumnChanged)
    EVT_SPINCTRL    (ID_DXLOAD_COLY,      DLoadDlg::OnColumnChanged)
    EVT_BUTTON      (wxID_CLOSE,          DLoadDlg::OnClose)
    EVT_BUTTON      (ID_DXLOAD_OPENHERE,  DLoadDlg::OnOpenHere)
    EVT_BUTTON      (ID_DXLOAD_OPENNEW,   DLoadDlg::OnOpenNew)
    EVT_TREE_SEL_CHANGED (-1,             DLoadDlg::OnPathSelectionChanged)
    EVT_TEXT_ENTER(ID_DXLOAD_FN,          DLoadDlg::OnPathTextChanged)
END_EVENT_TABLE()

DLoadDlg::DLoadDlg (wxWindow* parent, wxWindowID id, int n, Data* data)
    : wxDialog(parent, id, wxT("Data load (custom)"), 
               wxDefaultPosition, wxSize(600, 500), 
               wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER),
      data_nr(n), initialized(false)
{


    // +------------------------------------------+
    // |                  |                       |
    // |                  |  rupper_panel         |
    // |                  |                       |
    // | left_panel       |                       |
    // |                  +-----------------------+
    // |                  |                       |
    // |                  |  rbottom_panel        |
    // |                  |                       |
    // |                  |                       |
    // +------------------------------------------+
    // |     buttons here, directly on the *this  |
    // +------------------------------------------+

    wxBoxSizer *top_sizer = new wxBoxSizer(wxVERTICAL);
    splitter = new ProportionalSplitter(this, -1, 0.5);
    left_panel = new wxPanel(splitter, -1);
    wxBoxSizer *left_sizer = new wxBoxSizer(wxVERTICAL);
    right_splitter = new ProportionalSplitter(splitter, -1, 0.5);
    rupper_panel = new wxPanel(right_splitter, -1);
    wxBoxSizer *rupper_sizer = new wxBoxSizer(wxVERTICAL);
    rbottom_panel = new wxPanel(right_splitter, -1);
    wxBoxSizer *rbottom_sizer = new wxBoxSizer(wxVERTICAL);

    // ----- left panel -----
    dir_ctrl = new wxGenericDirCtrl(left_panel, -1, wxDirDialogDefaultFolderStr,
                       wxDefaultPosition, wxDefaultSize,
// On MSW wxGenericDirCtrl with filteres vanishes 
//#ifndef __WXMSW__
//                       wxDIRCTRL_SHOW_FILTERS,
//#else
                       0,
//#endif
                       // multiple wildcards, eg. 
                       // |*.dat;*.DAT;*.xy;*.XY;*.fio;*.FIO
                       // are not supported by wxGenericDirCtrl  
                       wxT("all files (*)|*")
                       wxT("|ASCII x y files (*)|*" )
                       wxT("|rit files (*.rit)|*.rit")
                       wxT("|cpi files (*.cpi)|*.cpi")
                       wxT("|mca files (*.mca)|*.mca")
                       wxT("|Siemens/Bruker (*.raw)|*.raw"));
    left_sizer->Add(dir_ctrl, 1, wxALL|wxEXPAND, 5);
    wxFileName path = s2wx(data->get_filename());
    path.Normalize();
    dir_ctrl->SetPath(path.GetFullPath()); 
    filename_tc = new KFTextCtrl(left_panel, ID_DXLOAD_FN, path.GetFullPath());
    left_sizer->Add (filename_tc, 0, wxALL|wxEXPAND, 5);
                                     

    // selecting columns
    columns_panel = new wxPanel (left_panel, -1);
    wxStaticBoxSizer *h2a_sizer = new wxStaticBoxSizer(wxHORIZONTAL, 
                    columns_panel, wxT("Select columns (0 for point index):"));
    h2a_sizer->Add (new wxStaticText (columns_panel, -1, wxT("x")), 
                    0, wxALL|wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL, 5);
    x_column = new SpinCtrl (columns_panel, ID_DXLOAD_COLX, 1, 0, 99, 50);
    h2a_sizer->Add (x_column, 0, wxALL|wxALIGN_LEFT, 5);
    h2a_sizer->Add (new wxStaticText (columns_panel, -1, wxT("y")), 
                    0, wxALL|wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL, 5);
    y_column = new SpinCtrl (columns_panel, ID_DXLOAD_COLY, 2, 0, 99, 50);
    h2a_sizer->Add (y_column, 0, wxALL|wxALIGN_LEFT, 5);
    std_dev_cb = new wxCheckBox(columns_panel, ID_DXLOAD_STDDEV_CB, 
                                wxT("std.dev."));
    std_dev_cb->SetValue(false);
    h2a_sizer->Add(std_dev_cb, 0, wxALL|wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL,5);
    s_column = new SpinCtrl(columns_panel, -1, 3, 1, 99, 50);
    h2a_sizer->Add(s_column, 0, wxALL|wxALIGN_LEFT, 5);
    columns_panel->SetSizer(h2a_sizer);
    left_sizer->Add (columns_panel, 0, wxALL|wxEXPAND, 5);
    if (data->get_given_cols().size() > 1) {
        x_column->SetValue(data->get_given_cols()[0]);
        y_column->SetValue(data->get_given_cols()[1]);
        if (data->get_given_cols().size() > 2) {
            std_dev_cb->SetValue(true);
            s_column->SetValue(data->get_given_cols()[2]);
        }
    }

    bool def_sqrt = (ftk->get_settings()->getp("data-default-sigma") == "sqrt");
    sd_sqrt_cb = new wxCheckBox(left_panel, ID_DXLOAD_SDS, 
                                wxT("set std. dev. as max(sqrt(y), 1.0)"));
    sd_sqrt_cb->SetValue(def_sqrt);
    left_sizer->Add (sd_sqrt_cb, 0, wxALL|wxEXPAND, 5);

    wxStaticBoxSizer *dt_sizer = new wxStaticBoxSizer(wxVERTICAL, 
                                    left_panel, wxT("Data title (optional):"));
    htitle_cb = new wxCheckBox(left_panel, ID_DXLOAD_HTITLE, 
                               wxT("get from 1st line"));
    dt_sizer->Add(htitle_cb, 0, wxALL, 5);
    title_tc = new wxTextCtrl(left_panel, -1, wxT(""));
    dt_sizer->Add(title_tc, 0, wxALL|wxEXPAND, 5);
    left_sizer->Add (dt_sizer, 0, wxALL|wxEXPAND, 5);

    StdDevCheckBoxChanged();

    // ----- right upper panel -----
    text_preview =  new wxTextCtrl(rupper_panel, -1, wxT(""), 
                                   wxDefaultPosition, wxDefaultSize,
                                   wxTE_RICH|wxTE_READONLY|wxTE_MULTILINE);
    rupper_sizer->Add(text_preview, 1, wxEXPAND|wxALL, 5);
    auto_text_cb = new wxCheckBox(rupper_panel, ID_DXLOAD_AUTO_TEXT, 
                                  wxT("view the first 64kB of file as text"));
    auto_text_cb->SetValue(false);
    rupper_sizer->Add(auto_text_cb, 0, wxALL, 5);

    // ----- right bottom panel -----
    plot_preview = new PreviewPlot(rbottom_panel);
    rbottom_sizer->Add(plot_preview, 1, wxEXPAND|wxALL, 5);
    auto_plot_cb = new wxCheckBox(rbottom_panel, ID_DXLOAD_AUTO_PLOT, 
                                  wxT("plot"));
    auto_plot_cb->SetValue(false);
    rbottom_sizer->Add(auto_plot_cb, 0, wxALL, 5);

    // ------ finishing layout (+buttons) -----------
    left_panel->SetSizerAndFit(left_sizer);
    rupper_panel->SetSizerAndFit(rupper_sizer);
    rbottom_panel->SetSizerAndFit(rbottom_sizer);
    splitter->SplitVertically(left_panel, right_splitter);
    right_splitter->SplitHorizontally(rupper_panel, rbottom_panel);
    top_sizer->Add(splitter, 1, wxEXPAND);

    top_sizer->Add (new wxStaticLine(this, -1), 0, wxEXPAND|wxLEFT|wxRIGHT, 5);
    wxBoxSizer *button_sizer = new wxBoxSizer(wxHORIZONTAL);
    open_here = new wxButton(this, ID_DXLOAD_OPENHERE, 
                             s2wx("&Replace dataset @"+S(data_nr)));
    open_new = new wxButton(this, ID_DXLOAD_OPENNEW, 
                            wxT("&Open as new dataset"));
    button_sizer->Add(open_here, 0, wxALL, 5);
    button_sizer->Add(open_new, 0, wxALL, 5);
    button_sizer->Add(new wxButton(this, wxID_CLOSE, wxT("&Close")), 
                      0, wxALL, 5);
    top_sizer->Add(button_sizer, 0, wxALL|wxALIGN_CENTER, 0);
    initialized = true;
    SetSizer(top_sizer);
    on_filter_change();
}

void DLoadDlg::StdDevCheckBoxChanged()
{
    bool v = std_dev_cb->GetValue();
    s_column->Enable(v);
    sd_sqrt_cb->Enable(!v);
}

void DLoadDlg::OnHTitleCheckBox (wxCommandEvent& event)
{
    if (event.IsChecked()) 
        update_title_from_file();
    title_tc->Enable(!event.IsChecked());
}

void DLoadDlg::update_title_from_file()
{
    assert (htitle_cb->GetValue());
    ifstream f(dir_ctrl->GetFilePath().fn_str());
    int col = columns_panel->IsEnabled() ? y_column->GetValue() : 0;
    string title = Data::read_one_line_as_title(f, col);
    title_tc->SetValue(s2wx(title));
}

void DLoadDlg::OnAutoTextCheckBox (wxCommandEvent& event)
{
    if (event.IsChecked())
        update_text_preview();
    else
        text_preview->Clear();
}

void DLoadDlg::OnAutoPlotCheckBox (wxCommandEvent&)
{
    update_plot_preview();
}

void DLoadDlg::OnColumnChanged (wxSpinEvent&)
{
    if (auto_plot_cb->GetValue()) 
        update_plot_preview();
    if (htitle_cb->GetValue()) 
        update_title_from_file();
}

void DLoadDlg::OnClose (wxCommandEvent&)
{
    close_it(this);
}

void DLoadDlg::OnOpenHere (wxCommandEvent&)
{
    ftk->exec(get_command("@" + S(data_nr), data_nr));
    frame->add_recent_data_file(get_filename());
}

void DLoadDlg::OnOpenNew (wxCommandEvent&)
{
    int d_nr = ftk->get_ds_count();
    if (d_nr == 1 && !ftk->get_ds(0)->has_any_info())
        d_nr = 0; // special case, @+ will not add new data slot
    ftk->exec(get_command("@+", d_nr));
    frame->add_recent_data_file(get_filename());
}

void DLoadDlg::update_text_preview()
{
    static char buffer[65536];
    int buf_size = sizeof(buffer)/sizeof(buffer[0]);
    fill(buffer, buffer+buf_size, 0);
    wxString path = dir_ctrl->GetFilePath();
    text_preview->Clear();
    if (wxFileExists(path)) {
        wxFile(path).Read(buffer, buf_size-1);
        text_preview->SetValue(pchar2wx(buffer));
    }
}

void DLoadDlg::update_plot_preview()
{
    if (auto_plot_cb->GetValue()) {
        std::vector<int> cols;
        if (columns_panel->IsEnabled()) {
            cols.push_back(x_column->GetValue());
            cols.push_back(y_column->GetValue());
        }
        ftk->get_ui()->keep_quiet = true;
        try {
            plot_preview->data->load_file(wx2s(dir_ctrl->GetFilePath()), 
                                          "", cols, true);
        } catch (ExecuteError&) {
            plot_preview->data->clear();
        }
        ftk->get_ui()->keep_quiet = false;
    }
    else {
        plot_preview->data->clear();
    }
    plot_preview->Refresh();
}

void DLoadDlg::on_filter_change()
{
    int idx = dir_ctrl->GetFilterIndex();
    wxString path = dir_ctrl->GetFilePath();
    bool is_text = (idx == 0 && !path.IsEmpty() 
                             && Data::guess_file_type(wx2s(path)) == "text")
                    || idx == 1; 
    enable_text_options(is_text);
}

void DLoadDlg::enable_text_options(bool is_text)
{
    columns_panel->Enable(is_text);
    htitle_cb->Enable(is_text);
    if (!is_text && htitle_cb->IsChecked())
        htitle_cb->SetValue(false);
    sd_sqrt_cb->Enable(!(is_text  && std_dev_cb->GetValue()));
}

void DLoadDlg::on_path_change()
{
    if (!initialized)
        return;
    wxString path = dir_ctrl->GetFilePath();
    filename_tc->SetValue(path);
    if (dir_ctrl->GetFilterIndex() == 0) { // all files
        bool is_text = !path.IsEmpty() 
                             && Data::guess_file_type(wx2s(path)) == "text"; 
        enable_text_options(is_text);
    }
    open_here->Enable(!path.IsEmpty());
    open_new->Enable(!path.IsEmpty());
    if (auto_text_cb->GetValue())
        update_text_preview();
    if (auto_plot_cb->GetValue()) 
        update_plot_preview();
    if (htitle_cb->GetValue()) 
        update_title_from_file();
}

void DLoadDlg::OnPathTextChanged(wxCommandEvent&)
{
    wxString path = filename_tc->GetValue().Trim();
    if (wxDirExists(path) || wxFileExists(path)) {
        dir_ctrl->SetPath(path);
        on_path_change();
    }
    else
        filename_tc->SetValue(dir_ctrl->GetFilePath());
}

string DLoadDlg::get_filename()
{
    return wx2s(filename_tc->GetValue());
}

string DLoadDlg::get_command(string const& ds, int d_nr)
{
    string cmd;

    string cols;
    if (columns_panel->IsEnabled()) { // a:b[:c]
        cols = " " + S(x_column->GetValue()) + "," + S(y_column->GetValue());
        if (std_dev_cb->GetValue())
            cols += S(",") + S(s_column->GetValue());
    }

    string filetype;
    if (htitle_cb->IsChecked())
        filetype = " htext";

    bool def_sqrt = (ftk->get_settings()->getp("data-default-sigma") == "sqrt");
    bool set_sqrt = sd_sqrt_cb->GetValue();
    bool sigma_in_file = (columns_panel->IsEnabled() && std_dev_cb->GetValue());
    if (!sigma_in_file && set_sqrt != def_sqrt) {
        if (set_sqrt)
            cmd = "with data-default-sigma=sqrt ";
        else
            cmd = "with data-default-sigma=one ";
    }

    cmd += ds + " < '" + get_filename() + "'" + filetype + cols;

    if (title_tc->IsEnabled()) {
        wxString t = title_tc->GetValue().Trim();
        if (!t.IsEmpty())
            cmd += "; @" + S(d_nr) + ".title = '" + wx2s(t) + "'";
    }

    return cmd;
}


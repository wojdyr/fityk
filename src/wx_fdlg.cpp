// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$
// custom load/save/import/export/print related dialogs

// wxwindows headers, see wxwindows samples for description
#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif


#include <istream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <utility>
#include <map>
#include <wx/bmpbuttn.h>
#include <wx/statline.h>
#include <wx/splitter.h>
#include <wx/notebook.h>
#include <wx/file.h>
#include <wx/timer.h>
#include <wx/paper.h>
//TODO
#include <wx/generic/printps.h>
#include <wx/generic/prntdlgg.h>
#include "common.h"
#include "wx_common.h"
#include "wx_fdlg.h"
#include "wx_gui.h"
#include "wx_pane.h"
#include "wx_plot.h"
#include "wx_mplot.h"
#include "data.h"
#include "sum.h"
#include "ui.h"
#include "datatrans.h"
#include "logic.h"
#include "cmd.h"

#include "img/open.xpm"
#include "img/exec_selected.xpm"
#include "img/exec_down.xpm"
#include "img/save.xpm"
#include "img/save_as.xpm"
#include "img/close.xpm"

using namespace std;


enum {
    ID_DXLOAD_STDDEV_CB     =28000,
    ID_DXLOAD_COLX                ,
    ID_DXLOAD_COLY                ,
    ID_DXLOAD_HTITLE              ,
    ID_DXLOAD_AUTO_TEXT           ,
    ID_DXLOAD_AUTO_PLOT           ,
    ID_DXLOAD_OPENHERE            ,
    ID_DXLOAD_OPENNEW             ,
    ID_DED_RADIO                  ,
    ID_DED_INACT_CB               ,
    ID_DED_TEXT                   ,

    ID_SE_OPEN                    ,
    ID_SE_EXECSEL                 ,
    ID_SE_EXECDOWN                ,
    ID_SE_SAVE                    ,
    ID_SE_SAVE_AS                 ,
    ID_SE_CLOSE                   

};


class PreviewPlot : public wxPanel
{
public:
    auto_ptr<Data> data;

    PreviewPlot(wxWindow* parent, wxWindowID id, FDXLoadDlg* dlg_)
        : wxPanel(parent, id), data(new Data), dlg(dlg_) {}
    void OnPaint(wxPaintEvent &event);

private:
    FDXLoadDlg* dlg;
    fp xScale, yScale;
    int H;
    int getX(fp x) { return iround(x * xScale); }
    int getY(fp y) { return H - iround(y * yScale); }
    void prepare_scaling(wxDC &dc);
    void draw_scale(wxDC &dc);

    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE (PreviewPlot, wxPanel)
    EVT_PAINT (PreviewPlot::OnPaint)
END_EVENT_TABLE()

void PreviewPlot::OnPaint(wxPaintEvent &WXUNUSED(event))
{
    wxPaintDC dc(this);
    dc.SetLogicalFunction(wxCOPY);
    dc.SetBackground(*wxBLACK_BRUSH);
    dc.Clear();
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
    fp const margin = 0.1;
    fp dx = data->get_x_max() - data->get_x_min();
    fp dy = data->get_y_max() - data->get_y_min();
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
    vector<fp> minors;
    vector<fp> tics 
        = scale_tics_step(data->get_x_min(), data->get_x_max(), 4, minors);
    for (vector<fp>::const_iterator i = tics.begin(); i != tics.end(); ++i) {
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
    for (vector<fp>::const_iterator i = tics.begin(); i != tics.end(); ++i){
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


BEGIN_EVENT_TABLE(FDXLoadDlg, wxDialog)
    EVT_CHECKBOX    (ID_DXLOAD_STDDEV_CB, FDXLoadDlg::OnStdDevCheckBox)
    EVT_CHECKBOX    (ID_DXLOAD_HTITLE,    FDXLoadDlg::OnHTitleCheckBox)
    EVT_CHECKBOX    (ID_DXLOAD_AUTO_TEXT, FDXLoadDlg::OnAutoTextCheckBox)
    EVT_CHECKBOX    (ID_DXLOAD_AUTO_PLOT, FDXLoadDlg::OnAutoPlotCheckBox)
    EVT_SPINCTRL    (ID_DXLOAD_COLX,      FDXLoadDlg::OnColumnChanged)
    EVT_SPINCTRL    (ID_DXLOAD_COLY,      FDXLoadDlg::OnColumnChanged)
    EVT_BUTTON      (wxID_CLOSE,          FDXLoadDlg::OnClose)
    EVT_BUTTON      (ID_DXLOAD_OPENHERE,  FDXLoadDlg::OnOpenHere)
    EVT_BUTTON      (ID_DXLOAD_OPENNEW,   FDXLoadDlg::OnOpenNew)
    EVT_TREE_SEL_CHANGED (-1,             FDXLoadDlg::OnPathSelectionChanged)
END_EVENT_TABLE()

FDXLoadDlg::FDXLoadDlg (wxWindow* parent, wxWindowID id, int n, Data* data)
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
#ifndef __WXMSW__
                       wxDIRCTRL_SHOW_FILTERS,
#else
                       0,
#endif
                       // multiple wildcards, eg. 
                       // |*.dat;*.DAT;*.xy;*.XY;*.fio;*.FIO
                       // are not supported by wxGenericDirCtrl  
                       wxT("all files (*)|*"
                           "|ASCII x y files (*)|*" 
                           "|rit files (*.rit)|*.rit"
                           "|cpi files (*.cpi)|*.cpi"
                           "|mca files (*.mca)|*.mca"
                           "|Siemens/Bruker (*.raw)|*.raw"));
    left_sizer->Add(dir_ctrl, 1, wxALL|wxEXPAND, 5);
    wxFileName path = s2wx(data->get_filename());
    path.Normalize();
    dir_ctrl->SetPath(path.GetFullPath()); 
    filename_tc = new wxTextCtrl (left_panel, -1, wxT(""), 
                                  wxDefaultPosition, wxDefaultSize,
                                  wxTE_READONLY);
    left_sizer->Add (filename_tc, 0, wxALL|wxEXPAND, 5);
                                     

    // selecting columns
    columns_panel = new wxPanel (left_panel, -1);
    wxStaticBoxSizer *h2a_sizer = new wxStaticBoxSizer(wxHORIZONTAL, 
                                        columns_panel, wxT("Select columns:"));
    h2a_sizer->Add (new wxStaticText (columns_panel, -1, wxT("x")), 
                    0, wxALL|wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL, 5);
    x_column = new wxSpinCtrl (columns_panel, ID_DXLOAD_COLX, wxT("1"), 
                               wxDefaultPosition, wxSize(50, -1), 
                               wxSP_ARROW_KEYS, 1, 99, 1);
    h2a_sizer->Add (x_column, 0, wxALL|wxALIGN_LEFT, 5);
    h2a_sizer->Add (new wxStaticText (columns_panel, -1, wxT("y")), 
                    0, wxALL|wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL, 5);
    y_column = new wxSpinCtrl (columns_panel, ID_DXLOAD_COLY, wxT("2"),
                               wxDefaultPosition, wxSize(50, -1), 
                               wxSP_ARROW_KEYS, 1, 99, 2);
    h2a_sizer->Add (y_column, 0, wxALL|wxALIGN_LEFT, 5);
    std_dev_cb = new wxCheckBox(columns_panel, ID_DXLOAD_STDDEV_CB, 
                                wxT("std.dev."));
    std_dev_cb->SetValue (false);
    h2a_sizer->Add(std_dev_cb, 0, wxALL|wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL,5);
    s_column = new wxSpinCtrl (columns_panel, -1, wxT("3"),
                               wxDefaultPosition, wxSize(50, -1), 
                               wxSP_ARROW_KEYS, 1, 99, 3);
    h2a_sizer->Add (s_column, 0, wxALL|wxALIGN_LEFT, 5);
    columns_panel->SetSizer(h2a_sizer);
    left_sizer->Add (columns_panel, 0, wxALL|wxEXPAND, 5);
    OnStdDevCheckBox(dummy_cmd_event);

    wxStaticBoxSizer *dt_sizer = new wxStaticBoxSizer(wxVERTICAL, 
                                    left_panel, wxT("Data title (optional):"));
    htitle_cb = new wxCheckBox(left_panel, ID_DXLOAD_HTITLE, 
                               wxT("get from 1st line"));
    dt_sizer->Add(htitle_cb, 0, wxALL, 5);
    title_tc = new wxTextCtrl(left_panel, -1, wxT(""));
    dt_sizer->Add(title_tc, 0, wxALL|wxEXPAND, 5);
    left_sizer->Add (dt_sizer, 0, wxALL|wxEXPAND, 5);

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
    plot_preview = new PreviewPlot(rbottom_panel, -1, this);
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

void FDXLoadDlg::OnStdDevCheckBox (wxCommandEvent& WXUNUSED(event))
{
    s_column->Enable(std_dev_cb->GetValue());
}

void FDXLoadDlg::OnHTitleCheckBox (wxCommandEvent& event)
{
    if (event.IsChecked()) 
        update_title_from_file();
    title_tc->Enable(!event.IsChecked());
}

void FDXLoadDlg::update_title_from_file()
{
    assert (htitle_cb->GetValue());
    wxString title;
    ifstream f(dir_ctrl->GetFilePath().fn_str());
    if (f) {
        string s;
        getline(f, s);
        if (columns_panel->IsEnabled()) {
            int col = y_column->GetValue() - 1;
            vector<string> v = split_string(s, " \t");
            if (size(v) > col) 
                title = s2wx(v[col]);
        }
        else
            title = s2wx(s).Trim();
    }
    title_tc->SetValue(title);
}

void FDXLoadDlg::OnAutoTextCheckBox (wxCommandEvent& event)
{
    if (event.IsChecked())
        update_text_preview();
    else
        text_preview->Clear();
}

void FDXLoadDlg::OnAutoPlotCheckBox (wxCommandEvent& WXUNUSED(event))
{
    update_plot_preview();
}

void FDXLoadDlg::OnColumnChanged (wxSpinEvent& WXUNUSED(event))
{
    if (auto_plot_cb->GetValue()) 
        update_plot_preview();
    if (htitle_cb->GetValue()) 
        update_title_from_file();
}

void FDXLoadDlg::OnClose (wxCommandEvent& event)
{
    OnCancel(event);
}

void FDXLoadDlg::OnOpenHere (wxCommandEvent& WXUNUSED(event))
{
    exec_command("@" + S(data_nr) + " <" + get_command_tail(data_nr));
    frame->add_recent_data_file(get_filename());
}

void FDXLoadDlg::OnOpenNew (wxCommandEvent& WXUNUSED(event))
{
    exec_command("@+ <" + get_command_tail(AL->get_ds_count()));
    frame->add_recent_data_file(get_filename());
}

void FDXLoadDlg::update_text_preview()
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

void FDXLoadDlg::update_plot_preview()
{
    if (auto_plot_cb->GetValue()) {
        std::vector<int> cols;
        if (columns_panel->IsEnabled()) {
            cols.push_back(x_column->GetValue());
            cols.push_back(y_column->GetValue());
        }
        getUI()->keep_quiet = true;
        try {
            plot_preview->data->load_file(wx2s(dir_ctrl->GetFilePath()), 
                                          "", cols, true);
        } catch (ExecuteError&) {
            plot_preview->data->clear();
        }
        getUI()->keep_quiet = false;
    }
    else {
        plot_preview->data->clear();
    }
    plot_preview->Refresh();
}

void FDXLoadDlg::on_filter_change()
{
    int idx = dir_ctrl->GetFilterIndex();
    wxString path = dir_ctrl->GetFilePath();
    bool is_text = (idx == 0 && !path.IsEmpty() 
                             && Data::guess_file_type(wx2s(path)) == "text")
                    || idx == 1; 
    enable_text_options(is_text);
}

void FDXLoadDlg::enable_text_options(bool is_text)
{
    columns_panel->Enable(is_text);
    htitle_cb->Enable(is_text);
    if (!is_text && htitle_cb->IsChecked())
        htitle_cb->SetValue(false);
}

void FDXLoadDlg::on_path_change()
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

string FDXLoadDlg::get_filename()
{
    return wx2s(filename_tc->GetValue());
}

string FDXLoadDlg::get_command_tail(int data_nr)
{
    string cols;
    if (columns_panel->IsEnabled()) { // a:b[:c]
        cols = " " + S(x_column->GetValue()) + "," + S(y_column->GetValue());
        if (std_dev_cb->GetValue())
            cols += S(",") + S(s_column->GetValue());
    }
    string filetype;
    if (htitle_cb->IsChecked())
        filetype = " htext";
    string title_cmd;
    if (title_tc->IsEnabled()) {
        wxString t = title_tc->GetValue().Trim();
        if (!t.IsEmpty())
            title_cmd = "; @" + S(data_nr) + ".title = '" + wx2s(t) + "'";
    }
    return "'" + get_filename() + "'" + filetype + cols + title_cmd;
}



/// show "Export data" dialog
bool export_data_dlg(wxWindow *parent, bool load_exported)
{
    static wxString dir = wxT(".");
    string columns = "";
    string ds = frame->get_active_data_str();
    if (!load_exported) {
        DataExportDlg ded(parent, -1, ds);
        if (ded.ShowModal() != wxID_OK)
            return false;
        columns = " (" + ded.get_columns() + ")";
    }
    wxFileDialog fdlg (parent, wxT("Export data to file"), dir, wxT(""),
                       wxT("x y data (*.dat, *.xy)|*.dat;*.DAT;*.xy;*.XY"),
                       wxSAVE | wxOVERWRITE_PROMPT);
    dir = fdlg.GetDirectory();
    if (fdlg.ShowModal() == wxID_OK) {
        string path = wx2s(fdlg.GetPath());
        exec_command(ds + columns + " > '" + path + "'");
        if (load_exported)
            exec_command(ds + " < '" + path + "'");
        return true;
    }
    else
        return false;
}
//======================================================================
//                         DataExportDlg
//======================================================================
BEGIN_EVENT_TABLE(DataExportDlg, wxDialog)
    EVT_BUTTON(wxID_OK, DataExportDlg::OnOk)
    EVT_RADIOBOX(ID_DED_RADIO, DataExportDlg::OnRadioChanged)
    EVT_CHECKBOX(ID_DED_INACT_CB, DataExportDlg::OnInactiveChanged)
    EVT_TEXT(ID_DED_TEXT, DataExportDlg::OnTextChanged)
END_EVENT_TABLE()

DataExportDlg::DataExportDlg(wxWindow* parent, wxWindowID id, 
                             std::string const& ds)
    : wxDialog(parent, id, wxT("Export data/functions as points"), 
               wxDefaultPosition, wxSize(600, 500), 
               wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER)
{
    wxBoxSizer *top_sizer = new wxBoxSizer(wxVERTICAL);
    wxStaticText *st1 = new wxStaticText(this, -1, 
                                         wxT("       Step 1: Select columns"));
    top_sizer->Add(st1, 0, wxTOP|wxLEFT|wxRIGHT, 5);
    wxStaticText *st2 = new wxStaticText(this, -1, 
                                         wxT("       Step 2: Choose a file"));
    st2->Enable(false);
    top_sizer->Add(st2, 0, wxALL, 5);
    wxArrayString choices;
    choices.Add(wxT("x, y"));
    cv.Add(wxT("x, y"));
    choices.Add(wxT("x, y, sigma"));
    cv.Add(wxT("x, y, s"));
    choices.Add(wxT("x, sum"));
    cv.Add(wxT("x, ") + s2wx(ds) + wxT(".F(x)"));
    choices.Add(wxT("x, sum, zero shift"));
    cv.Add(wxT("x, ") + s2wx(ds) + wxT(".F(x), ") + s2wx(ds) + wxT(".Z(x)"));
    choices.Add(wxT("x, y, sigma, sum"));
    cv.Add(wxT("x, y, s, ") + s2wx(ds) + wxT(".F(x)"));
    choices.Add(wxT("x, y, sigma, sum, all functions..."));
    cv.Add(wxT("x, y, s, ") + s2wx(ds) + wxT(".F(x), *F(x)"));
    choices.Add(wxT("x, y, sigma, sum, residual"));
    cv.Add(wxT("x, y, s, ") + s2wx(ds) + wxT(".F(x), y-") + s2wx(ds) 
            + wxT(".F(x)"));
    choices.Add(wxT("x, y, sigma, weighted residual"));
    cv.Add(wxT("x, y, s, (y-") + s2wx(ds) + wxT(".F(x))/s"));
    choices.Add(wxT("custom"));
    rb = new wxRadioBox(this, ID_DED_RADIO, wxT("exported columns"),
                        wxDefaultPosition, wxDefaultSize, choices,
                        2, wxRA_SPECIFY_COLS);
    top_sizer->Add(rb, 0, wxALL|wxEXPAND, 5);
    inactive_cb = new wxCheckBox(this, ID_DED_INACT_CB, 
                                 wxT("export also inactive points"));
    top_sizer->Add(inactive_cb, 0, wxALL, 5);
    text = new wxTextCtrl(this, ID_DED_TEXT, wxT(""));
    top_sizer->Add(text, 0, wxEXPAND|wxALL, 5);
    top_sizer->Add (new wxStaticLine(this, -1), 0, wxEXPAND|wxLEFT|wxRIGHT, 5);
    top_sizer->Add(CreateButtonSizer(wxOK|wxCANCEL), 
                   0, wxALL|wxALIGN_CENTER, 5);
    SetSizerAndFit(top_sizer);
    //read settings
    wxString t = wxConfig::Get()->Read(wxT("/exportDataCols"), wxT("x, y, s"));
    for (size_t i = 0; i < cv.GetCount(); ++i) {
        if (t == cv[i]) {
            rb->SetSelection(i);
            inactive_cb->SetValue(false);
            on_widget_change();
            return;
        }
        else if (t == cv[i]+wxT(", a")) {
            rb->SetSelection(i);
            inactive_cb->SetValue(true);
            on_widget_change();
            return;
        }
    }
    rb->SetSelection(cv.GetCount());
    text->SetValue(t);
    text->MarkDirty();
}

void DataExportDlg::on_widget_change()
{
    int n = rb->GetSelection();
    bool is_custom = (n == (int) cv.GetCount());
    if (!is_custom) {
        text->SetValue(cv[n] + (inactive_cb->GetValue() ? wxT(", a"):wxT("")));
        FindWindow(wxID_OK)->Enable(true);
    }
    text->Enable(is_custom);
    inactive_cb->Enable(!is_custom);
}

void DataExportDlg::OnTextChanged(wxCommandEvent&) 
{ 
    if (!text->IsModified())
        return;
    vector<string> cols = split_string(wx2s(text->GetValue()), ",");
    bool has_a = false;
    bool parsable = true;
    for (vector<string>::const_iterator i = cols.begin(); i != cols.end(); ++i){
        string t = strip_string(*i);
        if (t == "a")
            has_a = true;
        if (!(startswith(t, "*F(") && *(t.end()-1) == ')' 
                    && validate_data_expression(string(t, 3, t.size()-4))
              || validate_data_expression(t)))
            parsable = false;
    }
    FindWindow(wxID_OK)->Enable(parsable);
    inactive_cb->SetValue(has_a);
}

void DataExportDlg::OnOk(wxCommandEvent& event) 
{
    wxConfig::Get()->Write(wxT("/exportDataCols"), text->GetValue());
    event.Skip();
}


//======================================================================
//                         ScriptDebugDlg
//======================================================================
BEGIN_EVENT_TABLE(ScriptDebugDlg, wxDialog)
    EVT_TOOL(ID_SE_OPEN, ScriptDebugDlg::OnOpenFile)
    EVT_TOOL(ID_SE_EXECSEL, ScriptDebugDlg::OnExecSelected)
    EVT_TOOL(ID_SE_EXECDOWN, ScriptDebugDlg::OnExecDown)
    EVT_TOOL(ID_SE_CLOSE, ScriptDebugDlg::OnClose)
END_EVENT_TABLE()

ScriptDebugDlg::ScriptDebugDlg(wxWindow* parent, wxWindowID id)
    : wxDialog(parent, id, wxT("(not working yet) Script Editor and Debugger"), 
               wxDefaultPosition, wxSize(600, 500), 
               wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER)
{
    wxBoxSizer *top_sizer = new wxBoxSizer(wxVERTICAL);
    tb = new wxToolBar(this, -1, wxDefaultPosition, wxDefaultSize, 
                       wxTB_HORIZONTAL | wxNO_BORDER);
    tb->SetToolBitmapSize(wxSize(24, 24));
    tb->AddTool(ID_SE_OPEN, wxT("Open"), wxBitmap(open_xpm), wxNullBitmap,
                wxITEM_NORMAL, wxT("Open File"), wxT("Open script file"));
    tb->AddSeparator();
    tb->AddTool(ID_SE_EXECSEL, wxT("Execute"), 
                wxBitmap(exec_selected_xpm), wxNullBitmap,
                wxITEM_NORMAL, wxT("Execute selected"), 
                wxT("Execute selected lines"));
    tb->AddTool(ID_SE_EXECDOWN, wxT("Execute"), 
                wxBitmap(exec_down_xpm), wxNullBitmap,
                wxITEM_NORMAL, wxT("Execute selected and select next"), 
                wxT("Execute selected lines"));
    tb->AddSeparator();
    tb->AddTool(ID_SE_SAVE, wxT("Save"), wxBitmap(save_xpm), wxNullBitmap,
                wxITEM_NORMAL, wxT("Save"), 
                wxT("Save to file"));
    tb->AddTool(ID_SE_SAVE_AS, wxT("Save as"), 
                wxBitmap(save_as_xpm), wxNullBitmap,
                wxITEM_NORMAL, wxT("Save as..."), 
                wxT("Save a copy to file"));
    tb->AddSeparator();
    //TODO Edit, Move up, Move down
    tb->AddTool(ID_SE_CLOSE, wxT("Close"), wxBitmap(close_xpm), wxNullBitmap,
                wxITEM_NORMAL, wxT("Exit debugger"), wxT("Close debugger"));
    tb->Realize();
    top_sizer->Add(tb, 0, wxEXPAND); 
    list = new wxListView(this, -1, 
                          wxDefaultPosition, wxDefaultSize,
                          wxLC_REPORT|wxLC_HRULES|wxLC_VRULES);
    list->InsertColumn(0, wxT("No"));
    list->InsertColumn(1, wxT("Text"));
    list->InsertColumn(2, wxT("Exec. time"));
    list->SetColumnWidth(0, 40);
    list->SetColumnWidth(1, 430);
    list->SetColumnWidth(2, 100);

    //TODO last execution status column (ok, warning, error)
    top_sizer->Add(list, 1, wxALL|wxEXPAND, 0);
    //TODO editor
    SetSizerAndFit(top_sizer);
    SetSize(600, 500);
}

void ScriptDebugDlg::OpenFile()
{
    wxFileDialog dialog(this, wxT("open script file"), wxT(""), wxT(""), 
                        wxT("fityk scripts (*.fit)|*.fit|all files|*"), 
                        wxOPEN | wxFILE_MUST_EXIST);
    if (dialog.ShowModal() != wxID_OK) 
        return;
    string path = wx2s(dialog.GetPath());
    ifstream f(path.c_str());
    if (!f) 
        return;
    list->DeleteAllItems();
    string line;
    int n = 0;
    while (getline(f, line)) {
        add_line(++n, line);
    }
    list->InsertItem(list->GetItemCount(), wxT("+"));
}

void ScriptDebugDlg::add_line(int n, string const& line)
{
    int pos = list->GetItemCount();
    list->InsertItem(pos, s2wx(n == -1 ? S("...") : S(n)));
    string::size_type sep = line.find(';');
    string::size_type hash = line.find('#');
    string head, tail;
    if (sep != string::npos && (hash == string::npos || sep < hash)) {
        head = string(line, 0, sep+1);
        tail = string(line, sep+1);
    }
    else
        head = line;
    
    list->SetItem(pos, 1, s2wx(head));
    //TODO categories: comment, syntax error, info, plot, ok
    string::size_type nb = head.find_first_not_of(" \r\n\t");
    bool is_comment = (nb == string::npos || head[nb] == '#');
    if (is_comment) 
        list->SetItemTextColour(pos, *wxLIGHT_GREY);
    else if (!check_command_syntax(head))
        list->SetItemBackgroundColour(pos, *wxRED);
    if (!tail.empty())
        add_line(-1, tail);
}

void ScriptDebugDlg::exec_line(int n)
{
    wxStopWatch sw;
    Commands::Status r = exec_command(wx2s(get_list_item(n)));
    long millisec = sw.Time();
    if (r == Commands::status_ok) {
        list->SetItem(n, 2, wxString::Format(wxT("%.2f"), millisec/1000.));
    }
    else {
        list->SetItem(n, 2, wxT("ERROR"));
    }
}

void ScriptDebugDlg::OnExecSelected(wxCommandEvent&)
{
    for (long i=list->GetFirstSelected(); i!=-1; i=list->GetNextSelected(i)) {
        exec_line(i);
    }
}

void ScriptDebugDlg::OnExecDown(wxCommandEvent&)
{
    long last = -1;
    for (long i=list->GetFirstSelected(); i!=-1; i=list->GetNextSelected(i)) {
        exec_line(i);
        last = i;
    }
    if (last == -1)
        return;
    ++last;
    for (long i = 0; i < list->GetItemCount(); ++i)
        list->Select(i, i == last);
    if (last < list->GetItemCount())
        list->Focus(last);
}

wxString ScriptDebugDlg::get_list_item(int i)
{
    wxListItem info;
    info.SetId(i);
    info.SetColumn(1);
    list->GetItem(info);
    return info.GetText();
}

//======================================================================
//                         PageSetupDialog
//======================================================================
BEGIN_EVENT_TABLE(PageSetupDialog, wxDialog)
    EVT_BUTTON(wxID_OK, PageSetupDialog::OnOk)
END_EVENT_TABLE()

PageSetupDialog::PageSetupDialog(wxWindow *parent, PrintManager *print_mgr)
    : wxDialog(parent, -1, wxT("Page Setup"), 
               wxDefaultPosition, wxDefaultSize, 
               wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER),
      pm(print_mgr)
{
    wxBoxSizer *top_sizer = new wxBoxSizer(wxVERTICAL);

    //paper size
    wxStaticBoxSizer *sizerp = new wxStaticBoxSizer(wxVERTICAL, this, 
                                                    wxT("Paper Size"));
    wxArrayString paper_sizes;
    int paper_sel = 0;
    for (size_t i = 0; i < wxThePrintPaperDatabase->GetCount(); ++i) {
        wxPrintPaperType *papertype = wxThePrintPaperDatabase->Item(i);
        paper_sizes.Add(papertype->GetName());
        if (pm->get_print_data().GetPaperId() == papertype->GetId())
            paper_sel = i;
    }
    papers = new  wxComboBox(this, -1, _("Paper Size"), 
                             wxDefaultPosition, wxDefaultSize, 
                             paper_sizes, wxCB_READONLY);
    papers->SetSelection(paper_sel);
    sizerp->Add(papers, 1, wxALL|wxEXPAND, 5);
    top_sizer->Add(sizerp, 0, wxALL|wxEXPAND, 5);

    //orientation
    wxString orient_choices[] = { wxT("Portrait"), wxT("Landscape") };
    orientation = new wxRadioBox(this, -1, wxT("Orientation"),
                                 wxDefaultPosition, wxDefaultSize, 
                                 2, orient_choices, 
                                 2, wxRA_SPECIFY_COLS);
    top_sizer->Add(orientation, 0, wxALL|wxEXPAND, 5);

    /* TODO what about margins?
    wxStaticBox *margbox = new wxStaticBox(this, -1, 
                                           wxT("Margins (millimetres)"));
    wxStaticBoxSizer *hsizer = new wxStaticBoxSizer(margbox, wxHORIZONTAL);
    hsizer->Add(new wxStaticText(this, -1, wxT(" Left")),
                0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    left_margin = new SpinCtrl(this, -1, 0, -100, 500);
    hsizer->Add(left_margin, 0, wxALL, 5);
    hsizer->Add(new wxStaticText(this, -1, wxT(" Right")),
                0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    right_margin = new SpinCtrl(this, -1, 0, -100, 500);
    hsizer->Add(right_margin, 0, wxALL, 5);
    hsizer->Add(new wxStaticText(this, -1, wxT(" Top")),
                0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    top_margin = new SpinCtrl(this, -1, 0, -100, 500);
    hsizer->Add(top_margin, 0, wxALL, 5);
    hsizer->Add(new wxStaticText(this, -1, wxT(" Bottom")),
                0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    bottom_margin = new SpinCtrl(this, -1, 0, -100, 500);
    hsizer->Add(bottom_margin, 0, wxALL, 5);
    top_sizer->Add(hsizer, 0, wxALL|wxEXPAND, 5);
    */

    wxBoxSizer *h2sizer = new wxBoxSizer(wxHORIZONTAL);
    h2sizer->Add(new wxStaticText(this, -1, wxT("Scale to")),
                 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    scale = new SpinCtrl(this, -1, 100, 1, 150);
    h2sizer->Add(scale, 0, wxTOP|wxBOTTOM, 5);
    h2sizer->Add(new wxStaticText(this, -1, wxT("% of page")),
                 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    top_sizer->Add(h2sizer);
    keep_ratio = new wxCheckBox(this, -1, wxT("keep width to height ratio"));
    top_sizer->Add(keep_ratio, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    wxString color_choices[] = { wxT("black lines on white background"), 
                                 wxT("colors from plots on white background") };
    colors = new wxRadioBox(this, -1, wxT("Colors"),
                            wxDefaultPosition, wxDefaultSize, 
                            2, color_choices, 
                            1, wxRA_SPECIFY_COLS);
    top_sizer->Add(colors, 0, wxALL|wxEXPAND, 5);
    wxStaticBoxSizer *boxsizer = new wxStaticBoxSizer(wxVERTICAL, this, 
                                                      wxT("Optional elements"));
    for (int i = 0; i < 2; ++i) {
        plot_aux[i] = new wxCheckBox(this, -1, 
                                wxString::Format(wxT("auxiliary plot %i"), i));
        boxsizer->Add(plot_aux[i], 0, wxALL, 5);
    }
    plot_borders = new wxCheckBox(this, -1, wxT("borders around plots"));
    boxsizer->Add(plot_borders, 0, wxALL, 5);
    top_sizer->Add(boxsizer, 0, wxALL|wxEXPAND, 5);
    top_sizer->Add (new wxStaticLine(this, -1), 0, wxEXPAND|wxLEFT|wxRIGHT, 5);
    top_sizer->Add(CreateButtonSizer(wxOK|wxCANCEL), 
                   0, wxALL|wxALIGN_CENTER, 5);
    SetSizerAndFit(top_sizer);
    orientation->SetSelection(pm->landscape ? 1 : 0);
    colors->SetSelection(pm->colors ? 1 : 0);
    keep_ratio->SetValue(pm->keep_ratio);
    scale->SetValue(pm->scale);
    for (int i = 0; i < 2; ++i) {
        if (pm->plot_pane->aux_visible(i)) {
            plot_aux[i]->SetValue(pm->plot_aux[i]);
        }
        else {
            plot_aux[i]->SetValue(false);
            plot_aux[i]->Enable(false);
        }
    }
    plot_borders->SetValue(pm->plot_borders);
    /*
    left_margin->SetValue(pm->get_page_data().GetMarginTopLeft().x);
    right_margin->SetValue(pm->get_page_data().GetMarginBottomRight().x);
    top_margin->SetValue(pm->get_page_data().GetMarginTopLeft().y); 
    bottom_margin->SetValue(pm->get_page_data().GetMarginBottomRight().y);
    */
}

void PageSetupDialog::OnOk(wxCommandEvent& event) 
{
    if (papers->GetSelection() != -1) {
        wxPrintPaperType *ppt 
                      = wxThePrintPaperDatabase->Item(papers->GetSelection());
        if (ppt) {
            pm->get_page_data().SetPaperSize(wxSize(ppt->GetWidth()/10, 
                                             ppt->GetHeight()/10));
            pm->get_print_data().SetPaperId(ppt->GetId());
        }
    }

    pm->landscape = (orientation->GetSelection() == 1);
    pm->colors = colors->GetSelection() == 1;
    pm->keep_ratio = keep_ratio->GetValue();
    pm->scale = scale->GetValue();
    for (int i = 0; i < 2; ++i) 
        pm->plot_aux[i] = plot_aux[i]->GetValue();
    pm->plot_borders = plot_borders->GetValue();

    /*
    pm->get_page_data().GetMarginTopLeft().x = left_margin->GetValue();
    pm->get_page_data().GetMarginBottomRight().x = right_margin->GetValue();
    pm->get_page_data().GetMarginTopLeft().y = top_margin->GetValue();
    pm->get_page_data().GetMarginBottomRight().y = bottom_margin->GetValue();
    */

    wxDialog::OnOK(event);
}


//===============================================================
//                    Printing utilities
//===============================================================

void do_print_plots(wxDC *dc, PrintManager const* pm)
{
    // Set the scale and origin
    const int space = 10; //vertical space between plots
    const int marginX = 50, marginY = 50; //page margins
    //width is the same for all plots
    int width = pm->plot_pane->GetClientSize().GetWidth(); 
    vector<FPlot*> vp; 
    vp.push_back(pm->plot_pane->get_plot());
    for (int i = 0; i < 2; ++i)
        if (pm->plot_pane->aux_visible(i) && pm->plot_aux[i])
            vp.push_back(pm->plot_pane->get_aux_plot(i));
    int height = -space;  //height = sum of all heights + (N-1)*space
    for (vector<FPlot*>::const_iterator i = vp.begin(); i != vp.end(); ++i) 
        height += (*i)->GetClientSize().GetHeight() + space;
    int w, h;
    dc->GetSize(&w, &h);
    fp scaleX = w / (width + 2.*marginX) * pm->scale/100.;
    fp scaleY = h / (height + 2.*marginY) * pm->scale/100.;
    if (pm->keep_ratio) {
        if (scaleX > scaleY)
            scaleX = scaleY;
        else
            scaleY = scaleX;
    }
    dc->SetUserScale (scaleX, scaleY);
    const int posX = iround((w - width * scaleX) / 2.);
    int posY = iround((h - height * scaleY) / 2.);

    //drawing all visible plots, every at different posY
    for (vector<FPlot*>::const_iterator i = vp.begin(); i != vp.end(); ++i) {
        dc->SetDeviceOrigin (posX, posY);
        if (pm->plot_borders && i != vp.begin()) {
            int Y = -space/2;
            dc->DrawLine(0, Y, width, Y);
        }
        int plot_height = (*i)->GetClientSize().GetHeight();
        dc->SetClippingRegion(0, 0, width, plot_height);
        (*i)->Draw(*dc, !pm->colors);
        dc->DestroyClippingRegion();
        posY += iround((plot_height+space) * scaleY);
    }
}

//===============================================================

FPrintout::FPrintout(PrintManager const* print_manager) 
    : wxPrintout(wxT("fityk")), pm(print_manager) 
{}

bool FPrintout::OnPrintPage(int page)
{
    if (page != 1) 
        return false;
    wxDC *dc = GetDC();
    if (!dc) 
        return false;
    do_print_plots(dc, pm);
    return true;
}

//===============================================================

class FPreviewFrame : public wxPreviewFrame
{
public:
    FPreviewFrame(wxPrintPreview* preview, wxWindow* parent) 
        : wxPreviewFrame (preview, parent, wxT("Print Preview"), 
                          wxDefaultPosition, wxSize(600, 550)) {}
    void CreateControlBar() { 
        m_controlBar = new wxPreviewControlBar(m_printPreview, 
                                        wxPREVIEW_PRINT|wxPREVIEW_ZOOM, this);
        m_controlBar->CreateButtons();
        m_controlBar->SetZoomControl(110);
    }
};

//===============================================================

PrintManager::PrintManager(PlotPane* pane)
    :  plot_pane(pane), print_data(0), page_setup_data(0) 
{ 
    read_settings(wxConfig::Get());
}

PrintManager::~PrintManager() 
{
    save_settings(wxConfig::Get());
    if (print_data)
        delete print_data;
    if (page_setup_data)
        delete page_setup_data;
}

void PrintManager::save_settings(wxConfigBase *cf) const
{
    cf->Write(wxT("/print/colors"), colors);
    cf->Write(wxT("/print/scale"), scale);
    cf->Write(wxT("/print/keepRatio"), keep_ratio);
    cf->Write(wxT("/print/plotBorders"), plot_borders);
    for (int i = 0; i < 2; ++i)
        cf->Write(wxString::Format(wxT("/print/plotAux%i"), i), plot_aux[i]);
    cf->Write(wxT("/print/landscape"), landscape);
}

void PrintManager::read_settings(wxConfigBase *cf)
{
    colors = cfg_read_bool(cf, wxT("/print/colors"), false);
    scale = cf->Read(wxT("/print/scale"), 100);
    keep_ratio = cfg_read_bool(cf, wxT("/print/keepRatio"), false);
    plot_borders = cfg_read_bool(cf, wxT("/print/plotBorders"), true);
    for (int i = 0; i < 2; ++i)
        plot_aux[i] = cfg_read_bool(cf, 
                          wxString::Format(wxT("/print/plotAux%i"), i), true);
    landscape = cfg_read_bool(cf, wxT("/print/landscape"), true);
}

wxPrintData& PrintManager::get_print_data() 
{
    if (!print_data) {
        print_data = new wxPrintData;
        print_data->SetOrientation(landscape ? wxLANDSCAPE : wxPORTRAIT);
    }
    return *print_data;
}

wxPageSetupDialogData& PrintManager::get_page_data() 
{
    if (!page_setup_data)
        page_setup_data = new wxPageSetupDialogData;
    return *page_setup_data;
}

void PrintManager::printPreview()
{
    // Pass two printout objects: for preview, and possible printing.
    wxPrintDialogData print_dialog_data(get_print_data());
    wxPrintPreview *preview = new wxPrintPreview (new FPrintout(this),
                                                  new FPrintout(this), 
                                                  &print_dialog_data);
    if (!preview->Ok()) {
        delete preview;
        wxMessageBox(wxT("There was a problem previewing.\n")
                     wxT("Perhaps your current printer is not set correctly?"),
                     wxT("Previewing"), wxOK);
        return;
    }
    FPreviewFrame *preview_frame = new FPreviewFrame (preview, frame);
    preview_frame->Centre(wxBOTH);
    preview_frame->Initialize();
    preview_frame->Show(true);
}

void PrintManager::pageSetup()
{
    PageSetupDialog dlg(frame, this);
    dlg.ShowModal();
    /*
     * Old standard wxWidgets "page setup" dlg worked in this way:
    if (!page_setup_data) 
        page_setup_data = new wxPageSetupDialogData(get_print_data());
    else
        (*page_setup_data) = get_print_data();

    wxPageSetupDialog page_setup_dialog(this, page_setup_data);
    page_setup_dialog.ShowModal();

    get_print_data()=page_setup_dialog.GetPageSetupData().GetPrintData();
    (*page_setup_data) = page_setup_dialog.GetPageSetupData();
    */
}

void PrintManager::print()
{
    wxPrintDialogData print_dialog_data(get_print_data());
    print_dialog_data.SetFromPage(1);
    print_dialog_data.SetToPage(1);
    print_dialog_data.EnablePageNumbers(false);
    print_dialog_data.EnableSelection(false);
    wxPrinter printer (&print_dialog_data);
    FPrintout printout(this);
    bool r = printer.Print(frame, &printout, true);
    if (r) {
        get_print_data() = printer.GetPrintDialogData().GetPrintData();
        landscape = (get_print_data().GetOrientation() == wxLANDSCAPE);
    }
    else if (wxPrinter::GetLastError() == wxPRINTER_ERROR)
        wxMessageBox(wxT("There was a problem printing.\nPerhaps your current ")
                     wxT("printer is not set correctly?"), 
                     wxT("Printing"), wxOK);
}


void PrintManager::print_to_psfile()
{
    wxFileDialog dialog(frame, wxT("PostScript file"), wxT(""), wxT(""), 
                        wxT("*.ps"), wxSAVE | wxOVERWRITE_PROMPT);
    if (dialog.ShowModal() != wxID_OK) 
        return;
    get_print_data().SetPrintMode(wxPRINT_MODE_FILE);
    get_print_data().SetFilename(dialog.GetPath());
    wxPrintDialogData print_dialog_data(get_print_data());
    print_dialog_data.SetFromPage(1);
    print_dialog_data.SetToPage(1);
    wxPostScriptPrinter printer (&print_dialog_data);
    FPrintout printout(this);
    bool r = printer.Print(frame, &printout, false);
    if (!r)
        wxMessageBox(wxT("Can't save plots as a file:\n") + dialog.GetPath(),
                     wxT("Exporting to PostScript"), wxOK|wxICON_ERROR);
}



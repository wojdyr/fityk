// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
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

#include <xylib/xylib.h>
#include <xylib/cache.h>

#include "dload.h"
#include "frame.h"  // frame->add_recent_data_file(get_filename())
#include "plot.h" // scale_tics_step()
#include "../logic.h"
#include "../settings.h"
#include "../data.h" // get_file_basename()
#include "../common.h" //iround

using namespace std;

enum {
    ID_DXLOAD_STDDEV_CB     =28000,
    ID_DXLOAD_BLOCK               ,
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


class PreviewPlot : public PlotWithTics
{
public:
    int block_nr, idx_x, idx_y;

    PreviewPlot(wxWindow* parent)
        : PlotWithTics(parent), block_nr(0), idx_x(1), idx_y(2),
          data_updated(false)
        { set_bg_color(*wxBLACK); }

    void OnPaint(wxPaintEvent &event);
    void draw(wxDC &dc, bool);
    void load_dataset(string const& filename, string const& filetype,
                      string const& options);
    shared_ptr<const xylib::DataSet> get_data() const { return data; }
    void make_outdated() { data_updated = false; }

private:
    shared_ptr<const xylib::DataSet> data;
    bool data_updated; // if false, draw() doesn't do anything (plot is clear)

    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE (PreviewPlot, PlotWithTics)
    EVT_PAINT (PreviewPlot::OnPaint)
END_EVENT_TABLE()

void PreviewPlot::OnPaint(wxPaintEvent&)
{
    buffered_draw();
}

void PreviewPlot::draw(wxDC &dc, bool)
{
    if (data.get() == NULL || !data_updated
        || block_nr < 0 || block_nr >= data->get_block_count())
        return;

    xylib::Block const *block = data->get_block(block_nr);

    if (idx_x < 0 || idx_x > block->get_column_count()
            || idx_y < 0 || idx_y > block->get_column_count())
        return;

    xylib::Column const& xcol = block->get_column(idx_x);
    xylib::Column const& ycol = block->get_column(idx_y);
    const int np = block->get_point_count();
    draw_tics(dc, xcol.get_min(), xcol.get_max(np),
                  ycol.get_min(), ycol.get_max(np));
    draw_axis_labels(dc, xcol.get_name(), ycol.get_name());

    // draw data
    dc.SetPen(*wxGREEN_PEN);
    for (int i = 0; i < np; ++i)
        draw_point(dc, xcol.get_value(i), ycol.get_value(i));
}


void PreviewPlot::load_dataset(string const& filename,
                               string const& filetype,
                               string const& options)
{
    try {
        data = xylib::cached_load_file(filename, filetype, options);
        data_updated = true;
    } catch (runtime_error const& /*e*/) {
        data_updated = false;
    }
}


BEGIN_EVENT_TABLE(DLoadDlg, wxDialog)
    EVT_CHECKBOX    (ID_DXLOAD_STDDEV_CB, DLoadDlg::OnStdDevCheckBox)
    EVT_CHECKBOX    (ID_DXLOAD_HTITLE,    DLoadDlg::OnHTitleCheckBox)
    EVT_CHECKBOX    (ID_DXLOAD_AUTO_TEXT, DLoadDlg::OnAutoTextCheckBox)
    EVT_CHECKBOX    (ID_DXLOAD_AUTO_PLOT, DLoadDlg::OnAutoPlotCheckBox)
    EVT_SPINCTRL    (ID_DXLOAD_COLX,      DLoadDlg::OnColumnChanged)
    EVT_SPINCTRL    (ID_DXLOAD_COLY,      DLoadDlg::OnColumnChanged)
    EVT_CHOICE      (ID_DXLOAD_BLOCK,     DLoadDlg::OnBlockChanged)
    EVT_BUTTON      (wxID_CLOSE,          DLoadDlg::OnClose)
    EVT_BUTTON      (ID_DXLOAD_OPENHERE,  DLoadDlg::OnOpenHere)
    EVT_BUTTON      (ID_DXLOAD_OPENNEW,   DLoadDlg::OnOpenNew)
    EVT_TREE_SEL_CHANGED (-1,             DLoadDlg::OnPathSelectionChanged)
    EVT_TEXT_ENTER(ID_DXLOAD_FN,          DLoadDlg::OnPathTextChanged)
END_EVENT_TABLE()

/// n - data slot to be used by "Replace ..." button, -1 means none
/// data - used for default settings (path, columns, etc.), not NULL
DLoadDlg::DLoadDlg (wxWindow* parent, wxWindowID id, int n, Data* data)
    : wxDialog(parent, id, wxT("Data load (custom)"),
               wxDefaultPosition, wxDefaultSize,
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
                       wxT("All Files (*)|*|")
                           + s2wx(xylib::get_wildcards_string()));
    left_sizer->Add(dir_ctrl, 1, wxALL|wxEXPAND, 5);
    wxFileName path = s2wx(data->get_filename());
    path.Normalize();
    dir_ctrl->SetPath(path.GetFullPath());
    filename_tc = new KFTextCtrl(left_panel, ID_DXLOAD_FN, path.GetFullPath());
    left_sizer->Add (filename_tc, 0, wxALL|wxEXPAND, 5);


    // selecting block
    block_ch = new wxChoice(left_panel, ID_DXLOAD_BLOCK);
    left_sizer->Add(block_ch, 0, wxALL|wxEXPAND, 5);

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
    if (data->get_given_x() != INT_MAX)
        x_column->SetValue(data->get_given_x());
    if (data->get_given_y() != INT_MAX)
        y_column->SetValue(data->get_given_y());
    if (data->get_given_s() != INT_MAX) {
        std_dev_cb->SetValue(true);
        s_column->SetValue(data->get_given_s());
    }

    bool def_sqrt = (S(ftk->get_settings()->data_default_sigma()) == "sqrt");
    sd_sqrt_cb = new wxCheckBox(left_panel, ID_DXLOAD_SDS,
                                wxT("std. dev. = max(sqrt(y), 1)"));
    sd_sqrt_cb->SetValue(def_sqrt);
    left_sizer->Add (sd_sqrt_cb, 0, wxALL|wxEXPAND, 5);

    wxBoxSizer *dt_sizer = new wxBoxSizer(wxHORIZONTAL);
    title_cb = new wxCheckBox(left_panel, ID_DXLOAD_HTITLE,
                              wxT("data title:"));
    dt_sizer->Add(title_cb, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    title_tc = new wxTextCtrl(left_panel, -1, wxT(""));
    title_tc->Enable(false);
    dt_sizer->Add(title_tc, 1, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    left_sizer->Add (dt_sizer, 0, wxEXPAND);

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
    auto_plot_cb->SetValue(true);
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
    string data_nr_str = (data_nr >= 0 ? S(data_nr) : S("?"));
    open_here = new wxButton(this, ID_DXLOAD_OPENHERE,
                                        wxT("&Replace @") + s2wx(data_nr_str));
    if (data_nr < 0)
        open_here->Enable(false);
    open_new = new wxButton(this, ID_DXLOAD_OPENNEW,
                            wxT("&Open in new slot"));
    button_sizer->Add(open_here, 0, wxALL, 5);
    button_sizer->Add(open_new, 0, wxALL, 5);
    button_sizer->Add(new wxButton(this, wxID_CLOSE, wxT("&Close")),
                      0, wxALL, 5);
    top_sizer->Add(button_sizer, 0, wxALL|wxALIGN_CENTER, 0);
    initialized = true;
    SetSizerAndFit(top_sizer);
    SetSize(wxSize(700, 600));
    update_block_list();
}

void DLoadDlg::StdDevCheckBoxChanged()
{
    bool v = std_dev_cb->GetValue();
    s_column->Enable(v);
    sd_sqrt_cb->Enable(!v);
}

void DLoadDlg::OnHTitleCheckBox (wxCommandEvent& event)
{
    if (!event.IsChecked())
        update_title_from_file();
    title_tc->Enable(event.IsChecked());
}

void DLoadDlg::update_block_list()
{
    vector<string> bb;
    if (plot_preview && plot_preview->get_data().get() != NULL)
        for (int i = 0; i < plot_preview->get_data()->get_block_count(); ++i) {
            const string& name =
                plot_preview->get_data()->get_block(i)->get_name();
            bb.push_back(name.empty() ? "Block #" + S(i+1) : name);
        }
    else {
        bb.push_back("<default block>");
    }
    updateControlWithItems(block_ch, bb);
    block_ch->SetSelection(0);
    block_ch->Enable(block_ch->GetCount() > 1);
}

void DLoadDlg::update_title_from_file()
{
    if (title_cb->GetValue())
        return;
    string path = wx2s(dir_ctrl->GetFilePath());
    if (path.empty()) {
        title_tc->Clear();
        return;
    }
    // TODO: use class Data for this
    string title = get_file_basename(path);
    int x = x_column->GetValue();
    int y = y_column->GetValue();
    if (x != 1 || y != 2 || std_dev_cb->GetValue())
        title += ":" + S(x) + ":" + S(y);
    title_tc->SetValue(s2wx(title));
}

void DLoadDlg::OnAutoTextCheckBox (wxCommandEvent& event)
{
    if (event.IsChecked())
        update_text_preview();
    else
        text_preview->Clear();
}

void DLoadDlg::OnAutoPlotCheckBox (wxCommandEvent& event)
{
    update_plot_preview();
    if (event.IsChecked())
        update_title_from_file();
}

void DLoadDlg::OnBlockChanged (wxCommandEvent&)
{
    update_plot_preview();
}

void DLoadDlg::OnColumnChanged (wxSpinEvent&)
{
    if (auto_plot_cb->GetValue())
        update_plot_preview();
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
    int d_nr = ftk->get_dm_count();
    if (d_nr == 1 && !ftk->get_dm(0)->has_any_info())
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
        plot_preview->idx_x = x_column->GetValue();
        plot_preview->idx_y = y_column->GetValue();
        plot_preview->block_nr = block_ch->GetSelection();
        plot_preview->load_dataset(wx2s(dir_ctrl->GetFilePath()), "", "");
    }
    else
        plot_preview->make_outdated();
    plot_preview->refresh();
}

void DLoadDlg::on_path_change()
{
    if (!initialized)
        return;
    wxString path = dir_ctrl->GetFilePath();
    filename_tc->SetValue(path);
    open_here->Enable(!path.IsEmpty());
    open_new->Enable(!path.IsEmpty());
    if (auto_text_cb->GetValue())
        update_text_preview();
    block_ch->SetSelection(0);
    if (auto_plot_cb->GetValue())
        update_plot_preview();
    update_block_list();
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
    int x = x_column->GetValue();
    int y = y_column->GetValue();
    bool has_s = std_dev_cb->GetValue();
    int b = block_ch->GetSelection();
    // default parameter values are not passed explicitely
    if (x != 1 || y != 2 || has_s || b != 0) {
        cols = ":" + S(x) + ":" + S(y) + ":";
        if (has_s)
            cols += S(s_column->GetValue());
        cols += ":";
        if (b != 0)
            cols += S(b);
    }

    string filetype;
    //if (title_cb->IsChecked())
    //    filetype = " text, first_line_header";

    bool def_sqrt = (S(ftk->get_settings()->data_default_sigma()) == "sqrt");
    bool set_sqrt = sd_sqrt_cb->GetValue();
    bool sigma_in_file = std_dev_cb->GetValue();
    if (!sigma_in_file && set_sqrt != def_sqrt) {
        if (set_sqrt)
            cmd = "with data_default_sigma=sqrt ";
        else
            cmd = "with data_default_sigma=one ";
    }

    cmd += ds + " < '" + get_filename() + cols + "'" + filetype;

    if (title_tc->IsEnabled()) {
        wxString t = title_tc->GetValue().Trim();
        if (!t.IsEmpty())
            cmd += "; set @" + S(d_nr) + ".title = '" + wx2s(t) + "'";
    }

    return cmd;
}


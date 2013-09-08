// Author: Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#include <wx/wx.h>
#include <wx/filename.h>
#include <wx/cmdline.h>
#include <wx/splitter.h>
#include <wx/spinctrl.h>
#include <wx/file.h>

#include <xylib/xylib.h>
#include <xylib/cache.h>

#include "xybrowser.h"
#include "cmn.h" // SpinCtrl, pchar2wx, wx2s, updateControlWithItems

using namespace std;

#ifndef XYCONVERT
#include "fityk/data.h"
using fityk::get_file_basename;
#else
// copied from common.h
template <typename T, int N>
std::string format1(const char* fmt, T t)
{
    char buffer[N];
    snprintf(buffer, N, fmt, t);
    buffer[N-1] = '\0';
    return std::string(buffer);
}
inline std::string S(int n) { return format1<int, 16>("%d", n); }
inline std::string S(double d) { return format1<double, 16>("%g", d); }
#endif // XYCONVERT


PreviewPlot::PreviewPlot(wxWindow* parent)
    : PlotWithTics(parent), block_nr(0), idx_x(1), idx_y(2),
      data_updated_(false)
{
    set_bg_color(*wxBLACK);
    support_antialiasing_ = false;
}

void PreviewPlot::draw(wxDC &dc, bool)
{
    if (data_.get() == NULL || !data_updated_
        || block_nr < 0 || block_nr >= data_->get_block_count())
        return;

    xylib::Block const *block = data_->get_block(block_nr);

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
        data_ = xylib::cached_load_file(filename, filetype, options);
        data_updated_ = true;
    } catch (runtime_error const& /*e*/) {
        data_updated_ = false;
    }
}


XyFileBrowser::XyFileBrowser(wxWindow* parent)
    : wxSplitterWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                       wxSP_NOBORDER),
      auto_plot_cb(NULL)
{
    // +----------------------------+
    // |            | rupper_panel  |
    // | left_panel +---------------+
    // |            | rbottom_panel |
    // +----------------------------+

    SetSashGravity(0.5);
    SetMinimumPaneSize(20);
    wxPanel *left_panel = new wxPanel(this, -1);
    wxBoxSizer *left_sizer = new wxBoxSizer(wxVERTICAL);
    wxSplitterWindow *right_splitter = new wxSplitterWindow(this, -1,
                                             wxDefaultPosition, wxDefaultSize,
                                             wxSP_NOBORDER);
    right_splitter->SetSashGravity(0.5);
    right_splitter->SetMinimumPaneSize(20);
    wxPanel *rupper_panel = new wxPanel(right_splitter, -1);
    wxBoxSizer *rupper_sizer = new wxBoxSizer(wxVERTICAL);
    wxPanel *rbottom_panel = new wxPanel(right_splitter, -1);
    wxBoxSizer *rbottom_sizer = new wxBoxSizer(wxVERTICAL);

    // ----- left panel -----
    wxString all(wxFileSelectorDefaultWildcardStr);
    wxString wild = "All Files (" + all + ")|" + all
                    + "|" + s2wx(xylib::get_wildcards_string());
    filectrl = new wxFileCtrl(left_panel, -1, wxEmptyString, wxEmptyString,
                              wild, wxFC_OPEN|wxFC_MULTIPLE|wxFC_NOSHOWHIDDEN);
    left_sizer->Add(filectrl, 1, wxALL|wxEXPAND, 5);

    // selecting block
    wxBoxSizer *block_sizer = new wxBoxSizer(wxHORIZONTAL);
    block_ch = new wxChoice(left_panel, -1);
    block_sizer->Add(new wxStaticText(left_panel, -1, "block:"),
                     wxSizerFlags().Border(wxRIGHT).Center());
    block_sizer->Add(block_ch, wxSizerFlags(1));
    left_sizer->Add(block_sizer, wxSizerFlags().Border().Expand());

    // selecting columns
    wxPanel *columns_panel = new wxPanel (left_panel, -1);
    wxStaticBoxSizer *h2a_sizer = new wxStaticBoxSizer(wxHORIZONTAL,
                    columns_panel, wxT("Select columns (0 for point index):"));
    h2a_sizer->Add (new wxStaticText (columns_panel, -1, wxT("x")),
                    0, wxALL|wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL, 5);
    x_column = new SpinCtrl(columns_panel, wxID_ANY, 1, 0, 999, 50);
    h2a_sizer->Add (x_column, 0, wxALL|wxALIGN_LEFT, 5);
    h2a_sizer->Add (new wxStaticText (columns_panel, -1, wxT("y")),
                    0, wxALL|wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL, 5);
    y_column = new SpinCtrl(columns_panel, wxID_ANY, 2, 0, 999, 50);
    h2a_sizer->Add (y_column, 0, wxALL|wxALIGN_LEFT, 5);
    std_dev_cb = new wxCheckBox(columns_panel, -1, wxT("std.dev."));
    std_dev_cb->SetValue(false);
    h2a_sizer->Add(std_dev_cb, 0, wxALL|wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL,5);
    s_column = new SpinCtrl(columns_panel, wxID_ANY, 3, 1, 999, 50);
    h2a_sizer->Add(s_column, 0, wxALL|wxALIGN_LEFT, 5);
    columns_panel->SetSizer(h2a_sizer);
    left_sizer->Add (columns_panel, 0, wxALL|wxEXPAND, 5);

#ifndef XYCONVERT
    sd_sqrt_cb = new wxCheckBox(left_panel, wxID_ANY,
                                wxT("std. dev. = max(sqrt(y), 1)"));
    left_sizer->Add (sd_sqrt_cb, 0, wxALL|wxEXPAND, 5);

    wxBoxSizer *dt_sizer = new wxBoxSizer(wxHORIZONTAL);
    title_cb = new wxCheckBox(left_panel, wxID_ANY,
                              wxT("data title:"));
    dt_sizer->Add(title_cb, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    title_tc = new wxTextCtrl(left_panel, -1, wxT(""));
    title_tc->Enable(false);
    dt_sizer->Add(title_tc, 1, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    left_sizer->Add (dt_sizer, 0, wxEXPAND);

    StdDevCheckBoxChanged();
#endif

    // ----- right upper panel -----
    text_preview =  new wxTextCtrl(rupper_panel, -1, wxT(""),
                                   wxDefaultPosition, wxDefaultSize,
                                   wxTE_RICH|wxTE_READONLY|wxTE_MULTILINE);
    rupper_sizer->Add(text_preview, 1, wxEXPAND|wxALL, 5);
    auto_text_cb = new wxCheckBox(rupper_panel, -1,
                                  wxT("view the first 64kB of file as text"));
    auto_text_cb->SetValue(false);
    rupper_sizer->Add(auto_text_cb, 0, wxALL, 5);

    // ----- right bottom panel -----
    plot_preview = new PreviewPlot(rbottom_panel);
    rbottom_sizer->Add(plot_preview, 1, wxEXPAND|wxALL, 5);
    auto_plot_cb = new wxCheckBox(rbottom_panel, -1, wxT("plot"));
    auto_plot_cb->SetValue(true);
    rbottom_sizer->Add(auto_plot_cb, 0, wxALL, 5);

    // ------ finishing layout -----------
    left_panel->SetSizerAndFit(left_sizer);
    rupper_panel->SetSizerAndFit(rupper_sizer);
    rbottom_panel->SetSizerAndFit(rbottom_sizer);
    SplitVertically(left_panel, right_splitter);
    right_splitter->SplitHorizontally(rupper_panel, rbottom_panel);

    update_block_list();

    Connect(std_dev_cb->GetId(), wxEVT_COMMAND_CHECKBOX_CLICKED,
            wxCommandEventHandler(XyFileBrowser::OnStdDevCheckBox));
    Connect(auto_text_cb->GetId(), wxEVT_COMMAND_CHECKBOX_CLICKED,
            wxCommandEventHandler(XyFileBrowser::OnAutoTextCheckBox));
    Connect(auto_plot_cb->GetId(), wxEVT_COMMAND_CHECKBOX_CLICKED,
            wxCommandEventHandler(XyFileBrowser::OnAutoPlotCheckBox));
    Connect(x_column->GetId(), wxEVT_COMMAND_SPINCTRL_UPDATED,
            wxSpinEventHandler(XyFileBrowser::OnColumnChanged));
    Connect(y_column->GetId(), wxEVT_COMMAND_SPINCTRL_UPDATED,
            wxSpinEventHandler(XyFileBrowser::OnColumnChanged));
    Connect(block_ch->GetId(), wxEVT_COMMAND_CHOICE_SELECTED,
            wxCommandEventHandler(XyFileBrowser::OnBlockChanged));
    Connect(filectrl->GetId(), wxEVT_FILECTRL_SELECTIONCHANGED,
            wxFileCtrlEventHandler(XyFileBrowser::OnPathChanged));
#ifndef XYCONVERT
    Connect(title_cb->GetId(), wxEVT_COMMAND_CHECKBOX_CLICKED,
            wxCommandEventHandler(XyFileBrowser::OnTitleCheckBox));
#endif
}


void XyFileBrowser::StdDevCheckBoxChanged()
{
    bool v = std_dev_cb->GetValue();
    s_column->Enable(v);
#ifndef XYCONVERT
    sd_sqrt_cb->Enable(!v);
#endif
}

#ifndef XYCONVERT
void XyFileBrowser::OnTitleCheckBox(wxCommandEvent& event)
{
    if (!event.IsChecked())
        update_title_from_file();
    title_tc->Enable(event.IsChecked());
}
#endif

void XyFileBrowser::update_block_list()
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

void XyFileBrowser::update_title_from_file()
{
#ifndef XYCONVERT
    if (title_cb->GetValue())
        return;
    wxArrayString paths;
    filectrl->GetPaths(paths);
    string title;
    if (paths.GetCount() >= 1) {
        title = get_file_basename(wx2s(paths[0]));
        int x_idx = x_column->GetValue();
        int y_idx = y_column->GetValue();
        if (x_idx != 1 || y_idx != 2 || std_dev_cb->GetValue())
            title += ":" + S(x_idx) + ":" + S(y_idx);
    }

    title_tc->ChangeValue(s2wx(title));
#endif
}

void XyFileBrowser::OnAutoTextCheckBox (wxCommandEvent& event)
{
    if (event.IsChecked())
        update_text_preview();
    else
        text_preview->Clear();
}

void XyFileBrowser::OnAutoPlotCheckBox(wxCommandEvent& event)
{
    update_plot_preview();
    if (event.IsChecked())
        update_title_from_file();
}

void XyFileBrowser::OnBlockChanged(wxCommandEvent&)
{
    update_plot_preview();
}

void XyFileBrowser::OnColumnChanged(wxSpinEvent&)
{
    if (auto_plot_cb->GetValue())
        update_plot_preview();
    update_title_from_file();
}

wxString XyFileBrowser::get_one_path()
{
    wxArrayString a;
    filectrl->GetPaths(a);
    if (a.GetCount() == 1)
        return a[0];
    else
        return wxEmptyString;
}

void XyFileBrowser::update_text_preview()
{
    static char buffer[65536];
    const int buf_size = 65536;
    fill(buffer, buffer+buf_size, 0);
    text_preview->Clear();
    wxString path = get_one_path();
    if (!path.empty() && wxFileExists(path)) {
        wxFile(path).Read(buffer, buf_size-1);
        text_preview->SetValue(pchar2wx(buffer));
    }
}

void XyFileBrowser::update_plot_preview()
{
    if (auto_plot_cb->GetValue()) {
        wxString path = get_one_path();
        if (!path.IsEmpty()) {
            plot_preview->idx_x = x_column->GetValue();
            plot_preview->idx_y = y_column->GetValue();
            plot_preview->block_nr = block_ch->GetSelection();
            plot_preview->load_dataset(wx2s(path), get_filetype(), "");
        }
    }
    else
        plot_preview->make_outdated();
    plot_preview->refresh();
}

string XyFileBrowser::get_filetype() const
{
    int idx = filectrl->GetFilterIndex();
    if (idx > 0)
        return xylib_get_format(idx - 1)->name;
    else
        return "";
}

void XyFileBrowser::update_file_options()
{
    if (auto_text_cb->GetValue())
        update_text_preview();
    block_ch->SetSelection(0);
    if (auto_plot_cb->GetValue())
        update_plot_preview();
    update_block_list();
    update_title_from_file();
}

#ifdef XYCONVERT

#include <wx/aboutdlg.h>
#include <wx/filepicker.h>
#include "img/xyconvert16.xpm"
#include "img/xyconvert48.xpm"

class App : public wxApp
{
public:
    bool OnInit();
    void OnAbout(wxCommandEvent&);
    void OnConvert(wxCommandEvent&);
    void OnClose(wxCommandEvent&) { GetTopWindow()->Close(); }
    void OnDirCheckBox(wxCommandEvent&);
    void OnFolderChanged(wxFileCtrlEvent& event);
private:
    wxCheckBox *dir_cb, *overwrite, *header;
    wxDirPickerCtrl *dirpicker;
    XyFileBrowser *browser;
    wxTextCtrl *ext_tc;
};

DECLARE_APP(App)
IMPLEMENT_APP(App)


static const wxCmdLineEntryDesc cmdLineDesc[] = {
    { wxCMD_LINE_SWITCH, "V", "version",
          "output version information and exit", wxCMD_LINE_VAL_NONE, 0 },
    { wxCMD_LINE_PARAM,  0, 0, "default-path", wxCMD_LINE_VAL_STRING,
                                                wxCMD_LINE_PARAM_OPTIONAL},
    { wxCMD_LINE_NONE, 0, 0, 0,  wxCMD_LINE_VAL_NONE, 0 }
};


bool App::OnInit()
{
    // to make life simpler, use the same version number as xylib
    wxString version = xylib_get_version();

    // reading numbers won't work with decimal points different than '.'
    setlocale(LC_NUMERIC, "C");

    SetAppName("xyConvert");
    wxCmdLineParser cmdLineParser(cmdLineDesc, argc, argv);
    if (cmdLineParser.Parse(false) != 0) {
        cmdLineParser.Usage();
        return false;
    }
    if (cmdLineParser.Found(wxT("V"))) {
        wxMessageOutput::Get()->Printf("xyConvert, powered by xylib "
                                       + version + "\n");
        return false;
    }

    wxFrame *frame = new wxFrame(NULL, wxID_ANY, "xyConvert");

    //frame->SetIcon(wxICON(xyconvert));
#ifdef __WXMSW__
    frame->SetIcon(wxIcon("xyconvert")); // load from a resource
#else
    wxIconBundle ib;
    ib.AddIcon(wxIcon(xyconvert48_xpm));
    ib.AddIcon(wxIcon(xyconvert16_xpm));
    frame->SetIcons(ib);
#endif

    wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
    browser = new XyFileBrowser(frame);
    sizer->Add(browser, wxSizerFlags(1).Expand());

    wxStaticBoxSizer *outsizer = new wxStaticBoxSizer(wxVERTICAL, frame,
                                                      "TSV output");
    wxBoxSizer *hsizer = new wxBoxSizer(wxHORIZONTAL);
    dir_cb = new wxCheckBox(frame, wxID_ANY, "directory:");
    hsizer->Add(dir_cb, wxSizerFlags().Centre().Border());
    dirpicker = new wxDirPickerCtrl(frame, wxID_ANY);
    hsizer->Add(dirpicker, wxSizerFlags(1));
    hsizer->AddSpacer(10);
    hsizer->Add(new wxStaticText(frame, wxID_ANY, "extension:"),
                  wxSizerFlags().Centre().Border());
    ext_tc = new wxTextCtrl(frame, wxID_ANY, "xy");
    ext_tc->SetMinSize(wxSize(50, -1));
    hsizer->Add(ext_tc, wxSizerFlags().Centre());
    hsizer->AddSpacer(10);
    overwrite = new wxCheckBox(frame, wxID_ANY, "allow overwrite");
    hsizer->Add(overwrite, wxSizerFlags().Centre());
    outsizer->Add(hsizer, wxSizerFlags().Expand());
    header = new wxCheckBox(frame, wxID_ANY, "add header");
    outsizer->Add(header, wxSizerFlags().Border());
    sizer->Add(outsizer, wxSizerFlags().Expand().Border());

    wxBoxSizer *btn_sizer = new wxBoxSizer(wxHORIZONTAL);
    wxButton *about = new wxButton(frame, wxID_ABOUT);
    wxButton *convert = new wxButton(frame, wxID_CONVERT);
    wxButton *close = new wxButton(frame, wxID_EXIT);
    btn_sizer->Add(about, wxSizerFlags().Border());
    btn_sizer->AddStretchSpacer();
    btn_sizer->Add(convert, wxSizerFlags().Border());
    btn_sizer->Add(close, wxSizerFlags().Border());
    sizer->Add(btn_sizer, wxSizerFlags().Expand().Border());

    if (cmdLineParser.GetParamCount() > 0) {
        wxFileName fn(cmdLineParser.GetParam(0));
        if (fn.FileExists()) {
            browser->filectrl->SetPath(fn.GetFullPath());
            browser->update_file_options();
        }
    }
    dirpicker->SetPath(browser->filectrl->GetDirectory());
    dirpicker->Enable(false);

    frame->SetSizerAndFit(sizer);
#ifdef __WXGTK__
    frame->SetSize(-1, 550);
#endif

#ifdef __WXMSW__
    // wxMSW bug workaround
    frame->SetBackgroundColour(browser->GetBackgroundColour());
#endif

    frame->Show();

    Connect(dir_cb->GetId(), wxEVT_COMMAND_CHECKBOX_CLICKED,
            (wxObjectEventFunction) &App::OnDirCheckBox);
    browser->Connect(browser->filectrl->GetId(), wxEVT_FILECTRL_FOLDERCHANGED,
            (wxObjectEventFunction) &App::OnFolderChanged, NULL, this);

    Connect(about->GetId(), wxEVT_COMMAND_BUTTON_CLICKED,
            (wxObjectEventFunction) &App::OnAbout);
    Connect(convert->GetId(), wxEVT_COMMAND_BUTTON_CLICKED,
            (wxObjectEventFunction) &App::OnConvert);
    Connect(close->GetId(), wxEVT_COMMAND_BUTTON_CLICKED,
            (wxObjectEventFunction) &App::OnClose);
    return true;
}

void App::OnConvert(wxCommandEvent&)
{
    bool with_header = header->GetValue();
    int block_nr = browser->block_ch->GetSelection();
    int idx_x = browser->x_column->GetValue();
    int idx_y = browser->y_column->GetValue();
    bool has_err = browser->std_dev_cb->GetValue();
    int idx_err = browser->s_column->GetValue();

    wxArrayString paths;
    browser->filectrl->GetPaths(paths);
    string options;

    for (size_t i = 0; i < paths.GetCount(); ++i) {
        wxFileName old_filename(paths[i]);
        wxString fn = old_filename.GetName() + "." + ext_tc->GetValue();
        wxString new_filename = dirpicker->GetPath() + wxFILE_SEP_PATH + fn;
        if (!overwrite->GetValue() && wxFileExists(new_filename)) {
            int answer = wxMessageBox("File " + fn + " exists.\n"
                                      "Overwrite?",
                                      "Overwrite?",
                                      wxYES|wxNO|wxCANCEL|wxICON_QUESTION);
            if (answer == wxCANCEL)
                break;
            if (answer != wxYES)
                continue;

        }
        FILE *f = fopen(new_filename.mb_str(), "w");
        try {
            wxBusyCursor wait;
            xylib::DataSet const *ds = xylib::load_file(wx2s(paths[i]),
                                            browser->get_filetype(), options);
            xylib::Block const *block = ds->get_block(block_nr);
            xylib::Column const& xcol = block->get_column(idx_x);
            xylib::Column const& ycol = block->get_column(idx_y);
            xylib::Column const* ecol = (has_err ? &block->get_column(idx_err)
                                                 : NULL);
            const int np = block->get_point_count();

            if (with_header) {
                fprintf(f, "# converted by xyConvert %s from file:\n# %s\n",
                        xylib_get_version(),
                        wx2s(new_filename).c_str());
                if (ds->get_block_count() > 1)
                    fprintf(f, "# (block %d) %s\n", block_nr,
                                                    block->get_name().c_str());
                if (block->get_column_count() > 2) {
                    string xname = (xcol.get_name().empty() ? string("x")
                                                            : xcol.get_name());
                    string yname = (ycol.get_name().empty() ? string("y")
                                                            : ycol.get_name());
                    fprintf(f, "#%s\t%s", xname.c_str(), yname.c_str());
                    if (has_err) {
                        string ename = (ecol->get_name().empty() ? string("err")
                                                            : ecol->get_name());
                        fprintf(f, "\t%s", ename.c_str());
                    }
                    fprintf(f, "\n");
                }
            }

            for (int j = 0; j < np; ++j) {
                fprintf(f, "%.9g\t%.9g", xcol.get_value(j), ycol.get_value(j));
                if (has_err)
                    fprintf(f, "\t%.9g", ecol->get_value(j));
                fprintf(f, "\n");
            }
        } catch (runtime_error const& e) {
            wxMessageBox(e.what(), "Error", wxCANCEL|wxICON_ERROR);
        }
        fclose(f);
    }
}

void App::OnAbout(wxCommandEvent&)
{
    wxAboutDialogInfo adi;
    adi.SetVersion(xylib_get_version());
    wxString desc = "A simple converter of files supported by xylib library\n"
                    "to two- or three-column text format.\n";
    adi.SetDescription(desc);
    adi.SetWebSite("http://www.nieto.pl/xyconvert/");
    adi.SetCopyright("(C) 2008-2013 Marcin Wojdyr <wojdyr@gmail.com>");
    wxAboutBox(adi);
}

void App::OnDirCheckBox(wxCommandEvent& event)
{
    bool checked = event.IsChecked();
    dirpicker->Enable(checked);
    if (!checked)
        dirpicker->SetPath(browser->filectrl->GetDirectory());
}

void App::OnFolderChanged(wxFileCtrlEvent& event)
{
    if (!dir_cb->GetValue())
        dirpicker->SetPath(event.GetDirectory());
}

#endif


// Author: Marcin Wojdyr 
// Licence: GNU General Public License version 2 or (at your option) 3
// $Id$

#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>  // >= 2.9
#endif
#include <wx/filectrl.h>
#include <wx/filepicker.h>
#include <wx/filename.h>
#include <wx/cmdline.h>
//#include <wx/statline.h>
#include <wx/splitter.h>
#include <wx/spinctrl.h>
#include <wx/file.h>

#include <xylib/xylib.h>

#include "uplot.h" // BufferedPanel, scale_tics_step()


//#include "cmn.h"
// --------- copied from cmn.h
class SpinCtrl: public wxSpinCtrl
{
public:
    SpinCtrl(wxWindow* parent, wxWindowID id, int val, 
             int min, int max, int width=50)
        : wxSpinCtrl (parent, id, wxString::Format(wxT("%i"), val),
                      wxDefaultPosition, wxSize(width, -1), 
                      wxSP_ARROW_KEYS, min, max, val) 
    {}
};

inline wxString pchar2wx(char const* pc)
{
    return wxString(pc, wxConvUTF8);
}

inline std::string wx2s(wxString const& w) 
{ 
    return std::string((const char*) w.mb_str(wxConvUTF8)); 
}

void updateControlWithItems(wxControlWithItems *cwi, 
                            std::vector<std::string> const& v)
{
    if (v.size() != (size_t) cwi->GetCount()) {
        cwi->Clear();
        for (size_t i = 0; i < v.size(); ++i)
            cwi->Append(v[i]);
    }
    else
        for (size_t i = 0; i < v.size(); ++i)
            if (cwi->GetString(i) != v[i])
                cwi->SetString(i, v[i]);
}

// --------- end

// --------- copied from ../common.h
#include <sstream>

/// Round real to integer.
inline int iround(double d) { return static_cast<int>(floor(d+0.5)); }

/// S() converts to string
template <typename T>
inline std::string S(T k) {
    return static_cast<std::ostringstream&>(std::ostringstream() << k).str();
}

inline std::string S(bool b) { return b ? "true" : "false"; }
inline std::string S(char const *k) { return std::string(k); }
inline std::string S(char *k) { return std::string(k); }
inline std::string S(char const k) { return std::string(1, k); }
inline std::string S(std::string const &k) { return k; }
inline std::string S() { return std::string(); }


// --------- end


using namespace std;

enum {
    ID_STDDEV_CB     =22200,
    ID_BLOCK               ,
    ID_COLX                ,
    ID_COLY                ,
    ID_SDS                 ,
    ID_HTITLE              ,
    ID_AUTO_TEXT           ,
    ID_AUTO_PLOT           ,
};

class PreviewPlot : public BufferedPanel
{
public:
    int block_nr, idx_x, idx_y;

    PreviewPlot(wxWindow* parent)
        : BufferedPanel(parent), block_nr(0), idx_x(1), idx_y(2),
          data(NULL), data_updated(false)
        { backgroundCol = *wxBLACK; }

    ~PreviewPlot() { delete data; }
    void OnPaint(wxPaintEvent &event);
    void draw(wxDC &dc, bool);
    void load_dataset(string const& filename, string const& filetype,
                      vector<string> const& options);
    xylib::DataSet* const get_data() { return data_updated ? data : NULL; }
    void make_outdated() { data_updated = false; }

private:
    xylib::DataSet* data;
    bool data_updated;

    double xScale, yScale;
    double xOffset, yOffset;
    int getX(double x) { return iround(x * xScale + xOffset); }
    int getY(double y) { return iround(y * yScale + yOffset); }

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
    if (!data || !data_updated 
        || block_nr < 0 || block_nr >= data->get_block_count())
        return;

    xylib::Block const *block = data->get_block(block_nr);

    if (idx_x < 0 || idx_x > block->get_column_count() 
            || idx_y < 0 || idx_y > block->get_column_count()) 
        return;

    xylib::Column const& xcol = block->get_column(idx_x);
    xylib::Column const& ycol = block->get_column(idx_y);
    const int np = block->get_point_count();

    // prepare scaling
    double const margin = 0.1;
    double dx = xcol.get_max(np) - xcol.get_min();
    double dy = ycol.get_max(np) - ycol.get_min();
    int W = dc.GetSize().GetWidth();
    int H = dc.GetSize().GetHeight();
    xScale = (1 - 1.2 * margin) *  W / dx;
    yScale = - (1 - 1.2 * margin) * H / dy;
    xOffset = - xcol.get_min() * xScale + margin * W;
    yOffset = H - ycol.get_min() * yScale - margin * H;
    
    // draw scale
    dc.SetPen(*wxWHITE_PEN);
    dc.SetTextForeground(*wxWHITE);
    dc.SetFont(*wxSMALL_FONT);  

    // ... horizontal
    vector<double> minors;
    vector<double> tics 
        = scale_tics_step(xcol.get_min(), xcol.get_max(np), 4, minors);
    for (vector<double>::const_iterator i = tics.begin(); i != tics.end(); ++i){
        int X = getX(*i);
        wxString label = S(*i);
        wxCoord tw, th;
        dc.GetTextExtent (label, &tw, &th);
        int Y = dc.DeviceToLogicalY(H - th - 2);
        dc.DrawText (label, X - tw/2, Y + 1);
        dc.DrawLine (X, Y, X, Y - 4);
    }
    if (!xcol.name.empty()) {
        wxCoord tw, th;
        dc.GetTextExtent (xcol.name, &tw, &th);
        dc.DrawText (xcol.name, (W - tw)/2, 2);
    }

    // ... vertical
    tics = scale_tics_step(ycol.get_min(), ycol.get_max(np), 4, minors);
    for (vector<double>::const_iterator i = tics.begin(); i != tics.end(); ++i){
        int Y = getY(*i);
        wxString label = S(*i);
        wxCoord tw, th;
        dc.GetTextExtent (label, &tw, &th);
        dc.DrawText (label, dc.DeviceToLogicalX(5), Y - th/2);
        dc.DrawLine (dc.DeviceToLogicalX(0), Y, dc.DeviceToLogicalX(4), Y);
    }
    if (!ycol.name.empty()) {
        wxCoord tw, th;
        dc.GetTextExtent (ycol.name, &tw, &th);
        dc.DrawRotatedText (ycol.name, W - 2, (H - tw)/2, 270);
    }

    // draw data
    dc.SetPen(*wxGREEN_PEN);
    for (int i = 0; i < np; ++i)
        dc.DrawPoint(getX(xcol.get_value(i)), getY(ycol.get_value(i)));
}


// wrapper around xylib::load_file()
// with added caching (if the same filename and options are given)
void PreviewPlot::load_dataset(string const& filename, 
                               string const& filetype,
                               vector<string> const& options)
{
    static string old_filename;
    static string old_filetype;
    static vector<string> old_options;
    if (filename == old_filename && filetype == old_filetype 
                                                && options == old_options) {
        data_updated = true;
        return;
    }

    try {
        // if xylib::load_file() throws exception, we keep value of data
        xylib::DataSet *new_ds = xylib::load_file(filename, filetype, options);
        assert(new_ds);
        delete data;
        data = new_ds;
        old_filename = filename;
        old_filetype = filetype;
        old_options = options;
        data_updated = true;
    } catch (runtime_error const& e) {
        data_updated = false;
    }
}



class XyFileBrowser : public wxSplitterWindow
{
public:
    wxFileCtrl *filectrl;
    wxSpinCtrl *x_column, *y_column, *s_column;
    wxCheckBox *std_dev_cb;
    wxChoice *block_ch;

    XyFileBrowser(wxWindow* parent, wxWindowID id);
    void SetPath(wxString const& path) 
        { filectrl->SetPath(path); on_path_change(); }
    string get_filetype() const;

private:
#if 0
    wxTextCtrl *title_tc; 
#endif
    wxTextCtrl *text_preview;
    PreviewPlot *plot_preview;
    wxCheckBox *auto_text_cb, *auto_plot_cb;
#if 0
    wxCheckBox *sd_sqrt_cb, *title_cb;
#endif

    void StdDevCheckBoxChanged();
    void OnStdDevCheckBox(wxCommandEvent&) { StdDevCheckBoxChanged(); }
#if 0
    void OnHTitleCheckBox (wxCommandEvent& event);
#endif
    void OnAutoTextCheckBox (wxCommandEvent& event);
    void OnAutoPlotCheckBox (wxCommandEvent& event);
    void OnColumnChanged (wxSpinEvent& event);
    void OnBlockChanged (wxCommandEvent& event);
    void on_path_change();
    void OnPathChanged(wxFileCtrlEvent&) { on_path_change(); }
    void update_text_preview();
    void update_plot_preview();
    void update_block_list();
#if 0
    void update_title_from_file();
#endif
    wxString get_one_path();

    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(XyFileBrowser, wxSplitterWindow)
    EVT_CHECKBOX    (ID_STDDEV_CB, XyFileBrowser::OnStdDevCheckBox)
    EVT_CHECKBOX    (ID_AUTO_TEXT, XyFileBrowser::OnAutoTextCheckBox)
    EVT_CHECKBOX    (ID_AUTO_PLOT, XyFileBrowser::OnAutoPlotCheckBox)
    EVT_SPINCTRL    (ID_COLX,      XyFileBrowser::OnColumnChanged)
    EVT_SPINCTRL    (ID_COLY,      XyFileBrowser::OnColumnChanged)
    EVT_CHOICE      (ID_BLOCK,     XyFileBrowser::OnBlockChanged)
    EVT_FILECTRL_SELECTIONCHANGED(wxID_ANY, XyFileBrowser::OnPathChanged)
END_EVENT_TABLE()

XyFileBrowser::XyFileBrowser(wxWindow* parent, wxWindowID id)
    : wxSplitterWindow(parent, id, wxDefaultPosition, wxDefaultSize,
                       wxSP_NOBORDER), 
      auto_plot_cb(NULL)
{
    // +----------------------------+
    // |            | rupper_panel  |
    // | left_panel +---------------+
    // |            | rbottom_panel |
    // +----------------------------+

    this->SetSashGravity(0.5);
    this->SetMinimumPaneSize(20);
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
                    + "|" + xylib::get_wildcards_string();
    filectrl = new wxFileCtrl(left_panel, -1, wxEmptyString, wxEmptyString,
                              wild, wxFC_OPEN|wxFC_MULTIPLE|wxFC_NOSHOWHIDDEN);
    left_sizer->Add(filectrl, 1, wxALL|wxEXPAND, 5);

    // selecting block
    wxBoxSizer *block_sizer = new wxBoxSizer(wxHORIZONTAL);
    block_ch = new wxChoice(left_panel, ID_BLOCK);
    block_sizer->Add(new wxStaticText(left_panel, -1, "block:"), 
                     wxSizerFlags().Border(wxRIGHT).Center());
    block_sizer->Add(block_ch, wxSizerFlags(1));
    left_sizer->Add(block_sizer, wxSizerFlags().Border().Expand());

    // selecting columns
    wxPanel *columns_panel = new wxPanel (left_panel, -1);
    wxStaticBoxSizer *h2a_sizer = new wxStaticBoxSizer(wxHORIZONTAL, 
                    columns_panel, wxT("Select columns (0 for point index)"));
    h2a_sizer->Add (new wxStaticText (columns_panel, -1, wxT("x")), 
                    0, wxALL|wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL, 5);
    x_column = new SpinCtrl (columns_panel, ID_COLX, 1, 0, 99, 50);
    h2a_sizer->Add (x_column, 0, wxALL|wxALIGN_LEFT, 5);
    h2a_sizer->Add (new wxStaticText (columns_panel, -1, wxT("y")), 
                    0, wxALL|wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL, 5);
    y_column = new SpinCtrl (columns_panel, ID_COLY, 2, 0, 99, 50);
    h2a_sizer->Add (y_column, 0, wxALL|wxALIGN_LEFT, 5);
    std_dev_cb = new wxCheckBox(columns_panel, ID_STDDEV_CB, 
                                wxT("std.dev."));
    std_dev_cb->SetValue(false);
    h2a_sizer->Add(std_dev_cb, 0, wxALL|wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL,5);
    s_column = new SpinCtrl(columns_panel, -1, 3, 1, 99, 50);
    h2a_sizer->Add(s_column, 0, wxALL|wxALIGN_LEFT, 5);
    columns_panel->SetSizer(h2a_sizer);
    left_sizer->Add (columns_panel, 0, wxALL|wxEXPAND, 5);

#if 0
    bool def_sqrt = (ftk->get_settings()->getp("data-default-sigma") == "sqrt");
    sd_sqrt_cb = new wxCheckBox(left_panel, ID_SDS, 
                                wxT("set std. dev. as max(sqrt(y), 1.0)"));
    sd_sqrt_cb->SetValue(def_sqrt);
    left_sizer->Add (sd_sqrt_cb, 0, wxALL|wxEXPAND, 5);

    wxStaticBoxSizer *dt_sizer = new wxStaticBoxSizer(wxVERTICAL, 
                                    left_panel, wxT("Data title:"));
    title_cb = new wxCheckBox(left_panel, ID_HTITLE, 
                              wxT("custom title"));
    dt_sizer->Add(title_cb, 0, wxALL, 5);
    title_tc = new wxTextCtrl(left_panel, -1, wxT(""));
    title_tc->Enable(false);
    dt_sizer->Add(title_tc, 0, wxALL|wxEXPAND, 5);
    left_sizer->Add (dt_sizer, 0, wxALL|wxEXPAND, 5);
#endif

    // ----- right upper panel -----
    text_preview =  new wxTextCtrl(rupper_panel, -1, wxT(""), 
                                   wxDefaultPosition, wxDefaultSize,
                                   wxTE_RICH|wxTE_READONLY|wxTE_MULTILINE);
    rupper_sizer->Add(text_preview, 1, wxEXPAND|wxALL, 5);
    auto_text_cb = new wxCheckBox(rupper_panel, ID_AUTO_TEXT, 
                                  wxT("view the first 64kB of file as text"));
    auto_text_cb->SetValue(false);
    rupper_sizer->Add(auto_text_cb, 0, wxALL, 5);

    // ----- right bottom panel -----
    plot_preview = new PreviewPlot(rbottom_panel);
    rbottom_sizer->Add(plot_preview, 1, wxEXPAND|wxALL, 5);
    auto_plot_cb = new wxCheckBox(rbottom_panel, ID_AUTO_PLOT, 
                                  wxT("plot"));
    auto_plot_cb->SetValue(true);
    rbottom_sizer->Add(auto_plot_cb, 0, wxALL, 5);

    // ------ finishing layout -----------
    left_panel->SetSizerAndFit(left_sizer);
    rupper_panel->SetSizerAndFit(rupper_sizer);
    rbottom_panel->SetSizerAndFit(rbottom_sizer);
    this->SplitVertically(left_panel, right_splitter);
    right_splitter->SplitHorizontally(rupper_panel, rbottom_panel);

    StdDevCheckBoxChanged();
    update_block_list();
}

void XyFileBrowser::StdDevCheckBoxChanged()
{
    bool v = std_dev_cb->GetValue();
    s_column->Enable(v);
#if 0
    sd_sqrt_cb->Enable(!v);
#endif
}

#if 0
void XyFileBrowser::OnHTitleCheckBox (wxCommandEvent& event)
{
    if (!event.IsChecked()) 
        update_title_from_file();
    title_tc->Enable(event.IsChecked());
}
#endif

void XyFileBrowser::update_block_list()
{
    vector<string> bb;
    if (plot_preview && plot_preview->get_data())
        for (int i = 0; i < plot_preview->get_data()->get_block_count(); ++i) {
            const string& name = plot_preview->get_data()->get_block(i)->name;
            bb.push_back(name.empty() ? "Block #" + S(i+1) : name);
        }
    else {
        bb.push_back("<default block>");
    }
    updateControlWithItems(block_ch, bb);
    block_ch->SetSelection(0);
    block_ch->Enable(block_ch->GetCount() > 1);
}

#if 0
void XyFileBrowser::update_title_from_file()
{
    if (title_cb->GetValue()) 
        return;
    string path = filectrl->GetFilePath();
    if (path.empty()) {
        title_tc->Clear();
        return;
    }
    string title = get_file_basename(path);
    int x = x_column->GetValue();
    int y = y_column->GetValue();
    if (x != 1 || y != 2 || std_dev_cb->GetValue()) 
        title += ":" + S(x) + ":" + S(y);
    title_tc->SetValue(title);
}
#endif

void XyFileBrowser::OnAutoTextCheckBox (wxCommandEvent& event)
{
    if (event.IsChecked())
        update_text_preview();
    else
        text_preview->Clear();
}

void XyFileBrowser::OnAutoPlotCheckBox (wxCommandEvent& /*event*/)
{
    update_plot_preview();
#if 0
    if (event.IsChecked()) 
        update_title_from_file();
#endif
}

void XyFileBrowser::OnBlockChanged (wxCommandEvent&)
{
    update_plot_preview();
}

void XyFileBrowser::OnColumnChanged (wxSpinEvent&)
{
    if (auto_plot_cb->GetValue()) 
        update_plot_preview();
#if 0
    update_title_from_file();
#endif
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
    const int buf_size = sizeof(buffer) / sizeof(buffer[0]);

    text_preview->Clear();
    wxString path = get_one_path();
    if (path.IsEmpty())
        return;
    if (wxFileExists(path)) {
        fill(buffer, buffer+buf_size, 0);
        wxFile(path).Read(buffer, buf_size-1);
        text_preview->SetValue(pchar2wx(buffer));
    }
}

void XyFileBrowser::update_plot_preview()
{
    plot_preview->make_outdated();
    if (auto_plot_cb->GetValue()) {
        wxString path = get_one_path();
        if (!path.IsEmpty()) {
            vector<string> options;
            plot_preview->load_dataset(wx2s(path), get_filetype(), options);
            plot_preview->idx_x = x_column->GetValue();
            plot_preview->idx_y = y_column->GetValue();
            plot_preview->block_nr = block_ch->GetSelection();
        }
    }
    plot_preview->refresh();
}

string XyFileBrowser::get_filetype() const
{
    int idx = filectrl->GetFilterIndex();
    if (idx > 0)
        return xylib::get_format(idx - 1)->name;
    else
        return "";
}

void XyFileBrowser::on_path_change()
{
    if (!auto_plot_cb) // ctor not finished yet
        return;
    if (auto_text_cb->GetValue())
        update_text_preview();
    block_ch->SetSelection(0);
    if (auto_plot_cb->GetValue()) 
        update_plot_preview();
    update_block_list();
#if 0
    update_title_from_file();
#endif
}

#ifdef XYCONVERT

#include <wx/aboutdlg.h>
#include "img/xyconvert16.xpm"
#include "img/xyconvert48.xpm"

class App : public wxApp
{
public:
    wxString version;

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
    version = xylib::get_version();

    // reading numbers won't work with decimal points different than '.'
    setlocale(LC_NUMERIC, "C");

    SetAppName("xyConvert");
    wxCmdLineParser cmdLineParser(cmdLineDesc, argc, argv);
    if (cmdLineParser.Parse(false) != 0) {
        cmdLineParser.Usage();
        return false; 
    }
    if (cmdLineParser.Found(wxT("V"))) {
        wxMessageOutput::Get()->Printf("xyConvert " + version 
                                   + ", powered by xylib " + version + "\n");
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
    frame->SetIcons(ib); // load from a resource
#endif

    wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
    browser = new XyFileBrowser(frame, wxID_ANY);
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
    wxButton *convert = new wxButton(frame, wxID_ANY, "Con&vert");
    wxButton *close = new wxButton(frame, wxID_CLOSE);
    btn_sizer->Add(about, wxSizerFlags().Border());
    btn_sizer->AddStretchSpacer();
    btn_sizer->Add(convert, wxSizerFlags().Border());
    btn_sizer->Add(close, wxSizerFlags().Border());
    sizer->Add(btn_sizer, wxSizerFlags().Expand().Border());

    if (cmdLineParser.GetParamCount() > 0) {
        wxFileName fn(cmdLineParser.GetParam(0));
        if (fn.FileExists()) 
            browser->SetPath(fn.GetFullPath());
    }
    dirpicker->SetPath(browser->filectrl->GetDirectory());
    dirpicker->Enable(false);
    
    frame->SetSizerAndFit(sizer);
#ifdef __WXGTK__
    frame->SetSize(-1, 550);
#endif
    
    // wxMSW bug workaround
    frame->SetBackgroundColour(browser->GetBackgroundColour());

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
    vector<string> options;

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
        ofstream f(new_filename);
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
                f << "# converted by xyConvert " << version << " from file:\n"; 
                f << "# " << new_filename << endl;
                if (ds->get_block_count() > 1)
                    f << "# (block " << block_nr << ") " << block->name << endl;
                if (block->get_column_count() > 2) {
                    f << "#" << (xcol.name.empty() ? string("x") : xcol.name) 
                      << "\t" << (ycol.name.empty() ? string("y") : ycol.name);
                    if (has_err) 
                        f << "\t" << (ecol->name.empty() ? string("err") 
                                                         : ecol->name);
                    f << endl; 
                }
            }

            for (int i = 0; i < np; ++i) {
                f << xcol.get_value(i) << "\t" << ycol.get_value(i);
                if (has_err)
                    f << "\t" << ecol->get_value(i);
                f << endl;
            }
        } catch (runtime_error const& e) {
            wxMessageBox(e.what(), "Error", wxCANCEL|wxICON_ERROR);
        }
    }
}

void App::OnAbout(wxCommandEvent&)
{
    wxAboutDialogInfo adi;
    adi.SetVersion(version);
    wxString desc = "A simple converter of files supported by xylib library\n"
                    "to two- or three-column text format.\n";
    adi.SetDescription(desc);
    adi.SetWebSite("http://www.unipress.waw.pl/fityk/xyconvert/");
    wxString copyright = "(C) 2008 Marcin Wojdyr <wojdyr@gmail.com>";
#ifdef __WXGTK__
    copyright.Replace("(C)", "\xc2\xa9");
#endif
    adi.SetCopyright(copyright);
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


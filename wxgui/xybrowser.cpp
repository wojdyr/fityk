// Author: Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
// (It is also part of xyconvert and can be distributed under LGPL2.1)

#include <wx/wx.h>
#include <wx/file.h>

#include "xylib/xylib.h"
#include "xylib/cache.h"

#include "xybrowser.h"

using namespace std;

#ifndef XYCONVERT
#include "cmn.h"
#include "fityk/data.h"
using fityk::get_file_basename;
#else
// copied from common.h
#ifdef _MSC_VER
#define snprintf sprintf_s
#endif
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

// copied from cmn.h
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
                    + "|" + wxString(xylib::get_wildcards_string());
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
    std_dev_cb = new wxCheckBox(columns_panel, -1, wxT("\u03C3"));
    std_dev_cb->SetValue(false);
    h2a_sizer->Add(std_dev_cb, 0, wxALL|wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL,5);
    s_column = new SpinCtrl(columns_panel, wxID_ANY, 3, 1, 999, 50);
    h2a_sizer->Add(s_column, 0, wxALL|wxALIGN_LEFT, 5);
#ifndef XYCONVERT
    h2a_sizer->Add(new wxStaticText(columns_panel, wxID_ANY, "or"),
                   0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    sd_sqrt_cb = new wxCheckBox(columns_panel, wxID_ANY,
                                wxT("\u03C3 = max{\u221Ay, 1}"));
    h2a_sizer->Add(sd_sqrt_cb, 0, wxALL|wxEXPAND, 5);
#endif
    columns_panel->SetSizer(h2a_sizer);
    left_sizer->Add (columns_panel, 0, wxALL|wxEXPAND, 5);

    comma_cb = new wxCheckBox(left_panel, wxID_ANY, "decimal comma");
    comma_cb->SetValue(false);
    left_sizer->Add(comma_cb, 0, wxALL|wxEXPAND, 5);

#ifndef XYCONVERT
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
                                   wxDefaultPosition, wxSize(300, -1),
                                   wxTE_RICH|wxTE_READONLY|wxTE_MULTILINE);
    rupper_sizer->Add(text_preview, 1, wxEXPAND|wxALL, 5);
    auto_text_cb = new wxCheckBox(rupper_panel, -1,
                                  wxT("file preview (64kB)"));
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
    Connect(comma_cb->GetId(), wxEVT_COMMAND_CHECKBOX_CLICKED,
            wxCommandEventHandler(XyFileBrowser::OnCommaCheckBox));
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
        } else {
        bb.push_back("<default block>");
    }

    if (bb.size() != (size_t) block_ch->GetCount()) {
        block_ch->Clear();
        for (size_t i = 0; i < bb.size(); ++i)
            block_ch->Append(wxString(bb[i]));
    } else {
        for (size_t i = 0; i < bb.size(); ++i)
            if (block_ch->GetString(i) != wxString(bb[i]))
                block_ch->SetString(i, wxString(bb[i]));
    }

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
        title = get_file_basename(paths[0].ToStdString());
        int x_idx = x_column->GetValue();
        int y_idx = y_column->GetValue();
        if (x_idx != 1 || y_idx != 2 || std_dev_cb->GetValue())
            title += ":" + S(x_idx) + ":" + S(y_idx);
    }

    title_tc->ChangeValue(wxString(title));
#endif
}

void XyFileBrowser::OnCommaCheckBox(wxCommandEvent&)
{
    update_plot_preview();
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
        int bytes_read = wxFile(path).Read(buffer, buf_size-1);
        wxString str(buffer); // implicit conversion using current locale
        if (str.empty())
            str = wxString::From8BitData(buffer, bytes_read);
        // remove nulls to display binary files (it looks better than randomly
        // truncated binary file)
        for (wxString::iterator i = str.begin(); i != str.end(); ++i)
            if (*i == '\0')
                *i = '\1';
        text_preview->SetValue(str);
        if (!str.empty() && bytes_read == buf_size-1) {
            text_preview->SetDefaultStyle(wxTextAttr(*wxBLACK, *wxYELLOW));
            text_preview->AppendText(
                    "\nThis preview shows only the first 64kb of file.\n");
            text_preview->SetDefaultStyle(wxTextAttr());
            text_preview->ShowPosition(0);
        }
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
            string options;
            if (comma_cb->GetValue())
                options = "decimal-comma";
            plot_preview->load_dataset((const char*) path.ToUTF8(),
                                       get_filetype(),
                                       options);
        }
    } else
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



// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
///  Custom Data Load Dialog (DLoadDlg) and helpers

#include <vector>
#include <wx/wx.h>
#include <wx/statline.h>
#include <wx/file.h>
#include <wx/filename.h>

#include <xylib/xylib.h>
#include <xylib/cache.h>

#include "dload.h"
#include "xybrowser.h"
#include "frame.h"  // frame->add_recent_data_file()
#include "plot.h" // scale_tics_step()
#include "fityk/logic.h" // ftk->get_settings()
#include "fityk/settings.h"
#include "fityk/data.h" // get_file_basename()
#include "fityk/common.h"

using namespace std;
using fityk::Data;

/// data_idx - data slot to be used by "Replace ..." button, -1 means none
/// data - used for default settings (path, columns, etc.), not NULL
DLoadDlg::DLoadDlg(wxWindow* parent, int data_idx, Data* data,
                   const wxString& dir)
    : wxDialog(parent, wxID_ANY, wxT("Load Data"),
               wxDefaultPosition, wxDefaultSize,
               wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER),
      data_idx_(data_idx)
{
    wxBoxSizer *top_sizer = new wxBoxSizer(wxVERTICAL);
    browser_ = new XyFileBrowser(this);
    top_sizer->Add(browser_, 1, wxEXPAND);
    top_sizer->Add(new wxStaticLine(this, -1), 0, wxEXPAND|wxLEFT|wxRIGHT, 5);
    wxBoxSizer *button_sizer = new wxBoxSizer(wxHORIZONTAL);
    wxString repl = "&Replace @" + s2wx(data_idx >= 0 ? S(data_idx) : S("?"));
    open_here_btn_ = new wxButton(this, -1, repl);
    if (data_idx < 0)
        open_here_btn_->Enable(false);
    open_new_btn_ = new wxButton(this, -1, wxT("&Open in new slot"));
    button_sizer->Add(open_here_btn_, 0, wxALL, 5);
    button_sizer->Add(open_new_btn_, 0, wxALL, 5);
    button_sizer->Add(new wxButton(this, wxID_CLOSE, wxT("&Close")),
                      0, wxALL, 5);
    top_sizer->Add(button_sizer, 0, wxALL|wxALIGN_CENTER, 0);
    SetSizerAndFit(top_sizer);
    SetSize(wxSize(700, 600));
    SetEscapeId(wxID_CLOSE);

    wxFileName path;
    if (!data->get_filename().empty())
        browser_->filectrl->SetPath(s2wx(data->get_filename()));
    else
        browser_->filectrl->SetDirectory(dir);
    browser_->update_file_options();

    if (data->get_given_x() != INT_MAX)
        browser_->x_column->SetValue(data->get_given_x());
    if (data->get_given_y() != INT_MAX)
        browser_->y_column->SetValue(data->get_given_y());
    if (data->get_given_s() != INT_MAX) {
        browser_->std_dev_cb->SetValue(true);
        browser_->s_column->SetValue(data->get_given_s());
    }

    bool def_sqrt = (S(ftk->get_settings()->default_sigma) == "sqrt");
    browser_->sd_sqrt_cb->SetValue(def_sqrt);

    Connect(open_here_btn_->GetId(), wxEVT_COMMAND_BUTTON_CLICKED,
            wxCommandEventHandler(DLoadDlg::OnOpenHere));
    Connect(open_new_btn_->GetId(), wxEVT_COMMAND_BUTTON_CLICKED,
            wxCommandEventHandler(DLoadDlg::OnOpenNew));
}


void DLoadDlg::exec_command(bool replace)
{
    string cols;
    int x = browser_->x_column->GetValue();
    int y = browser_->y_column->GetValue();
    bool has_s = browser_->std_dev_cb->GetValue();
    int b = browser_->block_ch->GetSelection();
    // default parameter values are not passed explicitely
    if (x != 1 || y != 2 || has_s || b != 0) {
        cols = ":" + S(x) + ":" + S(y) + ":";
        if (has_s)
            cols += S(browser_->s_column->GetValue());
        cols += ":";
        if (b != 0)
            cols += S(b);
    }

    string cmd;
    bool def_sqrt = (S(ftk->get_settings()->default_sigma) == "sqrt");
    bool set_sqrt = browser_->sd_sqrt_cb->GetValue();
    bool sigma_in_file = browser_->std_dev_cb->GetValue();
    if (!sigma_in_file && set_sqrt != def_sqrt) {
        if (set_sqrt)
            cmd = "with default_sigma=sqrt ";
        else
            cmd = "with default_sigma=one ";
    }
    wxArrayString paths;
    browser_->filectrl->GetPaths(paths);
    for (size_t i = 0; i < paths.GetCount(); ++i) {
        string filename = wx2s(paths[i]);
        exec(cmd + "@" + (replace ? S(data_idx_) : S("+")) +
                  " < '" + filename + cols + "'");
        if (browser_->title_tc->IsEnabled()) {
            wxString t = browser_->title_tc->GetValue().Trim();
            if (!t.IsEmpty()) {
                int slot = (replace ? data_idx_ : ftk->get_dm_count() - 1);
                exec("@" + S(slot) + ": title = '" + wx2s(t) + "'");
            }
        }
        frame->add_recent_data_file(filename);
    }
}


// This file is part of fityk program. Copyright 2001-2013 Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
///  Custom Data Load Dialog (DLoadDlg) and helpers

#include <vector>
#include <wx/wx.h>
#include <wx/statline.h>
#include <wx/file.h>
#include <wx/filename.h>

#include "dload.h"
#include "xybrowser.h"
#include "frame.h"  // frame->add_recent_data_file()
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

    if (data->get_given_x() != fityk::LoadSpec::NN)
        browser_->x_column->SetValue(data->get_given_x());
    if (data->get_given_y() != fityk::LoadSpec::NN)
        browser_->y_column->SetValue(data->get_given_y());
    if (data->get_given_s() != fityk::LoadSpec::NN) {
        browser_->std_dev_b->SetValue(true);
        browser_->s_column->SetValue(data->get_given_s());
    } else if (S(ftk->get_settings()->default_sigma) == "sqrt") {
        browser_->sd_sqrt_rb->SetValue(true);
    } else {
        browser_->sd_1_rb->SetValue(true);
    }
    browser_->update_s_column();

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
    bool has_s = browser_->std_dev_b->GetValue();
    int sig = browser_->s_column->GetValue();
    int b = browser_->block_ch->GetSelection();
    // default parameter values are not passed explicitely
    if (x != 1 || y != 2 || has_s || b != 0) {
        cols = ":" + S(x) + ":" + S(y) + ":";
        if (has_s)
            cols += S(sig);
        cols += ":";
        if (b != 0)
            cols += S(b);
    }

    string with_options;
    if (!has_s) {
        bool default_sqrt = (S(ftk->get_settings()->default_sigma) == "sqrt");
        bool set_sqrt = browser_->sd_sqrt_rb->GetValue();
        if (set_sqrt != default_sqrt) {
            if (set_sqrt)
                with_options = "with default_sigma=sqrt ";
            else
                with_options = "with default_sigma=one ";
        }
    }
    wxArrayString paths;
    browser_->filectrl->GetPaths(paths);
    string trailer;
    string fmt = browser_->get_filetype();
    if (!fmt.empty() || browser_->comma_cb->GetValue()) {
        trailer = " " + (fmt.empty() ? "_" : fmt);
        if (browser_->comma_cb->GetValue())
            trailer += " decimal_comma";
    }
    for (size_t i = 0; i < paths.GetCount(); ++i) {
        string cmd;
        if (paths[i].Find('\'') == wxNOT_FOUND)
            cmd = "@" + (replace ? S(data_idx_) : S("+")) +
                   " < '" + wx2s(paths[i]) + cols + "'" + trailer;
        else // very special case
            cmd = "lua " +
                  make_lua_load((replace ? data_idx_ : -1), paths[i], b,
                                x, y, (has_s ? sig : fityk::LoadSpec::NN),
                                fmt, browser_->comma_cb->GetValue());
        exec(with_options + cmd);
        wxString title = browser_->title_tc->GetValue().Trim();
        if (!title.empty() && title != browser_->auto_title_) {
            int slot = (replace ? data_idx_ : ftk->dk.count() - 1);
            exec("@" + S(slot) + ": title = '" + wx2s(title) + "'");
        }
        frame->add_recent_data_file(paths[i]);
    }
}

std::string make_lua_load(int data_idx, const wxString& path, int b,
                          int x, int y, int sig,
                          const std::string& fmt, bool comma) {
    std::string cmd = "_spec=fityk.LoadSpec([[" + wx2s(path) + "]]); ";
    if (b != 0)
        cmd += "_spec.blocks=fityk.IntVector(1," + S(b) + "); ";
    if (x != fityk::LoadSpec::NN)
        cmd += "_spec.x_col=" + S(x) + "; ";
    if (y != fityk::LoadSpec::NN)
        cmd += "_spec.y_col=" + S(y) + "; ";
    if (sig != fityk::LoadSpec::NN)
        cmd += "_spec.sig_col=" + S(sig) + "; ";
    if (!fmt.empty())
        cmd += "_spec.format='" + fmt + "'; ";
    if (comma)
        cmd += "_spec.options='decimal_comma'; ";
    cmd += "F:load(_spec, " + S(data_idx) + ")";
    return cmd;
}

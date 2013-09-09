// This file is part of fityk program. Copyright 2001-2013 Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#ifndef FITYK_WX_XYBROWSER_H_
#define FITYK_WX_XYBROWSER_H_

#include <wx/filectrl.h>
#include <wx/splitter.h>
#include <wx/spinctrl.h>
#include "uplot.h"

class PreviewPlot : public PlotWithTics
{
public:
    int block_nr, idx_x, idx_y;

    PreviewPlot(wxWindow* parent);
    virtual void draw(wxDC &dc, bool);
    void load_dataset(const std::string& filename, const std::string& filetype,
                      const std::string& options);
    shared_ptr<const xylib::DataSet> get_data() const { return data_; }
    void make_outdated() { data_updated_ = false; }

private:
    shared_ptr<const xylib::DataSet> data_;
    bool data_updated_; // if false, draw() doesn't do anything (plot is clear)
};

class XyFileBrowser : public wxSplitterWindow
{
public:
    wxFileCtrl *filectrl;
    wxSpinCtrl *x_column, *y_column, *s_column;
    wxCheckBox *std_dev_cb;
    wxChoice *block_ch;
#ifndef XYCONVERT
    wxCheckBox *sd_sqrt_cb, *title_cb;
    wxTextCtrl *title_tc;
#endif

    XyFileBrowser(wxWindow* parent);
    std::string get_filetype() const;
    void update_file_options();

private:
    wxTextCtrl *text_preview;
    PreviewPlot *plot_preview;
    wxCheckBox *auto_text_cb, *auto_plot_cb;

    void StdDevCheckBoxChanged();
    void OnStdDevCheckBox(wxCommandEvent&) { StdDevCheckBoxChanged(); }
    void OnAutoTextCheckBox(wxCommandEvent& event);
    void OnAutoPlotCheckBox(wxCommandEvent& event);
    void OnColumnChanged(wxSpinEvent& event);
    void OnBlockChanged(wxCommandEvent& event);
    void OnPathChanged(wxFileCtrlEvent&) { update_file_options(); }
    void update_text_preview();
    void update_plot_preview();
    void update_block_list();
    wxString get_one_path();
    void update_title_from_file();
#ifndef XYCONVERT
    void OnTitleCheckBox(wxCommandEvent& event);
#endif
};


#endif // FITYK_WX_XYBROWSER_H_

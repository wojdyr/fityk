// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#ifndef FITYK__WX_DLOAD__H__
#define FITYK__WX_DLOAD__H__

#include <wx/treectrl.h>
#include <wx/spinctrl.h>
#include <wx/dirctrl.h>
#include "cmn.h" //ProportionalSplitter, KFTextCtrl, ...

class Data;

class PreviewPlot;

class DLoadDlg : public wxDialog
{
public:
    DLoadDlg (wxWindow* parent, wxWindowID id, int n, Data* data);

protected:
    int data_nr;
    ProportionalSplitter *splitter, *right_splitter;
    wxGenericDirCtrl *dir_ctrl;
    KFTextCtrl *filename_tc;
    wxTextCtrl *title_tc, *text_preview;
    wxSpinCtrl *x_column, *y_column, *s_column;
    wxPanel *left_panel, *rupper_panel, *rbottom_panel, *columns_panel;
    PreviewPlot *plot_preview;
    wxCheckBox *std_dev_cb, *sd_sqrt_cb, *title_cb, *auto_text_cb,
               *auto_plot_cb;
    wxChoice *block_ch;
    wxButton *open_here, *open_new;
    bool initialized;

    std::string get_command(std::string const& ds, int d_nr);
    std::string get_filename();
    void StdDevCheckBoxChanged();
    void OnStdDevCheckBox(wxCommandEvent&) { StdDevCheckBoxChanged(); }
    void OnHTitleCheckBox (wxCommandEvent& event);
    void OnAutoTextCheckBox (wxCommandEvent& event);
    void OnAutoPlotCheckBox (wxCommandEvent& event);
    void OnColumnChanged (wxSpinEvent& event);
    void OnBlockChanged (wxCommandEvent& event);
    void OnOpenHere (wxCommandEvent& event);
    void OnOpenNew (wxCommandEvent& event);
    void on_path_change();
    void OnPathSelectionChanged(wxTreeEvent&) { on_path_change(); }
    void OnPathTextChanged(wxCommandEvent&);
    void update_text_preview();
    void update_plot_preview();
    void update_block_list();
    void update_title_from_file();
    DECLARE_EVENT_TABLE()
};


#endif


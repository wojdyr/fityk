// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#ifndef FITYK__WX_DLG__H__
#define FITYK__WX_DLG__H__

#include <wx/treectrl.h>
#include <wx/spinctrl.h>
#include <wx/listctrl.h> 
#include <wx/dirctrl.h>
#include "wx_common.h"

class wxGrid;
class DataTable;
class ProportionalSplitter;

class PreviewPlot;

class FDXLoadDlg : public wxDialog
{
public:
    FDXLoadDlg (wxWindow* parent, wxWindowID id, int n, Data* data);

protected:
    int data_nr;
    ProportionalSplitter *splitter, *right_splitter;
    wxGenericDirCtrl *dir_ctrl;
    wxTextCtrl *filename_tc, *text_preview;
    wxSpinCtrl *x_column, *y_column, *s_column;
    wxPanel *left_panel, *rupper_panel, *rbottom_panel, *columns_panel;
    PreviewPlot *plot_preview;
    wxCheckBox *std_dev_cb, *auto_text_cb, *auto_plot_cb;
    wxButton *open_here, *open_new;
    bool initialized;

    std::string get_command_tail();
    std::string get_filename();
    void OnStdDevCheckBox (wxCommandEvent& event);
    void OnAutoTextCheckBox (wxCommandEvent& event);
    void OnAutoPlotCheckBox (wxCommandEvent& event);
    void OnColumnChanged (wxSpinEvent& event);
    void OnOpenHere (wxCommandEvent& event);
    void OnOpenNew (wxCommandEvent& event);
    void OnClose (wxCommandEvent& event);
    void on_path_change();
    void on_filter_change();
    void OnPathSelectionChanged(wxTreeEvent &WXUNUSED(event)){on_path_change();}
    void update_text_preview();
    void update_plot_preview();
    DECLARE_EVENT_TABLE()
};

#if 0
class SumHistoryDlg : public wxDialog
{
public:
    SumHistoryDlg (wxWindow* parent, wxWindowID id);
    void OnUpButton           (wxCommandEvent& event);
    void OnDownButton         (wxCommandEvent& event);
    void OnToggleSavedButton  (wxCommandEvent& event);
    void OnComputeWssrButton  (wxCommandEvent& event);
    void OnSelectedItem       (wxListEvent&    event);
    void OnActivatedItem      (wxListEvent&    event); 
    void OnViewSpinCtrlUpdate (wxSpinEvent&    event); 
protected:
    int view[3], view_max;
    wxListCtrl *lc;
    wxBitmapButton *up_arrow, *down_arrow;
    wxButton *compute_wssr_button;

    void initialize_lc();
    void update_selection();
    DECLARE_EVENT_TABLE()
};
#endif

struct DataTransExample
{
    std::string name;
    std::string category;
    std::string description;
    std::string code;
    bool in_menu;

    DataTransExample(const std::string& name_, const std::string& category_, 
                     const std::string& description_, const std::string& code_,
                     bool in_menu_=false)
        : name(name_), category(category_), description(description_),
          code(code_), in_menu(in_menu_) {}
   DataTransExample(std::string line);
   std::string as_fileline() const;
};

class DataEditorDlg : public wxDialog
{
    friend class DataTable;
    typedef std::vector<std::pair<int,Data*> > ndnd_type;
public:
    DataEditorDlg (wxWindow* parent, wxWindowID id, ndnd_type const& dd);
    void OnRevert (wxCommandEvent& event);
    void OnSaveAs (wxCommandEvent& event);
    void OnAdd (wxCommandEvent& event);
    void OnRemove (wxCommandEvent& event);
    void OnUp (wxCommandEvent& event);
    void OnDown (wxCommandEvent& event);
    void OnSave (wxCommandEvent& event);
    void OnReset (wxCommandEvent& event);
    void OnApply (wxCommandEvent& event);
    void OnReZoom (wxCommandEvent& event);
    void OnHelp (wxCommandEvent& event);
    void OnClose (wxCommandEvent& event);
    void OnCodeText (wxCommandEvent&) { CodeText(); }
    void CodeText();
    void OnESelected (wxListEvent&) { ESelected(); }
    void ESelected();
    void OnEActivated (wxListEvent& event);
    void update_data(ndnd_type const& dd);
    static const std::vector<DataTransExample>& get_examples() 
                                                    { return examples; }
    static void read_examples(bool reset=false);
    static void execute_tranform(std::string code);
protected:
    static std::vector<DataTransExample> examples;
    wxGrid *grid;
    ndnd_type ndnd;
    wxStaticText *filename_label, *title_label, *description;
    wxListCtrl *example_list; 
    wxTextCtrl *code;
    wxButton *revert_btn, *save_as_btn, *apply_btn, *rezoom_btn, *help_btn,
             *add_btn, *remove_btn, *up_btn, *down_btn, 
             *save_btn, *reset_btn;

    void initialize_examples(bool reset=false);
    int get_selected_item();
    void insert_example_list_item(int n);
    void select_example(int item);
    void refresh_grid();
    DECLARE_EVENT_TABLE()
};


class ExampleEditorDlg : public wxDialog
{
public:
    ExampleEditorDlg(wxWindow* parent, wxWindowID id, DataTransExample& ex_,
                     const std::vector<DataTransExample>& examples_, int pos_);
    void OnOK(wxCommandEvent &event);
protected:
    DataTransExample& ex;
    const std::vector<DataTransExample>& examples;
    int pos;
    wxTextCtrl *name_tc, *description_tc, *code_tc;
    wxComboBox *category_c;
    wxCheckBox *inmenu_cb;

    DECLARE_EVENT_TABLE()
};

wxString get_user_conffile(std::string const& filename);
bool export_data_dlg(wxWindow *parent, bool load_exported=false);


class RealNumberCtrl : public wxTextCtrl
{
public:
    RealNumberCtrl(wxWindow* parent, wxWindowID id, wxString const& value)
        : wxTextCtrl(parent, id, value) {}
    RealNumberCtrl(wxWindow* parent, wxWindowID id, std::string const& value)
        : wxTextCtrl(parent, id, s2wx(value)) {}
};

class SettingsDlg : public wxDialog
{
public:
    typedef std::vector<std::pair<std::string, std::string> > pair_vec;
    SettingsDlg(wxWindow* parent, const wxWindowID id);
    pair_vec get_changed_items();
private:
    wxRadioBox *autoplot_rb;
    wxChoice *verbosity_ch;
    wxCheckBox *exit_cb;
    RealNumberCtrl *cut_func, *height_correction, *width_correction;
    wxCheckBox *cancel_poos;
};


#endif


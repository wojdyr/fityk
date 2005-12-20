// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#ifndef FITYK__WX_DLG__H__
#define FITYK__WX_DLG__H__

#include <wx/treectrl.h>
#include <wx/spinctrl.h>
#include <wx/listctrl.h> 
#include <wx/dirctrl.h>

class wxGrid;
class DataTable;


class FDXLoadDlg;

/// helper class used in FDXLoadDlg
class LoadDataDirCtrl : public wxGenericDirCtrl
{
public:
    LoadDataDirCtrl(FDXLoadDlg* parent);
    void OnPathSelectionChanged(wxTreeEvent &event);
    void SetFilterIndex(int n);
    DECLARE_EVENT_TABLE()
private:
    FDXLoadDlg *load_dlg;
};

class FDXLoadDlg : public wxDialog
{
public:
    FDXLoadDlg (wxWindow* parent, wxWindowID id);
    std::string get_command();
    std::string get_filename();
    void on_filter_change();
    void on_path_change();

protected:
    LoadDataDirCtrl *dir_ctrl;
    wxTextCtrl *filename_tc;
    wxSpinCtrl *x_column, *y_column, *s_column;
    wxPanel *columns_panel;
    wxCheckBox *std_dev_cb, *append_cb;
    void OnStdDevCheckBox (wxCommandEvent& event);
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
public:
    DataEditorDlg (wxWindow* parent, wxWindowID id, Data *data_);
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
    void update_data(Data *data_);
    static const std::vector<DataTransExample>& get_examples() 
                                                    { return examples; }
    static void read_examples(bool reset=false);
    static void execute_tranform(std::string code);
protected:
    static std::vector<DataTransExample> examples;
    wxGrid *grid;
    Data *data;
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

wxString get_user_conffile(const wxString &filename);
bool export_data_dlg(wxWindow *parent, bool load_exported=false);


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
};


#endif


// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#ifndef FITYK__WX_DLG__H__
#define FITYK__WX_DLG__H__

#include <wx/spinctrl.h>
#include <wx/listctrl.h> 
#include <wx/grid.h> 
#include "wx_common.h"

class wxGrid;
class DataTable;
class ProportionalSplitter;

class SumHistoryDlg : public wxDialog
{
public:
    SumHistoryDlg (wxWindow* parent, wxWindowID id);
    void OnUpButton           (wxCommandEvent& event);
    void OnDownButton         (wxCommandEvent& event);
    void OnComputeWssrButton  (wxCommandEvent& event);
    void OnSelectedItem       (wxListEvent&    event);
    void OnActivatedItem      (wxListEvent&    event); 
    void OnViewSpinCtrlUpdate (wxSpinEvent&    event); 
    void OnClose (wxCommandEvent& ) { close_it(this); }
protected:
    int view[4], view_max;
    wxListCtrl *lc;
    wxBitmapButton *up_arrow, *down_arrow;
    wxButton *compute_wssr_button;

    void initialize_lc();
    void update_selection();
    void add_item_to_lc(int pos, std::vector<fp> const& item);
    DECLARE_EVENT_TABLE()
};

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
    void OnChangeButton(wxCommandEvent& event);
    void OnOK(wxCommandEvent& event);
    pair_vec get_changed_items();
private:
    wxRadioBox *autoplot_rb;
    wxChoice *verbosity_ch;
    wxCheckBox *exit_cb;
    SpinCtrl *seed_sp, *mwssre_sp;
    RealNumberCtrl *cut_func, *height_correction, *width_correction,
                   *domain_p, *lm_lambda_ini, *lm_lambda_up, *lm_lambda_down,
                   *lm_stop, *lm_max_lambda;
    wxCheckBox *cancel_poos;
    wxTextCtrl *dir_ld_tc, *dir_xs_tc;

    void add_persistence_note(wxWindow *parent, wxSizer *sizer);

    DECLARE_EVENT_TABLE()
};

class FitRunDlg : public wxDialog
{
public:
    FitRunDlg(wxWindow* parent, wxWindowID id, bool initialize);
    void OnOK(wxCommandEvent& event);
    void OnSpinEvent(wxSpinEvent &) { update_unlimited(); }
    void OnChangeDsOrMethod(wxCommandEvent&) { update_allow_continue(); }
private:
    wxRadioBox* ds_rb;
    wxChoice* method_c;
    wxCheckBox* initialize_cb;
    SpinCtrl *maxiter_sc, *maxeval_sc;
    wxStaticText *nomaxeval_st, *nomaxiter_st;

    void update_unlimited();
    void update_allow_continue();
    DECLARE_EVENT_TABLE()
};

class DefinitionMgrDlg : public wxDialog
{
public:
    struct FunctionDefinitonElems
    {
        std::string name;
        std::vector<std::string> parameters;
        std::vector<std::string> defvalues;
        std::string rhs;
        int builtin;

        std::string get_full_definition() const;
    };

    DefinitionMgrDlg(wxWindow* parent);
    void OnFunctionChanged(wxCommandEvent &) { select_function(); }
    void OnEndCellEdit(wxGridEvent &event);
    void OnNameChanged(wxCommandEvent &);
    void OnDefChanged(wxCommandEvent &);
    void OnAddButton(wxCommandEvent &);
    void OnRemoveButton(wxCommandEvent &);
    void OnOk(wxCommandEvent &event);
    std::string get_command();

private:
    int selected;
    wxListBox *lb;
    wxTextCtrl *name_tc, *def_tc;
    wxStaticText *name_comment_st, *def_label_st;
    wxGrid *par_g;
    wxButton *add_btn, *remove_btn;
    std::vector<FunctionDefinitonElems> orig, modified;

    void select_function(bool init=false);
    void fill_function_list();
    bool check_definition();
    bool is_name_in_modified(std::string const& name);
    bool save_changes();

    DECLARE_EVENT_TABLE()
};


#endif


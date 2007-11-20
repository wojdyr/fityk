// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License version 2
// $Id$

#ifndef FITYK__WX_DATAEDIT__H__
#define FITYK__WX_DATAEDIT__H__

#include <string>
#include <vector>
#include <wx/grid.h>
#include <wx/listctrl.h>

class Data;

struct DataTransform
{
    std::string name;
    std::string category;
    std::string description;
    std::string code;
    bool in_menu;

    DataTransform(const std::string& name_, const std::string& category_, 
                     const std::string& description_, const std::string& code_,
                     bool in_menu_=false)
        : name(name_), category(category_), description(description_),
          code(code_), in_menu(in_menu_) {}
   DataTransform(std::string line);
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
    static std::vector<DataTransform> const& get_transforms() 
                                                    { return transforms; }
    static void read_transforms(bool reset=false);
    static void execute_tranform(std::string code);
protected:
    static std::vector<DataTransform> transforms;
    wxGrid *grid;
    ndnd_type ndnd;
    wxStaticText *filename_label, *title_label, *description;
    wxListCtrl *trans_list; 
    wxTextCtrl *code;
    wxButton *revert_btn, *save_as_btn, *apply_btn, *rezoom_btn, *help_btn,
             *add_btn, *remove_btn, *up_btn, *down_btn, 
             *save_btn, *reset_btn;

    void initialize_transforms(bool reset=false);
    int get_selected_item();
    void insert_trans_list_item(int n);
    void select_transform(int item);
    bool is_revertable() const;
    void refresh_grid();
    DECLARE_EVENT_TABLE()
};


class TransEditorDlg : public wxDialog
{
public:
    TransEditorDlg(wxWindow* parent, wxWindowID id, DataTransform& ex_,
                   const std::vector<DataTransform>& transforms_, int pos_);
    void OnOK(wxCommandEvent &event);
protected:
    DataTransform& ex;
    const std::vector<DataTransform>& transforms;
    int pos;
    wxTextCtrl *name_tc, *description_tc, *code_tc;
    wxComboBox *category_c;
    wxCheckBox *inmenu_cb;

    DECLARE_EVENT_TABLE()
};

#endif


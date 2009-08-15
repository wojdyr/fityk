// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
// $Id$

/// In this file:
///  Definition Manager Dialog (DefinitionMgrDlg)

#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/statline.h>
//#include <wx/msgdlg.h>

#include "defmgr.h"
#include "cmn.h" //s2wx, wx2s, close_it
#include "../func.h"
#include "../guess.h" //FunctionKind

using namespace std;

enum {
    ID_DMD_NAME             = 26300,
    ID_DMD_DEF
};

string DefinitionMgrDlg::FunctionDefinitonElems::get_full_definition() const
{
    std::string s = name + "(";
    for (size_t i = 0; i < parameters.size(); ++i) {
        s += (i == 0 ? "" : ", ") + parameters[i];
        if (!defvalues[i].empty())
            s += "=" + defvalues[i];
    }
    return s + ") = " + rhs;
}

// EVT_GRID_CMD_CELL_CHANGED is new in wx2.9, it replaced .._CHANGE
#ifndef EVT_GRID_CMD_CELL_CHANGED
    #define EVT_GRID_CMD_CELL_CHANGED EVT_GRID_CMD_CELL_CHANGE
#endif

BEGIN_EVENT_TABLE(DefinitionMgrDlg, wxDialog)
    EVT_LISTBOX(-1, DefinitionMgrDlg::OnFunctionChanged)
    EVT_GRID_CMD_CELL_CHANGED(-1, DefinitionMgrDlg::OnEndCellEdit)
    EVT_TEXT(ID_DMD_NAME, DefinitionMgrDlg::OnNameChanged)
    EVT_TEXT(ID_DMD_DEF, DefinitionMgrDlg::OnDefChanged)
    EVT_BUTTON(wxID_ADD, DefinitionMgrDlg::OnAddButton)
    EVT_BUTTON(wxID_REMOVE, DefinitionMgrDlg::OnRemoveButton)
    EVT_BUTTON(wxID_OK, DefinitionMgrDlg::OnOk)
END_EVENT_TABLE()

DefinitionMgrDlg::DefinitionMgrDlg(wxWindow* parent)
    : wxDialog(parent, -1, wxT("Function Definition Manager"),
               wxDefaultPosition, wxSize(600, 500),
               wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER),
      selected(0)
{
    wxBoxSizer *top_sizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer *hsizer = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer *lb_sizer = new wxBoxSizer(wxVERTICAL);
    lb = new wxListBox(this, -1, wxDefaultPosition, wxDefaultSize,
                       0, 0, wxLB_SINGLE);
    lb_sizer->Add(lb, 1, wxEXPAND|wxALL, 5);
    add_btn = new wxButton(this, wxID_ADD, wxT("Add"));
    lb_sizer->Add(add_btn, 0, wxALL|wxALIGN_CENTER, 5);
    hsizer->Add(lb_sizer, 0, wxEXPAND);
    wxBoxSizer *vsizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer *name_sizer = new wxBoxSizer(wxHORIZONTAL);
    name_sizer->Add(new wxStaticText(this, -1, wxT("Name:")),
                    0, wxALIGN_CENTER_VERTICAL|wxALL, 5);
    wxBoxSizer *namev_sizer = new wxBoxSizer(wxVERTICAL);
    name_tc = new wxTextCtrl(this, ID_DMD_NAME, wxT(""),
                             wxDefaultPosition, wxSize(200, -1));
    namev_sizer->Add(name_tc, 1, wxALL, 5);
    name_comment_st = new wxStaticText(this, -1, wxT(""));
    namev_sizer->Add(name_comment_st, 0, wxALIGN_LEFT|wxALL, 1);
    name_sizer->Add(namev_sizer, 0, wxEXPAND);
    name_sizer->AddSpacer(20);
    remove_btn = new wxButton(this, wxID_REMOVE, wxT("Remove"));
    name_sizer->Add(remove_btn, 0, wxALIGN_RIGHT|wxALL, 5);
    vsizer->Add(name_sizer, 0, wxEXPAND);
    vsizer->AddSpacer(5);

    vsizer->Add(new wxStaticText(this, -1,
        wxT("Parameters (don't put 'x' here).\n")
        wxT("Default values of functions can be given in terms of:\n")
        wxT("- if it looks like peak: 'center', 'height', 'fwhm', 'area'\n")
        wxT("- if it looks like linear: 'slope', 'intercept', 'avgy'.")),
                0, wxALL, 5);

    par_g = new wxGrid(this, -1, wxDefaultPosition, wxDefaultSize);
    par_g->CreateGrid(1, 2);
    par_g->SetColSize(0, 120);
    par_g->SetColLabelValue(0, wxT("name"));
    par_g->SetColSize(1, 240);
    par_g->SetColLabelValue(1, wxT("default value"));
    par_g->SetDefaultRowSize(20, true);
    par_g->SetColLabelSize(20);
    par_g->SetRowLabelSize(0);
    par_g->EnableDragRowSize(false);
    par_g->SetLabelFont(*wxNORMAL_FONT);
    vsizer->Add(par_g, 1, wxALL|wxEXPAND, 5);
    guess_label_st = new wxStaticText(this, -1, wxT(""),
                                      wxDefaultPosition, wxDefaultSize,
                                      wxST_NO_AUTORESIZE|wxALIGN_RIGHT);
    vsizer->Add(guess_label_st, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 5);
    def_label_st = new wxStaticText(this, -1, wxT("definition:"),
                    wxDefaultPosition, wxDefaultSize, wxST_NO_AUTORESIZE);
    vsizer->Add(def_label_st, 0, wxEXPAND|wxALL, 5);
    def_tc = new wxTextCtrl(this, ID_DMD_DEF, wxT(""),
                            wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE);
    vsizer->Add(def_tc, 1, wxLEFT|wxRIGHT|wxBOTTOM|wxEXPAND, 5);


    hsizer->Add(vsizer, 1, wxEXPAND);
    top_sizer->Add(hsizer, 1, wxEXPAND);

    top_sizer->Add(new wxStaticLine(this, -1), 0, wxEXPAND|wxLEFT|wxRIGHT, 5);
    top_sizer->Add(CreateButtonSizer (wxOK|wxCANCEL),
                   0, wxALL|wxALIGN_CENTER, 5);

    SetSizer(top_sizer);
    top_sizer->SetSizeHints(this);

    fill_function_list();
    lb->SetSelection(selected);
    select_function(true);
}

void DefinitionMgrDlg::fill_function_list()
{
    vector<string> const& types = Function::get_all_types();
    orig.resize(types.size());
    lb->Clear();
    for (size_t i = 0; i != types.size(); ++i) {
        string formula = Function::get_formula(i);
        FunctionDefinitonElems fde;
        fde.name = types[i];
        fde.parameters = Function::get_varnames_from_formula(formula);
        fde.defvalues = Function::get_defvalues_from_formula(formula);
        fde.rhs = Function::get_rhs_from_formula(formula);
        fde.builtin = Function::is_builtin(i);
        orig[i] = fde;
        lb->Append(s2wx(fde.name));
    }
    modified = orig;
}

bool DefinitionMgrDlg::check_definition()
{
    FunctionDefinitonElems const& fde = modified[selected];
    if (!fde.builtin) {
        string value = wx2s(def_tc->GetValue());
        vector<string> lhs_vars(fde.parameters.size());
        for (size_t i = 0; i < fde.parameters.size(); ++i)
            lhs_vars[i] = fde.parameters[i];
        try {
            UdfContainer::check_rhs(value, lhs_vars);
        }
        catch (ExecuteError &e) {
            wxString what = s2wx(string(e.what()));
            def_label_st->SetLabel(wxT("definition: (error: ")+what+wxT(")"));
            add_btn->Enable(false);
            FindWindow(wxID_OK)->Enable(false);
            return false;
        }
    }
    def_label_st->SetLabel(wxT("definition:"));
    //Layout(); // to resize def_label_st
    add_btn->Enable(true);
    FindWindow(wxID_OK)->Enable(true);
    return true;
}

void DefinitionMgrDlg::update_guess_comment()
{
    FunctionDefinitonElems const& fde = modified[selected];
    FunctionKind fk;
    bool r = is_function_guessable(fde.parameters, fde.defvalues, &fk);
    if (!r)
        guess_label_st->SetLabel(wxT("The function can not be guessed."));
    else if (fk == fk_peak)
        guess_label_st->SetLabel(wxT("The function can be guessed as peak."));
    else if (fk == fk_linear)
        guess_label_st->SetLabel(wxT("The function can be guessed as linear."));
    else
        guess_label_st->SetLabel(wxT(""));
}

bool DefinitionMgrDlg::save_changes()
{
    FunctionDefinitonElems& prev = modified[selected];
    if (!prev.builtin) {
        // check if changed values are correct
        if (!name_comment_st->GetLabel().IsEmpty()) {
            lb->SetSelection(selected);
            name_tc->SetFocus();
            return false;
        }
        else if (!check_definition()) {
            lb->SetSelection(selected);
            def_tc->SetFocus();
            return false;
        }
        else {
            if (prev.name != wx2s(name_tc->GetValue().Trim())) {
                prev.name = wx2s(name_tc->GetValue().Trim());
                lb->SetString(selected, name_tc->GetValue().Trim());
            }
            if (prev.rhs != wx2s(def_tc->GetValue().Trim())) {
                prev.rhs = wx2s(def_tc->GetValue().Trim());
            }
        }
    }
    return true;
}

void DefinitionMgrDlg::select_function(bool init)
{
    int n = lb->GetSelection();
    if (!init && n == selected)
        return;
    if (n == wxNOT_FOUND) {
        lb->SetSelection(selected);
        return;
    }
    if (!init && !save_changes())
        return;

    selected = n;
    FunctionDefinitonElems const& fde = modified[n];
    name_tc->SetValue(s2wx(fde.name));
    int row_diff = fde.parameters.size() + 1 - par_g->GetNumberRows();
    par_g->BeginBatch();
    if (row_diff > 0)
        par_g->AppendRows(row_diff);
    else if (row_diff < 0)
        par_g->DeleteRows(0, -row_diff);
    for (size_t i = 0; i != fde.parameters.size(); ++i) {
        par_g->SetCellValue(i, 0, s2wx(fde.parameters[i]));
        par_g->SetCellValue(i, 1, s2wx(fde.defvalues[i]));
    }
    if (!fde.builtin) {
        par_g->SetCellValue(fde.parameters.size(), 0, wxT(""));
        par_g->SetCellValue(fde.parameters.size(), 1, wxT(""));
    }
    else {
        par_g->DeleteRows(fde.parameters.size(), 1);
    }
    par_g->EndBatch();
    wxString definition = s2wx(fde.rhs);
    if (fde.builtin == 2)
        definition += wxT("\n\n[This definition is for information only]")
                      wxT("\n[The function is coded in C++]");

    def_tc->SetValue(definition);
    name_tc->SetEditable(!fde.builtin);
    par_g->EnableEditing(!fde.builtin);
    def_tc->SetEditable(!fde.builtin);
    remove_btn->Enable(!fde.builtin);
    update_guess_comment();
}

std::string DefinitionMgrDlg::get_command()
{
    typedef vector<FunctionDefinitonElems>::const_iterator vfde_iter_type;
    vector<string> ss;

    for (vfde_iter_type i = orig.begin(); i != orig.end(); ++i) {
        bool found = false;
        for (vfde_iter_type j = modified.begin(); j != modified.end(); ++j) {
            if (i->name == j->name) {
                found = true;
                break;
            }
        }
        if (!found)
            ss.push_back("undefine " + i->name);
    }

    for (vfde_iter_type i = modified.begin(); i != modified.end(); ++i) {
        bool found = false;
        for (vfde_iter_type j = orig.begin(); j != orig.end(); ++j) {
            if (i->name == j->name) {
                found = true;
                if (i->parameters != j->parameters
                        || i->defvalues != j->defvalues || i->rhs != j->rhs) {
                    ss.push_back("undefine " + i->name);
                    ss.push_back("define " + i->get_full_definition());
                }
                break;
            }
        }
        if (!found)
            ss.push_back("define " + i->get_full_definition());
    }
    return join_vector(ss, "; ");
}

namespace {
bool is_valid_parameter_name(char const* name)
{
    assert(name && strlen(name) > 0);
    if (*name == 'x' && *(name+1) == 0)
        return false;
    if (!islower(*name))
        return false;
    while (*++name)
        if (!islower(*name) && !isdigit(*name) && *name != '_')
            return false;
    return true;
}
} //anonymous namespace

void DefinitionMgrDlg::OnEndCellEdit(wxGridEvent &event)
{
    FunctionDefinitonElems& fde = modified[selected];
    int row = event.GetRow();
    assert(row <= size(fde.parameters));
    bool new_row = (row == size(fde.parameters));
    int col = event.GetCol();
    wxString new_val_ = par_g->GetCellValue(row, col);
    if (new_val_.Lower() != new_val_) {
        new_val_.MakeLower();
        wxMessageBox(wxT("Parameter names should be lower case."),
                     wxT(""), wxOK|wxICON_INFORMATION, this);
        par_g->SetCellValue(row, col, new_val_);
    }
    string new_val = wx2s(new_val_);
    if (col == 0) {
        if (new_val.empty()) {
            if (!new_row) { //erased parameter
                fde.parameters.erase(fde.parameters.begin() + row);
                fde.defvalues.erase(fde.defvalues.begin() + row);
                par_g->DeleteRows(row);
                check_definition();
                update_guess_comment();
            }
        }
        else if (new_row) { //added parameter
            if (is_valid_parameter_name(new_val.c_str())
                            && !contains_element(fde.parameters, new_val)) {
                fde.parameters.push_back(new_val);
                fde.defvalues.push_back("");
                par_g->AppendRows(1);
                check_definition();
                update_guess_comment();
            }
            else {
                par_g->SetCellValue(row, col, wxT(""));
            }
        }
        else if (new_val != fde.parameters[row]) { // changed parameter
            if (is_valid_parameter_name(new_val.c_str())
                            && !contains_element(fde.parameters, new_val)) {
                fde.parameters[row] = new_val;
                check_definition();
            }
            else {
                par_g->SetCellValue(row, col, s2wx(fde.parameters[row]));
            }
        }
    }
    else {
        assert (col == 1);
        if (new_row)
            par_g->SetCellValue(row, col, wxT(""));
        else {
            if (is_defvalue_guessable(new_val, fk_linear)
                    || is_defvalue_guessable(new_val, fk_peak)) {
                fde.defvalues[row] = new_val;
                update_guess_comment();
            }
            else {
                par_g->SetCellValue(row, col, s2wx(fde.defvalues[row]));
            }
        }
    }
}

namespace {
bool valid_name_chars(char const* name)
{
    // don't check first char
    while (*++name)
        if (!isalnum(*name))
            return false;
    return true;
}
} //anonymous namespace

bool DefinitionMgrDlg::is_name_in_modified(string const& name)
{
    for (size_t i = 0; i != modified.size(); ++i)
        if (modified[i].name == name)
            return true;
    return false;
}

void DefinitionMgrDlg::OnNameChanged(wxCommandEvent &)
{
    if (modified[selected].builtin) {
        name_comment_st->SetLabel(wxT("[built-in, not editable]"));
        return;
    }
    string name = strip_string(wx2s(name_tc->GetValue()));
    if (name.size() < 2)
        name_comment_st->SetLabel(wxT("too short!"));
    else if (!isalpha(name[0]))
        name_comment_st->SetLabel(wxT("should start with letter!"));
    else if (!valid_name_chars(name.c_str()))
        name_comment_st->SetLabel(wxT("invalid character!"));
    else if (name != modified[selected].name && is_name_in_modified(name))
        name_comment_st->SetLabel(wxT("already used!"));
    else {
        name_comment_st->SetLabel(wxT(""));
        if (islower(name[0])) {
            name_tc->Replace(0, 1, s2wx(string(1, toupper(name[0]))));
        }
    }
}

void DefinitionMgrDlg::OnDefChanged(wxCommandEvent &)
{
    check_definition();
}

void DefinitionMgrDlg::OnAddButton(wxCommandEvent &)
{
    if (!check_definition())
        return;
    FunctionDefinitonElems fde;
    fde.builtin = 0;
    modified.push_back(fde);
    lb->Append(wxT(""));
    lb->SetSelection(lb->GetCount() - 1);
    select_function();
    name_tc->SetFocus();
}


void DefinitionMgrDlg::OnRemoveButton(wxCommandEvent &)
{
    int n = selected;
    if (modified[n].builtin)
        return;
    lb->SetSelection(0);
    select_function(true);
    modified.erase(modified.begin() + n);
    lb->Delete(n);
}

void DefinitionMgrDlg::OnOk(wxCommandEvent&)
{
    if (save_changes())
        close_it(this, wxID_OK);
}


// Author: Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#include "sgchooser.h"

#include <wx/wx.h>
#include <wx/listctrl.h>

#include "ceria.h"
#include "cmn.h"

using namespace std;

SpaceGroupChooser::SpaceGroupChooser(wxWindow* parent,
                                     const wxString& initial_value)
        : wxDialog(parent, -1, wxT("Choose space group"),
                   wxDefaultPosition, wxDefaultSize,
                   wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER)
{
    wxSizer *sizer = new wxBoxSizer(wxVERTICAL);

    wxSizer *ssizer = new wxBoxSizer(wxHORIZONTAL);
    ssizer->Add(new wxStaticText(this, -1, wxT("system:")),
                wxSizerFlags().Border().Center());
    wxArrayString s_choices;
    s_choices.Add(wxT("All lattice systems"));
    for (int i = 2; i < 9; ++i)
        s_choices.Add(pchar2wx(CrystalSystemNames[i]));
    system_c = new wxChoice(this, -1, wxDefaultPosition, wxDefaultSize,
                            s_choices);
    system_c->SetSelection(0);
    ssizer->Add(system_c, wxSizerFlags(1).Border());
    sizer->Add(ssizer, wxSizerFlags().Expand());

    wxSizer *hsizer = new wxBoxSizer(wxHORIZONTAL);
    hsizer->Add(new wxStaticText(this, -1, wxT("centering:")),
                wxSizerFlags().Border().Center());
    wxString letters = wxT("PABCIRF");
    for (size_t i = 0; i < sizeof(centering_cb)/sizeof(centering_cb[0]); ++i) {
        centering_cb[i] = new wxCheckBox(this, -1, letters[i]);
        centering_cb[i]->SetValue(true);
        hsizer->Add(centering_cb[i], wxSizerFlags().Border());
    }
    sizer->Add(hsizer);

    list = new wxListView(this, -1,
                          wxDefaultPosition, wxSize(300, 500),
                          wxLC_REPORT|wxLC_HRULES|wxLC_VRULES);
    list->InsertColumn(0, wxT("No"));
    list->InsertColumn(1, wxT("H-M symbol"));
    list->InsertColumn(2, wxT("Sch\u00F6nflies symbol"));
    list->InsertColumn(3, wxT("Hall symbol"));

    regenerate_list(initial_value);

    // set widths of columns
    int col_widths[4];
    int total_width = 0;
    for (int i = 0; i < 4; i++) {
        list->SetColumnWidth(i, wxLIST_AUTOSIZE);
        col_widths[i] = list->GetColumnWidth(i);
        total_width += col_widths[i];
    }
    // leave margin of 50 px for scrollbar
    int empty_width = GetClientSize().GetWidth() - total_width - 50;
    if (empty_width > 0)
        for (int i = 0; i < 4; i++)
            list->SetColumnWidth(i, col_widths[i] + empty_width / 4);

    sizer->Add(list, wxSizerFlags(1).Expand().Border());
    sizer->Add(CreateButtonSizer(wxOK|wxCANCEL),
               wxSizerFlags().Expand());
    SetSizerAndFit(sizer);

    for (size_t i = 0; i < sizeof(centering_cb)/sizeof(centering_cb[0]); ++i)
        Connect(centering_cb[i]->GetId(), wxEVT_COMMAND_CHECKBOX_CLICKED,
                wxCommandEventHandler(SpaceGroupChooser::OnCheckBox));

    Connect(system_c->GetId(), wxEVT_COMMAND_CHOICE_SELECTED,
            wxCommandEventHandler(SpaceGroupChooser::OnSystemChoice));
    //Connect(list->GetId(), wxEVT_COMMAND_LIST_ITEM_FOCUSED,
    //        wxCommandEventHandler(SpaceGroupChooser::OnListItemFocused));
    Connect(list->GetId(), wxEVT_COMMAND_LIST_ITEM_ACTIVATED,
            wxCommandEventHandler(SpaceGroupChooser::OnListItemActivated));
}

void SpaceGroupChooser::OnListItemActivated(wxCommandEvent&)
{
    EndModal(wxID_OK);
}

void SpaceGroupChooser::regenerate_list(const wxString& sel_value)
{
    list->DeleteAllItems();
    int syst = system_c->GetSelection();
    int sel_idx = -1;
    for (const SpaceGroupSetting* i = space_group_settings; i->sgnumber; ++i) {
        if (syst != 0 && get_crystal_system(i->sgnumber) != syst+1)
            continue;
        switch (i->HM[0]) {
            case 'P': if (!centering_cb[0]->IsChecked()) continue; break;
            case 'A': if (!centering_cb[1]->IsChecked()) continue; break;
            case 'B': if (!centering_cb[2]->IsChecked()) continue; break;
            case 'C': if (!centering_cb[3]->IsChecked()) continue; break;
            case 'I': if (!centering_cb[4]->IsChecked()) continue; break;
            case 'R': if (!centering_cb[5]->IsChecked()) continue; break;
            case 'F': if (!centering_cb[6]->IsChecked()) continue; break;
            default: assert(0);
        }
        int n = list->GetItemCount();
        list->InsertItem(n, wxString::Format(wxT("%d"), i->sgnumber));
        wxString hm = s2wx(fullHM(i));
        list->SetItem(n, 1, hm);
        wxString schoen(SchoenfliesSymbolsAsUTF8[i->sgnumber-1], wxConvUTF8);
        list->SetItem(n, 2, schoen);
        list->SetItem(n, 3, pchar2wx(i->Hall));
        if (hm == sel_value)
            sel_idx = n;
    }
    if (sel_idx != -1) {
        list->Select(sel_idx, true);
        list->EnsureVisible(sel_idx);
    }
}

wxString SpaceGroupChooser::get_value() const
{
    int n = list->GetFirstSelected();
    if (n < 0)
        return wxEmptyString;
    wxListItem info;
    info.SetId(n);
    info.SetColumn(1);
    info.SetMask(wxLIST_MASK_TEXT);
    list->GetItem(info);
    return info.GetText();
}


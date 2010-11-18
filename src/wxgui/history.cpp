// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
// $Id: dialogs.cpp 503 2009-06-09 15:47:31Z wojdyr $

#include "history.h"
#include "cmn.h"
#include "frame.h"

#include "../logic.h"
#include "../fit.h"

#include "img/up_arrow.xpm"
#include "img/down_arrow.xpm"

using namespace std;

enum {
    ID_SHIST_LC             = 26100,
    ID_SHIST_UP                    ,
    ID_SHIST_DOWN                  ,
    ID_SHIST_CWSSR                 ,
    ID_SHIST_V                     , // and next 3
};

BEGIN_EVENT_TABLE(SumHistoryDlg, wxDialog)
    EVT_ACTIVATE (SumHistoryDlg::OnActivate)
    EVT_BUTTON      (ID_SHIST_UP,     SumHistoryDlg::OnUpButton)
    EVT_BUTTON      (ID_SHIST_DOWN,   SumHistoryDlg::OnDownButton)
    EVT_BUTTON      (ID_SHIST_CWSSR,  SumHistoryDlg::OnComputeWssrButton)
    EVT_BUTTON      (wxID_CLEAR,      SumHistoryDlg::OnClearHistory)
    EVT_LIST_ITEM_SELECTED  (ID_SHIST_LC, SumHistoryDlg::OnSelectedItem)
    EVT_LIST_ITEM_ACTIVATED (ID_SHIST_LC, SumHistoryDlg::OnActivatedItem)
    EVT_SPINCTRL    (ID_SHIST_V+0,    SumHistoryDlg::OnViewSpinCtrlUpdate)
    EVT_SPINCTRL    (ID_SHIST_V+1,    SumHistoryDlg::OnViewSpinCtrlUpdate)
    EVT_SPINCTRL    (ID_SHIST_V+2,    SumHistoryDlg::OnViewSpinCtrlUpdate)
    EVT_SPINCTRL    (ID_SHIST_V+3,    SumHistoryDlg::OnViewSpinCtrlUpdate)
END_EVENT_TABLE()

SumHistoryDlg::SumHistoryDlg (wxWindow* parent, wxWindowID id)
    : wxDialog(parent, id, wxT("Parameters History"),
               wxDefaultPosition, wxDefaultSize,
               wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER),
      lc(NULL), compute_wssr_button(NULL), wssr_done(false)
{
    wxBoxSizer *top_sizer = new wxBoxSizer(wxVERTICAL);

    wxBoxSizer *hsizer = new wxBoxSizer(wxHORIZONTAL);
    initialize_lc(); //wxListCtrl
    hsizer->Add (lc, 1, wxEXPAND);

    wxBoxSizer *arrows_sizer = new wxBoxSizer(wxVERTICAL);
    up_arrow = new wxBitmapButton (this, ID_SHIST_UP, wxBitmap (up_arrow_xpm));
    arrows_sizer->Add (up_arrow, 0);
    arrows_sizer->Add (10, 10, 1);
    down_arrow = new wxBitmapButton (this, ID_SHIST_DOWN,
                                     wxBitmap (down_arrow_xpm));
    arrows_sizer->Add (down_arrow, 0);
    hsizer->Add (arrows_sizer, 0, wxALIGN_CENTER);
    top_sizer->Add (hsizer, 1, wxEXPAND);

    wxBoxSizer *buttons_sizer = new wxBoxSizer(wxHORIZONTAL);
    wxButton *clear_btn = new wxButton(this, wxID_CLEAR, wxT("Clear History"));
    buttons_sizer->Add(clear_btn, 0, wxALL, 5);
    clear_btn->Enable(ftk->get_fit_container()->get_param_history_size() != 0);
    //compute_wssr_button = new wxButton (this, ID_SHIST_CWSSR,
    //                                    wxT("Compute WSSRs"));
    //buttons_sizer->Add (compute_wssr_button, 0, wxALL, 5);
    buttons_sizer->Add (10, 10, 1);
    buttons_sizer->Add (new wxStaticText(this, -1, wxT("View parameters:")),
                        0, wxALL|wxALIGN_CENTER, 5);
    for (int i = 0; i < 4; i++)
        buttons_sizer->Add (new SpinCtrl(this, ID_SHIST_V + i, view[i],
                                         0, view_max, 40),
                            0, wxALL, 5);
    buttons_sizer->Add (10, 10, 1);
    buttons_sizer->Add (new wxButton (this, wxID_CLOSE, wxT("&Close")),
                        0, wxALL, 5);
    top_sizer->Add (buttons_sizer, 0, wxALIGN_CENTER);

    SetSizer (top_sizer);
    top_sizer->SetSizeHints (this);

    update_selection();
    SetEscapeId(wxID_CLOSE);
}

void SumHistoryDlg::initialize_lc()
{
    assert (lc == 0);
    view_max = ftk->parameters().size() - 1;
    assert (view_max != -1);
    for (int i = 0; i < 4; i++)
        view[i] = min (i, view_max);
    lc = new wxListCtrl (this, ID_SHIST_LC,
                         wxDefaultPosition, wxSize(450, 250),
                         wxLC_REPORT|wxLC_SINGLE_SEL|wxLC_HRULES|wxLC_VRULES
                             |wxSIMPLE_BORDER);
    lc->InsertColumn(0, wxT("No."));
    lc->InsertColumn(1, wxT("parameters"));
    lc->InsertColumn(2, wxT("WSSR"));
    for (int i = 0; i < 4; i++)
        lc->InsertColumn(3 + i, wxString::Format(wxT("par. %i"), view[i]));

    FitMethodsContainer const* fmc = ftk->get_fit_container();
    for (int i = 0; i != fmc->get_param_history_size(); ++i) {
        add_item_to_lc(i, fmc->get_item(i));
    }
    for (int i = 0; i < 3+4; i++)
        lc->SetColumnWidth(i, wxLIST_AUTOSIZE);
}

void SumHistoryDlg::add_item_to_lc(int pos, vector<double> const& item)
{
    lc->InsertItem(pos, wxString::Format(wxT("  %i  "), pos));
    lc->SetItem(pos, 1, wxString::Format(wxT("%i"), (int) item.size()));
    lc->SetItem(pos, 2, wxT("      ?      "));
    for (int j = 0; j < 4; j++) {
        int n = view[j];
        if (n < size(item))
            lc->SetItem(pos, 3 + j, wxString::Format(wxT("%g"), item[n]));
    }
}

void SumHistoryDlg::update_selection()
{
    FitMethodsContainer const* fmc = ftk->get_fit_container();
    int index = fmc->get_active_nr();
    lc->SetItemState (index, wxLIST_STATE_SELECTED|wxLIST_STATE_FOCUSED,
                             wxLIST_STATE_SELECTED|wxLIST_STATE_FOCUSED);
    up_arrow->Enable (index != 0);
    down_arrow->Enable (index != fmc->get_param_history_size() - 1);
}

void SumHistoryDlg::OnUpButton (wxCommandEvent&)
{
    ftk->exec("fit undo");

    // undo can cause adding new item to the history
    FitMethodsContainer const* fmc = ftk->get_fit_container();
    int ps = fmc->get_param_history_size();
    if (lc->GetItemCount() == ps - 1)
        add_item_to_lc(ps - 1, fmc->get_item(ps - 1));
    assert(lc->GetItemCount() == ps);

    update_selection();
}

void SumHistoryDlg::OnDownButton (wxCommandEvent&)
{
    ftk->exec("fit redo");
    update_selection();
}

void SumHistoryDlg::compute_wssr()
{
    if (wssr_done)
        return;
    FitMethodsContainer const* fmc = ftk->get_fit_container();
    vector<double> const orig = ftk->parameters();
    vector<DataAndModel*> dms = frame->get_selected_dms();

    for (int i = 0; i != fmc->get_param_history_size(); ++i) {
        vector<double> const& item = fmc->get_item(i);
        if (item.size() == orig.size()) {
            double wssr = ftk->get_fit()->do_compute_wssr(item, dms, true);
            lc->SetItem(i, 2, wxString::Format(wxT("%g"), wssr));
        }
    }
    lc->SetColumnWidth(2, wxLIST_AUTOSIZE);
    if (compute_wssr_button)
        compute_wssr_button->Enable(false);
    wssr_done = true;
}

void SumHistoryDlg::clear_history()
{
    ftk->exec("fit clear_history");
    // we assume that the history is empty now and disable almost everything
    lc->DeleteAllItems();
    up_arrow->Enable(false);
    down_arrow->Enable(false);
}

void SumHistoryDlg::OnSelectedItem (wxListEvent&)
{
    update_selection();
}

void SumHistoryDlg::OnActivatedItem (wxListEvent& event)
{
    int n = event.GetIndex();
    ftk->exec("fit history " + S(n));
    update_selection();
}

void SumHistoryDlg::OnViewSpinCtrlUpdate (wxSpinEvent& event)
{
    int v = event.GetId() - ID_SHIST_V;
    assert (0 <= v && v < 4);
    int n = event.GetPosition();
    assert (0 <= n && n <= view_max);
    view[v] = n;
    //update header in wxListCtrl
    wxListItem li;
    li.SetMask (wxLIST_MASK_TEXT);
    li.SetText(wxString::Format(wxT("par. %i"), n));
    lc->SetColumn(3 + v, li);
    //update data in wxListCtrl
    FitMethodsContainer const* fmc = ftk->get_fit_container();
    for (int i = 0; i != fmc->get_param_history_size(); ++i) {
        vector<double> const& item = fmc->get_item(i);
        wxString s = n < size(item) ? wxString::Format(wxT("%g"), item[n])
                                    : wxString();
        lc->SetItem(i, 3 + v, s);
    }
}



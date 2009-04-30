// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License version 2 or (at your option) 3
// $Id$

/// In this file:
///  part of the sidebar (ListPlusText, which contains ListWithColors)

#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/imaglist.h>
#include "listptxt.h"
#include "cmn.h" //SpinCtrl, ProportionalSplitter
#include "sidebar.h" //SideBar::delete_selected_items()
#include "frame.h" // frame->focus_input() and used in update_data_list()
#include "mplot.h" // used in update_data_list()
#include "../common.h" // s2wx
#include "../logic.h" // used in update_data_list()
#include "../model.h" // used in update_data_list()
#include "../data.h" // used in update_data_list()

using namespace std;

enum {
    ID_DL_CMENU_SHOW_START = 27800 ,
    ID_DL_CMENU_SHOW_END = ID_DL_CMENU_SHOW_START+20,
    ID_DL_CMENU_FITCOLS        ,
    ID_DL_SELECTALL            ,
    ID_DL_SWITCHINFO           
};


//===============================================================
//                            ListWithColors
//===============================================================

BEGIN_EVENT_TABLE(ListWithColors, wxListView)
    EVT_LIST_COL_CLICK(-1, ListWithColors::OnColumnMenu)
    EVT_LIST_COL_RIGHT_CLICK(-1, ListWithColors::OnColumnMenu)
    EVT_RIGHT_DOWN (ListWithColors::OnRightDown)
    EVT_MENU_RANGE (ID_DL_CMENU_SHOW_START, ID_DL_CMENU_SHOW_END, 
                    ListWithColors::OnShowColumn)
    EVT_MENU (ID_DL_CMENU_FITCOLS, ListWithColors::OnFitColumnWidths)
    EVT_MENU (ID_DL_SELECTALL, ListWithColors::OnSelectAll)
    EVT_KEY_DOWN (ListWithColors::OnKeyDown)
END_EVENT_TABLE()
    
ListWithColors::ListWithColors(wxWindow *parent, wxWindowID id, 
                               vector<pair<string,int> > const& columns_)
    : wxListView(parent, id, wxDefaultPosition, wxDefaultSize,
                 wxLC_REPORT|wxLC_HRULES|wxLC_VRULES),
      columns(columns_), sidebar(0)
{
    for (size_t i = 0; i < columns.size(); ++i)
        if (columns[i].second != 0)
            InsertColumn(i, s2wx(columns[i].first), wxLIST_FORMAT_LEFT,
                         columns[i].second);
}

void ListWithColors::populate(vector<string> const& data, 
                              wxImageList* image_list,
                              int active)
{
    assert(data.size() % columns.size() == 0);
    if (!image_list && data == list_data)
        return;
    int length = data.size() / columns.size();
    Freeze();
    if (image_list) 
        AssignImageList(image_list, wxIMAGE_LIST_SMALL);
    if (GetItemCount() != length) {
        DeleteAllItems();
        for (int i = 0; i < length; ++i)
            InsertItem(i, wxT(""));
    }
    for (int i = 0; i < length; ++i) {
        int c = 0;
        for (size_t j = 0; j < columns.size(); ++j) {
            if (columns[j].second) {
                SetItem(i, c, s2wx(data[i*columns.size()+j]), c == 0 ? i : -1);
                ++c;
            }
        }
        //if (active != -2)
        //    Select(i, i == active);
    }
    list_data = data;
    
    if (active >= 0 && active < length)
        Focus(active);
    Thaw();
}

void ListWithColors::OnColumnMenu(wxListEvent&)
{
    wxMenu popup_menu; 
    for (size_t i = 0; i < columns.size(); ++i) {
        popup_menu.AppendCheckItem(ID_DL_CMENU_SHOW_START+i, 
                                   s2wx(columns[i].first));
        popup_menu.Check(ID_DL_CMENU_SHOW_START+i, columns[i].second);
    }
    popup_menu.AppendSeparator();
    popup_menu.Append(ID_DL_CMENU_FITCOLS, wxT("Fit Columns"));
    PopupMenu (&popup_menu, 10, 3);
}

void ListWithColors::OnRightDown(wxMouseEvent &event)
{
    wxMenu popup_menu; 
    popup_menu.Append(ID_DL_SELECTALL, wxT("Select &All"));
    popup_menu.Append(ID_DL_SWITCHINFO, wxT("Show/Hide &Info"));
    PopupMenu (&popup_menu, event.GetX(), event.GetY());
}

void ListWithColors::OnShowColumn(wxCommandEvent &event)
{
    int n = event.GetId() - ID_DL_CMENU_SHOW_START;
    int col=0;
    for (int i = 0; i < n; ++i)
        if (columns[i].second)
            ++col;
    //TODO if col==0 take care about images
    bool show = event.IsChecked();
    if (show) {
        InsertColumn(col, s2wx(columns[n].first));
        for (int i = 0; i < GetItemCount(); ++i)
            SetItem(i, col, s2wx(list_data[i*columns.size()+n]));
    }
    else
        DeleteColumn(col);
    columns[n].second = show;
    Refresh();
}

void ListWithColors::OnFitColumnWidths(wxCommandEvent&)
{
    for (int i = 0; i < GetColumnCount(); ++i)
        SetColumnWidth(i, wxLIST_AUTOSIZE);
}

void ListWithColors::OnSelectAll(wxCommandEvent&)
{
    for (int i = 0; i < GetItemCount(); ++i)
        Select(i, true);
}

void ListWithColors::OnKeyDown (wxKeyEvent& event)
{
    switch (event.m_keyCode) {
        case WXK_DELETE:
            if (sidebar)
                sidebar->delete_selected_items();
            break;
        default:
            frame->focus_input(event);
    }
}


//===============================================================
//                            ListPlusText
//===============================================================
BEGIN_EVENT_TABLE(ListPlusText, ProportionalSplitter)
    EVT_MENU (ID_DL_SWITCHINFO, ListPlusText::OnSwitchInfo)
END_EVENT_TABLE()

ListPlusText::ListPlusText(wxWindow *parent, wxWindowID id, wxWindowID list_id,
                           vector<pair<string,int> > const& columns_)
: ProportionalSplitter(parent, id, 0.75) 
{
    list = new ListWithColors(this, list_id, columns_);
    inf = new wxTextCtrl(this, -1, wxT(""), wxDefaultPosition, wxDefaultSize,
                         wxTE_RICH|wxTE_READONLY|wxTE_MULTILINE);
}

void ListPlusText::OnSwitchInfo(wxCommandEvent&)
{
    if (IsSplit())
        Unsplit(inf);
    else
        SplitHorizontally(list, inf);
}


//===============================================================

void DataListPlusText::update_data_list(bool nondata_changed)
{
    if (!frame)
        return;

    MainPlot const* mplot = frame->get_main_plot();
    wxColour const& bg_col = mplot->get_bg_color();

    vector<string> data_data;
    for (int i = 0; i < ftk->get_dm_count(); ++i) {
        DataAndModel const* dm = ftk->get_dm(i);
        data_data.push_back(S(i));
        data_data.push_back(S(dm->model()->get_ff_names().size()) 
                            + "+" + S(dm->model()->get_zz_names().size()));
        data_data.push_back(dm->data()->get_title());
        data_data.push_back(dm->data()->get_filename());
    }
    wxImageList* data_images = 0;
    if (nondata_changed || ftk->get_dm_count() > list->GetItemCount()) {
        data_images = new wxImageList(16, 16);
        for (int i = 0; i < ftk->get_dm_count(); ++i) 
            data_images->Add(make_color_bitmap16(mplot->get_data_color(i), 
                                                 bg_col));
    }
    int focused = list->GetFocusedItem();
    if (focused < 0) 
        focused = 0;
    else if (focused >= list->GetItemCount())
        focused = list->GetItemCount() - 1;
    list->populate(data_data, data_images, focused);
}


vector<string> DataListPlusText::get_selected_data() const
{
    vector<string> dd;
    for (int i=list->GetFirstSelected(); i != -1; i = list->GetNextSelected(i))
        dd.push_back("@" + S(i));
    //if (dd.empty()) {
    //    int n = list->GetFocusedItem();
    //    dd.push_back("@" + S(n == -1 ? 0 : n));
    //}
    return dd;
}



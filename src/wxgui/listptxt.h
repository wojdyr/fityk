// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$

#ifndef FITYK__WX_LISTPTXT__H__
#define FITYK__WX_LISTPTXT__H__

#include <vector>
#include <string>
#include <wx/listctrl.h>

#include "img/color.xpm"
#include "cmn.h" //ProportionalSplitter

class SideBar;

class ListWithColors : public wxListView
{
public:
    ListWithColors(wxWindow *parent, wxWindowID id, 
                   std::vector<std::pair<std::string,int> > const& columns_);
    void populate(std::vector<std::string> const& data, 
                  wxImageList* image_list = 0,
                  int active = -2);
    void OnColumnMenu(wxListEvent &event);
    void OnRightDown(wxMouseEvent &event);
    void OnShowColumn(wxCommandEvent &event);
    void OnFitColumnWidths(wxCommandEvent &event);
    void OnSelectAll(wxCommandEvent &event);
    void OnKeyDown (wxKeyEvent& event);
    void set_side_bar(SideBar* sidebar_) { sidebar=sidebar_; }
    DECLARE_EVENT_TABLE()
private:
    std::vector<std::pair<std::string,int> > columns;
    std::vector<std::string> list_data;
    SideBar *sidebar;
};

class ListPlusText : public ProportionalSplitter
{
public:
    ListWithColors *list;
    wxTextCtrl* inf;

    ListPlusText(wxWindow *parent, wxWindowID id, wxWindowID list_id,
                 std::vector<std::pair<std::string,int> > const& columns_);

    void OnSwitchInfo(wxCommandEvent &event);
    void split(double prop) { SplitHorizontally(list, inf, prop); }
    DECLARE_EVENT_TABLE()
};

class DataListPlusText : public ListPlusText
{
public:
    DataListPlusText(wxWindow *parent, wxWindowID id, wxWindowID list_id,
                 std::vector<std::pair<std::string,int> > const& columns_)
        : ListPlusText(parent, id, list_id, columns_) {}
    void update_data_list(bool nondata_changed, bool with_stats);
    std::vector<std::string> get_selected_data() const;
};

inline
wxBitmap make_color_bitmap16(wxColour const& col, wxColour const& bg)
{
    wxImage image(color_xpm);
    image.Replace(0, 0, 0, bg.Red(), bg.Green(), bg.Blue());
    image.Replace(255, 255, 255, col.Red(), col.Green(), col.Blue());
    return wxBitmap(image);
}

#endif

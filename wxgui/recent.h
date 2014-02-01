// This file is part of fityk program. Copyright 2014 Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
/// Recent Files - for use in menu

#ifndef FITYK_WX_RECENT_H_
#define FITYK_WX_RECENT_H_

#include <list>
#include <wx/wx.h>
#include <wx/filename.h>
#include <wx/config.h>

class RecentFiles
{
public:
    struct Item {
        int id;
        wxFileName fn;
        wxString options;
    };
    RecentFiles(int first_item_id, const wxString& config_group)
        : first_item_id_(first_item_id), config_group_(config_group),
          menu_(NULL) {}
    void load_from_config(wxConfigBase *c);
    void save_to_config(wxConfigBase *c);
    wxMenu* menu() { return menu_; }
    void add(const wxString& path, const wxString& options);
    // moves the item to the top and returns file path
    const Item& pull(int id);

private:
    const int first_item_id_;
    const wxString config_group_;
    std::list<Item> items_;
    wxMenu *menu_;
};

#endif // FITYK_WX_RECENT_H_

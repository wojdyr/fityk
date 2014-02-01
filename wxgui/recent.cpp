// This file is part of fityk program. Copyright 2001-2014 Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#include "recent.h"

void RecentFiles::load_from_config(wxConfigBase *c)
{
    const int MAX_ITEMS_TO_READ = 20;
    filenames_.clear();
    if (c && c->HasGroup(config_group_)) {
        for (int i = 0; i < MAX_ITEMS_TO_READ; i++) {
            wxString key = wxString::Format("%s/%i", config_group_, i);
            if (c->HasEntry(key))
                filenames_.push_back(wxFileName(c->Read(key, wxT(""))));
        }
    }
}

void RecentFiles::save_to_config(wxConfigBase *c)
{
    const int MAX_ITEMS_TO_WRITE = 9;
    if (!c)
        return;
    if (c->HasGroup(config_group_))
        c->DeleteGroup(config_group_);
    int counter = 0;
    for (std::list<wxFileName>::const_iterator i = filenames_.begin();
         i != filenames_.end() && counter < MAX_ITEMS_TO_WRITE;
         ++i, ++counter) {
        wxString key = wxString::Format("%s/%i", config_group_, counter);
        c->Write(key, i->GetFullPath());
    }
}

void RecentFiles::add(const wxString& path)
{
    assert(menu_ != NULL);
    const int MAX_NUMBER_OF_ITEMS = 15;

    const int count = menu_->GetMenuItemCount();
    const wxMenuItemList& mlist = menu_->GetMenuItems();
    const wxFileName fn = wxFileName(path);
    filenames_.remove(fn);
    filenames_.push_front(fn);
    int id = 0;
    for (wxMenuItemList::compatibility_iterator i = mlist.GetFirst(); i;
                                                            i = i->GetNext())
        //FIXME
        if (i->GetData()->GetHelp() == fn.GetFullPath()) {
            id = i->GetData()->GetId();
            menu_->Delete(i->GetData());
            break;
        }
    if (id == 0) {
        if (count < MAX_NUMBER_OF_ITEMS) {
            id = first_item_id_ + count;
        } else {
            wxMenuItem *item = mlist.GetLast()->GetData();
            id = item->GetId();
            menu_->Delete(item);
        }
    }
    menu_->Prepend(id, fn.GetFullName(), fn.GetFullPath());
}

wxMenu* RecentFiles::menu()
{
    const int MAX_NUMBER_OF_ITEMS = 15;
    if (menu_ == NULL) {
        menu_ = new wxMenu;
        int counter = 0;
        for (std::list<wxFileName>::const_iterator i = filenames_.begin();
                                                 i != filenames_.end(); ++i) {
            menu_->Append(first_item_id_+counter,
                          i->GetFullName(), i->GetFullPath());
            counter++;
            if (counter == MAX_NUMBER_OF_ITEMS)
                break;
        }
    }
    return menu_;
}

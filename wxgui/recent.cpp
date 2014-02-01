// This file is part of fityk program. Copyright 2001-2014 Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#include "recent.h"

using std::list;
using std::string;

static const int MAX_NUMBER_OF_ITEMS = 15;
static const char* MAGIC_SEP = "   ";

void RecentFiles::load_from_config(wxConfigBase *c)
{
    items_.clear();
    if (menu_ != NULL)
        delete menu_;

    menu_ = new wxMenu;
    if (c && c->HasGroup(config_group_)) {
        // gaps shouldn't happend in the config file, but just in case...
        int counter = 0;
        for (int i = 0; i < MAX_NUMBER_OF_ITEMS; i++) {
            wxString key = wxString::Format("%s/%i", config_group_, i);
            if (c->HasEntry(key)) {
                int id = first_item_id_ + counter;
                ++counter;
                wxString value = c->Read(key, wxT(""));
                wxString opt;
                string::size_type sep = value.find(MAGIC_SEP);
                if (sep != string::npos) {
                    opt = value.substr(sep+3);
                    value.resize(sep);
                }
                wxFileName fn(value);
                Item item = { id, fn, opt };
                items_.push_back(item);
                wxString hint = fn.GetFullPath();
                if (!opt.empty())
                    hint += " " + opt;
                menu_->Append(id, fn.GetFullName(), hint);
            }
        }
    }
}

void RecentFiles::save_to_config(wxConfigBase *c)
{
    if (!c)
        return;
    if (c->HasGroup(config_group_))
        c->DeleteGroup(config_group_);
    int counter = 0;
    for (list<Item>::const_iterator i = items_.begin(); i != items_.end(); ++i){
        wxString key = wxString::Format("%s/%i", config_group_, counter);
        counter++;
        wxString value = i->fn.GetFullPath();
        if (!i->options.empty())
            value += MAGIC_SEP + i->options;
        c->Write(key, value);
    }
}

void RecentFiles::add(const wxString& path, const wxString& options)
{
    assert(menu_ != NULL);
    const wxFileName fn = wxFileName(path);

    // avoid duplicates
    for (list<Item>::iterator it = items_.begin(); it != items_.end(); ++it) {
        if (it->fn == fn && it->options == options) {
            pull(it->id);
            return;
        }
    }

    int id;
    if (items_.size() < MAX_NUMBER_OF_ITEMS) {
        id = first_item_id_ + items_.size();
    } else {
        id = items_.back().id;
        items_.pop_back();
        menu_->Destroy(id);
    }

    Item item = { id, fn, options };
    items_.push_front(item);
    wxString hint = fn.GetFullPath();
    if (!options.empty())
        hint += " " + options;
    menu_->Prepend(id, fn.GetFullName(), hint);
}

const RecentFiles::Item& RecentFiles::pull(int id)
{
    for (list<Item>::iterator it = items_.begin(); it != items_.end(); ++it) {
        if (it->id == id) {
            if (it != items_.begin()) {
                items_.push_front(*it); // it does not invalidate iterator
                items_.erase(it);
                menu_->Prepend(menu_->Remove(id));
            }
            break;
        }
    }
    return items_.front();
}


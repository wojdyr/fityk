// Author: Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
// $Id: $
// A dialog for choosing space group settings

// TODO:
// double-click SG list on wxGTK doesn't work


#ifndef FITYK_WX_SGCHOOSER_H_
#define FITYK_WX_SGCHOOSER_H_

#include <wx/dialog.h>

class wxChoice;
class wxListView;
class wxCheckBox;

class SpaceGroupChooser : public wxDialog
{
public:
    SpaceGroupChooser(wxWindow* parent);
    void OnCheckBox(wxCommandEvent&) { regenerate_list(); }
    void OnSystemChoice(wxCommandEvent&) { regenerate_list(); }
    void OnListItemActivated(wxCommandEvent&);
    void regenerate_list();
    wxString get_value() const;

private:
    wxChoice *system_c;
    wxListView *list;
    wxCheckBox *centering_cb[7];

    // disallow copy and assign
    SpaceGroupChooser(const SpaceGroupChooser&);
    void operator=(const SpaceGroupChooser&);
};

#endif //FITYK_WX_SGCHOOSER_H_

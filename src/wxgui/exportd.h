// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#ifndef FITYK_WX_EXPORTD_H_
#define FITYK_WX_EXPORTD_H_

#include <string>
#include <vector>

bool export_data_dlg(wxWindow *parent);

// helper used also in frame.cpp
void exec_redirected_command(const std::vector<int>& sel,
                             const std::string& cmd, const wxString& path);
#endif

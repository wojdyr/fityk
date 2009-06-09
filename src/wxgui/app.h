// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License version 2 or (at your option) 3
// $Id$

#ifndef FITYK__WX_APP__H__
#define FITYK__WX_APP__H__


class wxCmdLineParser;


/// Fityk-GUI "main loop"
class FApp: public wxApp
{
public:
    // default config name
    std::string conf_filename;
    // alternative config name
    std::string alt_conf_filename;
    // directory for (named by user) config files
    wxString config_dir;

    bool OnInit(void);
    int OnExit();

private:
    bool is_fityk_script(std::string filename);
    void process_argv(wxCmdLineParser &cmdLineParser);
};

wxString get_full_path_of_help_file (wxString const& name);

DECLARE_APP(FApp)

#endif


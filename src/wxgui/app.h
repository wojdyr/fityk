// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License version 2
// $Id$

#ifndef FITYK__WX_APP__H__
#define FITYK__WX_APP__H__


class wxCmdLineParser;


/// Fityk-GUI "main loop"
class FApp: public wxApp
{
public:
    wxString conf_filename, alt_conf_filename, conf_prefix;

    bool OnInit(void);
    int OnExit();

private:
    bool is_fityk_script(std::string filename);
    void process_argv(wxCmdLineParser &cmdLineParser);
};

std::string get_full_path_of_help_file (std::string const& name);

DECLARE_APP(FApp)

#endif 


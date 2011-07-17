// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#ifndef FITYK__WX_APP__H__
#define FITYK__WX_APP__H__


class wxCmdLineParser;


/// Fityk-GUI "main loop"
class FApp: public wxApp
{
public:
    // directory for (named by user) config files
    wxString config_dir;

    virtual bool OnInit(void);
    virtual int OnExit();
#ifdef __WXMAC__
    virtual void MacOpenFile(const wxString &fileName);
#endif

private:
    bool is_fityk_script(std::string filename);
    void process_argv(wxCmdLineParser &cmdLineParser);
};

wxString get_help_url(const wxString& name);
wxString get_sample_path(const wxString& name);
#ifdef __WXMAC__
void open_new_instance();
#endif

DECLARE_APP(FApp)

#endif


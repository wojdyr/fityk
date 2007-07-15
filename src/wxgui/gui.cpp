// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License version 2
// $Id$

// wxwindows headers, see wxwindows samples for description
#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/laywin.h>
#include <wx/splitter.h>
#include <wx/filedlg.h>
#include <wx/valtext.h>
#include <wx/textdlg.h>
#include <wx/numdlg.h>
#include <wx/utils.h>
#include <wx/statline.h>
#include <wx/html/htmlwin.h>
#include <wx/printdlg.h>
#include <wx/image.h>
#include <wx/config.h>
#include <wx/fileconf.h>
#include <wx/msgout.h>
#include <wx/metafile.h>
#include <wx/dir.h>
#include <wx/mstream.h>

#ifdef __WXMAC__
# include <wx/version.h>
# if wxCHECK_VERSION(2, 8, 0)
#  include <wx/stdpaths.h>
# else
#  error "wxWidgets 2.8 or later is required on Mac OSX"
# endif
#endif

#include <algorithm>
#include <locale.h>
#include <string.h>
#include <ctype.h>

#include "gui.h"
#include "plot.h"
#include "mplot.h"
#include "aplot.h"
#include "dialogs.h"
#include "dload.h"
#include "pane.h"
#include "sidebar.h"
#include "print.h"
#include "dataedit.h"
#include "defmgr.h"
#include "sdebug.h"
#include "setdlg.h"
#include "statbar.h"
#include "inputline.h"
#include "app.h"
#include "../common.h"
#include "../logic.h"
#include "../fit.h"
#include "../data.h"
#include "../settings.h"
#include "../cmd.h"
#include "../guess.h"
#include "../func.h"

#include "img/fityk.xpm"
//toolbars icons
#include "img/active_mode.xpm"
#include "img/addpeak_mode.xpm"
#include "img/add_peak.xpm"
#include "img/bg_mode.xpm"
#include "img/cont_fit.xpm"
#include "img/edit_data.xpm"
#include "img/manual.xpm"
#include "img/open_data_custom.xpm"
#include "img/open_data.xpm"
#include "img/right_pane.xpm"
#include "img/run_fit.xpm"
#include "img/run_script.xpm"
#include "img/save_data.xpm"
#include "img/save_script.xpm"
#include "img/strip_bg.xpm"
//#include "img/undo_fit.xpm"
#include "img/zoom_all.xpm"
#include "img/zoom_left.xpm"
#include "img/zoom_mode.xpm"
#include "img/zoom_prev.xpm"
#include "img/zoom_right.xpm"
#include "img/zoom_up.xpm"
#include "img/zoom_vert.xpm"

#include "img/book16.h"

// I have wxUSE_METAFILE=1 on wxGTK-2.6.1, which was probably a bug
#if wxUSE_METAFILE && defined(__WXGTK__)
#undef wxUSE_METAFILE
#endif

using namespace std;
FFrame *frame = NULL;
Fityk *ftk = NULL;

enum {
    ID_H_MANUAL        = 24001 ,
    ID_H_CONTACT               ,
    ID_D_LOAD                  ,
    ID_D_XLOAD                 ,
    ID_D_RECENT                , //and next ones
    ID_D_RECENT_END = ID_D_RECENT+30 , 
    ID_D_EDITOR                ,
    ID_D_FDT                   ,
    ID_D_FDT_END = ID_D_FDT+50 ,
    ID_D_ALLDS                 ,
    ID_D_MERGE                 ,
    ID_D_EXPORT                ,
    ID_DEFMGR                  ,
    ID_S_GUESS                 ,
    ID_S_PFINFO                ,
    ID_S_FUNCLIST              ,
    ID_S_VARLIST               ,
    ID_S_EXPORTP               ,
    ID_S_EXPORTF               ,
    ID_S_EXPORTD               ,
    ID_F_METHOD                ,
    ID_F_RUN                   ,
    ID_F_INFO                  ,
    ID_F_UNDO                  ,
    ID_F_REDO                  ,
    ID_F_HISTORY               ,
    ID_F_CLEARH                ,
    ID_F_M                     , 
    ID_F_M_END = ID_F_M+10     , 
    ID_SESSION_LOG             ,
    ID_LOG_START               ,
    ID_LOG_STOP                ,
    ID_LOG_WITH_OUTPUT         ,
    ID_LOG_DUMP                ,
    ID_O_RESET                 ,
    ID_PAGE_SETUP              ,
    ID_PRINT_PSFILE            ,
    ID_PRINT_CLIPB             ,
    ID_O_INCLUDE               ,
    ID_O_REINCLUDE             ,
    ID_S_DEBUGGER              ,
    ID_O_DUMP                  ,
    ID_SESSION_SET             ,
    ID_SESSION_EI              ,
    ID_G_MODE                  ,
    ID_G_M_ZOOM                ,
    ID_G_M_RANGE               ,
    ID_G_M_BG                  ,
    ID_G_M_ADD                 ,
    ID_G_M_BG_STRIP            ,
    ID_G_M_BG_UNDO             ,
    ID_G_M_BG_CLEAR            ,
    ID_G_M_BG_SPLINE           ,
    ID_G_M_BG_SUB              ,
    ID_G_M_PEAK                ,
    ID_G_M_PEAK_N              ,
    ID_G_M_PEAK_N_END = ID_G_M_PEAK_N+300,
    ID_G_SHOW                  ,
    ID_G_S_TOOLBAR             ,
    ID_G_S_STATBAR             ,
    ID_G_S_SIDEB               ,
    ID_G_S_A1                  ,
    ID_G_S_A2                  ,
    ID_G_S_IO                  ,
    ID_G_C_MAIN                ,
    ID_G_C_A1                  ,
    ID_G_C_A2                  ,
    ID_G_C_OUTPUT              ,
    ID_G_C_SB                  ,
    ID_G_CROSSHAIR             ,
    ID_G_FULLSCRN              ,
    ID_G_V_ALL                 ,
    ID_G_V_VERT                ,
    ID_G_V_SCROLL_L            ,
    ID_G_V_SCROLL_R            ,
    ID_G_V_SCROLL_U            ,
    ID_G_V_EXTH                ,
    ID_G_V_ZOOM_PREV           ,
    ID_G_V_ZOOM_PREV_END = ID_G_V_ZOOM_PREV+40 ,
    ID_G_LCONF1                ,
    ID_G_LCONF2                ,
    ID_G_LCONF                 ,
    ID_G_LCONFB                ,
    ID_G_LCONF_X               ,
    ID_G_LCONF_X_END = ID_G_LCONF_X+20,
    ID_G_SCONF                 ,
    ID_G_SCONF1                ,
    ID_G_SCONF2                ,
    ID_G_SCONFAS               ,

    ID_ft_m_zoom               ,
    ID_ft_m_range              ,
    ID_ft_m_bg                 ,
    ID_ft_m_add                ,
    ID_ft_v_pr                 ,
    ID_ft_b_strip              ,
    ID_ft_f_run                ,
    ID_ft_f_cont               ,
    ID_ft_f_undo               ,
    ID_ft_s_aa                 ,
    ID_ft_sideb                ,
    ID_ft_peakchoice
};


// from http://www.wxwidgets.org/wiki/index.php/Embedding_PNG_Images
wxBitmap GetBitmapFromMemory_(const unsigned char *data, int length) 
{
    wxMemoryInputStream is(data, length);
    return wxBitmap(wxImage(is, wxBITMAP_TYPE_PNG));
}

#define GET_BMP(name) \
            GetBitmapFromMemory_(name##_png, sizeof(name##_png))


void append_mi(wxMenu* menu, int id, wxBitmap const& bitmap,
               const wxString& text=wxT(""), const wxString& helpString=wxT(""))
{
    wxMenuItem *item = new wxMenuItem(menu, id, text, helpString);
#if wxUSE_OWNER_DRAWN || defined(__WXGTK__)
    item->SetBitmap(bitmap);
#endif
    menu->Append(item);
}


BEGIN_EVENT_TABLE(FFrame, wxFrame)
    EVT_MENU (ID_D_LOAD,        FFrame::OnDLoad)   
    EVT_MENU (ID_D_XLOAD,       FFrame::OnDXLoad)   
    EVT_MENU_RANGE (ID_D_RECENT+1, ID_D_RECENT_END, FFrame::OnDRecent)
    EVT_MENU (ID_D_EDITOR,      FFrame::OnDEditor)
    EVT_MENU_RANGE (ID_D_FDT+1, ID_D_FDT_END, FFrame::OnFastDT)
    EVT_UPDATE_UI (ID_D_ALLDS,  FFrame::OnAllDatasetsUpdate) 
    EVT_MENU (ID_D_MERGE,       FFrame::OnDMerge)
    EVT_MENU (ID_D_EXPORT,      FFrame::OnDExport) 

    EVT_MENU (ID_DEFMGR,        FFrame::OnDefinitionMgr)   
    EVT_MENU (ID_S_GUESS,       FFrame::OnSGuess)   
    EVT_MENU (ID_S_PFINFO,      FFrame::OnSPFInfo)   
    EVT_MENU (ID_S_FUNCLIST,    FFrame::OnSFuncList)    
    EVT_MENU (ID_S_VARLIST,     FFrame::OnSVarList)  
    EVT_MENU (ID_S_EXPORTP,     FFrame::OnSExport)   
    EVT_MENU (ID_S_EXPORTF,     FFrame::OnSExport)   
    EVT_MENU (ID_S_EXPORTD,     FFrame::OnDExport)   

    EVT_UPDATE_UI (ID_F_METHOD, FFrame::OnFMethodUpdate)
    EVT_MENU_RANGE (ID_F_M+0, ID_F_M_END, FFrame::OnFOneOfMethods)    
    EVT_MENU (ID_F_RUN,         FFrame::OnFRun)    
    EVT_MENU (ID_F_INFO,        FFrame::OnFInfo)    
    EVT_MENU (ID_F_UNDO,        FFrame::OnFUndo)    
    EVT_MENU (ID_F_REDO,        FFrame::OnFRedo)    
    EVT_MENU (ID_F_HISTORY,     FFrame::OnFHistory)    
    EVT_MENU (ID_F_CLEARH,      FFrame::OnFClearH)

    EVT_UPDATE_UI (ID_SESSION_LOG, FFrame::OnLogUpdate)    
    EVT_MENU (ID_LOG_START,     FFrame::OnLogStart)
    EVT_MENU (ID_LOG_STOP,      FFrame::OnLogStop)
    EVT_MENU (ID_LOG_WITH_OUTPUT, FFrame::OnLogWithOutput)
    EVT_MENU (ID_LOG_DUMP,      FFrame::OnLogDump)
    EVT_MENU (ID_O_RESET,       FFrame::OnReset)    
    EVT_MENU (ID_O_INCLUDE,     FFrame::OnInclude)    
    EVT_MENU (ID_O_REINCLUDE,   FFrame::OnReInclude)    
    EVT_MENU (ID_S_DEBUGGER,    FFrame::OnDebugger)    
    EVT_MENU (wxID_PRINT,         FFrame::OnPrint)
    EVT_MENU (ID_PRINT_PSFILE,  FFrame::OnPrintPSFile)
    EVT_MENU (ID_PRINT_CLIPB,   FFrame::OnPrintToClipboard)
    EVT_MENU (ID_PAGE_SETUP,    FFrame::OnPageSetup)
    EVT_MENU (wxID_PREVIEW,     FFrame::OnPrintPreview)
    EVT_MENU (ID_O_DUMP,        FFrame::OnDump)    
    EVT_MENU (ID_SESSION_SET,   FFrame::OnSettings)    
    EVT_MENU (ID_SESSION_EI,    FFrame::OnEditInit)    

    EVT_MENU (ID_G_M_ZOOM,      FFrame::OnChangeMouseMode)
    EVT_MENU (ID_G_M_RANGE,     FFrame::OnChangeMouseMode)
    EVT_MENU (ID_G_M_BG,        FFrame::OnChangeMouseMode)
    EVT_MENU (ID_G_M_ADD,       FFrame::OnChangeMouseMode)
    EVT_MENU_RANGE (ID_G_M_PEAK_N, ID_G_M_PEAK_N_END, FFrame::OnChangePeakType)
    EVT_UPDATE_UI(ID_G_M_BG_SUB,FFrame::OnGMBgUpdate)
    EVT_MENU (ID_G_M_BG_STRIP,  FFrame::OnStripBg)
    EVT_MENU (ID_G_M_BG_UNDO,   FFrame::OnUndoBg)
    EVT_MENU (ID_G_M_BG_CLEAR,  FFrame::OnClearBg)
    EVT_MENU (ID_G_M_BG_SPLINE, FFrame::OnSplineBg)
    EVT_UPDATE_UI (ID_G_SHOW,   FFrame::OnGuiShowUpdate)
    EVT_MENU (ID_G_S_SIDEB,     FFrame::OnSwitchSideBar)
    EVT_MENU_RANGE (ID_G_S_A1, ID_G_S_A2, FFrame::OnSwitchAuxPlot)
    EVT_MENU (ID_G_S_IO,        FFrame::OnSwitchIOPane)
    EVT_MENU (ID_G_S_TOOLBAR,   FFrame::OnSwitchToolbar)
    EVT_MENU (ID_G_S_STATBAR,   FFrame::OnSwitchStatbar)
    EVT_MENU_RANGE (ID_G_C_MAIN, ID_G_C_OUTPUT, FFrame::OnShowPopupMenu)
    EVT_MENU (ID_G_C_SB,        FFrame::OnConfigureStatusBar)
    EVT_MENU (ID_G_CROSSHAIR,   FFrame::OnSwitchCrosshair)
    EVT_MENU (ID_G_FULLSCRN,   FFrame::OnSwitchFullScreen)
    EVT_MENU (ID_G_V_ALL,       FFrame::OnGViewAll)
    EVT_MENU (ID_G_V_VERT,      FFrame::OnGFitHeight)
    EVT_MENU (ID_G_V_SCROLL_L,  FFrame::OnGScrollLeft)
    EVT_MENU (ID_G_V_SCROLL_R,  FFrame::OnGScrollRight)
    EVT_MENU (ID_G_V_SCROLL_U,  FFrame::OnGScrollUp)
    EVT_MENU (ID_G_V_EXTH,      FFrame::OnGExtendH)
    EVT_MENU_RANGE (ID_G_V_ZOOM_PREV+1, ID_G_V_ZOOM_PREV_END, 
                                FFrame::OnPreviousZoom)    
    EVT_MENU (ID_G_LCONF1,      FFrame::OnConfigRead)
    EVT_MENU (ID_G_LCONF2,      FFrame::OnConfigRead)
    EVT_MENU (ID_G_LCONFB,      FFrame::OnConfigBuiltin)
    EVT_UPDATE_UI (ID_G_LCONFB, FFrame::OnUpdateLConfMenu)
    EVT_MENU_RANGE (ID_G_LCONF_X, ID_G_LCONF_X_END, FFrame::OnConfigX)
    EVT_MENU (ID_G_SCONF1,      FFrame::OnConfigSave)
    EVT_MENU (ID_G_SCONF2,      FFrame::OnConfigSave)
    EVT_MENU (ID_G_SCONFAS,     FFrame::OnConfigSaveAs)

    EVT_MENU (ID_H_MANUAL,      FFrame::OnShowHelp)
    EVT_MENU (ID_H_CONTACT,     FFrame::OnContact)
    EVT_MENU (wxID_ABOUT,       FFrame::OnAbout)
    EVT_MENU (wxID_EXIT,        FFrame::OnQuit)
END_EVENT_TABLE()


    // Define my frame constructor
FFrame::FFrame(wxWindow *parent, const wxWindowID id, const wxString& title, 
                 const long style)
    : wxFrame(parent, id, title, wxDefaultPosition, wxDefaultSize, style), 
      main_pane(0), sidebar(0), status_bar(0), 
      toolbar(0), 
#ifdef __WXMSW__
      help()
#else //wxHtmlHelpController
      help(wxHF_TOOLBAR|wxHF_CONTENTS|wxHF_SEARCH|wxHF_BOOKMARKS|wxHF_PRINT
           |wxHF_MERGE_BOOKS)
#endif
{
    peak_type_nr = wxConfig::Get()->Read(wxT("/DefaultFunctionType"), 7);
    update_peak_type_list();
    // Load icon and bitmap
    SetIcon (wxICON (fityk));

    //sizer, splitters, etc.
    wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
    v_splitter = new ProportionalSplitter(this, -1, 0.8);
    main_pane = new ProportionalSplitter(v_splitter, -1, 0.7);
    plot_pane = new PlotPane(main_pane);
    io_pane = new IOPane(main_pane);
    main_pane->SplitHorizontally(plot_pane, io_pane);
    sidebar = new SideBar(v_splitter);
    sidebar->Show(false);
    v_splitter->Initialize(main_pane);
    sizer->Add(v_splitter, 1, wxEXPAND, 0);

    read_recent_data_files();
    set_menubar();

    toolbar = new FToolBar(this, -1);
    toolbar->update_peak_type(peak_type_nr, &peak_types);
    SetToolBar(toolbar);

    //status bar
    status_bar = new FStatusBar(this);
    SetStatusBar(status_bar);

    SetSizer(sizer);
    sizer->SetSizeHints(this);

    print_mgr = new PrintManager(plot_pane);

    string help_path = get_full_path_of_help_file("fitykhelp.htb"); 
    string help_path_no_exten = help_path.substr(0, help_path.size() - 4);
    help.Initialize(s2wx(help_path_no_exten));

    update_menu_functions();
    update_menu_fast_tranforms();
    io_pane->SetFocus();
}

FFrame::~FFrame() 
{
    write_recent_data_files();
    wxConfig::Get()->Write(wxT("/DefaultFunctionType"), peak_type_nr);
    delete print_mgr;
}

void FFrame::OnQuit(wxCommandEvent&)
{
    Close(true);
}

void FFrame::update_peak_type_list()
{
    peak_types = Function::get_all_types();
    if (peak_type_nr >= size(peak_types))
        peak_type_nr = 0;
    if (toolbar) 
        toolbar->update_peak_type(peak_type_nr, &peak_types);
}


void FFrame::read_recent_data_files()
{
    recent_data_files.clear();
    wxConfigBase *c = wxConfig::Get();
    if (c && c->HasGroup(wxT("/RecentDataFiles"))) {
        for (int i = 0; i < 20; i++) {
            wxString key = wxString::Format(wxT("/RecentDataFiles/%i"), i);
            if (c->HasEntry(key))
                recent_data_files.push_back(wxFileName(c->Read(key, wxT(""))));
        }
    }
}

void FFrame::write_recent_data_files()
{
    wxConfigBase *c = wxConfig::Get();
    wxString group(wxT("/RecentDataFiles"));
    if (c->HasGroup(group))
        c->DeleteGroup(group);
    int counter = 0;
    for (list<wxFileName>::const_iterator i = recent_data_files.begin(); 
         i != recent_data_files.end() && counter < 9; 
         i++, counter++) {
        wxString key = group + wxT("/") + s2wx(S(counter));
        c->Write(key, i->GetFullPath());
    }
}

void FFrame::add_recent_data_file(string const& filename)
{
    int const count = data_menu_recent->GetMenuItemCount();
    wxMenuItemList const& mlist = data_menu_recent->GetMenuItems();
    wxFileName const fn = wxFileName(s2wx(filename));
    recent_data_files.remove(fn);
    recent_data_files.push_front(fn);
    int id_new = 0;
	for (wxMenuItemList::compatibility_iterator i = mlist.GetFirst(); i; 
                                                            i = i->GetNext()) 
        if (i->GetData()->GetHelp() == fn.GetFullPath()) {
            id_new = i->GetData()->GetId();
            data_menu_recent->Delete(i->GetData());
            break;
        }
    if (id_new == 0) {
        if (count >= 15) {
            wxMenuItem *item = mlist.GetLast()->GetData();
            id_new = item->GetId();
            data_menu_recent->Delete(item);
        }
        else
            id_new = ID_D_RECENT+count+1;
    }
    data_menu_recent->Prepend(id_new, fn.GetFullName(), fn.GetFullPath());
}

void FFrame::read_all_settings(wxConfigBase *cf)
{
    read_settings(cf);
    plot_pane->read_settings(cf);
    io_pane->output_win->read_settings(cf);
    status_bar->read_settings(cf);
    sidebar->read_settings(cf);
    sidebar->update_lists();
}

void FFrame::read_settings(wxConfigBase *cf)
{
    // restore window layout, frame position and size
    cf->SetPath(wxT("/Frame"));
    SwitchToolbar(cfg_read_bool(cf, wxT("ShowToolbar"), true));
    SwitchStatbar(cfg_read_bool(cf, wxT("ShowStatbar"), true));
    int x = cf->Read(wxT("x"), 50),
        y = cf->Read(wxT("y"), 50),
        w = cf->Read(wxT("w"), 650),
        h = cf->Read(wxT("h"), 400);
    Move(x, y);
    SetClientSize(w, h);
    v_splitter->SetProportion(cfg_read_double(cf, wxT("VertSplitProportion"), 
                                              0.8));
    SwitchSideBar(cfg_read_bool(cf, wxT("ShowSideBar"), true));
    main_pane->SetProportion(cfg_read_double(cf, 
                                             wxT("MainPaneProportion"), 0.7));
    SwitchIOPane(cfg_read_bool(cf, wxT("ShowIOPane"), true));
    SwitchCrosshair(cfg_read_bool(cf, wxT("ShowCrosshair"), false));
}

void FFrame::save_all_settings(wxConfigBase *cf) const
{
    cf->Write(wxT("/FitykVersion"), pchar2wx(VERSION));
    save_settings(cf);
    plot_pane->save_settings(cf);
    io_pane->output_win->save_settings(cf);
    status_bar->save_settings(cf);
    sidebar->save_settings(cf);
}

void FFrame::save_settings(wxConfigBase *cf) const
{
    cf->SetPath(wxT("/Frame"));
    cf->Write(wxT("ShowToolbar"), toolbar != 0);
    cf->Write(wxT("ShowStatbar"), status_bar != 0);
    cf->Write(wxT("VertSplitProportion"), v_splitter->GetProportion());
    cf->Write(wxT("ShowSideBar"), v_splitter->IsSplit());
    cf->Write(wxT("MainPaneProportion"), main_pane->GetProportion());
    cf->Write(wxT("ShowIOPane"), main_pane->IsSplit());
    cf->Write(wxT("ShowCrosshair"), plot_pane->crosshair_cursor);
    int x, y, w, h;
    GetClientSize(&w, &h);
    GetPosition(&x, &y);
    cf->Write(wxT("x"), (long) x);
    cf->Write(wxT("y"), (long) y);
    cf->Write(wxT("w"), (long) w);
    cf->Write(wxT("h"), (long) h);
    //cf->Write (wxT("BotWinHeight"), bottom_window->GetClientSize().GetHeight()); 
}

void FFrame::set_menubar()
{
    wxMenu* session_menu = new wxMenu;
    session_menu->Append (ID_O_INCLUDE,   wxT("&Execute script\tCtrl-X"), 
                                           wxT("Execute commands from a file"));
    session_menu->Append (ID_O_REINCLUDE, wxT("R&e-Execute script"), 
             wxT("Reset & execute commands from the file included last time"));
    session_menu->Append (ID_S_DEBUGGER, wxT("Script debu&gger"), 
                                       wxT("Show script editor and debugger"));
    session_menu->Enable (ID_O_REINCLUDE, false);
    session_menu->Append (ID_O_RESET, wxT("&Reset"), 
                                      wxT("Reset current session"));
    session_menu->AppendSeparator();
    wxMenu *session_log_menu = new wxMenu;
    session_log_menu->Append(ID_LOG_START, wxT("Choose log file"), 
                            wxT("Start logging to file (it produces script)"));
    session_log_menu->Append(ID_LOG_STOP, wxT("Stop logging"),
                                          wxT("Finish logging to file"));
    session_log_menu->AppendCheckItem(ID_LOG_WITH_OUTPUT,wxT("Log also output"),
                          wxT("output can be included in logfile as comments"));
    session_log_menu->AppendSeparator();
    session_log_menu->Append(ID_LOG_DUMP, wxT("History Dump"),
                            wxT("Save all commands executed so far to file"));
    session_menu->Append(ID_SESSION_LOG, wxT("&Logging"), session_log_menu);
    session_menu->Append(ID_O_DUMP, wxT("&Dump to file"), 
                              wxT("Save current program state as script file"));
    session_menu->AppendSeparator();
    session_menu->Append(ID_PAGE_SETUP, wxT("Page Se&tup..."), 
                                        wxT("Page setup"));
    session_menu->Append(wxID_PREVIEW, wxT("Print previe&w")); 
    session_menu->Append(wxID_PRINT, wxT("&Print...\tCtrl-P"),
                         wxT("Print plots"));
#if 0
    //it doesn't work on Windows, because there is no way
    // to have wxPostScriptPrintNativeData on MSW
    // see: src/common/prntbase.cpp:
    //        wxNativePrintFactory::CreatePrintNativeData()
    //
    session_menu->Append(ID_PRINT_PSFILE, wxT("Print to PS &File"),
                         wxT("Export plots to postscript file."));
#endif
#if wxUSE_METAFILE
    session_menu->Append(ID_PRINT_CLIPB, wxT("&Copy to Clipboard"),
                         wxT("Copy plots to clipboard."));
#endif
    session_menu->AppendSeparator();
    session_menu->Append (ID_SESSION_SET, wxT("&Settings"),
                                          wxT("Preferences and options"));
    session_menu->Append (ID_SESSION_EI, wxT("Edit &Init File"),
                             wxT("Edit script executed when program starts"));
    session_menu->AppendSeparator();
    session_menu->Append(wxID_EXIT, wxT("&Quit"));

    wxMenu* data_menu = new wxMenu;
    data_menu->Append (ID_D_LOAD, wxT("&Quick Load File\tCtrl-O"), 
                                                 wxT("Load data from file"));
    data_menu->Append (ID_D_XLOAD, wxT("&Load File\tCtrl-M"), 
                               wxT("Load data from file, with some options"));
    this->data_menu_recent = new wxMenu;
    int rf_counter = 1;
    for (list<wxFileName>::const_iterator i = recent_data_files.begin(); 
         i != recent_data_files.end() && rf_counter < 16; i++, rf_counter++) 
        data_menu_recent->Append(ID_D_RECENT + rf_counter, 
                                 i->GetFullName(), i->GetFullPath());
    data_menu->Append(ID_D_RECENT, wxT("&Recent Files"), data_menu_recent); 
    data_menu->AppendSeparator();

    data_menu->Append (ID_D_EDITOR, wxT("&Editor\tCtrl-E"), 
                                                     wxT("Open data editor"));
    this->data_ft_menu = new wxMenu;
    data_menu->Append (ID_D_FDT, wxT("&Fast Transformations"), data_ft_menu, 
                                 wxT("Quick data transformations"));
    data_menu->AppendCheckItem (ID_D_ALLDS, wxT("Apply to &All Datasets"), 
                            wxT("Apply data transformations to all datasets."));
    data_menu->AppendSeparator();
    data_menu->Append (ID_D_MERGE, wxT("&Merge Points..."), 
                                        wxT("Reduce the number of points"));
    data_menu->AppendSeparator();
    data_menu->Append (ID_D_EXPORT, wxT("&Export\tCtrl-S"), 
                                                  wxT("Save data to file"));

    wxMenu* sum_menu = new wxMenu;
    func_type_menu = new wxMenu;
    sum_menu->Append (ID_G_M_PEAK, wxT("Function &type"), func_type_menu);
    // the function list is created in update_menu_functions()
    sum_menu->Append (ID_DEFMGR, wxT("&Definition Manager"),
                      wxT("Add or modify funtion types"));
    sum_menu->Append (ID_S_GUESS, wxT("&Guess Peak"),wxT("Guess and add peak"));
    sum_menu->Append (ID_S_PFINFO, wxT("Peak-Find &Info"), 
                                wxT("Show where guessed peak would be placed"));
    sum_menu->AppendSeparator();
    sum_menu->Append (ID_S_FUNCLIST, wxT("Show &Function List"),
                                wxT("Open `Functions' tab on right-hand pane"));
    sum_menu->Append (ID_S_VARLIST, wxT("Show &Variable List"),
                                wxT("Open `Variables' tab on right-hand pane"));
    sum_menu->AppendSeparator();
    sum_menu->Append (ID_S_EXPORTP, wxT("&Export Peak Parameters"), 
                                    wxT("Export function parameters to file"));
    sum_menu->Append (ID_S_EXPORTF, wxT("&Export Formula"), 
                                    wxT("Export mathematic formula to file"));
    sum_menu->Append (ID_S_EXPORTD, wxT("&Export Points"), 
                                    wxT("Export as points in TSV file"));

    wxMenu* fit_menu = new wxMenu;
    wxMenu* fit_method_menu = new wxMenu;
    fit_method_menu->AppendRadioItem (ID_F_M+0, wxT("&Levenberg-Marquardt"), 
                                                wxT("gradient based method"));
    fit_method_menu->AppendRadioItem (ID_F_M+1, wxT("Nelder-Mead &simplex"), 
                                  wxT("slow but simple and reliable method"));
    fit_method_menu->AppendRadioItem (ID_F_M+2, wxT("&Genetic Algorithm"), 
                                                wxT("almost AI"));
    fit_menu->Append (ID_F_METHOD, wxT("&Method"), fit_method_menu, wxT(""));
    fit_menu->AppendSeparator();
    fit_menu->Append (ID_F_RUN, wxT("&Run...\tCtrl-R"), 
                                             wxT("Fit sum to data"));
    fit_menu->Append (ID_F_INFO, wxT("&Info"), wxT("Info about current fit")); 
    fit_menu->AppendSeparator();
    fit_menu->Append (ID_F_UNDO, wxT("&Undo"), 
                            wxT("Undo change of parameter")); 
    fit_menu->Append (ID_F_REDO, wxT("R&edo"), 
                            wxT("Redo change of parameter")); 
    fit_menu->Append (ID_F_HISTORY, wxT("&Parameter History"), 
                            wxT("Go back or forward in parameter history")); 
    fit_menu->Append (ID_F_CLEARH, wxT("&Clear History"), 
                            wxT("Clear parameter history")); 

    wxMenu* gui_menu = new wxMenu;
    wxMenu* gui_menu_mode = new wxMenu;
    gui_menu_mode->AppendRadioItem (ID_G_M_ZOOM, wxT("&Normal\tCtrl-N"), 
                              wxT("Use mouse for zooming, moving peaks etc."));
    gui_menu_mode->AppendRadioItem (ID_G_M_RANGE, wxT("&Range\tCtrl-I"), 
                     wxT("Use mouse for activating and disactivating data"));
    gui_menu_mode->AppendRadioItem (ID_G_M_BG, wxT("&Baseline\tCtrl-B"), 
                                wxT("Use mouse for subtracting background"));
    gui_menu_mode->AppendRadioItem (ID_G_M_ADD, wxT("&Peak-Add\tCtrl-K"), 
                                    wxT("Use mouse for adding new peaks"));
    gui_menu_mode->AppendSeparator();
    wxMenu* baseline_menu = new wxMenu;
    baseline_menu->Append (ID_G_M_BG_STRIP, wxT("&Strip baseline"), 
                           wxT("Subtract selected baseline from data"));
    baseline_menu->Append (ID_G_M_BG_UNDO, wxT("&Undo strip baseline"), 
                           wxT("Subtract selected baseline from data"));
    baseline_menu->Append (ID_G_M_BG_CLEAR, wxT("&Clear/forget baseline"), 
                           wxT("Clear baseline points, disable undo"));
    baseline_menu->AppendSeparator();
    baseline_menu->AppendCheckItem(ID_G_M_BG_SPLINE, 
                                   wxT("&Spline interpolation"), 
                                   wxT("Cubic spline interpolation of points"));
    baseline_menu->Check(ID_G_M_BG_SPLINE, true);
    gui_menu_mode->Append(ID_G_M_BG_SUB, wxT("Baseline handling"), 
                          baseline_menu);
    gui_menu_mode->Enable(ID_G_M_BG_SUB, false);
    gui_menu->Append(ID_G_MODE, wxT("&Mode"), gui_menu_mode);
    gui_menu->AppendSeparator();

    wxMenu* gui_menu_show = new wxMenu;
    gui_menu_show->AppendCheckItem (ID_G_S_TOOLBAR, wxT("&Toolbar"), 
                                    wxT("Show/hide toolbar"));
    gui_menu_show->Check(ID_G_S_TOOLBAR, true);
    gui_menu_show->AppendCheckItem (ID_G_S_STATBAR, wxT("&Status Bar"), 
                                    wxT("Show/hide status bar"));
    gui_menu_show->Check(ID_G_S_STATBAR, true);
    gui_menu_show->AppendCheckItem (ID_G_S_SIDEB, wxT("&SideBar"), 
                                    wxT("Show/hide pane at right hand side"));  
    gui_menu_show->AppendCheckItem (ID_G_S_A1, wxT("&Auxiliary Plot 1"), 
                                    wxT("Show/hide auxiliary plot I"));  
    gui_menu_show->Check(ID_G_S_A1, true);
    gui_menu_show->AppendCheckItem (ID_G_S_A2, wxT("A&uxiliary Plot 2"), 
                                    wxT("Show/hide auxiliary plot II"));  
    gui_menu_show->Check(ID_G_S_A2, false);
    gui_menu_show->AppendCheckItem (ID_G_S_IO, wxT("&Input/Output Text Pane"), 
                                    wxT("Show/hide text input/output"));  
    gui_menu_show->Check(ID_G_S_IO, true);
    gui_menu->Append(ID_G_SHOW, wxT("S&how"), gui_menu_show);

    wxMenu* gui_menu_config = new wxMenu;
    gui_menu_config->Append(ID_G_C_MAIN, wxT("&Main Plot"), 
                            wxT("Show main plot pop-up menu"));
    gui_menu_config->Append(ID_G_C_A1, wxT("&Auxliliary Plot 1"), 
                            wxT("Show aux. plot 1 pop-up menu"));
    gui_menu_config->Append(ID_G_C_A2, wxT("A&uxliliary Plot 2"), 
                            wxT("Show aux. plot 2 pop-up menu"));
    gui_menu_config->Append(ID_G_C_OUTPUT, wxT("&Output Window"), 
                            wxT("Show output window pop-up menu"));
    gui_menu_config->Append(ID_G_C_SB, wxT("&Status Bar"), 
                            wxT("Configure Status Bar"));
    gui_menu->Append(-1, wxT("Confi&gure"), gui_menu_config);

    gui_menu->AppendCheckItem(ID_G_CROSSHAIR, wxT("&Crosshair Cursor"), 
                                              wxT("Show crosshair cursor"));
    gui_menu->AppendCheckItem(ID_G_FULLSCRN, wxT("&Full Screen\tF11"), 
                                              wxT("Switch full screen"));
    gui_menu->AppendSeparator();
    gui_menu->Append (ID_G_V_ALL, wxT("Zoom &All\tCtrl-A"), 
                                  wxT("View whole data"));
    gui_menu->Append (ID_G_V_VERT, wxT("Fit &vertically\tCtrl-V"), 
                      wxT("Adjust vertical zoom"));
    gui_menu->Append (ID_G_V_SCROLL_L, wxT("Scroll &Left\tCtrl-["), 
                      wxT("Scroll view left"));
    gui_menu->Append (ID_G_V_SCROLL_R, wxT("Scroll &Right\tCtrl-]"), 
                      wxT("Scroll view right"));
    gui_menu->Append (ID_G_V_SCROLL_U, wxT("Extend Zoom &Up\tCtrl--"), 
                      wxT("Double vertical range"));
    gui_menu->Append (ID_G_V_EXTH, wxT("Extend &Horizontally\tCtrl-;"), 
                      wxT("Extend zoom horizontally"));

    wxMenu* gui_menu_zoom_prev = new wxMenu;
    gui_menu->Append(ID_G_V_ZOOM_PREV, wxT("&Previous Zooms"), 
                     gui_menu_zoom_prev);
    gui_menu->AppendSeparator();
    gui_menu->Append(ID_G_LCONF1, wxT("Load &default config"), 
                                            wxT("Default configuration file"));
    gui_menu->Append(ID_G_LCONF2, wxT("Load &alt. config"), 
                                        wxT("Alternative configuration file"));
    wxMenu* gui_menu_lconfig = new wxMenu;
    gui_menu_lconfig->Append(ID_G_LCONFB, wxT("&built-in"), 
                                               wxT("Built-in configuration"));
    gui_menu_lconfig->AppendSeparator();
    gui_menu->Append(ID_G_LCONF, wxT("&Load config..."), gui_menu_lconfig);
    wxMenu* gui_menu_sconfig = new wxMenu;
    gui_menu_sconfig->Append(ID_G_SCONF1, wxT("as default"), 
                     wxT("Save current configuration to default config file"));
    gui_menu_sconfig->Append(ID_G_SCONF2, wxT("as alternative"),
                 wxT("Save current configuration to alternative config file"));
    gui_menu_sconfig->Append(ID_G_SCONFAS, wxT("as ..."),
                 wxT("Save current configuration to other config file"));
    gui_menu->Append(ID_G_SCONF, wxT("&Save current config"), gui_menu_sconfig);

    wxMenu *help_menu = new wxMenu;


    append_mi(help_menu, ID_H_MANUAL, GET_BMP(book16), wxT("&Manual\tF1"), 
              wxT("User's Manual"));
    help_menu->Append(ID_H_CONTACT, wxT("&Report bug (on-line)"), 
                      wxT("Feedback is always appreciated."));
    help_menu->Append(wxID_ABOUT, wxT("&About..."), wxT("Show about dialog"));

    wxMenuBar *menu_bar = new wxMenuBar();
    menu_bar->Append (session_menu, wxT("&Session") );
    menu_bar->Append (data_menu, wxT("&Data") );
    menu_bar->Append (sum_menu, wxT("&Functions") );
    menu_bar->Append (fit_menu, wxT("Fi&t") );
    menu_bar->Append (gui_menu, wxT("&GUI"));
    menu_bar->Append (help_menu, wxT("&Help"));

    SetMenuBar(menu_bar);
}


    //construct GUI->Previous Zooms menu
void FFrame::update_menu_previous_zooms()
{
    static vector<string> old_zoom_hist;
    const vector<string> &zoom_hist = plot_pane->get_zoom_hist();
    if (old_zoom_hist == zoom_hist) 
        return;
    wxMenu *menu = GetMenuBar()->FindItem(ID_G_V_ZOOM_PREV)->GetSubMenu(); 
    while (menu->GetMenuItemCount() > 0) //clear 
        menu->Delete(menu->GetMenuItems().GetLast()->GetData());
    int last = zoom_hist.size() - 1;
    for (int i = last, j = 1; i >= 0 && i > last - 10; i--, j++) 
        menu->Append(ID_G_V_ZOOM_PREV + j, s2wx(zoom_hist[i]));
    old_zoom_hist = zoom_hist;
}
           

void FFrame::OnShowHelp(wxCommandEvent&)
{
        help.DisplayContents();
}

bool FFrame::display_help_section(const string &s)
{
    return help.DisplaySection(s2wx(s));
}

void FFrame::OnAbout(wxCommandEvent&)
{
    AboutDlg* dlg = new AboutDlg(this);    
    dlg->ShowModal();
    dlg->Destroy();
}

void FFrame::OnContact(wxCommandEvent&)
{
    wxString url = wxT("http://www.unipress.waw.pl/fityk/contact.html");
    bool r = wxLaunchDefaultBrowser(url);
    if (!r)
        wxMessageBox(wxT("Read instructions at:\n") + url,
                     wxT("feedback"), wxOK|wxICON_INFORMATION);
}

void FFrame::OnDLoad (wxCommandEvent&)
{
    static wxString dir = wxConfig::Get()->Read(wxT("/loadDataDir"));
    wxFileDialog fdlg (this, wxT("Load data from a file"), dir, wxT(""),
                              wxT("all files (*)|*")
                              wxT("|x y files (*.dat, *.xy, *.fio)")
                                      wxT("|*.dat;*.DAT;*.xy;*.XY;*.fio;*.FIO") 
                              wxT("|rit files (*.rit)|*.rit;*.RIT")
                              wxT("|cpi files (*.cpi)|*.cpi;*.CPI")
                              wxT("|mca files (*.mca)|*.mca;*.MCA")
                              wxT("|Siemens/Bruker (*.raw)|*.raw;*.RAW"),
                              wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE);
    if (fdlg.ShowModal() == wxID_OK) {
        wxArrayString paths;
        fdlg.GetPaths(paths);
        int count = paths.GetCount();
        string cmd;
        for (int i = 0; i < count; ++i) {
            if (i == 0)
                cmd = get_active_data_str() + " <'" + wx2s(paths[i]) + "'";
            else
                cmd += " ; @+ <'" + wx2s(paths[i]) + "'"; 
            add_recent_data_file(wx2s(paths[i]));
        }
        ftk->exec(cmd);
        if (count > 1)
            SwitchSideBar(true);
    }
    dir = fdlg.GetDirectory();
}

void FFrame::OnDXLoad (wxCommandEvent&)
{
    int n = ftk->get_active_ds_position();
    DLoadDlg dload_dialog(this, -1, n, ftk->get_data(n));
    dload_dialog.ShowModal();
}

void FFrame::OnDRecent (wxCommandEvent& event)
{
    string s = wx2s(GetMenuBar()->GetHelpString(event.GetId()));
    ftk->exec(get_active_data_str() + " <'" + s + "'");
    add_recent_data_file(s);
}

void FFrame::OnDEditor (wxCommandEvent&)
{
    vector<pair<int,Data*> > dd;
    if (get_apply_to_all_ds()) {
        for (int i = 0; i < ftk->get_ds_count(); ++i) 
            dd.push_back(make_pair(i, ftk->get_data(i)));
    }
    else {
        int p = ftk->get_active_ds_position();
        dd.push_back(make_pair(p, ftk->get_data(p)));
    }
    DataEditorDlg data_editor(this, -1, dd);
    data_editor.ShowModal();
}

void FFrame::update_menu_fast_tranforms ()
{
    const vector<DataTransform> &all = DataEditorDlg::get_transforms();
    vector<DataTransform> transforms;
    for (vector<DataTransform>::const_iterator i = all.begin(); 
            i != all.end(); ++i)
        if (i->in_menu)
            transforms.push_back(*i);
    int menu_len = data_ft_menu->GetMenuItemCount();
    for (int i = 0; i < size(transforms); ++i) {
        int id = ID_D_FDT+i+1;
        wxString name = s2wx(transforms[i].name);
        if (i >= menu_len)
            data_ft_menu->Append(id, name);
        else if (data_ft_menu->GetLabel(id) == name)
            continue;
        else
            data_ft_menu->SetLabel(id, name);
    }
    for (int i = size(transforms); i < menu_len; ++i) 
        data_ft_menu->Delete(ID_D_FDT+i+1);
}

void FFrame::OnFastDT (wxCommandEvent& event)
{
    string name = wx2s(GetMenuBar()->GetLabel(event.GetId()));
    const vector<DataTransform> &transforms = DataEditorDlg::get_transforms();
    for (vector<DataTransform>::const_iterator i = transforms.begin();
            i != transforms.end(); ++i)
        if (i->name == name) {
            DataEditorDlg::execute_tranform(i->code);
            return;
        }
}

void FFrame::OnAllDatasetsUpdate (wxUpdateUIEvent& event)
{
    event.Enable(ftk->get_ds_count() > 1);
}

void FFrame::OnDMerge (wxCommandEvent&)
{
    MergePointsDlg *dlg = new MergePointsDlg(this);
    if (dlg->ShowModal() == wxID_OK)
        ftk->exec(dlg->get_command());
    dlg->Destroy();
}

void FFrame::OnDExport (wxCommandEvent&)
{
    export_data_dlg(this);
}

void FFrame::OnDefinitionMgr(wxCommandEvent&)
{
    DefinitionMgrDlg* dlg = new DefinitionMgrDlg(this);
    if (dlg->ShowModal() == wxID_OK)
        ftk->exec(dlg->get_command());
    dlg->Destroy();
}

void FFrame::OnSGuess (wxCommandEvent&)
{
    ftk->exec("guess " + frame->get_peak_type() + get_in_dataset());
}

void FFrame::OnSPFInfo (wxCommandEvent&)
{
    ftk->exec("info guess" + get_in_dataset());
    //TODO animations showing peak positions
}
        
void FFrame::OnSFuncList (wxCommandEvent&)
{
    SwitchSideBar(true);
    sidebar->set_selection(1);
}
         
void FFrame::OnSVarList (wxCommandEvent&)
{
    SwitchSideBar(true);
    sidebar->set_selection(2);
}
           
void FFrame::OnSExport (wxCommandEvent& event)
{
    static wxString dir = wxConfig::Get()->Read(wxT("/exportDir"));
    bool as_peaks = (event.GetId() == ID_S_EXPORTP);
    int pos = ftk->get_active_ds_position();
    string const& filename = ftk->get_data(pos)->get_filename();
    wxString name = wxFileName(s2wx(filename)).GetName()
                                + (as_peaks ? wxT(".peaks") : wxT(".formula"));
    wxFileDialog fdlg (this, wxT("Export curve to file"), dir, name,
                       (as_peaks 
                            ? wxT("parameters of functions (*.peaks)|*.peaks")
                            : wxT("mathematic formula (*.formula)|*.formula"))
                        + wxString(wxT("|all files|*")),
                       wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (fdlg.ShowModal() == wxID_OK) 
        ftk->exec(string("info ") + (as_peaks ? "peaks" : "formula")
                     + " in " +  get_active_data_str() + 
                     + " > '" + wx2s(fdlg.GetPath()) + "'");
    dir = fdlg.GetDirectory();
}
           
        
void FFrame::OnFMethodUpdate (wxUpdateUIEvent& event)
{
    FitMethodsContainer const* fmc = ftk->get_fit_container();
    int n = ftk->get_settings()->get_e("fitting-method");
    GetMenuBar()->Check (ID_F_M + n, true);
    // to make it simpler, history menu items are also updated here
    GetMenuBar()->Enable(ID_F_UNDO, fmc->can_undo());
    GetMenuBar()->Enable(ID_F_REDO, fmc->has_param_history_rel_item(1));
    GetMenuBar()->Enable(ID_F_HISTORY, fmc->get_param_history_size() != 0);
    GetMenuBar()->Enable(ID_F_CLEARH, fmc->get_param_history_size() != 0);
    event.Skip();
}

void FFrame::OnFOneOfMethods (wxCommandEvent& event)
{
    int m = event.GetId() - ID_F_M;
    ftk->exec("set fitting-method=" 
             + ftk->get_fit_container()->get_method(m)->name);
}
           
void FFrame::OnFRun (wxCommandEvent&)
{
    FitRunDlg(this, -1, true).ShowModal();
}
        
void FFrame::OnFInfo (wxCommandEvent&)
{
    ftk->exec("info fit");
}
         
void FFrame::OnFUndo (wxCommandEvent&)
{
    ftk->exec("fit undo");
}
         
void FFrame::OnFRedo (wxCommandEvent&)
{
    ftk->exec("fit redo");
}
         
void FFrame::OnFHistory (wxCommandEvent&)
{
    SumHistoryDlg *dialog = new SumHistoryDlg(this, -1);
    dialog->ShowModal();
    dialog->Destroy();
}
            
void FFrame::OnFClearH (wxCommandEvent&)
{
    ftk->exec("fit history clear");
}
         
         
void FFrame::OnLogUpdate (wxUpdateUIEvent& event)        
{
    string const& logfile = ftk->get_ui()->get_commands().get_log_file();
    if (logfile.empty()) {
        GetMenuBar()->Enable(ID_LOG_START, true);
        GetMenuBar()->Enable(ID_LOG_STOP, false);
    }
    else {
        GetMenuBar()->Enable(ID_LOG_START, false);
        GetMenuBar()->Enable(ID_LOG_STOP, true);
        GetMenuBar()->Check(ID_LOG_WITH_OUTPUT, 
                          ftk->get_ui()->get_commands().get_log_with_output());
    }
    event.Skip();
}

void FFrame::OnLogStart (wxCommandEvent&)
{
    static wxString dir = wxConfig::Get()->Read(wxT("/exportDir"));
    wxFileDialog fdlg (this, wxT("Log to file"), dir, wxT(""),
                       wxT("Fityk script file (*.fit)|*.fit;*.FIT")
                       wxT("|All files |*"),
                       wxFD_SAVE);
    string const& logfile = ftk->get_ui()->get_commands().get_log_file();
    if (!logfile.empty())
        fdlg.SetPath(s2wx(logfile));
    if (fdlg.ShowModal() == wxID_OK) {
        string plus = GetMenuBar()->IsChecked(ID_LOG_WITH_OUTPUT) ? "+" : "";
        ftk->exec("commands" + plus + " > '" + wx2s(fdlg.GetPath()) + "'");
    }
    dir = fdlg.GetDirectory();
}

void FFrame::OnLogStop (wxCommandEvent&)
{
    ftk->exec("commands > /dev/null");
}

void FFrame::OnLogWithOutput (wxCommandEvent& event)
{
    bool checked = event.IsChecked();
    GetMenuBar()->Check(ID_LOG_WITH_OUTPUT, checked);
    string const& logfile = ftk->get_ui()->get_commands().get_log_file();
    if (!logfile.empty())
        ftk->exec("commands+ > " + logfile);
}

void FFrame::OnLogDump (wxCommandEvent&)
{
    static wxString dir = wxConfig::Get()->Read(wxT("/exportDir"));
    wxFileDialog fdlg(this, wxT("Dump all commands executed so far to file"),
                      dir, wxT(""), wxT("fityk file (*.fit)|*.fit;*.FIT"),
                      wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (fdlg.ShowModal() == wxID_OK) {
        ftk->exec("commands[:] > '" + wx2s(fdlg.GetPath()) + "'");
    }
    dir = fdlg.GetDirectory();
}
         
void FFrame::OnReset (wxCommandEvent&)
{
    int r = wxMessageBox(wxT("Do you really want to reset current session \n")
                      wxT("and lose everything you have done in this session?"),
                         wxT("Are you sure?"), 
                         wxYES_NO | wxCANCEL | wxCENTRE | wxICON_QUESTION);
    if (r == wxYES)
        ftk->exec("reset");
}
        
void FFrame::OnInclude (wxCommandEvent&)
{
    static wxString dir = wxConfig::Get()->Read(wxT("/execScriptDir"));
    wxFileDialog fdlg (this, wxT("Execute commands from file"), dir, wxT(""),
                              wxT("fityk file (*.fit)|*.fit;*.FIT|all files|*"),
                              wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (fdlg.ShowModal() == wxID_OK) {
        ftk->exec("commands < '" + wx2s(fdlg.GetPath()) + "'");
        last_include_path = wx2s(fdlg.GetPath());
        GetMenuBar()->Enable(ID_O_REINCLUDE, true);
    }
    dir = fdlg.GetDirectory();
}
            
void FFrame::OnReInclude (wxCommandEvent&)
{
    ftk->exec("reset; commands < '" + last_include_path + "'");
}

void FFrame::show_debugger(wxString const& path)
{
    static ScriptDebugDlg *dlg = 0;
    if (!dlg) 
        dlg = new ScriptDebugDlg(this, -1);
    if (path.IsEmpty()) {
        if (dlg->get_path().IsEmpty())
            dlg->OpenFile(this);
    }
    else
        dlg->do_open_file(path);
    dlg->Show(true);
}
            
void FFrame::OnDump (wxCommandEvent&)
{
    static wxString dir = wxConfig::Get()->Read(wxT("/exportDir"));
    wxFileDialog fdlg(this, wxT("Dump current program state to file as script"),
                      dir, wxT(""), wxT("fityk file (*.fit)|*.fit;*.FIT"),
                      wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (fdlg.ShowModal() == wxID_OK) {
        ftk->exec("dump > '" + wx2s(fdlg.GetPath()) + "'");
        //ftk->exec("commands[:] > '" + wx2s(fdlg.GetPath()) + "'");
    }
    dir = fdlg.GetDirectory();
}
         
void FFrame::OnSettings (wxCommandEvent&)
{
    SettingsDlg *dialog = new SettingsDlg(this, -1);
    dialog->ShowModal();
    dialog->Destroy();
}

void FFrame::OnEditInit (wxCommandEvent&)
{
    wxString startup_file = get_user_conffile(startup_commands_filename);
    show_debugger(startup_file);
}

void FFrame::OnChangeMouseMode (wxCommandEvent& event)
{
    //const MainPlot* plot = plot_pane->get_plot();
    //if (plot->get_mouse_mode() == mmd_bg && plot->can_strip()) {
    //    int r = wxMessageBox(wxT("You have selected the baseline,"
    //                         wxT(" but have not\n"))
    //                         wxT("stripped it. Do you want to change data\n")
    //                         wxT("and strip selected baseline?\n") 
    //                         wxT("If you want to stay in background mode,\n")
    //                         wxT("press Cancel."),
    //                         wxT("Want to strip background?"), 
    //                         wxICON_QUESTION|wxYES_NO|wxCANCEL);
    //    if (r == wxYES)
    //        plot_pane->get_bg_manager()->strip_background();
    //    else if (r == wxNO)
    //        plot_pane->get_bg_manager()->clear_background();
    //    else { //wxCANCEL
    //        GetMenuBar()->Check(ID_G_M_BG, true);
    //        if (toolbar) 
    //            toolbar->ToggleTool(ID_ft_m_bg, true);
    //        return;
    //    }
    //}
    MouseModeEnum mode = mmd_zoom;
    switch (event.GetId()) {
        case ID_G_M_ZOOM:   
        case ID_ft_m_zoom:   
            mode = mmd_zoom;   
            GetMenuBar()->Check(ID_G_M_ZOOM, true);
            if (toolbar) 
                toolbar->ToggleTool(ID_ft_m_zoom, true);
            break;
        case ID_G_M_RANGE:   
        case ID_ft_m_range:  
            mode = mmd_range;  
            GetMenuBar()->Check(ID_G_M_RANGE, true);
            if (toolbar) 
                toolbar->ToggleTool(ID_ft_m_range, true);
            break;
        case ID_G_M_BG:   
        case ID_ft_m_bg:     
            mode = mmd_bg;     
            GetMenuBar()->Check(ID_G_M_BG, true);
            if (toolbar) 
                toolbar->ToggleTool(ID_ft_m_bg, true);
            break;
        case ID_G_M_ADD:   
        case ID_ft_m_add:    
            mode = mmd_add;    
            GetMenuBar()->Check(ID_G_M_ADD, true);
            if (toolbar) 
                toolbar->ToggleTool(ID_ft_m_add, true);
            break;  
        default: assert(0);
    }
    toolbar->EnableTool(ID_ft_b_strip, (mode == mmd_bg));
    GetMenuBar()->Enable(ID_G_M_BG_SUB, (mode == mmd_bg));
    plot_pane->set_mouse_mode(mode);
}

void FFrame::update_menu_functions()
{
    size_t cnt = this->func_type_menu->GetMenuItemCount();
    size_t pcnt = peak_types.size();
    for (size_t i = 0; i < min(pcnt, cnt); i++)
        if (func_type_menu->GetLabel(ID_G_M_PEAK_N+i) != s2wx(peak_types[i]))
            func_type_menu->SetLabel(ID_G_M_PEAK_N+i, s2wx(peak_types[i]));
    for (size_t i = cnt; i < pcnt; i++)
        func_type_menu->AppendRadioItem(ID_G_M_PEAK_N+i, s2wx(peak_types[i]));
    for (size_t i = pcnt; i < cnt; i++)
        func_type_menu->Destroy(ID_G_M_PEAK_N+i);

    func_type_menu->Check(ID_G_M_PEAK_N + peak_type_nr, true);
    func_type_menu->UpdateUI();
}

void FFrame::OnChangePeakType(wxCommandEvent& event)
{
    peak_type_nr = event.GetId() - ID_G_M_PEAK_N;
    if (toolbar) 
        toolbar->update_peak_type(peak_type_nr);
    update_autoadd_enabled();
}

void FFrame::OnGMBgUpdate(wxUpdateUIEvent& event)
{
    BgManager* bgm = plot_pane->get_bg_manager();
    GetMenuBar()->Enable(ID_G_M_BG_STRIP, bgm->can_strip());
    GetMenuBar()->Enable(ID_G_M_BG_UNDO, bgm->can_undo());
    GetMenuBar()->Enable(ID_G_M_BG_CLEAR, bgm->can_strip() || bgm->can_undo());
    event.Skip();
}

void FFrame::OnStripBg(wxCommandEvent&)
{
    plot_pane->get_bg_manager()->strip_background();
}

void FFrame::OnUndoBg(wxCommandEvent&)
{
    plot_pane->get_bg_manager()->undo_strip_background();
}

void FFrame::OnClearBg(wxCommandEvent&)
{
    plot_pane->get_bg_manager()->forget_background();
    refresh_plots(false, true);
}

void FFrame::OnSplineBg(wxCommandEvent& event)
{
    plot_pane->get_bg_manager()->set_spline_bg(event.IsChecked());
    refresh_plots(false, true);
}

void FFrame::SwitchToolbar(bool show)
{
    if (show && !GetToolBar()) {
        toolbar = new FToolBar(this, -1);
        SetToolBar(toolbar);
        update_toolbar();
    }
    else if (!show && GetToolBar()){
        SetToolBar(0);
        delete toolbar; 
        toolbar = 0;
    }
    GetMenuBar()->Check(ID_G_S_TOOLBAR, show);
}

void FFrame::SwitchStatbar (bool show)
{
    if (show && !GetStatusBar()) {
        status_bar = new FStatusBar(this);
        SetStatusBar(status_bar);
        plot_pane->update_mouse_hints();
    }
    else if (!show && GetStatusBar()) {
        SetStatusBar(0);
        delete status_bar; 
        status_bar = 0;
    }
    GetMenuBar()->Check(ID_G_S_STATBAR, show);
}

void FFrame::SwitchSideBar(bool show)
{
    //v_splitter->IsSplit() means sidebar is visible 
    if (show && !v_splitter->IsSplit()) {
        sidebar->Show(true);
        v_splitter->SplitVertically(main_pane, sidebar);
    }
    else if (!show && v_splitter->IsSplit()) {
        v_splitter->Unsplit();
    }
    GetMenuBar()->Check(ID_G_S_SIDEB, show);
    if (toolbar) 
        toolbar->ToggleTool(ID_ft_sideb, show);
}

void FFrame::OnSwitchAuxPlot(wxCommandEvent& ev) 
{
    plot_pane->show_aux(ev.GetId() - ID_G_S_A1, ev.IsChecked());
}

void FFrame::SwitchIOPane (bool show)
{
    //main_pane->IsSplit() means io_pane is visible 
    if (show && !main_pane->IsSplit()) {
        io_pane->Show(true);
        main_pane->SplitHorizontally(plot_pane, io_pane);
    }
    else if (!show && main_pane->IsSplit()) {
        main_pane->Unsplit();
    }
    GetMenuBar()->Check(ID_G_S_IO, show);
    //if (toolbar) toolbar->ToggleTool(ID_ft_..., show);
}

void FFrame::OnShowPopupMenu(wxCommandEvent& ev) 
{
    wxMouseEvent me(wxEVT_RIGHT_DOWN);
    me.m_x = wxGetMousePosition().x;
    me.m_y = 5;
    if (ev.GetId() == ID_G_C_MAIN)
        plot_pane->get_plot()->show_popup_menu(me);
    else if (ev.GetId() == ID_G_C_OUTPUT)
        io_pane->output_win->OnRightDown(me);
    else
        plot_pane->get_aux_plot(ev.GetId() - ID_G_C_A1)->OnRightDown(me);
}

void FFrame::OnConfigureStatusBar(wxCommandEvent&) 
{
    if (!status_bar)
        return;
    ConfStatBarDlg *dialog = new ConfStatBarDlg(this, -1, status_bar);
    dialog->ShowModal();
    dialog->Destroy();
}

void FFrame::SwitchCrosshair (bool show)
{
    plot_pane->crosshair_cursor = show;
    GetMenuBar()->Check(ID_G_CROSSHAIR, show);
}

void FFrame::OnSwitchFullScreen(wxCommandEvent& event)
{
    ShowFullScreen(event.IsChecked(), 
              wxFULLSCREEN_NOBORDER|wxFULLSCREEN_NOCAPTION);
}

void FFrame::OnGuiShowUpdate (wxUpdateUIEvent& event)
{
    for (int i = 0; i < 2; i++) 
        if (GetMenuBar()->IsChecked(ID_G_S_A1+i) != plot_pane->aux_visible(i))
            GetMenuBar()->Check(ID_G_S_A1+i, plot_pane->aux_visible(i));
    event.Skip();
}

void FFrame::GViewAll()
{
    change_zoom("[]");
}

void FFrame::OnGFitHeight (wxCommandEvent&)
{
    change_zoom(". []");
}

void FFrame::OnGScrollLeft (wxCommandEvent&)
{
    scroll_view_horizontally(-0.5);
}

void FFrame::OnGScrollRight (wxCommandEvent&)
{
    scroll_view_horizontally(+0.5);
}

void FFrame::OnGScrollUp (wxCommandEvent&)
{
    fp const factor = 2.;
    fp new_top = ftk->view.bottom + factor * ftk->view.height(); 
    change_zoom(". [.:" + S(new_top, 12) + "]");
}

void FFrame::OnGExtendH (wxCommandEvent&)
{
    fp const factor = 0.5;
    View const &vw = ftk->view;
    fp diff = vw.width() * factor;
    fp new_left = vw.left - diff; 
    fp new_right = vw.right + diff;
    change_zoom("[" + S(new_left) + " : " + S(new_right) + "] .");
}


void FFrame::OnPreviousZoom(wxCommandEvent& event)
{
    int id = event.GetId();
    string s = plot_pane->zoom_backward(id ? id - ID_G_V_ZOOM_PREV : 1);
    if (s.size()) 
        ftk->exec("plot " + s + " in " + get_active_data_str());
}

void FFrame::change_zoom(const string& s)
{
    plot_pane->zoom_forward();
    string cmd = "plot " + s + " in " + sidebar->get_datasets_str();
    ftk->exec(cmd);
}

void FFrame::scroll_view_horizontally(fp step)
{
    View const &vw = ftk->view;
    fp diff = vw.width() * step;
    if (plot_pane->get_plot()->get_x_reversed())
        diff = -diff;
    fp new_left = vw.left + diff; 
    fp new_right = vw.right + diff;
    change_zoom("[" + S(new_left) + " : " + S(new_right) + "] .");
}


void FFrame::OnConfigSave (wxCommandEvent& event)
{
    wxString name = (event.GetId() == ID_G_SCONF1 ?  wxGetApp().conf_filename 
                                               : wxGetApp().alt_conf_filename);
    save_config_as(name);
}

void FFrame::OnConfigSaveAs (wxCommandEvent&)
{
    wxString const& dir = wxGetApp().config_dir;
    wxString txt = wxGetTextFromUser(
                      wxT("Choose config name.\n") 
                        wxT("This will be the name of file in directory:\n") 
                        + dir,
                      wxT("config name"), 
                      wxT("other"));
    if (!txt.IsEmpty())
        save_config_as(dir+txt);
}

void FFrame::save_config_as(wxString const& name)
{
    wxFileConfig *config = new wxFileConfig(wxT(""), wxT(""), name, wxT(""), 
                                            wxCONFIG_USE_LOCAL_FILE);
    save_all_settings(config);
    delete config;
}

void FFrame::OnConfigRead (wxCommandEvent& event)
{
    wxString name = (event.GetId() == ID_G_LCONF1 ? wxGetApp().conf_filename 
                                               : wxGetApp().alt_conf_filename);
    read_config(name);
}

void FFrame::read_config(wxString const& name)
{
    wxFileConfig *config = new wxFileConfig(wxT(""), wxT(""), name, wxT(""), 
                                            wxCONFIG_USE_LOCAL_FILE);
    read_all_settings(config);
    delete config;
}

void FFrame::OnConfigBuiltin (wxCommandEvent&)
{
    // using fake config file, that does not exists, will get us default values
    // the file is not created when we only do reading
    wxString name = wxT("fake_d6DyY30KeMn9a3EyoM");
    wxConfig *config = new wxConfig(wxT(""), wxT(""), name, wxT(""), 
                                        wxCONFIG_USE_LOCAL_FILE);
    // just in case -- if the config was found, delete everything
    if (config->GetNumberOfEntries(true))
        config->DeleteAll();
    read_all_settings(config);
    delete config;
}

void FFrame::OnUpdateLConfMenu (wxUpdateUIEvent& event)
{
    // delete old menu items
    wxMenu *menu = GetMenuBar()->FindItem(ID_G_LCONF)->GetSubMenu(); 
    while (menu->GetMenuItemCount() > 2) //clear 
        menu->Delete(menu->GetMenuItems().GetLast()->GetData());

    // prepare listing config directory
    wxDir dir(wxGetApp().config_dir);
    if (!dir.IsOpened())
        return;

    // add new menu items
    wxString filename;     
    int j = 0;
    bool cont = dir.GetFirst(&filename, wxT(""), wxDIR_FILES|wxDIR_HIDDEN);
    while (cont && j < 15) { 
        menu->Append(ID_G_LCONF_X + j, filename);
        ++j;
        cont = dir.GetNext(&filename); 
    }

    event.Skip();
}


void FFrame::OnConfigX (wxCommandEvent& event)
{
    wxMenu *menu = GetMenuBar()->FindItem(ID_G_LCONF)->GetSubMenu(); 
    wxString name = menu->GetLabel(event.GetId());
    if (!name.IsEmpty())
        read_config(wxGetApp().config_dir + name);
}


void FFrame::OnPrintPreview(wxCommandEvent&)
{
    print_mgr->printPreview();
}

void FFrame::OnPageSetup(wxCommandEvent&)
{
    print_mgr->pageSetup();
}

void FFrame::OnPrint(wxCommandEvent&)
{
    print_mgr->print();
}

void FFrame::OnPrintPSFile(wxCommandEvent&)
{
    print_mgr->print_to_psfile();
}

void FFrame::OnPrintToClipboard(wxCommandEvent&)
{
#if wxUSE_METAFILE
    wxMetafileDC dc;   
    if (dc.Ok()) { 
        do_print_plots(&dc, print_mgr);
        wxMetafile *mf = dc.Close(); 
        if (mf) { 
            mf->SetClipboard(dc.MaxX() + 10, dc.MaxY() + 10); 
            delete mf; 
        } 
    } 
#endif
}

string FFrame::get_peak_type() const
{
    return Function::get_all_types()[peak_type_nr];
}

void FFrame::set_status_hint(string const& left, string const& right)
{
    if (status_bar)
        status_bar->set_hint(left, right);  
}

void FFrame::set_status_coord_info(fp x, fp y, bool aux)
{
    if (status_bar)
        status_bar->set_coord_info(x, y, aux);  
}

void FFrame::output_text(OutputStyle style, const string& str)
{
    io_pane->output_win->append_text(style, s2wx(str));
}

void FFrame::refresh_plots(bool now, bool only_main)
{
    plot_pane->refresh_plots(now, only_main);
}

void FFrame::draw_crosshair(int X, int Y)
{
    plot_pane->draw_crosshair(X, Y);
}

void FFrame::focus_input(wxKeyEvent& event)
{
    if (should_focus_input(event)) 
        io_pane->input_field->RedirectKeyPress(event);
    else
        event.Skip();
} 

void FFrame::edit_in_input(string const& s)
{
    io_pane->edit_in_input(s);
}

/// here we update all GUI buttons, lists etc. that can be changed
/// after execCommand() and can't be updated in another way
void FFrame::after_cmd_updates()
{
    sidebar->update_lists(false);
    update_peak_type_list();
    update_menu_functions();
    update_menu_previous_zooms();
    update_toolbar();
}

void FFrame::update_toolbar()
{
    update_autoadd_enabled();
    if (!toolbar) 
        return;
    toolbar->ToggleTool(ID_ft_b_strip, plot_pane->get_bg_manager()->can_undo());
    //DataWithSum const* ds = ftk->get_ds(ftk->get_active_ds_position());
    toolbar->EnableTool(ID_ft_f_cont, ftk->get_fit()->is_initialized());
    toolbar->EnableTool(ID_ft_v_pr, !plot_pane->get_zoom_hist().empty());
}

void FFrame::update_autoadd_enabled()
{
    static int old_nr = -1; //this is for optimization, if nr is the same, ...
    bool diff = (peak_type_nr != old_nr); //... default values are not parsed
    bool enable = ftk->get_data(ftk->get_active_ds_position())->get_n() > 2
          && is_function_guessable(Function::get_formula(peak_type_nr), diff);
    GetMenuBar()->Enable(ID_S_GUESS, enable);
    if (toolbar)
        toolbar->EnableTool(ID_ft_s_aa, enable);
}

string FFrame::get_active_data_str()
{
    return "@" + S(ftk->get_active_ds_position());
}

string FFrame::get_in_dataset()
{
    return ftk->get_ds_count() > 1 ? " in " + get_active_data_str() : string();
}

string FFrame::get_in_one_or_all_datasets()
{
    if (ftk->get_ds_count() > 1) {
        if (get_apply_to_all_ds())
            return " in @*";
        else
            return " in " + get_active_data_str();
    }
    else
        return "";
}

MainPlot* FFrame::get_main_plot() 
{ 
    return plot_pane->get_plot(); 
}

MainPlot const* FFrame::get_main_plot() const
{ 
    return plot_pane->get_plot(); 
}

void FFrame::update_data_pane()
{
    sidebar->update_lists();
}

bool FFrame::get_apply_to_all_ds()
{ 
    return ftk->get_ds_count() > 1 && GetMenuBar()->IsChecked(ID_D_ALLDS); 
}

void FFrame::activate_function(int n)
{
    sidebar->activate_function(n);
}

void FFrame::update_app_title()
{
    string title = "fityk";
    int pos = ftk->get_active_ds_position();
    string const& filename = ftk->get_data(pos)->get_filename();
    if (!filename.empty())
        title += " - " + filename;
    SetTitle(s2wx(title));
}

//===============================================================
//                    FToolBar 
//===============================================================

BEGIN_EVENT_TABLE (FToolBar, wxToolBar)
    EVT_TOOL_RANGE (ID_ft_m_zoom, ID_ft_m_add, FToolBar::OnChangeMouseMode)
    EVT_TOOL_RANGE (ID_ft_v_pr, ID_ft_s_aa, FToolBar::OnClickTool)
    EVT_TOOL (ID_ft_sideb, FToolBar::OnSwitchSideBar)
    EVT_CHOICE (ID_ft_peakchoice, FToolBar::OnPeakChoice)
END_EVENT_TABLE()

FToolBar::FToolBar (wxFrame *parent, wxWindowID id)
        : wxToolBar (parent, id, wxDefaultPosition, wxDefaultSize,
                     wxNO_BORDER | /*wxTB_FLAT |*/ wxTB_DOCKABLE) 
{
    SetToolBitmapSize(wxSize(24, 24));
    // mode
    MouseModeEnum m = frame ? frame->plot_pane->get_plot()->get_mouse_mode() 
                              : mmd_zoom;
    AddRadioTool(ID_ft_m_zoom, wxT("Zoom"), 
                 wxBitmap(zoom_mode_xpm), wxNullBitmap, wxT("Normal Mode"), 
                 wxT("Use mouse for zooming, moving peaks etc.")); 
    ToggleTool(ID_ft_m_zoom, m == mmd_zoom);
    AddRadioTool(ID_ft_m_range, wxT("Range"), 
                 wxBitmap(active_mode_xpm), wxNullBitmap, 
                 wxT("Data-Range Mode"), 
                 wxT("Use mouse for activating and disactivating data (try also with [Shift])")); 
    ToggleTool(ID_ft_m_range, m == mmd_range);
    AddRadioTool(ID_ft_m_bg, wxT("Background"), 
                 wxBitmap(bg_mode_xpm), wxNullBitmap, 
                 wxT("Baseline Mode"), 
                 wxT("Use mouse for subtracting background")); 
    ToggleTool(ID_ft_m_bg, m == mmd_bg);
    AddRadioTool(ID_ft_m_add, wxT("Add peak"), 
                 wxBitmap(addpeak_mode_xpm), wxNullBitmap,
                 wxT("Add-Peak Mode"), wxT("Use mouse for adding new peaks")); 
    ToggleTool(ID_ft_m_add, m == mmd_add);
    AddSeparator();
    // view
    AddTool (ID_G_V_ALL, wxT("Whole"), wxBitmap(zoom_all_xpm), wxNullBitmap, 
             wxITEM_NORMAL, wxT("View whole"), 
             wxT("Fit data in window"));
    AddTool(ID_G_V_VERT, wxT("Fit height"), 
            wxBitmap(zoom_vert_xpm), wxNullBitmap, wxITEM_NORMAL, 
            wxT("Fit vertically"), wxT("Set optimal y scale")); 
    AddTool(ID_G_V_SCROLL_L, wxT("<-- scroll"),
            wxBitmap(zoom_left_xpm), wxNullBitmap, wxITEM_NORMAL, 
            wxT("Scroll left"), wxT("Scroll view left")); 
    AddTool(ID_G_V_SCROLL_R, wxT("scroll -->"),
            wxBitmap(zoom_right_xpm), wxNullBitmap, wxITEM_NORMAL, 
            wxT("Scroll right"), wxT("Scroll view right")); 
    AddTool(ID_G_V_SCROLL_U, wxT("V-zoom-out"), 
            wxBitmap(zoom_up_xpm), wxNullBitmap, wxITEM_NORMAL, 
            wxT("Extend zoom up"), wxT("Double vertical range")); 
    AddTool(ID_ft_v_pr, wxT("Back"), wxBitmap(zoom_prev_xpm), wxNullBitmap, 
            wxITEM_NORMAL, wxT("Previous view"), 
            wxT("Go to previous View"));
    AddSeparator();
    //file
    AddTool(ID_O_INCLUDE, wxT("Execute"), 
            wxBitmap(run_script_xpm), wxNullBitmap, wxITEM_NORMAL, 
            wxT("Execute script"), wxT("Execute (include) script from file"));
    AddTool(ID_O_DUMP, wxT("Dump"), wxBitmap(save_script_xpm), wxNullBitmap,  
            wxITEM_NORMAL, wxT("Dump session to file"),
            wxT("Dump current session to file"));
    AddSeparator();
    //data
    AddTool(ID_D_LOAD, wxT("Load"), wxBitmap(open_data_xpm), wxNullBitmap,  
            wxITEM_NORMAL, wxT("Load file"),
            wxT("Load data from file"));
    AddTool(ID_D_XLOAD, wxT("Load2"), 
            wxBitmap(open_data_custom_xpm), wxNullBitmap, wxITEM_NORMAL, 
            wxT("Load file (custom)"), wxT("Load data from file"));
    AddTool(ID_D_EDITOR, wxT("Edit data"), 
            wxBitmap(edit_data_xpm), wxNullBitmap, wxITEM_NORMAL, 
            wxT("View and transform current dataset"),
            wxT("Save data to file"));
    AddTool(ID_D_EXPORT, wxT("Save"), wxBitmap(save_data_xpm), wxNullBitmap,  
            wxITEM_NORMAL, wxT("Save data as..."),
            wxT("Save data to file"));
    AddSeparator();
    //background
    AddTool(ID_ft_b_strip, wxT("Strip Bg"), 
            wxBitmap(strip_bg_xpm), wxNullBitmap,  wxITEM_CHECK, 
            wxT("Strip background"),
            wxT("(Un)Remove selected background from data"));
    EnableTool(ID_ft_b_strip, (m == mmd_bg));
    AddSeparator();
    peak_choice = new wxChoice(this, ID_ft_peakchoice); 
    AddControl (peak_choice);
    AddTool (ID_ft_s_aa, wxT("add"), wxBitmap(add_peak_xpm), wxNullBitmap, 
             wxITEM_NORMAL, wxT("auto-add"), wxT("Add peak automatically"));
    AddSeparator();
    //fit
    AddTool(ID_ft_f_run, wxT("Run"), 
            wxBitmap(run_fit_xpm), wxNullBitmap, wxITEM_NORMAL, 
            wxT("Start fitting"), wxT("Start fitting sum to data"));
    AddTool(ID_ft_f_cont, wxT("Continue"), wxBitmap(cont_fit_xpm), wxNullBitmap,
            wxITEM_NORMAL, wxT("Continue fitting"), 
            wxT("Continue fitting sum to data"));
    //AddTool (ID_ft_f_undo, "Undo", wxBitmap(undo_fit_xpm), wxNullBitmap,
    //         wxITEM_NORMAL, "Undo fitting", "Previous set of parameters");
    AddSeparator();
    //help
    AddTool(ID_H_MANUAL, wxT("Help"), wxBitmap(manual_xpm), wxNullBitmap,
            wxITEM_NORMAL, wxT("Manual"), wxT("Open user manual"));
    AddSeparator();
    AddTool(ID_ft_sideb, wxT("Datasets"), 
            wxBitmap(right_pane_xpm), wxNullBitmap, wxITEM_CHECK, 
            wxT("Datasets Pane"), wxT("Show/hide datasets pane"));
    Realize();
}

void FToolBar::OnPeakChoice(wxCommandEvent &event) 
{
    if (frame) 
        frame->peak_type_nr = event.GetSelection();
    frame->update_autoadd_enabled();
}

void FToolBar::update_peak_type(int nr, vector<string> const* peak_types) 
{ 
    if (peak_types) {
        if (peak_types->size() != (size_t) peak_choice->GetCount()) {
            peak_choice->Clear();
            for (size_t i = 0; i < peak_types->size(); ++i)
                peak_choice->Append(s2wx((*peak_types)[i]));
        }
        else
            for (size_t i = 0; i < peak_types->size(); ++i)
                if (peak_choice->GetString(i) != s2wx((*peak_types)[i]))
                    peak_choice->SetString(i, s2wx((*peak_types)[i]));
    }
    peak_choice->SetSelection(nr); 
}

//FIXME move these small functions to header file?
void FToolBar::OnChangeMouseMode (wxCommandEvent& event)
{
    frame->OnChangeMouseMode(event);
}

void FToolBar::OnSwitchSideBar (wxCommandEvent& event)
{
    frame->OnSwitchSideBar(event);
}

void FToolBar::OnClickTool (wxCommandEvent& event)
{
    switch (event.GetId()) {
        case ID_ft_v_pr:  
            frame->OnPreviousZoom(dummy_cmd_event);
            break;
        case ID_ft_b_strip: 
            if (event.IsChecked())
                frame->plot_pane->get_bg_manager()->strip_background();
            else
                frame->plot_pane->get_bg_manager()->undo_strip_background();
            break; 
        case ID_ft_f_run : 
            ftk->exec("fit" + frame->get_in_dataset()); 
            break; 
        case ID_ft_f_cont: 
            ftk->exec("fit+"); 
            break; 
        case ID_ft_f_undo: 
            ftk->exec("fit undo"); 
            break; 
        case ID_ft_s_aa: 
            ftk->exec("guess " + frame->get_peak_type() 
                         + frame->get_in_dataset());
            break; 
        default: 
            assert(0);
    }
}



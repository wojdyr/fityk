// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/splitter.h>
#include <wx/filedlg.h>
#include <wx/textdlg.h>
#include <wx/numdlg.h>
#include <wx/utils.h>
#include <wx/statline.h>
#include <wx/printdlg.h>
#include <wx/image.h>
#include <wx/config.h>
#include <wx/fileconf.h>
#include <wx/msgout.h>
#include <wx/metafile.h>
#include <wx/dir.h>
#include <wx/stdpaths.h>

#include <algorithm>
#include <string.h>
#include <ctype.h>

#include <xylib/xylib.h>
#include <xylib/cache.h>

#include "frame.h"
#include "plot.h"
#include "mplot.h"
#include "aplot.h"
#include "dialogs.h"
#include "exportd.h"
#include "history.h"
#include "about.h"
#include "dload.h"
#include "pane.h"
#include "pplot.h"
#include "sidebar.h"
#include "print.h"
#include "datatable.h"
#include "dataedit.h"
#include "defmgr.h"
#include "sdebug.h"
#include "setdlg.h"
#include "statbar.h"
#include "fitinfo.h"
#include "inputline.h"
#include "app.h"
#include "powdifpat.h"
#include "../common.h"
#include "../logic.h"
#include "../fit.h"
#include "../data.h"
#include "../settings.h"
#include "../guess.h"
#include "../func.h"
#include "../tplate.h"

#include "img/fityk.xpm"
//toolbars icons
#include "img/active_mode.xpm"
#include "img/addpeak_mode.xpm"
#include "img/add_peak.xpm"
#include "img/bg_mode.xpm"
#include "img/edit_data.xpm"
//#include "img/manual.xpm"
#include "img/open_data_custom.xpm"
#include "img/open_data.xpm"
//#include "img/right_pane.xpm"
#include "img/run_fit.xpm"
#include "img/run_script.xpm"
#include "img/save_data.xpm"
#include "img/save_script.xpm"
#include "img/strip_bg.xpm"
#include "img/undo_fit.xpm"
#include "img/zoom_all.xpm"
#include "img/zoom_left.xpm"
#include "img/zoom_mode.xpm"
#include "img/zoom_prev.xpm"
#include "img/zoom_right.xpm"
#include "img/zoom_up.xpm"
#include "img/zoom_vert.xpm"

#include "img/book16.h"
#include "img/editor16.h"
#include "img/export16.h"
#include "img/fileopen16.h"
#include "img/filereload16.h"
#include "img/filesaveas16.h"
#include "img/function16.h"
#include "img/image16.h"
#include "img/preferences16.h"
#include "img/recordmacro16.h"
#include "img/redo16.h"
#include "img/reload16.h"
#include "img/run16.h"
#include "img/info16.h"
#include "img/runmacro16.h"
#include "img/stopmacro16.h"
#include "img/undo16.h"
#include "img/revert16.h"
#include "img/web16.h"
#include "img/zoom-fit16.h"
#include "img/powdifpat16.xpm"

using namespace std;
FFrame *frame = NULL;
Ftk *ftk = NULL;

static const wxString discussions_url(
        wxT("http://groups.google.com/group/fityk-users/topics"));
static const wxString website_url(
        wxT("http://fityk.nieto.pl/"));
static const wxString wiki_url(
        wxT("https://github.com/wojdyr/fityk/wiki"));
static const wxString feedback_url(
        wxT("http://fityk.nieto.pl/feedback/") + pchar2wx(VERSION));

enum {
    // menu
    ID_H_MANUAL        = 24001 ,
    ID_H_WEBSITE               ,
    ID_H_WIKI                  ,
    ID_H_DISCUSSIONS           ,
    ID_H_FEEDBACK              ,
    ID_D_QLOAD                 ,
    ID_D_XLOAD                 ,
    ID_D_RECENT                , //and next ones
    ID_D_RECENT_END = ID_D_RECENT+50 ,
    ID_D_REVERT                ,
    ID_D_TABLE                 ,
    ID_D_EDITOR                ,
    ID_D_SDT                   ,
    ID_D_SDT_END = ID_D_SDT+50 ,
    ID_D_MERGE                 ,
    ID_D_RM_SHIRLEY            ,
    ID_D_CALC_SHIRLEY          ,
    ID_D_EXPORT                ,
    ID_DEFMGR                  ,
    ID_S_GUESS                 ,
    ID_S_PFINFO                ,
    ID_S_AUTOFREEZE            ,
    ID_S_EXPORTP               ,
    ID_S_EXPORTF               ,
    ID_S_EXPORTD               ,
    ID_F_METHOD                ,
    ID_F_RUN                   ,
    ID_F_INFO                  ,
    ID_F_UNDO                  ,
    ID_F_REDO                  ,
    ID_F_HISTORY               ,
    ID_T_PD                    ,
    ID_F_M                     ,
    ID_F_M_END = ID_F_M+10     ,
    ID_SESSION_LOG             ,
    ID_LOG_START               ,
    ID_LOG_STOP                ,
    ID_LOG_WITH_OUTPUT         ,
    ID_SAVE_HISTORY            ,
    ID_SESSION_RESET           ,
    ID_PAGE_SETUP              ,
    ID_PRINT_PSFILE            ,
    ID_PRINT_CLIPB             ,
    ID_SAVE_IMAGE              ,
    ID_SESSION_INCLUDE         ,
    ID_SESSION_REINCLUDE       ,
    ID_SCRIPT_EDIT             ,
    ID_SESSION_DUMP            ,
    ID_SESSION_SET             ,
    ID_SESSION_EI              ,
    ID_G_MODE                  ,
    ID_G_M_ZOOM                ,
    ID_G_M_RANGE               ,
    ID_G_M_BG                    ,
    ID_G_M_ADD                 ,
    ID_G_BG_STRIP              ,
    ID_G_BG_UNDO               ,
    ID_G_BG_CLEAR              ,
    ID_G_BG_SPLINE             ,
    ID_G_BG_RECENT             ,
    ID_G_BG_RECENT_END=ID_G_BG_RECENT+50,
    ID_G_BG_HULL               ,
    ID_G_BG_SUB                ,
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
    ID_G_SHOWY0                ,
    ID_G_V_SCROLL_L            ,
    ID_G_V_SCROLL_R            ,
    ID_G_V_SCROLL_U            ,
    ID_G_V_EXTH                ,
    ID_G_V_ZOOM_PREV           ,
    ID_G_V_ZOOM_PREV_END = ID_G_V_ZOOM_PREV+50 ,
    ID_G_LCONF1                ,
    ID_G_LCONF2                ,
    ID_G_LCONF                 ,
    ID_G_LCONFB                ,
    ID_G_LCONF_X               ,
    ID_G_LCONF_X_END = ID_G_LCONF_X+50,
    ID_G_SCONF                 ,
    ID_G_SCONF1                ,
    ID_G_SCONF2                ,
    ID_G_SCONFAS               ,

    // toolbar
    ID_T_ZOOM                 ,
    ID_T_RANGE                ,
    ID_T_BG                   ,
    ID_T_ADD                  ,
    ID_T_PZ                   ,
    ID_T_STRIP                ,
    ID_T_RUN                  ,
    ID_T_UNDO                 ,
    ID_T_AUTO                 ,
    //ID_T_BAR                  ,
    ID_T_CHOICE
};


void append_mi(wxMenu* menu, int id, wxBitmap const& bitmap,
               const wxString& text=wxT(""), const wxString& helpString=wxT(""))
{
    wxMenuItem *item = new wxMenuItem(menu, id, text, helpString);
//#if wxUSE_OWNER_DRAWN || defined(__WXGTK__)
#if defined(__WXGTK__)
    item->SetBitmap(bitmap);
#endif
    menu->Append(item);
}


BEGIN_EVENT_TABLE(FFrame, wxFrame)
    EVT_UPDATE_UI (ID_LOG_START, FFrame::OnMenuLogStartUpdate)
    EVT_MENU (ID_LOG_START,     FFrame::OnLogStart)
    EVT_UPDATE_UI (ID_LOG_STOP, FFrame::OnMenuLogStopUpdate)
    EVT_MENU (ID_LOG_STOP,      FFrame::OnLogStop)
    EVT_UPDATE_UI (ID_LOG_WITH_OUTPUT, FFrame::OnMenuLogOutputUpdate)
    EVT_MENU (ID_LOG_WITH_OUTPUT, FFrame::OnLogWithOutput)
    EVT_MENU (ID_SAVE_HISTORY,  FFrame::OnSaveHistory)
    EVT_MENU (ID_SESSION_RESET, FFrame::OnReset)
    EVT_MENU (ID_SESSION_INCLUDE, FFrame::OnInclude)
    EVT_MENU (ID_SESSION_REINCLUDE, FFrame::OnReInclude)
    EVT_MENU (ID_SCRIPT_EDIT,   FFrame::OnShowEditor)
    EVT_MENU (wxID_PRINT,       FFrame::OnPrint)
    EVT_MENU (ID_PRINT_PSFILE,  FFrame::OnPrintPSFile)
    EVT_MENU (ID_PRINT_CLIPB,   FFrame::OnPrintToClipboard)
    EVT_MENU (ID_PAGE_SETUP,    FFrame::OnPageSetup)
    EVT_MENU (wxID_PREVIEW,     FFrame::OnPrintPreview)
    EVT_MENU (ID_SAVE_IMAGE,    FFrame::OnSaveAsImage)
    EVT_MENU (ID_SESSION_DUMP,  FFrame::OnDump)
    EVT_MENU (ID_SESSION_SET,   FFrame::OnSettings)
    EVT_MENU (ID_SESSION_EI,    FFrame::OnEditInit)

    EVT_MENU (ID_D_QLOAD,       FFrame::OnDataQLoad)
    EVT_MENU (ID_D_XLOAD,       FFrame::OnDataXLoad)
    EVT_MENU_RANGE (ID_D_RECENT+1, ID_D_RECENT_END, FFrame::OnDataRecent)
    EVT_UPDATE_UI (ID_D_REVERT, FFrame::OnDataRevertUpdate)
    EVT_MENU (ID_D_REVERT,      FFrame::OnDataRevert)
    EVT_MENU (ID_D_TABLE,       FFrame::OnDataTable)
    EVT_MENU (ID_D_EDITOR,      FFrame::OnDataEditor)
    EVT_MENU_RANGE (ID_D_SDT+1, ID_D_SDT_END, FFrame::OnSavedDT)
    EVT_MENU (ID_D_MERGE,       FFrame::OnDataMerge)
    EVT_MENU (ID_D_RM_SHIRLEY,  FFrame::OnDataRmShirley)
    EVT_MENU (ID_D_CALC_SHIRLEY,FFrame::OnDataCalcShirley)
    EVT_UPDATE_UI (ID_D_EXPORT, FFrame::OnDataExportUpdate)
    EVT_MENU (ID_D_EXPORT,      FFrame::OnDataExport)

    EVT_MENU (ID_DEFMGR,        FFrame::OnDefinitionMgr)
    EVT_MENU (ID_S_GUESS,       FFrame::OnSGuess)
    EVT_MENU (ID_S_PFINFO,      FFrame::OnSPFInfo)
    EVT_MENU (ID_S_AUTOFREEZE,  FFrame::OnAutoFreeze)
    EVT_MENU (ID_S_EXPORTP,     FFrame::OnSExport)
    EVT_MENU (ID_S_EXPORTF,     FFrame::OnSExport)
    EVT_MENU (ID_S_EXPORTD,     FFrame::OnDataExport)

    EVT_UPDATE_UI_RANGE (ID_F_M, ID_F_M_END, FFrame::OnFMethodUpdate)
    EVT_MENU_RANGE (ID_F_M, ID_F_M_END, FFrame::OnFOneOfMethods)
    EVT_UPDATE_UI (ID_F_RUN,    FFrame::OnMenuFitRunUpdate)
    EVT_MENU (ID_F_RUN,         FFrame::OnFRun)
    EVT_MENU (ID_F_INFO,        FFrame::OnFInfo)
    EVT_UPDATE_UI (ID_F_UNDO, FFrame::OnMenuFitUndoUpdate)
    EVT_MENU (ID_F_UNDO,        FFrame::OnFUndo)
    EVT_UPDATE_UI (ID_F_REDO, FFrame::OnMenuFitRedoUpdate)
    EVT_MENU (ID_F_REDO,        FFrame::OnFRedo)
    EVT_UPDATE_UI (ID_F_HISTORY, FFrame::OnMenuFitHistoryUpdate)
    EVT_MENU (ID_F_HISTORY,     FFrame::OnFHistory)

    EVT_MENU (ID_T_PD,          FFrame::OnPowderDiffraction)

    EVT_MENU (ID_G_M_ZOOM,      FFrame::OnChangeMouseMode)
    EVT_MENU (ID_G_M_RANGE,     FFrame::OnChangeMouseMode)
    EVT_MENU (ID_G_M_BG,        FFrame::OnChangeMouseMode)
    EVT_MENU (ID_G_M_ADD,       FFrame::OnChangeMouseMode)
    EVT_MENU_RANGE (ID_G_M_PEAK_N, ID_G_M_PEAK_N_END, FFrame::OnChangePeakType)
    EVT_UPDATE_UI(ID_G_BG_STRIP, FFrame::OnMenuBgStripUpdate)
    EVT_UPDATE_UI(ID_G_BG_UNDO, FFrame::OnMenuBgUndoUpdate)
    EVT_UPDATE_UI(ID_G_BG_CLEAR, FFrame::OnMenuBgClearUpdate)
    EVT_MENU (ID_G_BG_STRIP,    FFrame::OnStripBg)
    EVT_MENU (ID_G_BG_UNDO,     FFrame::OnUndoBg)
    EVT_MENU (ID_G_BG_CLEAR,    FFrame::OnClearBg)
    EVT_MENU_RANGE (ID_G_BG_RECENT+1, ID_G_BG_RECENT_END, FFrame::OnRecentBg)
    EVT_MENU (ID_G_BG_HULL,     FFrame::OnConvexHullBg)
    EVT_MENU (ID_G_BG_SPLINE,   FFrame::OnSplineBg)
    EVT_MENU (ID_G_S_SIDEB,     FFrame::OnSwitchSideBar)
    EVT_MENU_RANGE (ID_G_S_A1, ID_G_S_A2, FFrame::OnSwitchAuxPlot)
    EVT_UPDATE_UI_RANGE (ID_G_S_A1, ID_G_S_A2,  FFrame::OnMenuShowAuxUpdate)
    EVT_MENU (ID_G_S_IO,        FFrame::OnSwitchIOPane)
    EVT_MENU (ID_G_S_TOOLBAR,   FFrame::OnSwitchToolbar)
    EVT_MENU (ID_G_S_STATBAR,   FFrame::OnSwitchStatbar)
    EVT_MENU_RANGE (ID_G_C_MAIN, ID_G_C_A2, FFrame::OnShowPrefDialog)
    EVT_MENU (ID_G_C_OUTPUT,    FFrame::OnConfigureOutputWin)
    EVT_MENU (ID_G_C_SB,        FFrame::OnConfigureStatusBar)
    EVT_MENU (ID_G_CROSSHAIR,   FFrame::OnSwitchCrosshair)
    EVT_MENU (ID_G_FULLSCRN,    FFrame::OnSwitchFullScreen)
    EVT_MENU (ID_G_V_ALL,       FFrame::OnGViewAll)
    EVT_MENU (ID_G_V_VERT,      FFrame::OnGFitHeight)
    EVT_MENU (ID_G_SHOWY0,      FFrame::OnGShowY0)
    EVT_MENU (ID_G_V_SCROLL_L,  FFrame::OnGScrollLeft)
    EVT_MENU (ID_G_V_SCROLL_R,  FFrame::OnGScrollRight)
    EVT_MENU (ID_G_V_SCROLL_U,  FFrame::OnGScrollUp)
    EVT_MENU (ID_G_V_EXTH,      FFrame::OnGExtendH)
    EVT_MENU_RANGE (ID_G_V_ZOOM_PREV+1, ID_G_V_ZOOM_PREV_END,
                                FFrame::OnPreviousZoom)
    EVT_MENU (ID_G_LCONF1,      FFrame::OnConfigRead)
    EVT_MENU (ID_G_LCONF2,      FFrame::OnConfigRead)
    EVT_MENU (ID_G_LCONFB,      FFrame::OnConfigBuiltin)
    EVT_MENU_RANGE (ID_G_LCONF_X, ID_G_LCONF_X_END, FFrame::OnConfigX)
    EVT_MENU (ID_G_SCONF1,      FFrame::OnConfigSave)
    EVT_MENU (ID_G_SCONF2,      FFrame::OnConfigSave)
    EVT_MENU (ID_G_SCONFAS,     FFrame::OnConfigSaveAs)

    EVT_MENU (ID_H_MANUAL,      FFrame::OnShowHelp)
    EVT_MENU (ID_H_WEBSITE,     FFrame::OnOnline)
    EVT_MENU (ID_H_WIKI,        FFrame::OnOnline)
    EVT_MENU (ID_H_DISCUSSIONS, FFrame::OnOnline)
    EVT_MENU (ID_H_FEEDBACK,    FFrame::OnOnline)
    EVT_MENU (wxID_ABOUT,       FFrame::OnAbout)
    EVT_MENU (wxID_EXIT,        FFrame::OnQuit)
END_EVENT_TABLE()


    // Define my frame constructor
FFrame::FFrame(wxWindow *parent, const wxWindowID id, const wxString& title,
                 const long style)
    : wxFrame(parent, id, title, wxDefaultPosition, wxDefaultSize, style),
      main_pane(NULL), sidebar(NULL), status_bar(NULL), toolbar(NULL)
{
    const int default_peak_nr = 7; // Gaussian
    wxConfigBase *config = wxConfig::Get();
    peak_type_nr = config->Read(wxT("/DefaultFunctionType"), default_peak_nr);
    update_peak_type_list();
    // Load icon and bitmap
    SetIcon (wxICON (fityk));

    script_dir_ = config->Read(wxT("/execScriptDir"));
    export_dir_ = config->Read(wxT("/exportDir"));
    data_dir_ = config->Read(wxT("/loadDataDir"));

    //sizer, splitters, etc.
    wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
    v_splitter = new ProportionalSplitter(this);
    main_pane = new ProportionalSplitter(v_splitter);
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
    sizer->Add(status_bar, 0, wxEXPAND, 0);

    SetSizer(sizer);
    sizer->SetSizeHints(this);

    print_mgr = new PrintManager(plot_pane);

    update_menu_functions();
    update_menu_saved_transforms();
    update_menu_recent_baselines();
    plot_pane->get_plot()->set_hint_receiver(status_bar);
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
    peak_types.clear();
    v_foreach(Tplate::Ptr, i, ftk->get_tpm()->tpvec())
        peak_types.push_back((*i)->name);
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
    if (!c)
        return;
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
    int display_w, display_h;
    wxDisplaySize(&display_w, &display_h);
    int default_h = display_h >= 768 ? 670 : 400;
    int default_w = default_h * 3 / 2;
    int w = cf->Read(wxT("w"), default_w),
        h = cf->Read(wxT("h"), default_h);
    SetSize(w, h);
    v_splitter->SetProportion(
                        cfg_read_double(cf, wxT("VertSplitProportion"), 0.75));
    SwitchSideBar(cfg_read_bool(cf, wxT("ShowSideBar"), true));
    main_pane->SetProportion(
                        cfg_read_double(cf, wxT("MainPaneProportion"), 0.84));
    SwitchIOPane(cfg_read_bool(cf, wxT("ShowIOPane"), true));
    SwitchCrosshair(cfg_read_bool(cf, wxT("ShowCrosshair"), false));
    ftk->view.set_y0_factor(cfg_read_double(cf, wxT("ShowY0"), 10));
    GetMenuBar()->Check(ID_G_SHOWY0, ftk->view.y0_factor() != 0);
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
    cf->Write(wxT("ShowY0"), ftk->view.y0_factor());
    int w, h;
    GetSize(&w, &h);
    cf->Write(wxT("w"), (long) w);
    cf->Write(wxT("h"), (long) h);
    //cf->Write (wxT("BotWinHeight"), bottom_window->GetClientSize().GetHeight());
}

void FFrame::set_menubar()
{
    wxMenu* session_menu = new wxMenu;
    append_mi(session_menu, ID_SESSION_INCLUDE, GET_BMP(runmacro16),
              wxT("&Execute Script\tCtrl-X"),
              wxT("Execute commands from a file"));
    session_menu->Append (ID_SESSION_REINCLUDE, wxT("R&e-Execute script"),
             wxT("Reset & execute commands from the file included last time"));
    session_menu->Enable (ID_SESSION_REINCLUDE, false);
    append_mi(session_menu, ID_SCRIPT_EDIT, GET_BMP(editor16),
              wxT("E&dit Script"), wxT("Show script editor"));
    append_mi(session_menu, ID_SESSION_RESET, GET_BMP(reload16), wxT("&Reset"),
                                      wxT("Reset current session"));
    session_menu->AppendSeparator();
    wxMenu *session_log_menu = new wxMenu;
    append_mi(session_log_menu, ID_LOG_START, GET_BMP(recordmacro16),
           wxT("Choose Log File"), wxT("Start logging to file (make script)"));
    append_mi(session_log_menu, ID_LOG_STOP, GET_BMP(stopmacro16),
              wxT("Stop Logging"), wxT("Finish logging to file"));
    session_log_menu->AppendCheckItem(ID_LOG_WITH_OUTPUT,wxT("Log also output"),
                          wxT("output can be included in logfile as comments"));
    session_menu->Append(ID_SESSION_LOG, wxT("&Logging"), session_log_menu);
    append_mi(session_menu, ID_SESSION_DUMP, GET_BMP(filesaveas16),
              wxT("&Save Session..."),
              wxT("Save current program state as script file"));
    append_mi(session_menu, ID_SAVE_HISTORY, GET_BMP(filesaveas16),
              wxT("Save History..."),
              wxT("Save all commands executed so far to file")
                      wxT(" (it also creates a script)"));
    session_menu->AppendSeparator();
    session_menu->Append(ID_PAGE_SETUP, wxT("Page Se&tup..."),
                                        wxT("Page setup"));
    session_menu->Append(wxID_PREVIEW, wxT("Print Previe&w"));
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
    append_mi(session_menu, ID_SAVE_IMAGE, GET_BMP(image16),
              wxT("Sa&ve As Image..."), wxT("Save plot as PNG image."));
    session_menu->AppendSeparator();
    append_mi(session_menu, ID_SESSION_SET, GET_BMP(preferences16),
              wxT("&Settings"), wxT("Preferences and options"));
    session_menu->Append (ID_SESSION_EI, wxT("Edit &Init File"),
                         wxT("Edit the script run at startup"));
    session_menu->AppendSeparator();
    session_menu->Append(wxID_EXIT, wxT("&Quit"));

    wxMenu* data_menu = new wxMenu;
    append_mi(data_menu, ID_D_QLOAD, GET_BMP(fileopen16),
              wxT("&Quick Load File\tCtrl-O"), wxT("Load data from file"));
    append_mi(data_menu, ID_D_XLOAD, GET_BMP(fileopen16),
              wxT("&Load File\tCtrl-M"),
              wxT("Load data from file, with some options"));
    this->data_menu_recent = new wxMenu;
    int rf_counter = 1;
    for (list<wxFileName>::const_iterator i = recent_data_files.begin();
         i != recent_data_files.end() && rf_counter < 16; i++, rf_counter++)
        data_menu_recent->Append(ID_D_RECENT + rf_counter,
                                 i->GetFullName(), i->GetFullPath());
    data_menu->Append(ID_D_RECENT, wxT("&Recent Files"), data_menu_recent);
    append_mi(data_menu, ID_D_REVERT, GET_BMP(revert16), wxT("Re&vert"),
              wxT("Reload data from file(s)"));
    data_menu->AppendSeparator();

    data_menu->Append (ID_D_TABLE, wxT("T&able"), wxT("Data Table"));
    this->data_ft_menu = new wxMenu;
    data_menu->Append (ID_D_SDT, wxT("&Transformations"), data_ft_menu,
                                 wxT("Custom data transformations"));
    data_menu->Append (ID_D_EDITOR, wxT("E&dit Transformations..."),
                                    wxT("Open editor of data operations"));
    data_menu->AppendSeparator();
    data_menu->Append (ID_D_MERGE, wxT("&Merge Points..."),
                                        wxT("Reduce the number of points"));
    wxMenu* data_xps_menu = new wxMenu;
    data_xps_menu->Append(ID_D_RM_SHIRLEY, wxT("&Remove Shirley BG"),
                                                    wxT("Remove Shirley BG"));
    data_xps_menu->Append(ID_D_CALC_SHIRLEY, wxT("&Calculate Shirley BG"),
                                        wxT("put Shirley BG to new dataset"));
    data_menu->Append(-1, wxT("&XPS"), data_xps_menu);
    data_menu->AppendSeparator();
    append_mi(data_menu, ID_D_EXPORT, GET_BMP(export16),
              wxT("&Export\tCtrl-S"), wxT("Save data to file"));

    wxMenu* sum_menu = new wxMenu;
    func_type_menu = new wxMenu;
    sum_menu->Append (ID_G_M_PEAK, wxT("Function &Type"), func_type_menu);
    // the function list is created in update_menu_functions()
    append_mi(sum_menu, ID_DEFMGR, GET_BMP(function16),
              wxT("&Definition Manager"), wxT("Add or modify funtion types"));
    sum_menu->Append (ID_S_GUESS, wxT("&Guess Peak"),wxT("Guess and add peak"));
    sum_menu->Append (ID_S_PFINFO, wxT("Peak-Find &Info"),
                                wxT("Show where guessed peak would be placed"));
    sum_menu->AppendCheckItem(ID_S_AUTOFREEZE, wxT("Auto-Freeze"),
        wxT("In Data-Range mode: freeze functions in disactivated range"));
    sum_menu->AppendSeparator();
    append_mi(sum_menu, ID_S_EXPORTP, GET_BMP(export16),
              wxT("&Export Peak Parameters"),
              wxT("Export function parameters to file"));
    append_mi(sum_menu, ID_S_EXPORTF, GET_BMP(export16), wxT("&Export Formula"),
              wxT("Export mathematic formula to file"));
    append_mi(sum_menu, ID_S_EXPORTD, GET_BMP(export16), wxT("&Export Points"),
              wxT("Export as points in TSV file"));

    wxMenu* fit_menu = new wxMenu;
    wxMenu* fit_method_menu = new wxMenu;
    fit_method_menu->AppendRadioItem (ID_F_M+0, wxT("&Levenberg-Marquardt"),
                                                wxT("gradient based method"));
    fit_method_menu->AppendRadioItem (ID_F_M+1, wxT("Nelder-Mead &Simplex"),
                                  wxT("slow but simple and reliable method"));
    fit_method_menu->AppendRadioItem (ID_F_M+2, wxT("&Genetic Algorithm"),
                                                wxT("almost AI"));
    fit_menu->Append (ID_F_METHOD, wxT("&Method"), fit_method_menu, wxT(""));
    fit_menu->AppendSeparator();
    append_mi(fit_menu, ID_F_RUN, GET_BMP(run16), wxT("&Run...\tCtrl-R"),
                                             wxT("Fit sum to data"));
    append_mi(fit_menu, ID_F_INFO, GET_BMP(info16), wxT("&Info"),
                                            wxT("Info about current fit"));
    fit_menu->AppendSeparator();
    append_mi(fit_menu, ID_F_UNDO, GET_BMP(undo16), wxT("&Undo"),
                            wxT("Undo change of parameter"));
    append_mi(fit_menu, ID_F_REDO, GET_BMP(redo16), wxT("R&edo"),
                            wxT("Redo change of parameter"));
    fit_menu->Append (ID_F_HISTORY, wxT("&Parameter History"),
                            wxT("Go back or forward in parameter history"));

    wxMenu* tools_menu = new wxMenu;
    append_mi(tools_menu, ID_T_PD, wxBitmap(powdifpat16_xpm),
              wxT("&Powder Diffraction"),
              wxT("A tool for Pawley fitting"));

    wxMenu* gui_menu = new wxMenu;
    wxMenu* gui_menu_mode = new wxMenu;
    gui_menu_mode->AppendRadioItem (ID_G_M_ZOOM, wxT("&Normal\tF1"),
                              wxT("Use mouse for zooming, moving peaks etc."));
    gui_menu_mode->AppendRadioItem (ID_G_M_RANGE, wxT("&Range\tF2"),
                     wxT("Use mouse for activating and disactivating data"));
    gui_menu_mode->AppendRadioItem (ID_G_M_BG, wxT("&Baseline\tF3"),
                                wxT("Use mouse for subtracting background"));
    gui_menu_mode->AppendRadioItem (ID_G_M_ADD, wxT("&Peak-Add\tF4"),
                                    wxT("Use mouse for adding new peaks"));
    gui_menu->Append(ID_G_MODE, wxT("&Mode"), gui_menu_mode);
    wxMenu* baseline_menu = new wxMenu;
    baseline_menu->Append (ID_G_BG_STRIP, wxT("&Subtract Baseline"),
                           wxT("Subtract baseline function from data"));
    baseline_menu->Append (ID_G_BG_UNDO, wxT("&Add Baseline"),
                           wxT("Add baseline function to data"));
    baseline_menu->Append (ID_G_BG_CLEAR, wxT("&Clear Baseline"),
                           wxT("Clear baseline points, disable undo"));
    wxMenu* recent_b_menu = new wxMenu;
    baseline_menu->AppendSeparator();
    baseline_menu->Append(ID_G_BG_RECENT, wxT("&Recent"), recent_b_menu);
    baseline_menu->Append (ID_G_BG_HULL, wxT("&Set As Convex Hull"),
                           wxT("Set baseline as convex hull of data"));
    baseline_menu->AppendSeparator();
    baseline_menu->AppendCheckItem(ID_G_BG_SPLINE,
                                   wxT("&Spline Interpolation"),
                                   wxT("Cubic spline interpolation of points"));
    baseline_menu->Check(ID_G_BG_SPLINE, true);
    gui_menu->Append(ID_G_BG_SUB, wxT("Baseline Handling"), baseline_menu);
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
                            wxT("Configure output window"));
    gui_menu_config->Append(ID_G_C_SB, wxT("&Status Bar"),
                            wxT("Configure Status Bar"));
    gui_menu->Append(-1, wxT("Confi&gure"), gui_menu_config);

    gui_menu->AppendCheckItem(ID_G_CROSSHAIR, wxT("&Crosshair Cursor"),
                                              wxT("Show crosshair cursor"));
    gui_menu->AppendCheckItem(ID_G_FULLSCRN, wxT("&Full Screen\tF11"),
                                              wxT("Switch full screen"));
    gui_menu->AppendSeparator();
    append_mi(gui_menu, ID_G_V_ALL, GET_BMP(zoom_fit16),
              wxT("Zoom &All\tCtrl-A"), wxT("View whole data"));
    gui_menu->Append (ID_G_V_VERT, wxT("Fit &Vertically\tCtrl-V"),
                      wxT("Adjust vertical zoom"));
    gui_menu->AppendCheckItem(ID_G_SHOWY0, wxT("Zoom All Shows Y=&0"),
                        wxT("Tend to include Y=0 when adjusting view."));
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
    gui_menu->Append(ID_G_LCONF1, wxT("Load &Default Config"),
                                            wxT("Default configuration file"));
    gui_menu->Append(ID_G_LCONF2, wxT("Load &Alt. Config"),
                                        wxT("Alternative configuration file"));
    wxMenu* gui_menu_lconfig = new wxMenu;
    gui_menu_lconfig->Append(ID_G_LCONFB, wxT("&built-in"),
                                               wxT("Built-in configuration"));
    gui_menu_lconfig->AppendSeparator();
    update_config_menu(gui_menu_lconfig);
    gui_menu->Append(ID_G_LCONF, wxT("&Load Config..."), gui_menu_lconfig);
    wxMenu* gui_menu_sconfig = new wxMenu;
    gui_menu_sconfig->Append(ID_G_SCONF1, wxT("As Default"),
                     wxT("Save current configuration to default config file"));
    gui_menu_sconfig->Append(ID_G_SCONF2, wxT("As Alternative"),
                 wxT("Save current configuration to alternative config file"));
    gui_menu_sconfig->Append(ID_G_SCONFAS, wxT("As ..."),
                 wxT("Save current configuration to other config file"));
    gui_menu->Append(ID_G_SCONF, wxT("&Save Current Config"), gui_menu_sconfig);

    wxMenu *help_menu = new wxMenu;


    append_mi(help_menu, ID_H_MANUAL, GET_BMP(book16), wxT("&Manual"),
              wxT("User's Manual"));
    append_mi(help_menu, ID_H_WEBSITE, GET_BMP(web16), wxT("&Visit Website"),
              website_url);
    append_mi(help_menu, ID_H_WIKI, GET_BMP(web16), wxT("&Visit Wiki"),
              wiki_url);
    append_mi(help_menu, ID_H_DISCUSSIONS, GET_BMP(web16),
              wxT("&Visit Discussions"), discussions_url);
    append_mi(help_menu, ID_H_FEEDBACK, GET_BMP(web16),
              wxT("&Anonymous Feedback"), feedback_url);
    help_menu->Append(wxID_ABOUT, wxT("&About..."), wxT("Show about dialog"));

    wxMenuBar *menu_bar = new wxMenuBar();
    menu_bar->Append (session_menu, wxT("&Session") );
    menu_bar->Append (data_menu, wxT("&Data") );
    menu_bar->Append (sum_menu, wxT("&Functions") );
    menu_bar->Append (fit_menu, wxT("F&it") );
    menu_bar->Append (tools_menu, wxT("&Tools") );
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

// construct GUI -> Baseline Handling -> Previous menu
void FFrame::update_menu_recent_baselines()
{
    wxMenu *menu = GetMenuBar()->FindItem(ID_G_BG_RECENT)->GetSubMenu();
    // clear menu
    while (menu->GetMenuItemCount() > 0)
        menu->Delete(menu->GetMenuItems().GetLast()->GetData());

    for (int i = 0; i < 10; ++i) {
        wxString name = plot_pane->get_bg_manager()->get_recent_bg_name(i);
        if (name.empty())
            break;
        if (i == 0)
            name += wxT("\tShift-F3");
        menu->Append(ID_G_BG_RECENT+i+1, name);
    }
}


void FFrame::OnShowHelp(wxCommandEvent&)
{
    wxString help_url = get_help_url(wxT("fityk-manual.html"));
    bool r = wxLaunchDefaultBrowser(help_url);
    if (!r)
        wxMessageBox(wxT("Can't open browser.\nManual is here:\n") + help_url,
                     wxT("Manual"), wxOK|wxICON_INFORMATION);
}

void FFrame::OnAbout(wxCommandEvent&)
{
    AboutDlg* dlg = new AboutDlg(this);
    dlg->ShowModal();
    dlg->Destroy();
}

void FFrame::OnOnline(wxCommandEvent& event)
{
    const wxString* url;
    if (event.GetId() == ID_H_WEBSITE)
        url = &website_url;
    else if (event.GetId() == ID_H_WIKI)
        url = &wiki_url;
    else if (event.GetId() == ID_H_DISCUSSIONS)
        url = &discussions_url;
    else //if (event.GetId() == ID_H_FEEDBACK)
        url = &feedback_url;
    bool r = wxLaunchDefaultBrowser(*url);
    if (!r)
        wxMessageBox(wxT("Can not open URL in browser,")
                     wxT("\nplease open it manually:\n") + *url,
                     wxT("Browser Launch Failed"), wxOK);
}

void FFrame::OnDataQLoad (wxCommandEvent&)
{
    wxFileDialog fdlg (this, wxT("Load data from a file"), data_dir_, wxT(""),
                       wxT("All Files (*)|*|")
                                       + s2wx(xylib::get_wildcards_string()),
                       wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE);
    int ret = fdlg.ShowModal();
    data_dir_ = fdlg.GetDirectory();
    if (ret != wxID_OK)
        return;

    wxArrayString paths;
    fdlg.GetPaths(paths);
    int count = paths.GetCount();
    string cmd;
    if (count == 1) {
        string f = wx2s(paths[0]);
        shared_ptr<const xylib::DataSet> d = xylib::cached_load_file(f, "", "");
        if (d->get_block_count() > 1) {
            wxArrayString choices;
            for (int i = 0; i < d->get_block_count(); ++i) {
                wxString name = s2wx(d->get_block(i)->get_name());
                if (name.empty())
                    name.Printf(wxT("Block #%d"), i+1);
                choices.push_back(name);
            }
            wxMultiChoiceDialog mdlg(this, wxT("Select data blocks:"),
                                     wxT("Choose blocks"), choices);
            wxArrayInt sel;
            sel.push_back(0);
            mdlg.SetSelections(sel);
            if (mdlg.ShowModal() != wxID_OK)
                return;
            sel = mdlg.GetSelections();
            paths.clear();
            for (size_t i = 0; i < sel.size(); ++i)
                paths.push_back(s2wx(f + "::::" + S(i)));
        }
    }

    for (size_t i = 0; i < paths.size(); ++i) {
        if (i != 0)
            cmd += " ; ";
        cmd += "@+ <'" + wx2s(paths[i]) + "'";
        add_recent_data_file(wx2s(paths[i]));
    }

    ftk->exec(cmd);
    if (count > 1)
        SwitchSideBar(true);
}

void FFrame::OnDataXLoad (wxCommandEvent&)
{
    vector<int> sel = get_selected_data_indices();
    int n = (sel.size() == 1 ? sel[0] : -1);
    // in case of multi-selection, use the first item
    Data *data = ftk->get_data(sel[0]);
    DLoadDlg dload_dialog(this, -1, n, data);
    dload_dialog.ShowModal();
}

void FFrame::OnDataRecent (wxCommandEvent& event)
{
    string s = wx2s(GetMenuBar()->GetHelpString(event.GetId()));
    ftk->exec("@+ <'" + s + "'");
    add_recent_data_file(s);
}

void FFrame::OnDataRevertUpdate (wxUpdateUIEvent& event)
{
    vector<int> sel = get_selected_data_indices();
    event.Enable(sel.size() == 1
                 && !ftk->get_data(sel[0])->get_filename().empty());
}

void FFrame::OnDataRevert (wxCommandEvent&)
{
    vector<int> sel = get_selected_data_indices();
    string cmd;
    for (vector<int>::const_iterator i = sel.begin(); i != sel.end(); ++i) {
        if (i != sel.begin())
            cmd += "; ";
        cmd += "@" + S(*i) + "< .";
    }
    ftk->exec(cmd);
}

void FFrame::OnDataTable(wxCommandEvent&)
{
    int data_nr = get_focused_data_index();
    DataTableDlg data_table(this, -1, data_nr, ftk->get_data(data_nr));
    data_table.ShowModal();
}

void FFrame::OnDataEditor (wxCommandEvent&)
{
    vector<pair<int,Data*> > dd;
    vector<int> sel = get_selected_data_indices();
    for (vector<int>::const_iterator i = sel.begin(); i != sel.end(); ++i)
        dd.push_back(make_pair(*i, ftk->get_data(*i)));
    EditTransDlg data_editor(this, -1, dd);
    data_editor.ShowModal();
    update_menu_saved_transforms();
}

void FFrame::update_menu_saved_transforms()
{
    // delete old items
    int item_count = data_ft_menu->GetMenuItemCount();
    for (int i = 0; i != item_count; ++i)
        data_ft_menu->Delete(ID_D_SDT+i+1);
    // create new items
    const vector<DataTransform> &all = EditTransDlg::get_transforms();
    int counter = 0;
    for (vector<DataTransform>::const_iterator i = all.begin();
                                                        i != all.end(); ++i)
        if (i->in_menu) {
            ++counter;
            data_ft_menu->Append(ID_D_SDT+counter, i->name, i->description);
        }
}

void FFrame::OnSavedDT (wxCommandEvent& event)
{
    wxString name = GetMenuBar()->GetLabel(event.GetId());
    const vector<DataTransform> &transforms = EditTransDlg::get_transforms();
    for (vector<DataTransform>::const_iterator i = transforms.begin();
            i != transforms.end(); ++i)
        if (i->name == name) {
            EditTransDlg::execute_tranform(wx2s(i->code));
            return;
        }
}

void FFrame::OnDataMerge (wxCommandEvent&)
{
    MergePointsDlg *dlg = new MergePointsDlg(this);
    if (dlg->ShowModal() == wxID_OK)
        ftk->exec(dlg->get_command());
    dlg->Destroy();
}

void FFrame::OnDataCalcShirley (wxCommandEvent&)
{
    vector<int> sel = get_selected_data_indices();
    string cmd;
    for (vector<int>::const_iterator i = sel.begin(); i != sel.end(); ++i) {
        string title = ftk->get_data(*i)->get_title();
        int c = ftk->get_dm_count();
        if (i != sel.begin())
            cmd += "; ";
        cmd += "@+ = shirley_bg @" + S(*i)
              + "; @" + S(c) + ".title = '" + title + "-Shirley'";
    }
    ftk->exec(cmd);
}

void FFrame::OnDataRmShirley (wxCommandEvent&)
{
    vector<int> sel = get_selected_data_indices();
    string cmd;
    for (vector<int>::const_iterator i = sel.begin(); i != sel.end(); ++i) {
        string dstr = "@" + S(*i);
        if (i != sel.begin())
            cmd += "; ";
        cmd += dstr + " = rm_shirley_bg " + dstr;
    }
    ftk->exec(cmd);
}

void FFrame::OnDataExportUpdate (wxUpdateUIEvent& event)
{
    int nsel = get_selected_data_indices().size();
    event.Enable(nsel == 1 || nsel == ftk->get_dm_count());
}

void FFrame::OnDataExport (wxCommandEvent&)
{
    export_data_dlg(this);
}

void FFrame::OnDefinitionMgr(wxCommandEvent&)
{
    DefinitionMgrDlg* dlg = new DefinitionMgrDlg(this);
    if (dlg->ShowModal() == wxID_OK) {
        vector<string> commands = dlg->get_commands();
        v_foreach (string, c, commands)
            ftk->exec(*c);
    }
    dlg->Destroy();
}

void FFrame::OnSGuess (wxCommandEvent&)
{
    ftk->exec(get_datasets() + "guess " + get_guess_string(get_peak_type()));
}

void FFrame::OnSPFInfo (wxCommandEvent&)
{
    ftk->exec(get_datasets() + "info guess");
}

void FFrame::OnAutoFreeze(wxCommandEvent& event)
{
    plot_pane->get_plot()->set_auto_freeze(event.IsChecked());
}

void FFrame::OnSExport (wxCommandEvent& event)
{
    bool as_peaks = (event.GetId() == ID_S_EXPORTP);
    wxString name;
    vector<int> sel = get_selected_data_indices();
    if (sel.size() == 1) {
        string const& filename = ftk->get_data(sel[0])->get_filename();
        name = wxFileName(s2wx(filename)).GetName()
                                + (as_peaks ? wxT(".peaks") : wxT(".formula"));
    }
    wxFileDialog fdlg (this, wxT("Export curve to file"), export_dir_, name,
                       (as_peaks
                            ? wxT("parameters of functions (*.peaks)|*.peaks")
                            : wxT("mathematic formula (*.formula)|*.formula"))
                        + wxString(wxT("|all files|*")),
                       wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (fdlg.ShowModal() == wxID_OK)
        ftk->exec(get_datasets() + "info " + (as_peaks ? "peaks" : "formula")
                  + " > '" + wx2s(fdlg.GetPath()) + "'");
    export_dir_ = fdlg.GetDirectory();
}


void FFrame::OnFMethodUpdate (wxUpdateUIEvent& event)
{
    int n = ftk->settings_mgr()->get_enum_index("fitting_method");
    event.Check(ID_F_M + n == event.GetId());
}

void FFrame::OnMenuFitRunUpdate(wxUpdateUIEvent& event)
{
    event.Enable(!ftk->parameters().empty());
}

void FFrame::OnMenuFitUndoUpdate(wxUpdateUIEvent& event)
{
    event.Enable(ftk->get_fit_container()->can_undo());
}

void FFrame::OnMenuFitRedoUpdate(wxUpdateUIEvent& event)
{
    event.Enable(ftk->get_fit_container()->has_param_history_rel_item(1));
}

void FFrame::OnMenuFitHistoryUpdate(wxUpdateUIEvent& event)
{
    event.Enable(ftk->get_fit_container()->get_param_history_size() != 0);
}

void FFrame::OnFOneOfMethods (wxCommandEvent& event)
{
    int m = event.GetId() - ID_F_M;
    const char** values
                  = ftk->settings_mgr()->get_allowed_values("fitting_method");
    ftk->exec("set fitting_method=" + S(values[m]));
}

void FFrame::OnFRun (wxCommandEvent&)
{
    FitRunDlg(this, -1, true).ShowModal();
}

void FFrame::OnFInfo (wxCommandEvent&)
{
    FitInfoDlg dlg(this, -1);
    if (dlg.Initialize())
        dlg.ShowModal();
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

void FFrame::OnPowderDiffraction (wxCommandEvent&)
{
    PowderDiffractionDlg(this, -1).ShowModal();
}

void FFrame::OnMenuLogStartUpdate (wxUpdateUIEvent& event)
{
    event.Enable(ftk->get_settings()->logfile.empty());
}

void FFrame::OnMenuLogStopUpdate (wxUpdateUIEvent& event)
{
    event.Enable(!ftk->get_settings()->logfile.empty());
}

void FFrame::OnMenuLogOutputUpdate (wxUpdateUIEvent& event)
{
    if (!ftk->get_settings()->logfile.empty())
        event.Check(ftk->get_settings()->log_full);
}

void FFrame::OnLogStart (wxCommandEvent&)
{
    wxFileDialog fdlg (this, wxT("Log to file"), export_dir_, wxT(""),
                       wxT("Fityk script file (*.fit)|*.fit;*.FIT")
                       wxT("|All files |*"),
                       wxFD_SAVE);
    const string& logfile = ftk->get_settings()->logfile;
    if (!logfile.empty())
        fdlg.SetPath(s2wx(logfile));
    if (fdlg.ShowModal() == wxID_OK) {
        string plus = GetMenuBar()->IsChecked(ID_LOG_WITH_OUTPUT) ? "+" : "";
        ftk->exec("commands" + plus + " > '" + wx2s(fdlg.GetPath()) + "'");
    }
    export_dir_ = fdlg.GetDirectory();
}

void FFrame::OnLogStop (wxCommandEvent&)
{
    ftk->exec("set logfile=''");
}

void FFrame::OnLogWithOutput (wxCommandEvent& event)
{
    bool checked = event.IsChecked();
    GetMenuBar()->Check(ID_LOG_WITH_OUTPUT, checked);
    ftk->exec("set log_full=" + S((int)checked));
}

void FFrame::OnSaveHistory (wxCommandEvent&)
{
    wxFileDialog fdlg(this, wxT("Save all commands from this session to file"),
                      export_dir_, wxT(""),
                      wxT("fityk file (*.fit)|*.fit;*.FIT"),
                      wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (fdlg.ShowModal() == wxID_OK) {
        ftk->exec("info history > '" + wx2s(fdlg.GetPath()) + "'");
    }
    export_dir_ = fdlg.GetDirectory();
}

void FFrame::OnReset (wxCommandEvent&)
{
    int r = wxMessageBox(wxT("Do you really want to reset current session \n")
                      wxT("and lose everything you have done in this session?"),
                         wxT("Are you sure?"),
                         wxYES_NO | wxCANCEL | wxCENTRE | wxICON_QUESTION);
    if (r == wxYES) {
        plot_pane->get_bg_manager()->clear_background();
        ftk->exec("reset");
    }
}

void FFrame::OnInclude (wxCommandEvent&)
{
    wxFileDialog fdlg (this, wxT("Execute commands from file"),
                       script_dir_, wxT(""),
                       wxT("fityk file (*.fit)|*.fit;*.FIT|all files|*"),
                       wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (fdlg.ShowModal() == wxID_OK) {
        ftk->exec("exec '" + wx2s(fdlg.GetPath()) + "'");
        last_include_path = wx2s(fdlg.GetPath());
        GetMenuBar()->Enable(ID_SESSION_REINCLUDE, true);
    }
    script_dir_ = fdlg.GetDirectory();
}

void FFrame::OnReInclude (wxCommandEvent&)
{
    ftk->exec("reset; exec '" + last_include_path + "'");
}

void FFrame::show_editor(wxString const& path)
{
    ScriptDebugDlg* dlg = new ScriptDebugDlg(this);
    if (!path.empty())
        dlg->do_open_file(path);
    dlg->Show(true);
}

void FFrame::OnDump (wxCommandEvent&)
{
    wxFileDialog fdlg(this, wxT("Save everything as a script"),
                      export_dir_, wxT(""),
                      wxT("fityk file (*.fit)|*.fit;*.FIT"),
                      wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (fdlg.ShowModal() == wxID_OK) {
        ftk->exec("info state > '" + wx2s(fdlg.GetPath()) + "'");
    }
    export_dir_ = fdlg.GetDirectory();
}

void FFrame::OnSettings (wxCommandEvent&)
{
    SettingsDlg *dialog = new SettingsDlg(this);
    dialog->ShowModal();
    dialog->Destroy();
}

void FFrame::OnEditInit (wxCommandEvent&)
{
    wxString startup_file = get_conf_file(startup_commands_filename);
    show_editor(startup_file);
}

void FFrame::change_mouse_mode(MouseModeEnum mode)
{
    //enum MouseModeEnum { mmd_zoom=0, mmd_bg, mmd_add, mmd_activate, ...
    int menu_ids[] = { ID_G_M_ZOOM, ID_G_M_BG, ID_G_M_ADD, ID_G_M_RANGE };
    int tool_ids[] = { ID_T_ZOOM, ID_T_BG, ID_T_ADD, ID_T_RANGE };
    int idx = mode;
    GetMenuBar()->Check(menu_ids[idx], true);
    if (toolbar)
        toolbar->ToggleTool(tool_ids[idx], true);
    toolbar->EnableTool(ID_T_STRIP, (mode == mmd_bg));
    plot_pane->set_mouse_mode(mode);
}

void FFrame::OnChangeMouseMode (wxCommandEvent& event)
{
    switch (event.GetId()) {
        case ID_G_M_ZOOM:
        case ID_T_ZOOM:
            change_mouse_mode(mmd_zoom);
            break;
        case ID_G_M_RANGE:
        case ID_T_RANGE:
            change_mouse_mode(mmd_activate);
            break;
        case ID_G_M_BG:
        case ID_T_BG:
            change_mouse_mode(mmd_bg);
            break;
        case ID_G_M_ADD:
        case ID_T_ADD:
            change_mouse_mode(mmd_add);
            break;
        default: assert(0);
    }
}

void FFrame::update_menu_functions()
{
    size_t cnt = this->func_type_menu->GetMenuItemCount();
    size_t pcnt = peak_types.size();

    // in wxGTK 2.9 (svn 11.2007) in wxMenu::GtkAppend() m_prevRadio is stored
    // and adding radio-item to menu when the last added radio-item is already
    // removed will cause error
    //  a workaround is to delete and add all items:
    static bool was_deleted = false;
    if (pcnt < cnt)
        was_deleted = true;
    else if (pcnt > cnt && was_deleted) {
        for (size_t i = 0; i < cnt; i++)
            func_type_menu->Destroy(ID_G_M_PEAK_N+i);
        cnt = 0;
        was_deleted = false;
    }
    // -- end of the workaround

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
}

void FFrame::OnMenuBgStripUpdate(wxUpdateUIEvent& event)
{
    event.Enable(plot_pane->get_bg_manager()->can_strip());
}

void FFrame::OnMenuBgUndoUpdate(wxUpdateUIEvent& event)
{
    event.Enable(plot_pane->get_bg_manager()->has_fn());
}

void FFrame::OnMenuBgClearUpdate(wxUpdateUIEvent& event)
{
    BgManager* bgm = plot_pane->get_bg_manager();
    event.Enable(bgm->can_strip() || bgm->has_fn());
}

void FFrame::OnStripBg(wxCommandEvent&)
{
    plot_pane->get_bg_manager()->strip_background();
    update_menu_recent_baselines();
}

void FFrame::OnUndoBg(wxCommandEvent&)
{
    plot_pane->get_bg_manager()->add_background();
}

void FFrame::OnClearBg(wxCommandEvent&)
{
    plot_pane->get_bg_manager()->clear_background();
    refresh_plots(false, kMainPlot);
}

void FFrame::OnRecentBg(wxCommandEvent& event)
{
    int n = event.GetId() - (ID_G_BG_RECENT + 1);
    change_mouse_mode(mmd_bg);
    plot_pane->get_bg_manager()->update_focused_data(get_focused_data_index());
    plot_pane->get_bg_manager()->set_as_recent(n);
    refresh_plots(false, kMainPlot);
}
void FFrame::OnConvexHullBg(wxCommandEvent&)
{
    change_mouse_mode(mmd_bg);
    plot_pane->get_bg_manager()->update_focused_data(get_focused_data_index());
    plot_pane->get_bg_manager()->set_as_convex_hull();
    refresh_plots(false, kMainPlot);
}

void FFrame::OnSplineBg(wxCommandEvent& event)
{
    plot_pane->get_bg_manager()->set_spline_bg(event.IsChecked());
    refresh_plots(false, kMainPlot);
}

void FFrame::SwitchToolbar(bool show)
{
    if (show && !GetToolBar()) {
        toolbar = new FToolBar(this, -1);
        SetToolBar(toolbar);
        update_toolbar();
        update_peak_type_list();
        //toolbar->ToggleTool(ID_T_BAR, v_splitter->IsSplit());
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
    status_bar->Show(show);
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
    //if (toolbar)
    //    toolbar->ToggleTool(ID_T_BAR, show);
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
    //if (toolbar) toolbar->ToggleTool(ID_T_..., show);
}

void FFrame::OnShowPrefDialog(wxCommandEvent& ev)
{
    if (ev.GetId() == ID_G_C_MAIN)
        plot_pane->get_plot()->OnConfigure(ev);
    else
        plot_pane->get_aux_plot(ev.GetId() - ID_G_C_A1)->show_pref_dialog();
}

void FFrame::OnConfigureStatusBar(wxCommandEvent& event)
{
    if (status_bar)
        status_bar->OnPrefButton(event);
}

void FFrame::OnConfigureOutputWin(wxCommandEvent&)
{
    io_pane->output_win->show_preferences_dialog();
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

void FFrame::OnMenuShowAuxUpdate (wxUpdateUIEvent& event)
{
    event.Check(plot_pane->aux_visible(event.GetId() - ID_G_S_A1));
}

void FFrame::GViewAll()
{
    RealRange all;
    change_zoom(all, all);
}

void FFrame::OnGFitHeight (wxCommandEvent&)
{
    RealRange all;
    change_zoom(ftk->view.hor, all);
}

void FFrame::OnGShowY0(wxCommandEvent& e)
{
    ftk->view.set_y0_factor(e.IsChecked() ? 10 : 0);
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
    View const& view = ftk->view;
    Scale const& scale = plot_pane->get_plot()->get_y_scale();
    fp top, bottom;
    if (scale.logarithm) {
        top = 10 * view.top();
        bottom = 0.1 * view.bottom();
    }
    else {
        fp const factor = 2.;
        int Y0 = scale.px(0);
        int H = plot_pane->get_plot()->GetSize().GetHeight();
        bool Y0_visible = (Y0 >= 0 && Y0 < H);
        double pivot = (Y0_visible ? 0. : (view.bottom() + view.top()) / 2);
        top = pivot + factor * (view.top() - pivot);
        bottom = pivot + factor * (view.bottom() - pivot);
    }

    change_zoom(view.hor, RealRange(bottom, top));
}

void FFrame::OnGExtendH (wxCommandEvent&)
{
    const fp factor = 0.5;
    fp diff = ftk->view.width() * factor;
    change_zoom(RealRange(ftk->view.left() - diff, ftk->view.right() + diff),
                ftk->view.ver);
}


void FFrame::OnPreviousZoom(wxCommandEvent& event)
{
    int id = event.GetId();
    string s = plot_pane->zoom_backward(id ? id - ID_G_V_ZOOM_PREV : 1);
    if (!s.empty())
        ftk->exec("plot " + s);
}

static
string format_range(const RealRange& r)
{
    string s = "[";
    if (!r.from_inf())
        s += eS(r.from) + ":";
    if (!r.to_inf())
        s += eS(r.to);
    return s + "]";
}

void FFrame::change_zoom(const RealRange& h, const RealRange& v)
{
    plot_pane->zoom_forward();
    string cmd = "plot " + format_range(h) + " " + format_range(v);
    if (h.from_inf() || h.to_inf() || v.from_inf() || v.to_inf())
        cmd += sidebar->get_sel_datasets_as_string();
    ftk->exec(cmd);
}

void FFrame::scroll_view_horizontally(fp step)
{
    fp diff = ftk->view.width() * step;
    if (plot_pane->get_plot()->get_x_reversed())
        diff = -diff;
    change_zoom(RealRange(ftk->view.left() + diff, ftk->view.right() + diff),
                ftk->view.ver);
}


void FFrame::OnConfigSave (wxCommandEvent& event)
{
    string name = (event.GetId() == ID_G_SCONF1 ? wxGetApp().conf_filename
                                                : wxGetApp().alt_conf_filename);
    save_config_as(get_conf_file(name));
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

    wxMenu *config_menu = GetMenuBar()->FindItem(ID_G_LCONF)->GetSubMenu();
    update_config_menu(config_menu);
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
    string name = (event.GetId() == ID_G_LCONF1 ? wxGetApp().conf_filename
                                                : wxGetApp().alt_conf_filename);
    read_config(get_conf_file(name));
}

void FFrame::read_config(wxString const& name)
{
    wxFileConfig *config = new wxFileConfig(wxT(""), wxT(""), name);
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

void FFrame::update_config_menu(wxMenu *menu)
{
    // the first two items are "built-in" and separator, not to be deleted
    int old_count = menu->GetMenuItemCount() - 2;
    // delete old menu items
    for (int i = 0; i < old_count; ++i)
        menu->Destroy(ID_G_LCONF_X + i);
    // add new items
    wxArrayString filenames;
    int n = wxDir::GetAllFiles(wxGetApp().config_dir, &filenames);
    int config_number_limit = ID_G_LCONF_X_END - ID_G_LCONF_X;
    for (int i = 0; i < min(n, config_number_limit); ++i) {
        wxFileName fn(filenames[i]);
        menu->Append(ID_G_LCONF_X + i, fn.GetFullName(), fn.GetFullPath());
    }
}


void FFrame::OnConfigX (wxCommandEvent& event)
{
    wxMenu *menu = GetMenuBar()->FindItem(ID_G_LCONF)->GetSubMenu();
    wxString name = menu->GetLabel(event.GetId());
    if (name.IsEmpty())
        return;
    wxString filename = wxGetApp().config_dir + name;
    // wxFileExists returns false for links, but it's not a problem if the menu
    // is updated
    if (!wxFileExists(filename))
        update_config_menu(menu);
    read_config(filename);
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
    if (dc.IsOk()) {
        do_print_plots(&dc, print_mgr);
        wxMetafile *mf = dc.Close();
        if (mf) {
            mf->SetClipboard(dc.MaxX() + 10, dc.MaxY() + 10);
            delete mf;
        }
    }
#endif
}

void FFrame::OnSaveAsImage(wxCommandEvent&)
{
    wxFileDialog fdlg(this, wxT("Save main plot as image"),
                      export_dir_, wxT(""),
                      wxT("PNG image (*.png)|*.png;*.PNG"),
                      wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (fdlg.ShowModal() == wxID_OK) {
        wxString path = fdlg.GetPath();
        wxBitmap const& bmp = get_main_plot()->get_bitmap();
        bmp.ConvertToImage().SaveFile(path, wxBITMAP_TYPE_PNG);
    }
    export_dir_ = fdlg.GetDirectory();
}


string FFrame::get_peak_type() const
{
    if (peak_type_nr >= (int) ftk->get_tpm()->tpvec().size())
        return "";
    return ftk->get_tpm()->tpvec()[peak_type_nr]->name;
}

void FFrame::set_status_text(std::string const& text)
{
    if (status_bar)
        status_bar->set_text(s2wx(text));
}

void FFrame::set_status_coords(fp x, fp y, PlotTypeEnum pte)
{
    if (status_bar)
        status_bar->set_coords(x, y, pte);
}

void FFrame::clear_status_coords()
{
    if (status_bar)
        status_bar->clear_coords();
}

void FFrame::output_text(UserInterface::Style style, const string& str)
{
    io_pane->output_win->append_text(style, s2wx(str));
}

void FFrame::refresh_plots(bool now, WhichPlot which_plot)
{
    plot_pane->refresh_plots(now, which_plot);
}

void FFrame::update_crosshair(int X, int Y)
{
    plot_pane->update_crosshair(X, Y);
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
    if (!toolbar)
        return;
    BgManager* bgm = plot_pane->get_bg_manager();
    toolbar->ToggleTool(ID_T_STRIP, bgm->has_fn() && bgm->stripped());
    toolbar->EnableTool(ID_T_RUN, !ftk->parameters().empty());
    toolbar->EnableTool(ID_T_UNDO, ftk->get_fit_container()->can_undo());
    toolbar->EnableTool(ID_T_PZ, !plot_pane->get_zoom_hist().empty());
}

int FFrame::get_focused_data_index()
{
    return sidebar->get_focused_data();
}

vector<int> FFrame::get_selected_data_indices()
{
    return sidebar->get_selected_data_indices();
}

string FFrame::get_datasets()
{
    if (ftk->get_dm_count() == 1)
        return "";
    vector<int> sel = get_selected_data_indices();
    if (ftk->get_dm_count() == size(sel))
        return "@*: ";
    else
        return "@" + join_vector(sel, ", @") + ": ";
}

string FFrame::get_guess_string(const std::string& name)
{
    string s;
    int nh = ftk->find_variable_nr("_hwhm");
    int ns = ftk->find_variable_nr("_shape");

    s = "(";
    if (nh != -1)
        s += "hwhm=$_hwhm";

    if (ns != -1) {
        if (nh != -1)
            s += ", ";
        s += "shape=$_shape";
    }

    const Tplate* tp = ftk->get_tpm()->get_tp(name);
    vector<string> missing;
    try {
        missing = tp->get_missing_default_values();
    }
    catch (fityk::SyntaxError&) {
    }
    v_foreach (string, arg, missing) {
        wxString value = wxGetTextFromUser(s2wx(*arg) + wxT(" = "),
                                           wxT("Initial value"), wxT("~0"));
        if (value.empty()) // pressing Cancel returns ""
            break;
        if (s.size() > 1)
            s += ", ";
        s += *arg + "=" + wx2s(value);
    }

    if (s.size() == 1)
        return name;
    else
        return name + s + ")";
}

vector<DataAndModel*> FFrame::get_selected_dms()
{
    vector<int> sel = get_selected_data_indices();
    vector<DataAndModel*> dms(sel.size());
    for (size_t i = 0; i < sel.size(); ++i)
        dms[i] = ftk->get_dm(sel[i]);
    return dms;
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

void FFrame::activate_function(int n)
{
    sidebar->activate_function(n);
}

void FFrame::update_app_title()
{
    string title = "fityk";
    int pos = get_focused_data_index();
    string const& filename = ftk->get_data(pos)->get_filename();
    if (!filename.empty())
        title += " - " + filename;
    SetTitle(s2wx(title));
}

// Overrides how menu items' help is displayed.
// We show the help in our status bar, which is not derived from wxStatusBar.
// Implementation based on wxFrameBase::DoGiveHelp(), see comments there.
void FFrame::DoGiveHelp(const wxString& help, bool show)
{
    if (!status_bar)
        return;

    wxString text;
    if (show) {
        if (m_oldStatusText.empty()) {
            m_oldStatusText = status_bar->get_text();
            if (m_oldStatusText.empty()) {
                // use special value to prevent us from doing this the next time
                m_oldStatusText += _T('\0');
            }
        }
        text = help;
    }
    else {
        text = m_oldStatusText;
        m_oldStatusText.clear();
    }
    status_bar->set_text(text);
}

//===============================================================
//                    FToolBar
//===============================================================

BEGIN_EVENT_TABLE (FToolBar, wxToolBar)
    EVT_TOOL_RANGE (ID_T_ZOOM, ID_T_ADD, FToolBar::OnChangeMouseMode)
    EVT_TOOL_RANGE (ID_T_PZ, ID_T_AUTO, FToolBar::OnClickTool)
    //EVT_TOOL (ID_T_BAR, FToolBar::OnSwitchSideBar)
    EVT_CHOICE (ID_T_CHOICE, FToolBar::OnPeakChoice)
END_EVENT_TABLE()

FToolBar::FToolBar (wxFrame *parent, wxWindowID id)
        : wxToolBar (parent, id, wxDefaultPosition, wxDefaultSize,
                     wxNO_BORDER | /*wxTB_FLAT |*/ wxTB_DOCKABLE)
{
    SetToolBitmapSize(wxSize(24, 24));
    // mode
    MouseModeEnum m = frame ? frame->plot_pane->get_plot()->get_mouse_mode()
                              : mmd_zoom;
    AddRadioTool(ID_T_ZOOM, wxT("Zoom"),
                 wxBitmap(zoom_mode_xpm), wxNullBitmap,
                 wxT("Normal Mode [F1]"),
                 wxT("Use mouse for zooming, moving peaks etc."));
    ToggleTool(ID_T_ZOOM, m == mmd_zoom);
    AddRadioTool(ID_T_RANGE, wxT("Range"),
                 wxBitmap(active_mode_xpm), wxNullBitmap,
                 wxT("Data-Range Mode [F2]"),
                 wxT("Use mouse for activating and disactivating data (try also with [Shift])"));
    ToggleTool(ID_T_RANGE, m == mmd_activate);
    AddRadioTool(ID_T_BG, wxT("Background"),
                 wxBitmap(bg_mode_xpm), wxNullBitmap,
                 wxT("Baseline Mode [F3]"),
                 wxT("Use mouse for subtracting background"));
    ToggleTool(ID_T_BG, m == mmd_bg);
    AddRadioTool(ID_T_ADD, wxT("Add peak"),
                 wxBitmap(addpeak_mode_xpm), wxNullBitmap,
                 wxT("Add-Peak Mode [F4]"),
                 wxT("Use mouse for adding new peaks"));
    ToggleTool(ID_T_ADD, m == mmd_add);
    AddSeparator();
    // view
    AddTool (ID_G_V_ALL, wxT("Whole"), wxBitmap(zoom_all_xpm), wxNullBitmap,
             wxITEM_NORMAL, wxT("View whole [Ctrl+A]"),
             wxT("Fit data in window"));
    AddTool(ID_G_V_VERT, wxT("Fit height"),
            wxBitmap(zoom_vert_xpm), wxNullBitmap, wxITEM_NORMAL,
            wxT("Fit vertically [Ctrl+V]"), wxT("Set optimal y scale"));
    AddTool(ID_G_V_SCROLL_L, wxT("<-- scroll"),
            wxBitmap(zoom_left_xpm), wxNullBitmap, wxITEM_NORMAL,
            wxT("Scroll left (Ctrl+[)"), wxT("Scroll view left"));
    AddTool(ID_G_V_SCROLL_R, wxT("scroll -->"),
            wxBitmap(zoom_right_xpm), wxNullBitmap, wxITEM_NORMAL,
            wxT("Scroll right (Ctrl+])"), wxT("Scroll view right"));
    AddTool(ID_G_V_SCROLL_U, wxT("V-zoom-out"),
            wxBitmap(zoom_up_xpm), wxNullBitmap, wxITEM_NORMAL,
            wxT("Extend zoom up [Ctrl+-]"), wxT("Double vertical range"));
    AddTool(ID_T_PZ, wxT("Back"), wxBitmap(zoom_prev_xpm), wxNullBitmap,
            wxITEM_NORMAL, wxT("Previous view"),
            wxT("Go to previous View"));
    AddSeparator();
    //file
    AddTool(ID_SESSION_INCLUDE, wxT("Execute"),
            wxBitmap(run_script_xpm), wxNullBitmap, wxITEM_NORMAL,
            wxT("Execute script [Ctrl+X]"),
            wxT("Execute (include) script from file"));
    AddTool(ID_SESSION_DUMP, wxT("Dump"),
            wxBitmap(save_script_xpm), wxNullBitmap, wxITEM_NORMAL,
            wxT("Save session to file"),
            wxT("Dump current session to file"));
    AddSeparator();
    //data
    AddTool(ID_D_QLOAD, wxT("Load"), wxBitmap(open_data_xpm), wxNullBitmap,
            wxITEM_NORMAL, wxT("Load file [Ctrl+O]"),
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
    AddTool(ID_T_STRIP, wxT("Strip Bg"),
            wxBitmap(strip_bg_xpm), wxNullBitmap,  wxITEM_CHECK,
            wxT("Strip background"),
            wxT("(Un)Remove selected background from data"));
    EnableTool(ID_T_STRIP, (m == mmd_bg));
    AddSeparator();
    peak_choice = new wxChoice(this, ID_T_CHOICE);
    peak_choice->SetSize(130, -1);
    //peak_choice->SetToolTip(wxT("function"));
    AddControl (peak_choice);
    AddTool (ID_T_AUTO, wxT("add"), wxBitmap(add_peak_xpm), wxNullBitmap,
             wxITEM_NORMAL, wxT("auto-add"), wxT("Add peak automatically"));
    AddSeparator();
    //fit
    AddTool(ID_T_RUN, wxT("Run"),
            wxBitmap(run_fit_xpm), wxNullBitmap, wxITEM_NORMAL,
            wxT("Start fitting"), wxT("Start fitting sum to data"));
    AddTool(ID_T_UNDO, wxT("Undo"),
             wxBitmap(undo_fit_xpm), wxNullBitmap, wxITEM_NORMAL,
             wxT("Undo fitting"), wxT("Previous set of parameters"));
    //AddSeparator();
    //AddTool(ID_T_BAR, wxT("SideBar"),
    //        wxBitmap(right_pane_xpm), wxNullBitmap, wxITEM_CHECK,
    //        wxT("Datasets Pane"), wxT("Show/hide datasets pane"));
    Realize();
}

void FToolBar::OnPeakChoice(wxCommandEvent &event)
{
    if (frame)
        frame->peak_type_nr = event.GetSelection();
}

void FToolBar::update_peak_type(int nr, vector<string> const* peak_types)
{
    if (peak_types)
        updateControlWithItems(peak_choice, *peak_types);
    peak_choice->SetSelection(nr);
}

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
    wxCommandEvent dummy_cmd_event;

    switch (event.GetId()) {
        case ID_T_PZ:
            frame->OnPreviousZoom(dummy_cmd_event);
            break;
        case ID_T_STRIP: {
            BgManager* bgm = frame->plot_pane->get_bg_manager();
            if (event.IsChecked()) {
                if (bgm->can_strip()) {
                    bgm->strip_background();
                    frame->update_menu_recent_baselines();
                }
                else
                    ToggleTool(ID_T_STRIP, false);
            }
            else
                bgm->add_background();
            break;
        }
        case ID_T_RUN:
            ftk->exec(frame->get_datasets() + "fit");
            break;
        case ID_T_UNDO:
            ftk->exec("fit undo");
            break;
        case ID_T_AUTO:
            frame->OnSGuess(dummy_cmd_event);
            break;
        default:
            assert(0);
    }
}



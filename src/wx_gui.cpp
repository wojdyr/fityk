// This file is part of fityk program. Copyright (C) Marcin Wojdyr

// wxwindows headers, see wxwindows samples for description
#ifdef __GNUG__
#pragma implementation
#endif
#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include "common.h"
RCSID ("$Id$")

#include <wx/laywin.h>
#include <wx/sashwin.h>
#include <wx/filedlg.h>
#include <wx/resource.h>
#include <wx/valtext.h>
#include <wx/textdlg.h>
#if wxUSE_TOOLTIPS
    #include <wx/tooltip.h>
#endif
#include <wx/colordlg.h>
#include <wx/fontdlg.h>
#include <wx/cmdline.h> 
#include <wx/tipdlg.h>
#include <wx/statline.h>
#include <wx/html/htmlwin.h>
#include <wx/fs_zip.h>
#include <wx/printdlg.h>
#include <wx/image.h>
#include <wx/config.h>
#include <algorithm>

#include "wx_plot.h"
#include "wx_gui.h"
#include "wx_dlg.h"
#include "other.h"
#include "v_fit.h"
#include "data.h"
#include "sum.h"
#include "fzgbase.h" 
#include "ffunc.h"
#include "manipul.h"
#include "v_IO.h"
#ifdef USE_XTAL
    #include "crystal.h"
#endif //USE_XTAL

#ifndef __WXMSW__
#include "img/fityk.xpm"
#endif

//toolbars icons
#include "img/addpoint.xpm"  
#include "img/delpoint.xpm"  
#include "img/clear.xpm"     
#include "img/info.xpm"   
#include "img/importb.xpm"  
#include "img/exportb.xpm"  
#include "img/exportd.xpm"  
#include "img/plusbg.xpm"  
#include "img/spline.xpm"
#include "img/close.xpm"     
#include "img/autoadd.xpm"     
#include "img/clickadd.xpm"     
#include "img/delpeak.xpm"     
#include "img/test.xpm"
#include "img/left_2arrows.xpm"
#include "img/left_arrow.xpm"
#include "img/right_arrow.xpm"
#include "img/right_2arrows.xpm"
#include "img/height.xpm"
#include "img/center.xpm"
#include "img/fwhm.xpm"
#include "img/hwhm.xpm"
#include "img/aparam.xpm"
#include "img/gparam.xpm"
#include "img/pparam.xpm"
#include "img/freeze.xpm" 
#include "img/tree.xpm" 

#ifndef VERSION
#   define VERSION "unknown"
#endif

char* app_name = "fityk"; 
char* about_html = 
"<html> <body bgcolor=white text=white>                                  "
"  <table cellspacing=3 cellpadding=4 width=100%>                        "
"   <tr bgcolor=#101010 align=center> <td>                               "
"      <h1> fityk </h1>                                                  "
"      <h3> ver. " VERSION "</h3>                                        "
"      <h6> $Date$</h6>                            "
"   </td> </tr>                                                          "
"   <tr> <td bgcolor=#73A183>                                            "
"     <font color=black>                                                 "
"      <b><font size=+1> <p align = center>                              "
"       Copyright (C) 2001 - 2003 Marcin Wojdyr                          "
"      </font></b><p>                                                    "
"      <font size=-1>                                                    "
"       This program is free software; you can redistribute it           "
"       and/or modify it under the terms of the GNU General Public       "
"       License, version 2, as published by the Free Software Foundation;"
"      </font> <p>                                                       "
"       Feel free to send comments, questions, patches, bugreports       "
"       and even feature requests to author:                             "
"       Marcin Wojdyr &lt;wojdyr@if.pw.edu.pl&gt;                        "
"     </font>                                                            "
"   </td> </tr>                                                          "
"</table> </body> </html>                                                ";


using namespace std;
MyFrame *frame = NULL;
std::vector<fp> a_copy4plot; 
gui_IO my_gui_IO;

static const wxCmdLineEntryDesc cmdLineDesc[] = {
    { wxCMD_LINE_SWITCH, _T("h"), _T("help"), "show this help message",
                                wxCMD_LINE_VAL_NONE, wxCMD_LINE_OPTION_HELP },
    { wxCMD_LINE_SWITCH, "v", "verbose", "be verbose", wxCMD_LINE_VAL_NONE, 0 },
    { wxCMD_LINE_SWITCH, "q", "quiet",   "be quiet", wxCMD_LINE_VAL_NONE, 0 },
    { wxCMD_LINE_OPTION, "o", "output", "output file", wxCMD_LINE_VAL_NONE, 0 },
    { wxCMD_LINE_PARAM,  0, 0, "input file", wxCMD_LINE_VAL_STRING, 
                        wxCMD_LINE_PARAM_OPTIONAL|wxCMD_LINE_PARAM_MULTIPLE },
    { wxCMD_LINE_NONE, 0, 0, 0,  wxCMD_LINE_VAL_NONE, 0 }
};  

enum {
    ID_QUIT             = 44001,
    ID_H_MANUAL                ,
    ID_H_TIP                   ,
    ID_D_LOAD                  ,
    ID_D_XLOAD                 ,
    ID_D_INFO                  ,
    ID_D_DEVIATION             ,
    ID_D_RANGE                 ,
    ID_D_BACKGROUND            ,
    ID_D_CALIBRATE             ,
    ID_D_EXPORT                ,
    ID_D_SET                   ,
    ID_S_ADD                   ,
    ID_S_SIM_TB                ,
    ID_S_HISTORY               ,
    ID_S_INFO                  ,
    ID_S_REMOVE                ,
    ID_S_CHANGE                ,
    ID_S_VALUE                 ,
    ID_S_EXPORT                ,
    ID_S_SET                   ,
    ID_M_FINDPEAK              ,
    ID_M_SET                   ,
    ID_F_METHOD                ,
    ID_F_RUN                   ,
    ID_F_CONTINUE              ,
    ID_F_INFO                  ,
    ID_F_SET                   ,
    ID_F_M             = 44060 , // and a few next IDs
    ID_C_WAVELENGTH    = 44071 ,
    ID_C_ADD                   ,
    ID_C_INFO                  ,
    ID_C_REMOVE                ,
    ID_C_ESTIMATE              ,
    ID_C_SET                   ,
    ID_O_PLOT                  ,
    ID_O_LOG                   ,
    ID_O_RESET                 ,
    ID_PRINT                   ,
    ID_PRINT_SETUP             ,
    ID_PRINT_PREVIEW           ,
    ID_O_INCLUDE               ,
    ID_O_REINCLUDE             ,
    ID_O_WAIT                  ,
    ID_O_DUMP                  ,
    ID_O_SET                   ,
    
    ID_WINDOW_TOP1             ,
    ID_WINDOW_TOP2             ,
    ID_WINDOW_BOTTOM           ,
    ID_COMBO                   ,
    ID_OUTPUT_TEXT             ,

    ID_OUTPUT_C_BG             ,
    ID_OUTPUT_C_IN             ,
    ID_OUTPUT_C_OU             ,
    ID_OUTPUT_C_QT             ,
    ID_OUTPUT_C_WR             ,
    ID_OUTPUT_C                ,
    ID_OUTPUT_P_FONT           ,
    ID_OUTPUT_P_CLEAR          ,

    ID_bgt_add_pt       = 44200,
    ID_bgt_del_pt              ,
    ID_bgt_plus_bg             ,
    ID_bgt_clear               ,
    ID_bgt_info                ,
    ID_bgt_dist                ,
    ID_bgt_spline              ,
    ID_bgt_import_B            ,
    ID_bgt_export_B            ,
    ID_bgt_export_D            ,
    ID_bgt_close               ,

    ID_sst_adda                ,
    ID_sst_addm                ,
    ID_sst_addt                ,
    ID_sst_rm                  ,
    ID_sst_p            = 44250, //and next 10
    ID_sst_dd           = 44265, //from here
    ID_sst_d                   ,
    ID_sst_i                   ,
    ID_sst_ii                  ,
    ID_sst_pvtc                ,
    ID_sst_f                   , //till here is sequence
    ID_sst_tree                , 
    ID_sst_p_sc                ,
    ID_sst_close                
};

IMPLEMENT_APP(MyApp)
string get_full_path_of_help_file (const string &name);

bool MyApp::OnInit(void)
{
    main_manager = new MainManager; 

    //Parsing command line
    wxCmdLineParser cmdLineParser(cmdLineDesc, argc, argv);
    if (cmdLineParser.Parse(false) != 0) {
        cmdLineParser.Usage();
        return false;
    }
    if (cmdLineParser.Found("v"))
        verbosity++;
    if (cmdLineParser.Found("q"))
        verbosity--;
    for (unsigned int i = 0; i < cmdLineParser.GetParamCount(); i++) {
        wxString par = cmdLineParser.GetParam(i);
        file_I_stdout_O f_IO;
        my_IO = &f_IO;
        my_IO->start(par.c_str());
    }

    //global settings
#if wxUSE_TOOLTIPS
    wxToolTip::Enable (true);
    wxToolTip::SetDelay (500);
#endif
    wxConfig::DontCreateOnDemand();
    conf_filename = wxString(".fityk") + wxFILE_SEP_PATH + "config";
    alt_conf_filename = wxString(".fityk") + wxFILE_SEP_PATH 
                                                                + "alt-config";
    wxConfig *config = new wxConfig("", "", conf_filename, "", 
                                    wxCONFIG_USE_LOCAL_FILE);
    wxConfig::Set (config);

    // Create the main frame window
    frame = new MyFrame(NULL, -1, app_name, 
                        wxDEFAULT_FRAME_STYLE | wxHSCROLL | wxVSCROLL);

    frame->plot->set_scale();//workaround on problem with diff plot scale
    clear_buffered_sum();
    frame->Show(true);
    frame->output_win->read_settings(); //it does not work earlier
    SetTopWindow(frame);
    my_IO = &my_gui_IO;

    if (read_bool_from_config(wxConfig::Get(), "/TipOfTheDay/ShowAtStartup", 
                              false)) {
        wxCommandEvent dummy;
        frame->OnTipOfTheDay(dummy);
    }

    return true;
}

int MyApp::OnExit()
{ 
    delete main_manager; 
    delete wxConfigBase::Set((wxConfigBase *) NULL);
    return 0;
}

BEGIN_EVENT_TABLE(MyFrame, wxFrame)
    EVT_SIZE (                  MyFrame::OnSize)

    EVT_MENU (ID_D_LOAD,        MyFrame::OnDLoad)   
    EVT_MENU (ID_D_XLOAD,       MyFrame::OnDXLoad)   
    EVT_MENU (ID_D_INFO,        MyFrame::OnDInfo)
    EVT_MENU (ID_D_DEVIATION  , MyFrame::OnDDeviation)  
    EVT_MENU (ID_D_RANGE,       MyFrame::OnDRange)
    EVT_MENU (ID_D_BACKGROUND,  MyFrame::OnDBackground)  
    EVT_MENU (ID_D_CALIBRATE,   MyFrame::OnDCalibrate)  
    EVT_MENU (ID_D_EXPORT,      MyFrame::OnDExport) 
    EVT_MENU (ID_D_SET,         MyFrame::OnDSet) 

    EVT_MENU (ID_S_HISTORY,     MyFrame::OnSHistory)    
    EVT_MENU (ID_S_SIM_TB,      MyFrame::OnSSimple)    
    EVT_MENU (ID_S_INFO,        MyFrame::OnSInfo)    
    EVT_MENU (ID_S_ADD,         MyFrame::OnSAdd)    
    EVT_MENU (ID_S_REMOVE,      MyFrame::OnSRemove)    
    EVT_MENU (ID_S_CHANGE,      MyFrame::OnSChange)    
    EVT_MENU (ID_S_VALUE,       MyFrame::OnSValue)    
    EVT_MENU (ID_S_EXPORT,      MyFrame::OnSExport)    
    EVT_MENU (ID_S_SET,         MyFrame::OnSSet)    

    EVT_MENU (ID_M_FINDPEAK,    MyFrame::OnMFindpeak)
    EVT_MENU (ID_M_SET,         MyFrame::OnMSet)

    EVT_MENU (ID_F_METHOD,      MyFrame::OnFMethod)    
    EVT_MENU_RANGE (ID_F_M+0, ID_F_M+8, MyFrame::OnFOneOfMethods)    
    EVT_MENU (ID_F_RUN,         MyFrame::OnFRun)    
    EVT_MENU (ID_F_CONTINUE,    MyFrame::OnFContinue)    
    EVT_MENU (ID_F_INFO,        MyFrame::OnFInfo)    
    EVT_MENU (ID_F_SET,         MyFrame::OnFSet)    

#ifdef USE_XTAL
    EVT_MENU (ID_C_WAVELENGTH,  MyFrame::OnCWavelength)    
    EVT_MENU (ID_C_ADD,         MyFrame::OnCAdd)    
    EVT_MENU (ID_C_INFO,        MyFrame::OnCInfo)    
    EVT_MENU (ID_C_REMOVE,      MyFrame::OnCRemove)    
    EVT_MENU (ID_C_ESTIMATE,    MyFrame::OnCEstimate)    
    EVT_MENU (ID_C_SET,         MyFrame::OnCSet)    
#endif

    EVT_MENU (ID_O_PLOT,        MyFrame::OnOPlot)    
    EVT_MENU (ID_O_LOG,         MyFrame::OnOLog)    
    EVT_MENU (ID_O_RESET,       MyFrame::OnO_Reset)    
    EVT_MENU (ID_O_INCLUDE,     MyFrame::OnOInclude)    
    EVT_MENU (ID_O_REINCLUDE,   MyFrame::OnOReInclude)    
    EVT_MENU (ID_PRINT,         MyFrame::OnPrint)
    EVT_MENU (ID_PRINT_SETUP,   MyFrame::OnPrintSetup)
    EVT_MENU (ID_PRINT_PREVIEW, MyFrame::OnPrintPreview)
    EVT_MENU (ID_O_WAIT,        MyFrame::OnOWait)    
    EVT_MENU (ID_O_DUMP,        MyFrame::OnODump)    
    EVT_MENU (ID_O_SET,         MyFrame::OnOSet)    

    EVT_MENU (ID_H_MANUAL,      MyFrame::OnShowHelp)
    EVT_MENU (ID_H_TIP,         MyFrame::OnTipOfTheDay)
    EVT_MENU (wxID_ABOUT,       MyFrame::OnAbout)
    EVT_MENU (ID_QUIT,          MyFrame::OnQuit)

    EVT_SASH_DRAGGED_RANGE (ID_WINDOW_TOP1, ID_WINDOW_BOTTOM, 
                                MyFrame::OnSashDrag)
    EVT_UPDATE_UI (ID_F_METHOD, MyFrame::OnFMethod)
    EVT_KEY_DOWN  (             MyFrame::OnKeyDown)
END_EVENT_TABLE()

    // Define my frame constructor
MyFrame::MyFrame(wxWindow *parent, const wxWindowID id, const wxString& title, 
                 const long style)
    : wxFrame(parent, id, title, wxDefaultPosition, wxDefaultSize, style), 
      dxload_dialog(0), plot_window(0), aux_window(0), bottom_window(0), 
      print_data(new wxPrintData), page_setup_data(new wxPageSetupData),
#ifdef __WXMSW__
      help()
#else
      help(wxHF_TOOLBAR|wxHF_CONTENTS|wxHF_INDEX|wxHF_SEARCH|wxHF_PRINT
           |wxHF_MERGE_BOOKS)
#endif
{
    // Load icon and bitmap
    SetIcon (wxICON (fityk));

    //window with main plot
    wxSashLayoutWindow* win = new wxSashLayoutWindow(this, ID_WINDOW_TOP1, 
            wxDefaultPosition, wxDefaultSize, wxNO_BORDER | wxSW_3D);
    //win->SetDefaultSize(wxSize(-1, main_height));
    win->SetOrientation(wxLAYOUT_HORIZONTAL);
    win->SetAlignment(wxLAYOUT_TOP);
    //win->SetBackgroundColour(wxColour(150, 0, 0));
    win->SetSashVisible(wxSASH_BOTTOM, true);
    plot_window = win;
    plot = new MainPlot (plot_window, plot_common);

    // window with diff plot
    win = new wxSashLayoutWindow(this, ID_WINDOW_TOP2, 
            wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxSW_3D);
    //win->SetDefaultSize(wxSize(-1, aux_height));
    win->SetOrientation(wxLAYOUT_HORIZONTAL);
    win->SetAlignment(wxLAYOUT_TOP);
    //win->SetBackgroundColour(wxColour(10, 10, 50));
    win->SetSashVisible(wxSASH_BOTTOM, true);
    aux_window = win;
    diff_plot = new DiffPlot (aux_window, plot_common);

    //window with input line (combo box) and output wxTextCtrl
    win = new wxSashLayoutWindow(this, ID_WINDOW_BOTTOM, 
                                wxDefaultPosition, wxDefaultSize, 
                                wxNO_BORDER|wxSW_3D);
    //win->SetDefaultSize(wxSize(-1, 100));
    win->SetOrientation(wxLAYOUT_HORIZONTAL);
    win->SetAlignment(wxLAYOUT_BOTTOM);
    //win->SetBackgroundColour(wxColour(100, 100, 100));
    bottom_window = win;
    
    wxPanel *io_panel = new wxPanel (bottom_window, -1);
    wxBoxSizer *io_sizer = new wxBoxSizer (wxVERTICAL);

    // wxTextCtrl which displays output of commands
    output_win = new Output_win (io_panel, ID_OUTPUT_TEXT);
    io_sizer->Add (output_win, 1, wxEXPAND);

    // MyCombo - wxComboBox used for user keybord input
    //wxString input_choices[] = { /*"help"*/ };
    input_combo = new MyCombo (io_panel, ID_COMBO, "",
                               wxDefaultPosition, wxDefaultSize, 
                               0, 0,//input_choices, 
                               wxCB_DROPDOWN|wxWANTS_CHARS|
                               wxTE_PROCESS_ENTER|wxTE_PROCESS_TAB);
    io_sizer->Add (input_combo, 0, wxEXPAND);

    io_panel->SetAutoLayout (true);
    io_panel->SetSizer (io_sizer);
    io_sizer->Fit (io_panel);
    io_sizer->SetSizeHints (io_panel);

    set_menubar();

    //status bar
    wxStatusBar *sb = CreateStatusBar();
    int widths[2] = { -1, 120 };
    sb->SetFieldsCount (2, widths);

    read_settings();

    wxFileSystem::AddHandler(new wxZipFSHandler);
    wxImage::AddHandler(new wxPNGHandler);
    string help_file = "fitykhelp.htb";
    string help_path = get_full_path_of_help_file(help_file); 
    string help_path_no_exten = help_path.substr(0, help_path.size() - 4);
    help.Initialize(help_path_no_exten.c_str());
    //bool r = help.AddBook(help_path.c_str());
    //if (!r)
    //    wxMessageBox(("Couldn't open help file:\n " + help_path).c_str());
    // help.UseConfig(    );
    // help.SetTempDir(    );
    input_combo->SetFocus();
}

MyFrame::~MyFrame() 
{
    delete print_data;
    delete page_setup_data;
}

void MyFrame::read_settings()
{
    // restore frame position and size
    wxConfigBase *cf = wxConfig::Get();
    cf->SetPath("/Frame");
    int x = cf->Read("x", 50),
        y = cf->Read("y", 50),
        w = cf->Read("w", 500),
        h = cf->Read("h", 400);
    Move(x, y);
    SetClientSize(w, h);

    int plot_height = cf->Read("PlotWinHeight", 200);
    if (plot_window) plot_window->SetDefaultSize (wxSize(-1, plot_height));
    int aux_height = cf->Read("AuxWinHeight", 70);
    if (aux_window) aux_window->SetDefaultSize(wxSize(-1, aux_height));
    int bot_height = cf->Read ("BotWinHeight", 100);
    if (bottom_window) bottom_window->SetDefaultSize(wxSize(-1, bot_height));
    if (plot)
        set_mode (static_cast<Plot_mode_enum>(cf->Read ("/mode", 
                                               static_cast<long>(norm_pm_ty))));
}

void MyFrame::read_all_settings()
{
    output_win->read_settings();
    plot->read_settings();
    diff_plot->read_settings();
    read_settings();
}

void MyFrame::OnQuit(wxCommandEvent& WXUNUSED(event))
{
    Close(true);
}

void MyFrame::save_all_settings()
{
    output_win->save_settings();
    plot->save_settings();
    diff_plot->save_settings();
    wxConfigBase *cf = wxConfig::Get();
    cf->SetPath ("/Frame");
    save_size_and_position(cf, this);
    cf->Write ("PlotWinHeight", plot_window->GetClientSize().GetHeight()); 
    cf->Write ("AuxWinHeight", aux_window->GetClientSize().GetHeight()); 
    cf->Write ("BotWinHeight", bottom_window->GetClientSize().GetHeight()); 
    cf->Write ("/mode", plot->mode);
}

void MyFrame::set_menubar()
{
    // Make a menubar
    wxMenu* data_menu = new wxMenu;
    data_menu->Append (ID_D_LOAD,     "&Load File", "Load data from file");
    data_menu->Append (ID_D_XLOAD,    "&Load File (Custom)", 
                                    "Load data from file, with some options");
    data_menu->Append (ID_D_INFO,     "&Info", "Info about loaded data");
    data_menu->Append (ID_D_DEVIATION  , "Std. &Dev.", 
                                            "Change errors for data points");
    data_menu->Append (ID_D_RANGE,    "&Range", "Change active points");
    data_menu->AppendCheckItem (ID_D_BACKGROUND, "&Background Mode", 
                                                 "Define background");
    data_menu->Append (ID_D_CALIBRATE, "&Calibrate","Define calibration curve");
    data_menu->Append (ID_D_EXPORT,   "&Export", "Save data to file");
    data_menu->Append (ID_D_SET,      "&Settings", "Preferences and options");

    wxMenu* sum_menu = new wxMenu;
    sum_menu->Append (ID_S_HISTORY,   "&History", "Go back or forward"
                                                    " in change history");      
    sum_menu->AppendCheckItem (ID_S_SIM_TB,   "Si&mple Mode", 
                                              "Simple add/change toolbar");
    sum_menu->Check (ID_S_SIM_TB, false);
    sum_menu->Append (ID_S_INFO,      "&Info", "Info about fitted curve");      
    sum_menu->Append (ID_S_ADD,       "&Add", "Add parameter or function"); 
    sum_menu->Append (ID_S_REMOVE,    "&Remove/Freeze",
         "Remove parameter/function or avoid fitting of selected parameters");
    sum_menu->Append (ID_S_CHANGE,    "&Change", "Change value of a-parameter");
    sum_menu->Append (ID_S_VALUE,     "&Value", "Computes value of sum"
                                                    " or selected function");
    sum_menu->Append (ID_S_EXPORT,    "&Export", "Export fitted curve to file");
    sum_menu->Append (ID_S_SET,       "&Settings", "Preferences and options"); 

    wxMenu* manipul_menu = new wxMenu;
    manipul_menu->Append (ID_M_FINDPEAK, "&Find peak", "Search for a peak");
    manipul_menu->Append (ID_M_SET,   "&Settings", "Preferences and options");

    wxMenu* fit_menu = new wxMenu;
    wxMenu* fit_method_menu = new wxMenu;
    fit_method_menu->AppendRadioItem (ID_F_M+0, "&Levenberg-Marquardt", 
                                                "gradient based method");
    fit_method_menu->AppendRadioItem (ID_F_M+1, "Nelder-Mead &simplex", 
                                      "old, slow but reliable method");
    fit_method_menu->AppendRadioItem (ID_F_M+2, "&Genetic Algorithm", 
                                                "almost AI");
    fit_menu->Append (ID_F_METHOD,    "&Method", fit_method_menu, 
                                            "It influences commands below");
    fit_menu->AppendSeparator();
    fit_menu->Append (ID_F_RUN,       "&Run", "Start fitting sum to data");
    fit_menu->Append (ID_F_CONTINUE,  "&Continue", "Continue fitting");      
    fit_menu->Append (ID_F_INFO,      "&Info", "Info about current fit");      
    fit_menu->Append (ID_F_SET,       "&Settings", "Preferences and options"); 

#ifdef USE_XTAL
    wxMenu* crystal_menu = new wxMenu;
    crystal_menu->Append (ID_C_WAVELENGTH, "&Wavelength", 
                                                "Defines X-rays wavelengths");
    crystal_menu->Append (ID_C_ADD,      "&Add", "Add phase or plane");      
    crystal_menu->Append (ID_C_INFO,     "&Info", "Crystalography info");      
    crystal_menu->Append (ID_C_REMOVE,   "&Remove", "Remove phase or plane");
    crystal_menu->Append (ID_C_ESTIMATE, "&Estimate peak", 
                                            "Guess peak parameters");      
    crystal_menu->Append (ID_C_SET,    "&Settings", "Preferences and options");
#endif //USE_XTAL

    wxMenu* other_menu = new wxMenu;
    other_menu->Append (ID_O_INCLUDE,   "&Include file", 
                                            "Execute commands from a file");
    other_menu->Append (ID_O_REINCLUDE, "&Re-Include file", 
                "Reset & execute commands from the file included last time");
    other_menu->Enable (ID_O_REINCLUDE, false);
    other_menu->Append (ID_O_RESET,     "&Reset", "Reset current session");
    other_menu->AppendSeparator();
    other_menu->Append (ID_O_LOG,       "&Log to file", 
                                            "Start/stop logging to file");
    other_menu->Append (ID_O_DUMP,      "&Dump to file", 
                                         "Save current program state to file");
    other_menu->AppendSeparator();
    other_menu->Append (ID_PRINT,       "&Print...", "Print plots");
    other_menu->Append (ID_PRINT_SETUP, "Print Se&tup",
                                                    "Printer and page setup");
    other_menu->Append (ID_PRINT_PREVIEW, "Print Pre&view", "Preview"); 
    other_menu->AppendSeparator();
    other_menu->Append (ID_O_PLOT,      "&Replot", "Plot data and sum");
    other_menu->Append (ID_O_WAIT,      "&Wait", "What for?");
    other_menu->AppendSeparator();
    other_menu->Append (ID_O_SET,       "&Settings", "Preferences and options");

    wxMenu *help_menu = new wxMenu;
    help_menu->Append(ID_H_MANUAL, "&Manual", "User's Manual");
    help_menu->Append(ID_H_TIP, "&Tip of the day", "Show tip of the day");
    help_menu->Append(wxID_ABOUT, "&About...", "Show about dialog");
    help_menu->AppendSeparator();
    help_menu->Append(ID_QUIT, "&Exit", "Exit the program");

    wxMenuBar *menu_bar = new wxMenuBar(wxMENU_TEAROFF);
    menu_bar->Append (other_menu, "&File" );
    menu_bar->Append (data_menu, "&Data" );
    menu_bar->Append (sum_menu, "&Sum" );
    menu_bar->Append (manipul_menu, "&Manipulate");
    menu_bar->Append (fit_menu, "Fi&t" );
#ifdef USE_XTAL
    menu_bar->Append (crystal_menu, "&Xtal/Diffr" );
#endif //USE_XTAL
    menu_bar->Append (help_menu, "&Help");

    SetMenuBar(menu_bar);
}

void MyFrame::OnTipOfTheDay(wxCommandEvent& WXUNUSED(event))
{
    string tip_file = "fityk_tips.txt";
    string tip_path = get_full_path_of_help_file(tip_file); 
    int idx = wxConfig::Get()->Read ("/TipOfTheDay/idx", 0L); 
    wxTipProvider *tipProvider = wxCreateFileTipProvider(tip_path.c_str(), idx);
    bool showAtStartup = wxShowTip(this, tipProvider);
    idx = tipProvider->GetCurrentTip();
    delete tipProvider;
    wxConfig::Get()->Write ("/TipOfTheDay/ShowAtStartup", showAtStartup); 
    wxConfig::Get()->Write ("/TipOfTheDay/idx", idx); 
}

void MyFrame::OnShowHelp(wxCommandEvent& WXUNUSED(event))
{
        help.DisplayContents();
}

void MyFrame::OnAbout(wxCommandEvent& WXUNUSED(event))
{
    wxDialog* dlg = new wxDialog(this, -1, wxString("About..."));
    wxBoxSizer *topsizer = new wxBoxSizer(wxVERTICAL);
    wxHtmlWindow *html = new wxHtmlWindow (dlg, -1, 
                                           wxDefaultPosition, wxSize(300, 160), 
                                           wxHW_SCROLLBAR_NEVER);
    html->SetBorders(0);
    html->SetPage(about_html);
    html->SetSize (html->GetInternalRepresentation()->GetWidth(), 
                   html->GetInternalRepresentation()->GetHeight());
    topsizer->Add (html, 1, wxALL, 10);
    topsizer->Add (new wxStaticLine(dlg, -1), 0, wxEXPAND|wxLEFT|wxRIGHT, 10);
    wxButton *bu_ok = new wxButton (dlg, wxID_OK, "OK");
    bu_ok->SetDefault();
    topsizer->Add (bu_ok, 0, wxALL | wxALIGN_RIGHT, 15);
    dlg->SetAutoLayout(true);
    dlg->SetSizer(topsizer);
    topsizer->Fit(dlg);
    dlg->ShowModal();
    dlg->Destroy();
}

void MyFrame::not_implemented_menu_item (const std::string &command) const 
{
    wxMessageBox (("This menu item is only reminding about command " + command 
                    + ".\n If you don't know how to use this command,\n"
                    " look for it in the manual.").c_str(), 
                  command.c_str(), wxOK|wxICON_INFORMATION);
}

void MyFrame::OnSashDrag(wxSashEvent& event)
{
    if (event.GetDragStatus() == wxSASH_STATUS_OUT_OF_RANGE)
        return;
    switch (event.GetId())
    {
    case ID_WINDOW_TOP1:
        plot_window->SetDefaultSize(wxSize(-1, event.GetDragRect().height));
        break;
    case ID_WINDOW_TOP2:
        aux_window->SetDefaultSize(wxSize(-1, event.GetDragRect().height));
        break;
    case ID_WINDOW_BOTTOM:
        bottom_window->SetDefaultSize(wxSize(-1, event.GetDragRect().height));
        break;
    }
    wxLayoutAlgorithm layout;
    layout.LayoutFrame(this);
}

void MyFrame::OnSize(wxSizeEvent& WXUNUSED(event))
{
    wxLayoutAlgorithm layout;
    layout.LayoutFrame(this);
}

void MyFrame::OnKeyDown (wxKeyEvent& event)
{
    if (event.GetKeyCode() == WXK_ESCAPE) {
        if (plot) plot->cancel_mouse_press(); 
        if (diff_plot) diff_plot->cancel_mouse_left_press();
    }
    else if (event.GetKeyCode() == ' ' || event.GetKeyCode() == WXK_TAB) {
        frame->get_input_combo()->SetFocus();
    }
    else
        event.Skip();
}

void MyFrame::OnDLoad (wxCommandEvent& WXUNUSED(event))
{
    static wxString dir = ".";
    wxFileDialog fdlg (frame, "Load data from a file", dir, "",
                              "x y files (*.dat, *.xy, *.fio)"
                                          "|*.dat;*.DAT;*.xy;*.XY;*.fio;*.FIO" 
                              "|rit files (*.rit)|*.rit;*.RIT"
                              "|cpi files (*.cpi)|*.cpi;*.CPI"
                              "|mca files (*.mca)|*.mca;*.MCA"
                              "|Siemens/Bruker (*.raw)|*.raw;*.RAW"
                              "|all files (*)|*",
                              wxOPEN | wxFILE_MUST_EXIST);
    if (fdlg.ShowModal() == wxID_OK) 
        exec_command ("d.load '" + fdlg.GetPath() + "'");
    dir = fdlg.GetDirectory();
}

void MyFrame::OnDXLoad (wxCommandEvent& WXUNUSED(event))
{
    if (!dxload_dialog)
        dxload_dialog = new MyDXLoadDlg(this, -1);

    string fname = my_data->get_filename();
    if (fname.empty()) {
        wxCommandEvent dummy;
        dxload_dialog->OnChangeButton(dummy);
        if (dxload_dialog->filename.empty())
            return;
    }
    else
        dxload_dialog->set_filename(fname);

    if (dxload_dialog->ShowModal() == wxID_OK) {
        exec_command (dxload_dialog->get_command());
    }
}

void MyFrame::OnDInfo (wxCommandEvent& WXUNUSED(event))
{
    exec_command ("d.info");
}

void MyFrame::OnDDeviation  (wxCommandEvent& WXUNUSED(event))
{
    MyDStdDevDlg *dialog = new MyDStdDevDlg(this, -1);
    if (dialog->ShowModal() == wxID_OK) {
        exec_command (dialog->get_command());
    }
    dialog->Destroy();
}

void MyFrame::OnDRange (wxCommandEvent& WXUNUSED(event))
{
    MyDRangeDlg *dialog = new MyDRangeDlg(this, -1);
    dialog->ShowModal();
    dialog->Destroy();
}

void MyFrame::OnDBackground  (wxCommandEvent& WXUNUSED(event))
{
    set_mode (plot->mode == bg_pm_ty ? norm_pm_ty : bg_pm_ty); //switch on/off
}

void MyFrame::OnDCalibrate (wxCommandEvent& WXUNUSED(event))
{
    not_implemented_menu_item ("d.calibrate");
}
               
void MyFrame::OnDExport (wxCommandEvent& WXUNUSED(event))
{
    wxFileDialog fdlg (frame, "Export data to file", "", "",
                       "x y data (*.dat)|*.dat;*.DAT"
                       "|fityk file (*.fit)|*.fit;*.FIT",
                       wxSAVE | wxOVERWRITE_PROMPT);
    if (fdlg.ShowModal() == wxID_OK) 
        exec_command ("d.export '" + fdlg.GetPath() + "'");
}

void MyFrame::OnDSet (wxCommandEvent& WXUNUSED(event))
{
    OnXSet ("Data", 'd');
}

void MyFrame::OnSHistory      (wxCommandEvent& WXUNUSED(event))
{
    if (my_sum->count_a() == 0) {
        wxMessageBox ("no @-parameters -- no history", "no history",
                      wxOK|wxICON_ERROR);
        return;
    }
    SumHistoryDlg *dialog = new SumHistoryDlg (this, -1);
    dialog->ShowModal();
    dialog->Destroy();
}
            
void MyFrame::OnSSimple  (wxCommandEvent& WXUNUSED(event))
{
    set_mode (plot->mode == sim_pm_ty ? norm_pm_ty : sim_pm_ty);//switch on/off
}

void MyFrame::OnSAdd          (wxCommandEvent& WXUNUSED(event))
{
    FuncBrowserDlg *dialog = new FuncBrowserDlg(this, -1, 1);
    dialog->ShowModal();
    dialog->Destroy();
}
        
void MyFrame::OnSInfo         (wxCommandEvent& WXUNUSED(event))
{
    FuncBrowserDlg *dialog = new FuncBrowserDlg(this, -1, 0);
    dialog->ShowModal();
    dialog->Destroy();
}
         
void MyFrame::OnSRemove       (wxCommandEvent& WXUNUSED(event))
{
    FuncBrowserDlg *dialog = new FuncBrowserDlg(this, -1, 3);
    dialog->ShowModal();
    dialog->Destroy();
}
           
void MyFrame::OnSChange       (wxCommandEvent& WXUNUSED(event))
{
    FuncBrowserDlg *dialog = new FuncBrowserDlg(this, -1, 2);
    dialog->ShowModal();
    dialog->Destroy();
}
           
void MyFrame::OnSValue        (wxCommandEvent& WXUNUSED(event))
{
    FuncBrowserDlg *dialog = new FuncBrowserDlg(this, -1, 4);
    dialog->ShowModal();
    dialog->Destroy();
}
          
void MyFrame::OnSExport       (wxCommandEvent& WXUNUSED(event))
{
    wxFileDialog fdlg (frame, "Export curve to file", "", "",
                       "x y data (*.dat)|*.dat;*.DAT"
                       "|XFIT peak listing (*xfit.txt)|*xfit.txt;*XFIT.TXT" 
                       "|mathematic formula (*.formula)|*.formula"
                       "|parameters of peaks (*.peaks)|*.peaks"
                       "|fityk file (*.fit)|*.fit;*.FIT",
                       wxSAVE | wxOVERWRITE_PROMPT);
    if (fdlg.ShowModal() == wxID_OK) 
        exec_command ("s.export '" + fdlg.GetPath() + "'");
}
           
void MyFrame::OnSSet          (wxCommandEvent& WXUNUSED(event))
{
    OnXSet ("Sum", 's');
}
        
void MyFrame::OnMFindpeak     (wxCommandEvent& WXUNUSED(event))
{
    exec_command ("m.findpeak ");
}
        
void MyFrame::OnMSet          (wxCommandEvent& WXUNUSED(event))
{
    OnXSet ("Manipulations", 'm');
}
        

void MyFrame::OnFMethod       (wxCommandEvent& event)
{
    wxMenuBar* mb = GetMenuBar();
    assert (mb);
    mb->Check (ID_F_M + fitMethodsContainer->current_method_number(), true);
    event.Skip();
}
           
void MyFrame::OnFOneOfMethods (wxCommandEvent& event)
{
    int m = event.GetId() - ID_F_M;
    exec_command ("f.method " + S(fitMethodsContainer->symbol(m)));
}
           
void MyFrame::OnFRun          (wxCommandEvent& WXUNUSED(event))
{
    int r = wxGetNumberFromUser ("Run fitting method", 
                                 "Max. number of iterations", "Fit->Run", 
                                 my_fit->default_max_iterations, 0, 9999);
    if (r != -1)
        exec_command (("f.run " + S(r)).c_str());
}
        
void MyFrame::OnFContinue     (wxCommandEvent& WXUNUSED(event))
{
    int r = wxGetNumberFromUser ("Continue previous fitting", 
                                 "Max. number of iterations", "Fit->Continue", 
                                 my_fit->default_max_iterations, 0, 9999);
    if (r != -1)
        exec_command (("f.continue " + S(r)).c_str());
}
             
void MyFrame::OnFInfo         (wxCommandEvent& WXUNUSED(event))
{
    exec_command ("f.info");
}
         
void MyFrame::OnFSet          (wxCommandEvent& WXUNUSED(event))
{
    OnXSet ("Fit (" + my_fit->method + ")", 'f');
}
        

void MyFrame::OnCWavelength   (wxCommandEvent& WXUNUSED(event))
{
    not_implemented_menu_item ("c.wavelength");
}
               
void MyFrame::OnCAdd          (wxCommandEvent& WXUNUSED(event))
{
    not_implemented_menu_item ("c.add");
}
        
void MyFrame::OnCInfo         (wxCommandEvent& WXUNUSED(event))
{
    not_implemented_menu_item ("c.info");
}
         
void MyFrame::OnCRemove       (wxCommandEvent& WXUNUSED(event))
{
    not_implemented_menu_item ("c.remove");
}
           
void MyFrame::OnCEstimate     (wxCommandEvent& WXUNUSED(event))
{
    not_implemented_menu_item ("c.estimate");
}
             
void MyFrame::OnCSet          (wxCommandEvent& WXUNUSED(event))
{
    OnXSet ("Crystalography", 'c');
}
        

void MyFrame::OnOPlot         (wxCommandEvent& WXUNUSED(event))
{
    exec_command ("o.plot");
}
         
void MyFrame::OnOLog          (wxCommandEvent& WXUNUSED(event))
{
    wxFileDialog fdlg (frame, "Log to file", "", "",
                       "Log input to file (*.fit)|*.fit;*.FIT"
                       "|Log output to file (*.fit)|*.fit;*.FIT"
                       "|Log input & output (*.fit)|*.fit;*.FIT"
                       "|Stop logging to file |none.none",
                       wxSAVE);
    char m = my_other->get_log_mode();
    if (m == 'i')
        fdlg.SetFilterIndex(0);
    else if (m == 'o')
        fdlg.SetFilterIndex(1);
    else if (m == 'a')
        fdlg.SetFilterIndex(2);
    else if (m == 'n')
        fdlg.SetFilterIndex(3);
    else 
        assert (0);
    if (m != 'n')
        fdlg.SetPath(my_other->get_log_filename().c_str());
    if (fdlg.ShowModal() == wxID_OK) {
        int mode = fdlg.GetFilterIndex();
        char *modes = "ioan";
        assert(mode >= 0 && mode < 4);
        if (mode < 3)
            exec_command ("o.log " + wxString(S(modes[mode]).c_str()) 
                           + " '" + fdlg.GetPath() + "'");
        else
            exec_command ("o.log !"); 
    }
}

void MyFrame::OnO_Reset   (wxCommandEvent& WXUNUSED(event))
{
    int r = wxMessageBox ("Do you really want to reset program \nand lose "
                          "everything you have done in this session?", 
                          "Are you sure?", 
                          wxYES_NO | wxCANCEL | wxCENTRE | wxICON_QUESTION);
    if (r == wxYES)
        exec_command ("o.include !");
}
        
void MyFrame::OnOInclude      (wxCommandEvent& WXUNUSED(event))
{
    static wxString dir = ".";
    wxFileDialog fdlg (frame, "Execute commands from a file", dir, "",
                              "fityk file (*.fit)|*.fit;*.FIT|all files|*",
                              wxOPEN | wxFILE_MUST_EXIST);
    if (fdlg.ShowModal() == wxID_OK) {
        exec_command ("o.include '" + fdlg.GetPath() + "'");
        last_include_path = fdlg.GetPath();
        GetMenuBar()->Enable(ID_O_REINCLUDE, true);
    }
    dir = fdlg.GetDirectory();
}
            
void MyFrame::OnOReInclude    (wxCommandEvent& WXUNUSED(event))
{
    exec_command ("o.include ! '" + last_include_path + "'");
}
            
void MyFrame::OnOWait         (wxCommandEvent& WXUNUSED(event))
{
    int r = wxGetNumberFromUser ("Note: Wait command has a sense only "
            "in script", "Number of seconds to wait", "", 5, 0, 60);
    if (r != -1)
        exec_command (("o.wait " + S(r)).c_str());
}
         
void MyFrame::OnODump         (wxCommandEvent& WXUNUSED(event))
{
    wxFileDialog fdlg (frame, "Dump current program state to file as script", 
                                "", "", "fityk file (*.fit)|*.fit;*.FIT",
                       wxSAVE | wxOVERWRITE_PROMPT);
    if (fdlg.ShowModal() == wxID_OK) 
        exec_command ("o.dump '" + fdlg.GetPath() + "'");
}
         
void MyFrame::OnOSet          (wxCommandEvent& WXUNUSED(event))
{
    OnXSet ("Other", 'o');
}
        
void MyFrame::OnXSet (string name, char letter)
{
    DotSet* myset = set_class_p (letter);
    vector<string> ev, vv, tv;
    myset->expanp ("", ev);
    for (vector<string>::iterator i = ev.begin(); i != ev.end(); i++){
        string v;
        myset->getp_core (*i, v);
        vv.push_back (v);
        string t;
        myset->typep (*i, t);
        tv.push_back (t);
    }
    string nm =  "Preferences - " + name;
    MySetDlg *dialog = new MySetDlg(this, -1, nm.c_str(), ev, vv, tv, myset);
    if (dialog->ShowModal() == wxID_OK) {
        for (unsigned int i = 0; i < ev.size(); i++) {
            string s;
            wxTextCtrl *tctrl = wxDynamicCast (dialog->tc_v[i], wxTextCtrl);
            wxCheckBox *chbox = wxDynamicCast (dialog->tc_v[i], wxCheckBox);
            wxChoice *chic = wxDynamicCast (dialog->tc_v[i], wxChoice);
            if (tctrl)
                s = tctrl->GetValue().Trim(true).Trim(false) .c_str();
            else if (chbox)
                s = S(chbox->GetValue());
            else if (chic)
                s = chic->GetStringSelection().c_str();
            else
                assert (0);
            if (s != vv[i]) {
                string cm = S(letter) + ".set " + ev[i] + " = " + s;
                exec_command (cm.c_str());
            }
        }
    }
    dialog->Destroy();
}

class MyPreviewFrame : public wxPreviewFrame
{
public:
    MyPreviewFrame(wxPrintPreview* preview, wxFrame* parent) 
        : wxPreviewFrame (preview, parent, "Print Preview", 
                          wxDefaultPosition, wxSize(600, 550)) {}
    void CreateControlBar() { 
        m_controlBar = new wxPreviewControlBar(m_printPreview, 
                                        wxPREVIEW_PRINT|wxPREVIEW_ZOOM, this);
        m_controlBar->CreateButtons();
        m_controlBar->SetZoomControl(110);
    }
};

void MyFrame::OnPrintPreview(wxCommandEvent& WXUNUSED(event))
{
    // Pass two printout objects: for preview, and possible printing.
    wxPrintDialogData print_dialog_data (*print_data);
    wxPrintPreview *preview = new wxPrintPreview (new MyPrintout,new MyPrintout,
                                                  &print_dialog_data);
    if (!preview->Ok()) {
        delete preview;
        wxMessageBox ("There was a problem previewing.\nPerhaps your current "
                      "printer is not set correctly?", "Previewing", wxOK);
        return;
    }
    MyPreviewFrame *frame = new MyPreviewFrame (preview, this);
    frame->Centre(wxBOTH);
    frame->Initialize();
    frame->Show(true);
}

void MyFrame::OnPrintSetup(wxCommandEvent& WXUNUSED(event))
{
    (*page_setup_data) = *print_data;

    wxPageSetupDialog page_setup_dialog(this, page_setup_data);
    page_setup_dialog.ShowModal();

    (*print_data) = page_setup_dialog.GetPageSetupData().GetPrintData();
    (*page_setup_data) = page_setup_dialog.GetPageSetupData();

}

void MyFrame::OnPrint(wxCommandEvent& WXUNUSED(event))
{
    if (plot->get_bg_color() != *wxWHITE)
        if (wxMessageBox ("Plots will be printed on white background, \n"
                            "to save ink/toner.\n"
                            "Now background of your plot on screen\n"
                            "is not white, so visiblity of lines and points\n"
                            "can be different.\n Do you want to continue?",
                          "Are you sure?", 
                          wxYES_NO|wxCANCEL|wxICON_QUESTION) != wxYES)
            return;
    wxPrintDialogData print_dialog_data (*print_data);
    wxPrinter printer (&print_dialog_data);
    MyPrintout printout;
    bool r = printer.Print(this, &printout, true);
    if (r) 
        (*print_data) = printer.GetPrintDialogData().GetPrintData();
    else if (wxPrinter::GetLastError() == wxPRINTER_ERROR)
        wxMessageBox("There was a problem printing.\nPerhaps your current "
                     "printer is not set correctly?", "Printing", wxOK);
}

void MyFrame::set_mode (Plot_mode_enum md)
{
    if (md == plot->mode)
        return;
    wxToolBarBase *toolBar = GetToolBar();
    if (toolBar) {
        SetToolBar(0);
        toolBar->Destroy();
        simsum_tb = 0;
    }
    if (plot->mode == bg_pm_ty)
        exec_command ("o.plot -");
    plot->mode = md;
    if (md == bg_pm_ty) {
        exec_command ("o.plot +");
        SetStatusText ("Mode: Background");
        wxToolBar *toolBar = new BgToolBar (this, -1);
        SetToolBar (toolBar);
    }
    else if (md == sim_pm_ty) {
        SetStatusText ("Mode: Simple");
        wxToolBar *toolBar = simsum_tb = new SimSumToolBar (this, -1);
        SetToolBar (toolBar);
    }
    else if (md == norm_pm_ty) {
        SetStatusText ("Mode: Normal");
    }
    GetMenuBar()->Check (ID_D_BACKGROUND, md == bg_pm_ty);
    GetMenuBar()->Check (ID_S_SIM_TB, md == sim_pm_ty);
}

MyPrintout::MyPrintout() 
    : wxPrintout(my_data->get_filename().c_str()) {}

bool MyPrintout::OnPrintPage(int page)
{
    if (page != 1) return false;
    if (my_data->is_empty()) return false;
    wxDC *dc = GetDC();
    if (!dc) return false;

    // Set the scale and origin
    int w, h;
    int space = 20;
    int maxX = frame->plot->GetClientSize().GetWidth();
    int maxY = frame->plot->GetClientSize().GetHeight();
    int maxYsum = maxY + space + frame->diff_plot->GetClientSize().GetHeight();
    int marginX = 50, marginY = 50;
    dc->GetSize(&w, &h);
    fp scaleX = static_cast<fp>(w) / (maxX + 2 * marginX);
    fp scaleY = static_cast<fp>(h) / (maxYsum + 2 * marginY);
    fp actualScale = min (scaleX, scaleY);
    int posX = static_cast<int>((w - maxX * actualScale) / 2. + 0.5);
    int posY = static_cast<int>((h - maxYsum * actualScale) / 2. + 0.5);
    dc->SetUserScale (actualScale, actualScale);
    dc->SetDeviceOrigin (posX, posY);
    
    frame->plot->Draw(*dc);//printing main plot  
    posY += static_cast<int>((maxY + space) * actualScale);   
    dc->SetDeviceOrigin (posX, posY);
    frame->diff_plot->Draw(*dc); //printing auxiliary plot  
    return true;
}

//===============================================================
//                            Output_win
//===============================================================

BEGIN_EVENT_TABLE(Output_win, wxTextCtrl)
    EVT_RIGHT_DOWN (                      Output_win::OnRightDown)
    EVT_MENU_RANGE (ID_OUTPUT_C_BG, ID_OUTPUT_C_WR, Output_win::OnPopupColor)
    EVT_MENU       (ID_OUTPUT_P_FONT    , Output_win::OnPopupFont)
    EVT_MENU       (ID_OUTPUT_P_CLEAR   , Output_win::OnPopupClear)
    EVT_KEY_DOWN   (                      Output_win::OnKeyDown)
END_EVENT_TABLE()

Output_win::Output_win (wxWindow *parent, wxWindowID id, 
                        const wxPoint& pos, const wxSize& size)
    : wxTextCtrl(parent, id, "", pos, size,
                 wxTE_MULTILINE|wxTE_RICH|wxNO_BORDER|wxTE_READONLY)
{
    //GetFont().SetFamily(wxMODERN);
    fancy_dashes();
    //SetScrollbar (wxVERTICAL, 0, 5, 50);
}

void Output_win::fancy_dashes() {
    for (int i = 0; i < 16; i++) {
        SetDefaultStyle (wxTextAttr (wxColour(i * 16, i * 16, i * 16)));
        AppendText ("-");
    }
    AppendText ("\n");
}

void Output_win::read_settings()
{
    wxConfigBase *cf = wxConfig::Get();
    cf->SetPath("/OutputWin/Colors");
    text_color[os_ty_normal] = read_color_from_config(cf, "normal", 
                                                      wxColour(150, 150, 150));
    text_color[os_ty_warn] = read_color_from_config(cf, "warn", 
                                                    wxColour(200, 0, 0));
    text_color[os_ty_quot] = read_color_from_config(cf, "quot", 
                                                    wxColour(50, 50, 255));
    text_color[os_ty_input] = read_color_from_config(cf, "input", 
                                                     wxColour(0, 200, 0));
    bg_color = read_color_from_config(cf, "bg", wxColour(20, 20, 20));

    SetBackgroundColour (bg_color);
    SetDefaultStyle (wxTextAttr(text_color[os_ty_quot], bg_color));
    Refresh();
}

void Output_win::save_settings()
{
    wxConfigBase *cf = wxConfig::Get();
    cf->SetPath("/OutputWin/Colors");
    write_color_to_config (cf, "normal", text_color[os_ty_normal]);  
    write_color_to_config (cf, "warn", text_color[os_ty_warn]); 
    write_color_to_config (cf, "quot", text_color[os_ty_quot]); 
    write_color_to_config (cf, "input", text_color[os_ty_input]); 
    write_color_to_config (cf, "bg", bg_color); 
}

wxColour read_color_from_config(const wxConfigBase *config, const wxString& key,
                                 const wxColour& default_value)
{
    return wxColour (config->Read (key + "/Red", default_value.Red()), 
                     config->Read (key + "/Green", default_value.Green()), 
                     config->Read (key + "/Blue", default_value.Blue()));
}

void write_color_to_config (wxConfigBase *config, const wxString& key,
                            const wxColour& value)
{
    config->Write (key + "/Red", value.Red());
    config->Write (key + "/Green", value.Green());
    config->Write (key + "/Blue", value.Blue());
}

void save_size_and_position (wxConfigBase *config, wxWindow *window)
{
    //SetPath should be used before
    int x, y, w, h;
    window->GetClientSize(&w, &h);
    window->GetPosition(&x, &y);
    config->Write("x", (long) x);
    config->Write("y", (long) y);
    config->Write("w", (long) w);
    config->Write("h", (long) h);
}

void Output_win::append_text (const wxString& str, Output_style_enum style)
{
    SetDefaultStyle (wxTextAttr (text_color[style]));
    AppendText (str);
}

void Output_win::OnPopupColor (wxCommandEvent& event)
{
    int n = event.GetId();
    wxColour *col;
    if (n == ID_OUTPUT_C_BG)
        col = &bg_color;
    else if (n == ID_OUTPUT_C_OU)
        col = &text_color[os_ty_normal];
    else if (n == ID_OUTPUT_C_QT)
        col = &text_color[os_ty_quot];
    else if (n == ID_OUTPUT_C_WR)
        col = &text_color[os_ty_warn];
    else if (n == ID_OUTPUT_C_IN)
        col = &text_color[os_ty_input];
    else 
        return;
    wxColourData col_data;
    col_data.SetCustomColour (0, *col);
    col_data.SetColour (*col);
    wxColourDialog dialog (this, &col_data);
    if (dialog.ShowModal() == wxID_OK) {
        *col = dialog.GetColourData().GetColour();
        SetBackgroundColour (bg_color);
        SetDefaultStyle (wxTextAttr(wxNullColour, bg_color));
        Refresh();
    }
}

void Output_win::OnPopupFont (wxCommandEvent& WXUNUSED(event))
{
    wxFontData data; 
    data.SetInitialFont (GetDefaultStyle().GetFont());
    wxFontDialog dlg (this, &data);
    int r = dlg.ShowModal();
    if (r == wxID_OK) {
        wxFont f = dlg.GetFontData().GetChosenFont();
        SetDefaultStyle (wxTextAttr (wxNullColour, wxNullColour, f));
        Refresh();
    }
}

void Output_win::OnPopupClear (wxCommandEvent& WXUNUSED(event))
{
    Clear();
    fancy_dashes();
}

    
void Output_win::OnRightDown (wxMouseEvent& event)
{
    wxMenu popup_menu ("output text menu");

    wxMenu *color_menu = new wxMenu;
    color_menu->Append (ID_OUTPUT_C_BG, "&Background");
    color_menu->Append (ID_OUTPUT_C_IN, "&Input");
    color_menu->Append (ID_OUTPUT_C_OU, "&Output");
    color_menu->Append (ID_OUTPUT_C_QT, "&Quotation");
    color_menu->Append (ID_OUTPUT_C_WR, "&Warning");
    popup_menu.Append  (ID_OUTPUT_C   , "&Color", color_menu);

    popup_menu.Append  (ID_OUTPUT_P_FONT, "&Font");
    popup_menu.Append  (ID_OUTPUT_P_CLEAR, "Clea&r");

    PopupMenu (&popup_menu, event.GetX(), event.GetY());
}

void Output_win::OnKeyDown (wxKeyEvent& event)
{
    if (event.GetKeyCode() == ' ' || event.GetKeyCode() == WXK_TAB) {
        frame->get_input_combo()->SetFocus();
    }
    else
        event.Skip();
}

//===============================================================
//                            combo
//===============================================================

BEGIN_EVENT_TABLE(MyCombo, wxComboBox)
    EVT_KEY_DOWN (MyCombo::OnKeyDown)
END_EVENT_TABLE()

void MyCombo::OnKeyDown (wxKeyEvent& event)
{
    const int n_list_items = 15;
    if (event.m_keyCode == WXK_RETURN || event.m_keyCode == WXK_NUMPAD_ENTER) {
        wxString s = GetValue().Trim();
        if (s.IsEmpty())
            return;
        frame->SetStatusText (s);
        // changing drop-down list
        vector<wxString> list;
        list.push_back(s);
        int n = std::min (n_list_items, GetCount() + 1);
        for (int i = 0; i < n - 1; i++)
            list.push_back(GetString(i));
        Clear();
        for (vector<wxString>::iterator i = list.begin(); i != list.end(); i++)
            Append (*i);
        SetValue("");

        //displaying and executing command
        exec_command (s);
    }
    else if (event.m_keyCode == WXK_TAB)
        frame->output_win->SetFocus();
    else
        event.Skip();
}

const char input_prompt[] = "=-> ";

void exec_command (const string& s)
{
    //TODO const int max_lines_in_output_win = 1000;
    string out_s = input_prompt + s + "\n";
    if (!strncmp(s.c_str(), "o.plot", 6))
        frame->SetStatusText((input_prompt + s).c_str());
    else
        frame->output_win->append_text (out_s.c_str(), os_ty_input);
    wxBusyCursor wait;
    bool r = parser(s);
    if (!r)
        frame->Close(true);
}

void exec_command (const wxString& s) { exec_command (S(s.c_str())); }
void exec_command (const char* s) { exec_command (wxString(s)); }

//=====================    set  dialog    ==================
MySetDlg::MySetDlg(wxWindow* parent, const wxWindowID id, const wxString& title,
             vector<string>& names, vector<string>& vals, vector<string>& types,
             DotSet* myset)
    :  wxDialog(parent, id, title), opt_names(names), opt_values(vals)
{
    const int max_rows = 10;
    assert (names.size() == vals.size());
    wxBoxSizer *sizer0 = new wxBoxSizer (wxVERTICAL);
    wxStaticText* st = new wxStaticText (this, -1, title);
    sizer0->Add (st, 0, wxALL|wxALIGN_CENTER, 10);
    wxBoxSizer *sizer_H0 = new wxBoxSizer (wxHORIZONTAL);
    sizer0->Add (sizer_H0, 0, wxALL|wxALIGN_CENTER, 0);
    wxBoxSizer *sizer_V0 = 0;
    for (unsigned int i = 0; i < names.size(); i++) { 
        if (i % max_rows == 0) {
            sizer_V0 = new wxBoxSizer (wxVERTICAL);
            sizer_H0->Add (sizer_V0, 0, wxALL|wxALIGN_CENTER, 10);
        }
        wxBoxSizer *sizerH = new wxBoxSizer (wxHORIZONTAL);
        wxControl* tc;
        if (types[i].find ("bool") != string::npos) {//boolean type
            wxCheckBox* tc_ = new wxCheckBox (this, -1, names[i].c_str());
            tc_->SetValue (vals[i] != "0");
            tc = tc_;
        }
            //enumaration of strings
        else if (types[i].find ("enum") != string::npos) {
            vector<string> std_choices;
            myset->expand_enum (names[i], "", std_choices);
            const int max_choice_items = 20;//it should be enough
            assert (size(std_choices) < max_choice_items);
            wxString wx_choice[max_choice_items]; 
            for (unsigned int j = 0; j < std_choices.size(); j++)
                wx_choice[j] = std_choices[j].c_str();
            wxChoice *tc_ = new wxChoice (this, -1, wxDefaultPosition, 
                                          wxDefaultSize,  
                                          std_choices.size(), wx_choice);
            tc_->SetStringSelection (vals[i].c_str());
            tc = tc_;
            wxStaticText* st = new wxStaticText (this,-1, names[i].c_str());
            sizerH->Add (st, 0, wxALL|wxALIGN_LEFT, 5);
        }
        else { //another type
            wxStaticText* st = new wxStaticText (this,-1, names[i].c_str());
            sizerH->Add (st, 0, wxALL|wxALIGN_LEFT, 5);
            tc = new wxTextCtrl (this, -1, vals[i].c_str(), 
                                        wxDefaultPosition, wxSize(50, -1));
#if wxUSE_TOOLTIPS
            tc->SetToolTip (types[i].c_str()); 
#endif
        }
        sizerH->Add (tc, 0, wxALL|wxALIGN_LEFT, 0);
        tc_v.push_back(tc);
        sizer_V0->Add (sizerH, 0, wxALL|wxALIGN_LEFT, 5);
    }
    wxBoxSizer *sizerH = new wxBoxSizer (wxHORIZONTAL);
    wxButton *btOk = new wxButton (this, wxID_OK, "OK");
    sizerH->Add (btOk, 0, wxALL|wxALIGN_CENTER, 5 );
    wxButton *btCancel = new wxButton (this, wxID_CANCEL, "Cancel");
    sizerH->Add (btCancel, 0, wxALL|wxALIGN_CENTER, 5 );
    sizer0->Add (sizerH, 0, wxALL|wxALIGN_CENTER, 5);
    SetAutoLayout (true);
    SetSizer (sizer0);
    sizer0->Fit (this);
    sizer0->SetSizeHints (this);
}


//===============================================================
//                    BgToolBar (Background Palette)
//===============================================================

BEGIN_EVENT_TABLE (BgToolBar, wxToolBar)
    EVT_TEXT_ENTER         (ID_bgt_dist    , BgToolBar::OnDistText   )
    EVT_COMMAND_KILL_FOCUS (ID_bgt_dist    , BgToolBar::OnDistText   )
    EVT_TOOL               (ID_bgt_add_pt  , BgToolBar::OnAddPoint   )     
    EVT_TOOL               (ID_bgt_del_pt  , BgToolBar::OnDelPoint   )     
    EVT_TOOL               (ID_bgt_plus_bg , BgToolBar::OnPlusBg     )     
    EVT_TOOL               (ID_bgt_clear   , BgToolBar::OnClearBg    )     
    EVT_TOOL               (ID_bgt_info    , BgToolBar::OnInfoBg     )     
    EVT_TOOL               (ID_bgt_spline  , BgToolBar::OnSplineBg   )     
    EVT_TOOL               (ID_bgt_import_B, BgToolBar::OnImportB    )
    EVT_TOOL               (ID_bgt_export_B, BgToolBar::OnExportB    )
    EVT_TOOL               (ID_bgt_export_D,   MyFrame::OnDExport    )
    EVT_TOOL               (ID_bgt_close   , BgToolBar::OnReturnToNormal)   
END_EVENT_TABLE()

BgToolBar::BgToolBar (wxFrame *parent, wxWindowID id)
        : wxToolBar (parent, id, wxDefaultPosition, wxDefaultSize,
                     wxNO_BORDER | /*wxTB_FLAT |*/ wxTB_DOCKABLE) 
{
    SetMargins (4, 4);
    AddTool (ID_bgt_add_pt, "", wxBitmap(addpoint_xpm), wxNullBitmap, 
             wxITEM_NORMAL, "Add background point", 
             "Info - how to add background points");
    AddTool (ID_bgt_del_pt, "", wxBitmap(delpoint_xpm), wxNullBitmap, 
             wxITEM_NORMAL, "Delete background point", 
             "Info - how to delete background points");
    AddTool (ID_bgt_clear, "", wxBitmap(clear_xpm), wxNullBitmap, 
             wxITEM_NORMAL, "Clear background", 
             "Delete all background points");
    AddTool (ID_bgt_info, "", wxBitmap(info_xpm), wxNullBitmap, 
             wxITEM_NORMAL, "Background info", 
             "Info about background");
    AddSeparator();
    AddTool (ID_bgt_plus_bg, "", wxBitmap(plusbg_xpm), wxNullBitmap,  
             wxITEM_CHECK, "Plot with/without background",
             "Draw plot with added (or not) background");
    ToggleTool (ID_bgt_plus_bg, my_other->plus_background);
    AddTool (ID_bgt_spline, "", wxBitmap(spline_xpm), wxNullBitmap, 
             wxITEM_CHECK, "Spline", 
             "Background interpolation: spline vs polyline");
    ToggleTool (ID_bgt_spline, my_data->spline_background[bg_ty]);
    AddSeparator();
    AddControl (new wxStaticText (this, -1, "min. dist:"));
    tc_dist = new wxTextCtrl (this, ID_bgt_dist, "",
                              wxDefaultPosition, wxSize(50, -1),
                              wxTE_PROCESS_ENTER, 
                              wxTextValidator (wxFILTER_NUMERIC, 0));
    tc_dist->SetValue (S(my_data->min_background_distance[bg_ty]).c_str());
#if wxUSE_TOOLTIPS
    tc_dist->SetToolTip ("minimal distance between background points."
                          "\nPress [Enter] to apply");
#endif
    AddControl (tc_dist);
    AddSeparator();
    AddTool (ID_bgt_import_B, "", wxBitmap(importb_xpm), wxNullBitmap, 
             wxITEM_NORMAL, "Import background", 
             "Import background from file");
    AddTool (ID_bgt_export_B, "", wxBitmap(exportb_xpm), wxNullBitmap, 
             wxITEM_NORMAL, "Export background", 
             "Export background to file");
    AddTool (ID_bgt_export_D, "", wxBitmap(exportd_xpm), wxNullBitmap, 
             wxITEM_NORMAL, "Export data", 
             "Export data without background");
    AddSeparator();
    AddTool (ID_bgt_close, "", wxBitmap(close_xpm), wxNullBitmap, 
             wxITEM_NORMAL, "Exit background mode", 
             "Return to normal mode");
    
    Realize();
}


void BgToolBar::OnReturnToNormal (wxCommandEvent& WXUNUSED(event))
{
    frame->set_mode (norm_pm_ty);
}
      
void BgToolBar::OnClearBg (wxCommandEvent& WXUNUSED(event))
{
    exec_command ("d.background !");
}

void BgToolBar::OnInfoBg (wxCommandEvent& WXUNUSED(event))
{
    exec_command ("d.background");
}

void BgToolBar::OnImportB (wxCommandEvent& WXUNUSED(event))
{
    wxMessageBox ("Sorry, background importing is not working yet.");
}

void BgToolBar::OnExportB (wxCommandEvent& WXUNUSED(event))
{
    wxFileDialog fdlg (frame, "Export data to file", "", "",
            "x y background (*.bck)|*.bck|All files (*)|*",
            wxSAVE | wxOVERWRITE_PROMPT);
    if (fdlg.ShowModal() == wxID_OK) 
        exec_command ("d.export b '" + fdlg.GetPath() + "'");
}

void BgToolBar::OnAddPoint (wxCommandEvent& WXUNUSED(event))
{
    wxMessageBox("To add backround point, \n" 
                     "just press left mouse button on plot", 
                 "How to add background point", 
                 wxOK | wxICON_INFORMATION, this);
}

void BgToolBar::OnDelPoint (wxCommandEvent& WXUNUSED(event))
{
    wxMessageBox("To delete background point, press Ctrl (or Alt) "
                     "and left mouse button \nnear to point that you want "
                     "to delete. \nThe distance from point (along x axis) "
                     "\nmust be smaller than \"min. dist.\"", 
                 "How to delete background point", 
                 wxOK | wxICON_INFORMATION, this);
}

void BgToolBar::OnPlusBg (wxCommandEvent& event)
{
    if (event.IsChecked())
        exec_command ("o.plot +");
    else
        exec_command ("o.plot -");
}

void BgToolBar::OnSplineBg (wxCommandEvent& event)
{
    if (event.IsChecked())
        exec_command ("d.set spline-background = 1; d.background .");
    else
        exec_command ("d.set spline-background = 0; d.background .");
}

void BgToolBar::OnDistText (wxCommandEvent& WXUNUSED(event))
{
    exec_command ("d.set min-background-points-distance = " 
                    + tc_dist->GetValue());
}


//===============================================================
//                    SimSumToolBar  -- Sum->Simple..
//===============================================================

BEGIN_EVENT_TABLE (SimSumToolBar, wxToolBar)
    EVT_TOOL               (ID_sst_adda,        SimSumToolBar::OnAddAuto     )
    EVT_TOOL               (ID_sst_rm,          SimSumToolBar::OnRmPeak      )
    EVT_TOOL_RANGE    (ID_sst_dd, ID_sst_ii,    SimSumToolBar::OnArrowTool   ) 
    EVT_TEXT_ENTER         (ID_sst_pvtc    ,    SimSumToolBar::OnArrowTool   )
    EVT_COMMAND_KILL_FOCUS (ID_sst_pvtc    ,    SimSumToolBar::OnArrowTool   )
    EVT_TOOL               (ID_sst_f       ,    SimSumToolBar::OnFreezeTool  )
    EVT_TOOL               (ID_sst_tree    ,    SimSumToolBar::OnTreeTool    )
    EVT_TOOL_RANGE (ID_sst_p, ID_sst_p + 10,    SimSumToolBar::OnParamTool   )
    EVT_TOOL_RCLICKED_RANGE (ID_sst_p, ID_sst_p+10, SimSumToolBar::OnRParamTool)
    EVT_SPINCTRL           (ID_sst_p_sc    ,    SimSumToolBar::OnPeakSpinC   )
    EVT_TOOL               (ID_sst_close   ,    BgToolBar::OnReturnToNormal  )
END_EVENT_TABLE()

SimSumToolBar::SimSumToolBar (wxFrame *parent, wxWindowID id)
        : wxToolBar (parent, id, wxDefaultPosition, wxDefaultSize,
                     wxNO_BORDER | /*wxTB_FLAT |*/ wxTB_DOCKABLE)
{
    SetMargins (4, 2);
    AddControl (new wxStaticText (this, -1, "Add: "));
    //type of peak -- wxChoice
    peak_choice = new wxChoice (this, -1);
    all_t = V_fzg::all_types(fType);
    for (vector<const z_names_type*>::const_iterator i = all_t.begin();
                                                         i != all_t.end(); i++)
        peak_choice->Append ((*i)->name.c_str());
    peak_choice->SetSelection(0);
    AddControl (peak_choice);
    //... add-manual add-auto test-toggle 
    AddTool (ID_sst_adda, "", wxBitmap(autoadd_xpm), wxNullBitmap, 
             wxITEM_NORMAL, "auto-add", "");
    AddTool (ID_sst_addm, "", wxBitmap(clickadd_xpm), wxNullBitmap, 
             wxITEM_CHECK, "click-add", "click here and than click on plot");
    AddTool (ID_sst_addt, "", wxBitmap(test_xpm), wxNullBitmap, 
             wxITEM_CHECK, "only test", "do not add peak, only test");
    AddSeparator();
    AddSeparator();

    AddControl (new wxStaticText (this, -1, "Change: ^"));
    // selected peak number
    peak_n_sc = new wxSpinCtrl (this, ID_sst_p_sc, "", 
                                wxDefaultPosition, wxSize(50, -1)); 
    AddControl (peak_n_sc);
    AddSeparator();
    //remove peak
    AddTool (ID_sst_rm, "", wxBitmap(delpeak_xpm), wxNullBitmap, 
             wxITEM_NORMAL, "remove", "remove peak");
    AddSeparator();
    /* parameters of selected peak  -- will be inserted later */
    AddSeparator();
    // "<<"  "<"  text-control ">" ">>" "F"
    AddTool (ID_sst_dd, "", wxBitmap(left_2arrows_xpm), wxNullBitmap, 
             wxITEM_NORMAL, "decrease (50%)", "");
    AddTool (ID_sst_d, "", wxBitmap(left_arrow_xpm), wxNullBitmap, 
             wxITEM_NORMAL, "decrease (5%)", "");
    par_val_tc = new wxTextCtrl (this, ID_sst_pvtc, "",
                                 wxDefaultPosition, wxSize(70, -1),
                                 wxTE_PROCESS_ENTER, 
                                 wxTextValidator (wxFILTER_NUMERIC, 0));
    AddControl (par_val_tc);
    AddTool (ID_sst_i, "", wxBitmap(right_arrow_xpm), wxNullBitmap, 
             wxITEM_NORMAL, "increase (5%)", "");
    AddTool (ID_sst_ii, "", wxBitmap(right_2arrows_xpm), wxNullBitmap, 
             wxITEM_NORMAL, "increase (50%)", "");
    AddTool (ID_sst_f, "", wxBitmap(freeze_xpm), wxNullBitmap,
             wxITEM_CHECK, "freeze/thaw", "frozen parameter is not fitted");
    AddTool (ID_sst_tree, "", wxBitmap(tree_xpm), wxNullBitmap,
             wxITEM_NORMAL, "show tree", "Show tree of functions");
    AddSeparator();
    AddTool (ID_sst_close, "", wxBitmap(close_xpm), wxNullBitmap, 
             wxITEM_NORMAL, "Exit simple mode", 
             "Return to normal mode");
    Realize();
    set_peakspin_value(0);
}

void SimSumToolBar::add_peak(fp height, fp ctr, fp hwhm)
{
    my_sum->use_param_a_for_value();
    fp center = ctr + my_sum->zero_shift(ctr);
    string stat = "Height: " + S(height) + " Ctr: " + S(center) 
                   + " HWHM: " + S(hwhm);
    int n = peak_choice->GetSelection();
    const f_names_type &f = V_f::f_names[n];
    string cmd = "s.add ^" + S(f.type);
    for (int i = 0; i < f.psize; i++) {
        string pname = f.pnames[i];
        if      (pname == "height") cmd += " ~" + S( height );
        else if (pname == "center") cmd += " ~" + S( center );
        else if (pname == "HWHM")   cmd += " ~" + S( hwhm   );
        else if (pname == "FWHM")   cmd += " ~" + S( 2*hwhm );
        else if (pname.find("width") < pname.size())   cmd += " ~" + S(hwhm); 
        else if (pname.find(':') < pname.size())
            cmd += " ~" + pname.substr(pname.find(':') + 1) + " ";
        else cmd += " ~0";
    }
    if (GetToolState (ID_sst_addt)) //only test
        frame->SetStatusText ((stat + " -> " + cmd).c_str());
    else {
        frame->SetStatusText (stat.c_str());
        exec_command (cmd);
    }
    set_peakspin_value (my_sum->fzg_size(fType) - 1);
}

void SimSumToolBar::add_peak_in_range (fp xmin, fp xmax)
{
    fp center, height, fwhm;
    bool r = my_manipul->estimate_peak_parameters ((xmax + xmin)/2, 
                                                   (xmax - xmin)/2, 
                                                   &center, &height, 0, &fwhm);
    if (r) add_peak (height, center, fwhm/2);
}

bool SimSumToolBar::left_button_clicked (fp x, fp y)
{
    if (GetToolState (ID_sst_addm)) { //add manually tool pressed 
        fp fwhm;
        bool r = my_manipul->estimate_peak_parameters (x, -1, 
                                                       0, 0, 0, &fwhm);
        if (r) add_peak (y, x, fwhm/2);
        return true;
    }
    else return false;
}

void SimSumToolBar::OnArrowTool (wxCommandEvent& event)
{
    if (!is_double (par_val_tc->GetValue().c_str())) {
        return;
    }
    fp p = strtod (par_val_tc->GetValue().c_str(), 0);
    switch (event.GetId()) {
        case ID_sst_dd:   p *= 0.50;  break;
        case ID_sst_d:    p *= 0.95;  break;
        case ID_sst_i:    p *= 1.05;  break;
        case ID_sst_ii:   p *= 1.50;  break;  
        case ID_sst_pvtc: /*nothing*/ break;
        default: assert(0);
    }
    par_val_tc->SetValue (S(p).c_str());
    exec_command ("s.change @"+ S(par_of_peak[selected_param].a()) +" "+ S(p));
}

void SimSumToolBar::OnFreezeTool (wxCommandEvent& event)
{
    exec_command ("s.freeze " + S(event.IsChecked() ? "@" : "! @") 
                   + S(par_of_peak[selected_param].a()));
}

void SimSumToolBar::OnTreeTool (wxCommandEvent& WXUNUSED(event))
{
    FuncBrowserDlg *dialog = new FuncBrowserDlg(frame, -1, 2);
    dialog->show_expanded(peak_n_sc->GetValue(), selected_param);
    dialog->ShowModal();
    dialog->Destroy();
}

void SimSumToolBar::OnParamTool (wxCommandEvent& event)
{
    int pn = event.GetId() - ID_sst_p;
    parameter_was_selected (pn);
}

void SimSumToolBar::OnRParamTool (wxCommandEvent& event)
{
    OnParamTool (event);
    OnTreeTool (event);
}

void SimSumToolBar::OnPeakSpinC (wxSpinEvent& event) 
{
    int n = event.GetPosition();
    set_peakspin_value(n);
}

void SimSumToolBar::OnAddAuto (wxCommandEvent& WXUNUSED(event))
{
    fp center, height, fwhm;
    bool r = my_manipul->global_peakfind (&center, &height, 0, &fwhm);
    if (r) add_peak (height, center, fwhm/2);
}

void SimSumToolBar::OnRmPeak (wxCommandEvent& WXUNUSED(event))
{
    if (!peak_n_sc->IsEnabled())
        return;
    int n = peak_n_sc->GetValue();
    exec_command ("s.remove ^" + S(n));
    set_peakspin_value(0);
}

void SimSumToolBar::set_peakspin_value (int n)
{
    //set range, if sum doesn't contain any ^-functions, disable some tools
    int max = my_sum->fzg_size(fType) - 1; 
    bool en = (max >= 0);
    peak_n_sc->SetRange (0, en ? max : 0);
    peak_n_sc->Enable (en);
    EnableTool (ID_sst_rm, en);
    //set value 
    if (n < 0 && n > max)
        n = 0;
    peak_n_sc->SetValue (n);
    for (unsigned int i = 0; i < par_of_peak.size(); i++)
        DeleteTool (ID_sst_p + i);
    if (en == false) {
        par_of_peak.clear();
        return;
    }
    // if sum is not empty, select peak and prepare parameters of it
    const V_fzg *ptr =  my_sum->get_fzg(fType, n);
    const f_names_type* ft = static_cast<const f_names_type*>
                                        (V_fzg::type_info(fType, ptr->type));
    par_of_peak = ptr->copy_of_pags();
    for (unsigned int i = 0; i < par_of_peak.size(); i++) {
        string p = ft->pnames[i];    //remove colon and following chars
        string pname = p.substr(0, p.find(':'));
        wxBitmap img;
        if      (pname == "height") img = wxBitmap(height_xpm);
        else if (pname == "center") img = wxBitmap(center_xpm);
        else if (pname == "HWHM")   img = wxBitmap(hwhm_xpm);
        else if (pname == "FWHM")   img = wxBitmap(fwhm_xpm);
        else if (par_of_peak[i].is_a()) img = wxBitmap(aparam_xpm);
        else if (par_of_peak[i].is_p()) img = wxBitmap(pparam_xpm);
        else if (par_of_peak[i].is_g()) img = wxBitmap(gparam_xpm);
        else assert(0);

        InsertTool (12 + i, ID_sst_p + i, "", img, wxNullBitmap, 
                    wxITEM_RADIO, pname.c_str(), pname.c_str()); 
    }
    Realize();
    for (unsigned int i = 0; i < par_of_peak.size(); i++) 
        if (par_of_peak[i].is_p())
            EnableTool (ID_sst_p + i, false);
    parameter_was_selected (0);
    frame->plot->draw_selected_peaktop(n);
}

void SimSumToolBar::parameter_was_selected (int n)
{
    assert (0 <= n && n < size(par_of_peak));
    selected_param = n;
    bool simple = par_of_peak[n].is_a();
    fp v = par_of_peak[n].value (my_sum->current_a(), my_sum->gfuncs_vec());
    par_val_tc->SetValue (S(v).c_str());
    //EnableTool (ID_sst_pvtc, simple); //-- doesn't work
    par_val_tc->Enable(simple);
    for (int id = ID_sst_dd; id <= ID_sst_f; id++)
        EnableTool (id, simple);
    if (simple)
        ToggleTool (ID_sst_f, my_sum->is_frozen(par_of_peak[n].a()));
}

#ifndef HELP_DIR
#    define HELP_DIR "."
#endif

string get_full_path_of_help_file (const string &name)
{
    vector<string> possible_prefixes;
    possible_prefixes.push_back("");
    const char *argv0 = wxGetApp().argv[0];
    possible_prefixes.push_back(wxPathOnly(argv0).c_str());
    char *possible_paths[] = {
        HELP_DIR,
        ".",
        "..",
        "doc",
        "../doc",
        0
    };
    for (vector<string>::const_iterator pref = possible_prefixes.begin(); 
            pref != possible_prefixes.end(); pref++) {
        string pref_path = *pref;
        if (!pref_path.empty() && *(pref_path.end()-1) != wxFILE_SEP_PATH)
                pref_path += wxFILE_SEP_PATH;
        for (int i = 0; possible_paths[i]; i++) {
            string path = pref_path + possible_paths[i];
            if (!path.empty() && *(path.end()-1) != wxFILE_SEP_PATH)
                path += wxFILE_SEP_PATH;
            //mesg("Looking for " + name + " in " + path);
            path += name;
            if (wxFileExists(path.c_str())) return path;
        }
    }
    return name;
}


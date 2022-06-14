// This file is part of fityk program. Copyright 2001-2013 Marcin Wojdyr
// Licence: GNU General Public License ver. 2+

#include "about.h"
#include <boost/version.hpp> // BOOST_VERSION
#include <xylib/xylib.h> // get_version()
#include <wx/hyperlink.h>

#include "cmn.h" // pchar2wx, s2wx, GET_BMP
#include "fityk/common.h" // VERSION, HAVE_LIBNLOPT
#include "fityk/info.h" // embedded_lua_version()
#include "img/fityk96.h"

#if HAVE_LIBNLOPT
# include <nlopt.h>  // nlopt_version()
#endif

AboutDlg::AboutDlg(wxWindow* parent)
    : wxDialog(parent, -1, wxT("About Fityk"), wxDefaultPosition,
               wxDefaultSize, wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER)
{
    wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer *hsizer = new wxBoxSizer(wxHORIZONTAL);
    hsizer->Add(new wxStaticBitmap(this, -1, GET_BMP(fityk96)),
                wxSizerFlags().Centre().Border());
    wxBoxSizer *name_sizer = new wxBoxSizer(wxVERTICAL);
    wxStaticText *name = new wxStaticText(this, -1,
                                          wxT("fityk ") + pchar2wx(VERSION));
    name->SetFont(wxFont(24, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                         wxFONTWEIGHT_BOLD));
    name_sizer->Add(name, wxSizerFlags().Centre().Border());
    wxStaticText *desc = new wxStaticText(this, -1,
                            wxT("A curve fitting and data analysis program"));
    name_sizer->Add(desc, wxSizerFlags().Centre().Border(wxLEFT|wxRIGHT));
    wxString link = wxT("http://fityk.nieto.pl");
    wxHyperlinkCtrl *link_ctrl = new wxHyperlinkCtrl(this, -1, link, link);
    name_sizer->Add(link_ctrl, wxSizerFlags().Centre().Border());

    hsizer->Add(name_sizer, wxSizerFlags(1).Expand());
    sizer->Add(hsizer, wxSizerFlags());
    wxTextCtrl *txt = new wxTextCtrl(this, -1, wxT(""), wxDefaultPosition,
#ifdef __WXMSW__
                                     wxSize(-1,120),
#else
                                     wxSize(-1,160),
#endif
                         wxTE_MULTILINE|wxTE_RICH|wxNO_BORDER |wxTE_READONLY);
#ifdef __WXGTK__
    //wxColour bg_col = wxStaticText::GetClassDefaultAttributes().colBg;
    wxColour bg_col = name->GetDefaultAttributes().colBg;
#else
    wxColour bg_col = GetBackgroundColour();
#endif
    txt->SetBackgroundColour(bg_col);
    txt->SetDefaultStyle(wxTextAttr(wxNullColour, bg_col, *wxITALIC_FONT));
    txt->AppendText(wxT("powered by: ") wxVERSION_STRING);
    txt->AppendText(wxString::Format(", Boost %d.%d.%d",
                                     BOOST_VERSION / 100000,
                                     BOOST_VERSION / 100 % 1000,
                                     BOOST_VERSION % 100));
    txt->AppendText(wxString(", ") + fityk::embedded_lua_version());
#if HAVE_LIBNLOPT
    int nl_ver[3];
    nlopt_version(nl_ver, nl_ver+1, nl_ver+2);
    txt->AppendText(wxString::Format(", NLopt %d.%d.%d",
                                     nl_ver[0], nl_ver[1], nl_ver[2]));
#endif
    txt->AppendText(" and xylib " + pchar2wx(xylib_get_version()) + "\n");
    txt->SetDefaultStyle(wxTextAttr(wxNullColour, bg_col, *wxNORMAL_FONT));
    txt->AppendText(wxT("\nCopyright 2001 - 2022 Marcin Wojdyr\n\n"));
    txt->SetDefaultStyle(wxTextAttr(wxNullColour, bg_col, *wxSMALL_FONT));
    txt->AppendText(
   wxT("This program is free software; you can redistribute it and/or modify ")
   wxT("it under the terms of the GNU General Public License as published by ")
   wxT("the Free Software Foundation; either version 2 of the License, or ")
   wxT("(at your option) any later version.")
    );
    txt->ShowPosition(0);
    sizer->Add(txt, 1, wxLEFT|wxRIGHT|wxTOP|wxEXPAND|wxFIXED_MINSIZE, 10);

    //sizer->Add (new wxStaticLine(this, -1), 0, wxEXPAND|wxLEFT|wxRIGHT, 10);
    wxButton *button = new wxButton (this, wxID_CLOSE);
    SetEscapeId(wxID_CLOSE);
    //button->SetDefault();
    sizer->Add(button, wxSizerFlags().Right().Border());
    SetSizerAndFit(sizer);
}


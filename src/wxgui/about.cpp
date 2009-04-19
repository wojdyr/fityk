// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License version 2 or (at your option) 3
// $Id: dialogs.cpp 453 2009-03-23 14:25:24Z wojdyr $


#include "about.h"
#include <boost/spirit/version.hpp> // SPIRIT_VERSION
#include <xylib/xylib.h> // get_version()
#include <wx/hyperlink.h>

#include "cmn.h" // pchar2wx, s2wx, GET_BMP
#include "../common.h" // VERSION
#include "img/fityk96.h"


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
    wxString link = wxT("http://www.unipress.waw.pl/fityk/");
    wxHyperlinkCtrl *link_ctrl = new wxHyperlinkCtrl(this, -1, link, link);
    name_sizer->Add(link_ctrl, wxSizerFlags().Centre().Border());

    hsizer->Add(name_sizer, wxSizerFlags(1).Expand()); 
    sizer->Add(hsizer, wxSizerFlags());
    wxTextCtrl *txt = new wxTextCtrl(this, -1, wxT(""), 
                                     wxDefaultPosition, wxSize(-1,200), 
                         wxTE_MULTILINE|wxTE_RICH|wxNO_BORDER |wxTE_READONLY);
    txt->SetBackgroundColour(GetBackgroundColour());
    txt->SetDefaultStyle(wxTextAttr(wxNullColour, wxNullColour, 
                                    *wxITALIC_FONT));
    txt->AppendText(wxT("powered by ") wxVERSION_STRING wxT("\n"));
    txt->AppendText(wxString::Format(wxT("powered by Boost.Spirit %d.%d.%d\n"), 
                                       SPIRIT_VERSION / 0x1000,
                                       SPIRIT_VERSION % 0x1000 / 0x0100,
                                       SPIRIT_VERSION % 0x0100));
    txt->AppendText(wxT("powered by xylib ") + s2wx(xylib::get_version())
                    + wxT("\n"));
    txt->SetDefaultStyle(wxTextAttr(wxNullColour, wxNullColour, 
                                    *wxNORMAL_FONT));
    txt->AppendText(wxT("\nCopyright (C) 2001 - 2009 Marcin Wojdyr\n\n"));
    txt->AppendText(
   wxT("This program is free software; you can redistribute it and/or modify ")
   wxT("it under the terms of the GNU General Public License as published by ")
   wxT("the Free Software Foundation; either version 2 of the License, or ")
   wxT("(at your option) version 3.")
    );
    sizer->Add (txt, 1, wxALL|wxEXPAND|wxFIXED_MINSIZE, 5);

    //sizer->Add (new wxStaticLine(this, -1), 0, wxEXPAND|wxLEFT|wxRIGHT, 10);
    wxButton *button = new wxButton (this, wxID_CLOSE);
    SetEscapeId(wxID_CLOSE);
    //button->SetDefault();
    sizer->Add (button, wxSizerFlags().Right().Border());
    SetSizerAndFit(sizer);
}


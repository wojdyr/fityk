// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License version 2 or (at your option) 3
// $Id: dialogs.cpp 453 2009-03-23 14:25:24Z wojdyr $


#include "about.h"
#include <boost/spirit/version.hpp> // SPIRIT_VERSION
#include <xylib/xylib.h> // get_version()

#include "cmn.h" // pchar2wx, s2wx
#include "../common.h" // VERSION
#include "img/fityk.xpm"

BEGIN_EVENT_TABLE (AboutDlg, wxDialog)
    EVT_TEXT_URL (wxID_ANY, AboutDlg::OnTextURL)
END_EVENT_TABLE()

AboutDlg::AboutDlg(wxWindow* parent)
    : wxDialog(parent, -1, wxT("About Fityk"), wxDefaultPosition, 
               wxDefaultSize, wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER)
{
    wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(new wxStaticBitmap(this, -1, wxBitmap(fityk_xpm)),
               0, wxALIGN_CENTER|wxALL, 5);
    wxStaticText *name = new wxStaticText(this, -1, 
                                          wxT("fityk ") + pchar2wx(VERSION));
    name->SetFont(wxFont(18, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                         wxFONTWEIGHT_BOLD));
    sizer->Add(name, 0, wxALIGN_CENTER|wxALL, 5); 
    txt = new wxTextCtrl(this, -1, wxT(""), wxDefaultPosition, wxSize(400,250), 
                         wxTE_MULTILINE|wxTE_RICH2|wxNO_BORDER
                             |wxTE_READONLY|wxTE_AUTO_URL);
    txt->SetBackgroundColour(GetBackgroundColour());
    txt->SetDefaultStyle(wxTextAttr(wxNullColour, wxNullColour, wxNullFont,
                                    wxTEXT_ALIGNMENT_CENTRE));
    
    txt->AppendText(wxT("A curve fitting and data analysis program\n\n"));
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
    txt->SetDefaultStyle(wxTextAttr(*wxBLUE));
    txt->AppendText(wxT("http://www.unipress.waw.pl/fityk/\n\n"));
    txt->SetDefaultStyle(wxTextAttr(*wxBLACK));
    txt->AppendText(
   wxT("This program is free software; you can redistribute it and/or modify ")
   wxT("it under the terms of the GNU General Public License as published by ")
   wxT("the Free Software Foundation; either version 2 of the License, or ")
   wxT("(at your option) version 3.")
    );
    sizer->Add (txt, 1, wxALL|wxEXPAND|wxFIXED_MINSIZE, 5);
    //sizer->Add (new wxStaticLine(this, -1), 0, wxEXPAND|wxLEFT|wxRIGHT, 10);
    wxButton *bu_ok = new wxButton (this, wxID_OK, wxT("OK"));
    bu_ok->SetDefault();
    sizer->Add (bu_ok, 0, wxALL|wxEXPAND, 10);
    SetSizerAndFit(sizer);
}

void AboutDlg::OnTextURL(wxTextUrlEvent& event) 
{
    if (!event.GetMouseEvent().LeftDown()) {
        event.Skip();
        return;
    }
    long start = event.GetURLStart(),
         end = event.GetURLEnd();
    wxString url = txt->GetValue().Mid(start, end - start);
    wxLaunchDefaultBrowser(url);
}



/*
 * guiPreparation.h - Implementation for Preparation Window
 *
 * Copyright 2014 Thomas Buck <xythobuz@xythobuz.de
 */

#ifndef _GUI_PREPARATION_H
#define _GUI_PREPARATION_H

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/listbox.h>

class PreparationFrame: public wxFrame {
public:
    PreparationFrame(const wxString& title, const wxPoint& pos, const wxSize& size);

    void buttonPress(wxCommandEvent& event);
    void listChange(wxCommandEvent& event);
    void OnExit(wxCloseEvent& event);

    wxListBox* questions;
    wxListBox* players;
    wxButton* start;

    wxDECLARE_EVENT_TABLE();
};

enum {
    BUTTON_Start = wxID_HIGHEST + 1,
    LIST_Questions = wxID_HIGHEST + 2
};

#endif


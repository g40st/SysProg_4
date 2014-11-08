/*
 * guiPreparation.cpp - Implementation for Preparation Window
 *
 * Copyright 2014 Thomas Buck <xythobuz@xythobuz.de
 */

#include "guiApp.h"
#include "guiPreparation.h"

extern "C" {
#include "../gui/gui_interface.h"
}

wxBEGIN_EVENT_TABLE(PreparationFrame, wxFrame)
    EVT_CLOSE(PreparationFrame::OnExit)
    EVT_BUTTON(BUTTON_Start, PreparationFrame::buttonPress)
    EVT_LISTBOX(LIST_Questions, PreparationFrame::listChange)
wxEND_EVENT_TABLE()

PreparationFrame::PreparationFrame(const wxString& title, const wxPoint& pos, const wxSize& size)
        : wxFrame(NULL, wxID_ANY, title, pos, size) {

    wxPanel* panel = new wxPanel(this);

    wxStaticBox* boxLeft = new wxStaticBox(panel, wxID_ANY, "");
    wxStaticText* textLeft = new wxStaticText(boxLeft, wxID_ANY, "Fragekataloge:");
    questions = new wxListBox(boxLeft, LIST_Questions);
    questions->Enable(false);

    wxBoxSizer* sizerLeft = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* sizerLeft2 = new wxBoxSizer(wxVERTICAL);
    sizerLeft2->Add(textLeft, 0, wxEXPAND | wxALL, 10);
    sizerLeft2->Add(questions, 1, wxEXPAND | wxALL, 10);
    sizerLeft->Add(sizerLeft2, 1, wxEXPAND | wxALL, 10);
    boxLeft->SetSizer(sizerLeft);

    wxStaticBox* boxRight = new wxStaticBox(panel, wxID_ANY, "");
    wxStaticText* textRight = new wxStaticText(boxRight, wxID_ANY, "Angemeldete Benutzer:");
    players = new wxListBox(boxRight, wxID_ANY);

    wxBoxSizer* sizerRight = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* sizerRight2 = new wxBoxSizer(wxVERTICAL);
    sizerRight2->Add(textRight, 0, wxEXPAND | wxALL, 10);
    sizerRight2->Add(players, 1, wxEXPAND | wxALL, 10);
    sizerRight->Add(sizerRight2, 1, wxEXPAND | wxALL, 10);
    boxRight->SetSizer(sizerRight);

    wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add(boxLeft, 1, wxEXPAND | wxALL, 10);
    sizer->Add(boxRight, 1, wxEXPAND | wxALL, 10);

    wxBoxSizer* outer = new wxBoxSizer(wxVERTICAL);
    start = new wxButton(panel, BUTTON_Start, "Start!");
    start->Enable(false);
    outer->Add(sizer, 1, wxEXPAND | wxALL, 10);
    outer->Add(start, 0, wxEXPAND | wxALL, 10);
    panel->SetSizer(outer);

    CreateStatusBar();
    SetStatusText("wxWidgets GUI by xythobuz is ready...");
}

void PreparationFrame::buttonPress(wxCommandEvent& event) {
    if (questions->GetSelection() == wxNOT_FOUND) {
        preparation_onStartClicked("");
    } else {
        preparation_onStartClicked(questions->GetString(questions->GetSelection()).c_str());
    }
}

void PreparationFrame::listChange(wxCommandEvent& event) {
    if (questions->GetSelection() == wxNOT_FOUND) {
        preparation_onCatalogChanged("");
    } else {
        preparation_onCatalogChanged(questions->GetString(questions->GetSelection()).c_str());
    }
}

void PreparationFrame::OnExit(wxCloseEvent& event) {
    Destroy();
    preparation_onWindowClosed();
}

extern "C" {

void preparation_setMode(PreparationMode mode) {
    wxGetApp().createPreparation();

    if (mode == PREPARATION_MODE_BUSY) {
        wxGetApp().preparation->SetStatusText("Bitte warten...");
    } else if (mode == PREPARATION_MODE_NORMAL) {
        wxGetApp().preparation->SetStatusText("Bitte warten, bis der Spielleiter das Spiel startet...");
    } else if (mode == PREPARATION_MODE_PRIVILEGED) {
        wxGetApp().preparation->SetStatusText("Bitte wählen Sie einen Fragekatalog und klicken Sie dann auf Start.");
    } else {
        wxGetApp().preparation->SetStatusText("");
    }

    bool state = !((mode == PREPARATION_MODE_BUSY) || (mode == PREPARATION_MODE_NORMAL));
    wxGetApp().preparation->start->Enable(state);
    wxGetApp().preparation->questions->Enable(state);
}

void preparation_showWindow(void) {
    wxGetApp().createPreparation();
    wxGetApp().preparation->Show(true);
}

void preparation_addCatalog(const char *name) {
    wxString s(name);
    wxGetApp().preparation->questions->InsertItems(1, &s, 0);
}

int preparation_selectCatalog(const char *name) {
    int p = wxGetApp().preparation->questions->FindString(name, true);
    if (p != wxNOT_FOUND) {
        wxGetApp().preparation->questions->SetSelection(p);
        return 1;
    }
    return 0;
}

void preparation_addPlayer(const char *name) {
    wxString s(name);
    wxGetApp().preparation->players->InsertItems(1, &s, 0);
}

int preparation_removePlayer(const char *name) {
    int p = wxGetApp().preparation->players->FindString(name, true);
    if (p != wxNOT_FOUND) {
        wxGetApp().preparation->players->SetString(p, "");
        return 1;
    }
    return 0;
}

void preparation_clearPlayers(void) {
    for (int i = 0; i < wxGetApp().preparation->players->GetCount(); i++)
        wxGetApp().preparation->players->SetString(i, "");
}

void preparation_hideWindow(void) {
    wxGetApp().createPreparation();
    wxGetApp().preparation->Show(false);
}

void preparation_reset(void) {
    preparation_clearPlayers();

    for (int i = 0; i < wxGetApp().preparation->questions->GetCount(); i++)
        wxGetApp().preparation->questions->SetString(i, "");

    wxGetApp().preparation->start->Enable(false);
    wxGetApp().preparation->questions->Enable(false);
    wxGetApp().preparation->SetStatusText("wxWidgets GUI by xythobuz was reset...");
}

}


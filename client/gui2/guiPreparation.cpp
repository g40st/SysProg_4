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
wxEND_EVENT_TABLE()

PreparationFrame::PreparationFrame(const wxString& title, const wxPoint& pos, const wxSize& size)
        : wxFrame(NULL, wxID_ANY, title, pos, size) {

    wxPanel* panel = new wxPanel(this);

    wxStaticBox* boxLeft = new wxStaticBox(panel, wxID_ANY, "");
    wxStaticText* textLeft = new wxStaticText(boxLeft, wxID_ANY, "Fragekataloge:");
    questions = new wxListBox(boxLeft, wxID_ANY);

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
    panel->SetSizer(sizer);

    start = new wxButton(panel, wxID_ANY, "Start!");

    CreateStatusBar();
    SetStatusText("wxWidgets GUI by xythobuz is ready...");
}

void PreparationFrame::OnExit(wxCloseEvent& event) {
    Destroy();
    preparation_onWindowClosed();
}

extern "C" {

void preparation_onCatalogChanged(const char *newSelection);
void preparation_onStartClicked(const char *currentSelection);

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

}

int preparation_selectCatalog(const char *name) {

}

void preparation_addPlayer(const char *name) {

}

int preparation_removePlayer(const char *name) {

}

void preparation_clearPlayers(void) {

}

void preparation_hideWindow(void) {
    wxGetApp().createPreparation();
    wxGetApp().preparation->Show(false);
}

void preparation_reset(void) {

}

}


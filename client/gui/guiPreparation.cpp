/*
 * guiPreparation.cpp - Implementation for Preparation Window
 *
 * Copyright 2014 Thomas Buck <xythobuz@xythobuz.de
 */

#include <pthread.h>
#include <sys/select.h>

#include "guiApp.h"
#include "guiPreparation.h"

extern "C" {
#include "gui_interface.h"
}

static pthread_mutex_t mutexPrep = PTHREAD_MUTEX_INITIALIZER;

wxBEGIN_EVENT_TABLE(PreparationFrame, wxFrame)
    EVT_CLOSE(PreparationFrame::OnExit)
    EVT_BUTTON(BUTTON_Start, PreparationFrame::buttonPress)
    EVT_LISTBOX(LIST_Questions, PreparationFrame::listChange)
wxEND_EVENT_TABLE()

PreparationFrame::PreparationFrame(const wxString& title, const wxPoint& pos, const wxSize& size)
        : wxFrame(NULL, wxID_ANY, title, pos, size) {

    wxStaticBox* boxLeft = new wxStaticBox(this, wxID_ANY, "");
    wxStaticText* textLeft = new wxStaticText(boxLeft, wxID_ANY, "Questionnaires:");

    questions = new wxListBox(boxLeft, LIST_Questions, wxDefaultPosition, wxSize(250, 250));
    questions->Enable(false);

    wxBoxSizer* sizerLeft = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* sizerLeft2 = new wxBoxSizer(wxVERTICAL);
    sizerLeft2->Add(textLeft, 0, wxEXPAND | wxALL, 10);
    sizerLeft2->Add(questions, 1, wxEXPAND | wxALL, 10);
    sizerLeft->Add(sizerLeft2, 1, wxEXPAND | wxALL, 10);
    boxLeft->SetSizer(sizerLeft);

    wxStaticBox* boxRight = new wxStaticBox(this, wxID_ANY, "");
    wxStaticText* textRight = new wxStaticText(boxRight, wxID_ANY, "Connected Users:");

    players = new wxListBox(boxRight, wxID_ANY, wxDefaultPosition, wxSize(250, 250));

    wxBoxSizer* sizerRight = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* sizerRight2 = new wxBoxSizer(wxVERTICAL);
    sizerRight2->Add(textRight, 0, wxEXPAND | wxALL, 10);
    sizerRight2->Add(players, 1, wxEXPAND | wxALL, 10);
    sizerRight->Add(sizerRight2, 1, wxEXPAND | wxALL, 10);
    boxRight->SetSizer(sizerRight);

    wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add(boxLeft, 1, wxEXPAND | wxALL, 10);
    sizer->Add(boxRight, 1, wxEXPAND | wxALL, 10);

    start = new wxButton(this, BUTTON_Start, "Start!");
    start->Enable(false);

    wxBoxSizer* outer = new wxBoxSizer(wxVERTICAL);
    outer->Add(sizer, 1, wxEXPAND | wxALL, 10);
    outer->Add(start, 0, wxEXPAND | wxALL, 10);

    CreateStatusBar();
    SetStatusText("wxWidgets GUI by xythobuz is ready...");

    SetSizerAndFit(outer);
}

void PreparationFrame::buttonPress(wxCommandEvent& event) {
    if (questions->GetSelection() == wxNOT_FOUND) {
        //preparation_onStartClicked("");
    } else {
        preparation_onStartClicked(questions->GetString(questions->GetSelection()).c_str());
    }
}

void PreparationFrame::listChange(wxCommandEvent& event) {
    if (questions->GetSelection() == wxNOT_FOUND) {
        preparation_onCatalogChanged("");
        start->Enable(false);
    } else {
        preparation_onCatalogChanged(questions->GetString(questions->GetSelection()).c_str());
        start->Enable(true);
    }
}

void PreparationFrame::OnExit(wxCloseEvent& event) {
    Destroy();
    preparation_onWindowClosed();
}

extern "C" {

void preparation_setMode(PreparationMode mode) {
    pthread_mutex_lock(&mutexPrep);
    wxGetApp().createPreparation();

    if (mode == PREPARATION_MODE_BUSY) {
        wxGetApp().preparation->SetStatusText("Please wait...");
    } else if (mode == PREPARATION_MODE_NORMAL) {
        wxGetApp().preparation->SetStatusText("Please wait until the game master started the game...");
    } else if (mode == PREPARATION_MODE_PRIVILEGED) {
        wxGetApp().preparation->SetStatusText("Please select a questionnaire and press Start.");
    } else {
        wxGetApp().preparation->SetStatusText("");
    }

    bool state = !((mode == PREPARATION_MODE_BUSY) || (mode == PREPARATION_MODE_NORMAL));
    wxGetApp().preparation->questions->Enable(state);

    if (state && (wxGetApp().preparation->questions->GetSelection() == wxNOT_FOUND))
        state = false;

    wxGetApp().preparation->start->Enable(state);
    pthread_mutex_unlock(&mutexPrep);
}

void preparation_showWindow(void) {
    pthread_mutex_lock(&mutexPrep);
    wxGetApp().createPreparation();
    wxGetApp().preparation->Show(true);
    pthread_mutex_unlock(&mutexPrep);
}

void preparation_addCatalog(const char *name) {
    pthread_mutex_lock(&mutexPrep);
    wxGetApp().createPreparation();
    wxString s(name);
    wxGetApp().preparation->questions->Append(1, &s);
    pthread_mutex_unlock(&mutexPrep);
}

int preparation_selectCatalog(const char *name) {
    pthread_mutex_lock(&mutexPrep);
    wxGetApp().createPreparation();
    int p = wxGetApp().preparation->questions->FindString(name, true);
    int r = 0;
    if (p != wxNOT_FOUND) {
        wxGetApp().preparation->questions->SetSelection(p);
        r = 1;
    }
    pthread_mutex_unlock(&mutexPrep);
    return r;
}

void preparation_addPlayer(const char *name) {
    pthread_mutex_lock(&mutexPrep);
    wxGetApp().createPreparation();
    wxString s(name);
    wxGetApp().preparation->players->Append(1, &s);
    pthread_mutex_unlock(&mutexPrep);
}

int preparation_removePlayer(const char *name) {
    pthread_mutex_lock(&mutexPrep);
    wxGetApp().createPreparation();
    int p = wxGetApp().preparation->players->FindString(name, true);
    int r = 0;
    if (p != wxNOT_FOUND) {
        wxGetApp().preparation->players->Delete(p);
        r = 1;
    }
    pthread_mutex_unlock(&mutexPrep);
    return r;
}

void preparation_clearPlayers(void) {
    pthread_mutex_lock(&mutexPrep);
    wxGetApp().createPreparation();
    wxGetApp().preparation->players->Clear();
    pthread_mutex_unlock(&mutexPrep);
}

void preparation_hideWindow(void) {
    pthread_mutex_lock(&mutexPrep);
    wxGetApp().createPreparation();
    wxGetApp().preparation->Show(false);
    pthread_mutex_unlock(&mutexPrep);
}

void preparation_reset(void) {
    pthread_mutex_lock(&mutexPrep);
    wxGetApp().createPreparation();
    pthread_mutex_unlock(&mutexPrep);

    preparation_clearPlayers();

    pthread_mutex_lock(&mutexPrep);
    for (int i = 0; i < wxGetApp().preparation->questions->GetCount(); i++)
        wxGetApp().preparation->questions->SetString(i, "");

    wxGetApp().preparation->start->Enable(false);
    wxGetApp().preparation->questions->Enable(false);
    wxGetApp().preparation->SetStatusText("wxWidgets GUI by xythobuz was reset...");
    pthread_mutex_unlock(&mutexPrep);
}

}


/*
 * guiGame.h - Implementation for Game Window
 *
 * Copyright 2014 Thomas Buck <xythobuz@xythobuz.de
 */

#ifndef _GUI_GAME_H
#define _GUI_GAME_H

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

extern "C" {
#include "common/util.h"
}

#define MAX_ANSWERS 4

typedef struct {
    wxPanel* panel;
    wxCheckBox* check;
} answers_t;

typedef struct {

} players_t;

class GameFrame: public wxFrame {
public:
    GameFrame(const wxString& title, const wxPoint& pos, const wxSize& size);

    void OnExit(wxCloseEvent& event);
    void buttonPress(wxCommandEvent& event);

    wxButton* send;

    answers_t answers[MAX_ANSWERS];
    players_t players[MAX_PLAYERS];

    wxDECLARE_EVENT_TABLE();
};

#endif


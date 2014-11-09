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

class ScrolledTextPane : public wxScrolledWindow {
public:
    wxStaticText* text;

    ScrolledTextPane(wxWindow* parent, wxWindowID id);
};

typedef struct {
    wxCheckBox* check;
    wxStaticBitmap* image;
} answers_t;

typedef struct {
    wxStaticText* text;
    wxGauge* bar;
    wxStaticText* score;
} players_t;

class GameFrame: public wxFrame {
public:
    GameFrame(const wxString& title, const wxPoint& pos, const wxSize& size);

    void OnExit(wxCloseEvent& event);
    void buttonPress(wxCommandEvent& event);

    wxStaticBox* questionBox;

    ScrolledTextPane* question;

    answers_t answers[MAX_ANSWERS];
    players_t players[MAX_PLAYERS];

    wxButton* send;

    wxStaticBitmap* statusIcon;
    wxStaticText* statusText;

    unsigned long maxScore;

    wxImage* imageNone;
    wxImage* imageOk;
    wxImage* imageError;
    wxImage* imageTimeout;

    wxDECLARE_EVENT_TABLE();

private:
    void createImages();
};

#endif


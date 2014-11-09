/*
 * guiGame.cpp - Implementation for Game Window
 *
 * Copyright 2014 Thomas Buck <xythobuz@xythobuz.de
 */

#include "guiApp.h"
#include "guiGame.h"

static wxColour defaultColour;

extern "C" {
#include "../gui/gui_interface.h"
}

wxBEGIN_EVENT_TABLE(GameFrame, wxFrame)
    EVT_CLOSE(GameFrame::OnExit)
    EVT_BUTTON(BUTTON_Send, GameFrame::buttonPress)
wxEND_EVENT_TABLE()

GameFrame::GameFrame(const wxString& title, const wxPoint& pos, const wxSize& size)
        : wxFrame(NULL, wxID_ANY, title, pos, size) {

    wxStaticBox* questionBox = new wxStaticBox(this, wxID_ANY, "", wxDefaultPosition,
            wxSize(size.GetWidth() * 3 / 4 - 20, size.GetHeight() - 20));
    wxStaticBox* scoreBox = new wxStaticBox(this, wxID_ANY, "", wxDefaultPosition,
            wxSize(size.GetWidth() / 4 - 20, size.GetHeight() - 20));

    defaultColour = questionBox->GetBackgroundColour();

    wxBoxSizer* panelSizer = new wxBoxSizer(wxHORIZONTAL);
    panelSizer->Add(questionBox, 1, wxEXPAND | wxALL, 10);
    panelSizer->Add(scoreBox, 0, wxEXPAND | wxALL, 10);

    wxBoxSizer* questionSizer = new wxBoxSizer(wxVERTICAL);

    wxStaticText* questionText = new wxStaticText(questionBox, wxID_ANY, "Question");
    questionSizer->Add(questionText, 0, wxEXPAND | wxALL, 10);

    for (int i = 0; i < MAX_ANSWERS; i++) {
        answers[i].panel = new wxPanel(questionBox);
        answers[i].check = new wxCheckBox(answers[i].panel, wxID_ANY, "");

        wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
        sizer->Add(answers[i].check, 1, wxEXPAND | wxALL, 10);
        answers[i].panel->SetSizer(sizer);

        questionSizer->Add(answers[i].panel, 1, wxEXPAND | wxALL, 10);

        answers[i].panel->SetBackgroundColour(defaultColour);
        answers[i].check->Enable(false);
    }

    send = new wxButton(questionBox, BUTTON_Start, "Send");
    send->Enable(false);
    questionSizer->Add(send, 0, wxEXPAND | wxALL, 10);

    questionBox->SetSizer(questionSizer);

    wxBoxSizer* scoreSizer = new wxBoxSizer(wxVERTICAL);
    wxStaticText* scoreText = new wxStaticText(scoreBox, wxID_ANY, "Scores");
    scoreSizer->Add(scoreText, 1, wxEXPAND | wxALL, 10);

    for (int i = 0; i < MAX_PLAYERS; i++) {

    }

    scoreBox->SetSizer(scoreSizer);

    SetSizerAndFit(panelSizer);
}

void GameFrame::OnExit(wxCloseEvent& event) {
    Destroy();
    game_onWindowClosed();
}

void GameFrame::buttonPress(wxCommandEvent& event) {
    unsigned char bitmask = 0;
    for (int i = 0; i < MAX_ANSWERS; i++)
        bitmask |= ((answers[i].check->GetValue() ? 1 : 0) << i);
    game_onSubmitClicked(bitmask);
}

extern "C" {

void game_showWindow(void) {
    wxGetApp().createGame();
    wxGetApp().game->Show(true);
}

void game_setStatusText(const char *text) {
    wxGetApp().createGame();
    // TODO
}

void game_setStatusIcon(StatusIcon icon) {
    wxGetApp().createGame();
    // TODO
}

void game_setQuestion(const char *text) {
    wxGetApp().createGame();
    // TODO
}

void game_setAnswer(int index, const char *text) {
    wxGetApp().createGame();
    if ((index >= 0) && (index < MAX_ANSWERS)) {
        wxGetApp().game->answers[index].check->SetLabel(wxString(text));
    } else {
        debugPrint("Invalid game_setAnswer: %d, %s", index, text);
    }
}

void game_markAnswerCorrect(int index) {
    wxGetApp().createGame();
    if ((index >= 0) && (index < MAX_ANSWERS)) {
        // TODO
    } else {
        debugPrint("Invalid game_markAnswerCorrect: %d", index);
    }
}

void game_markAnswerWrong(int index) {
    wxGetApp().createGame();
    if ((index >= 0) && (index < MAX_ANSWERS)) {
        // TODO
    } else {
        debugPrint("Invalid game_markAnswerWrong: %d", index);
    }
}

void game_highlightMistake(int index) {
    wxGetApp().createGame();
    if ((index >= 0) && (index < MAX_ANSWERS)) {
        wxGetApp().game->answers[index].panel->SetBackgroundColour(*wxRED);
    } else {
        debugPrint("Invalid game_highlightMistake: %d", index);
    }
}

void game_clearAnswerMarksAndHighlights(void) {
    wxGetApp().createGame();
    for (int i = 0; i < MAX_ANSWERS; i++) {
        wxGetApp().game->answers[i].panel->SetBackgroundColour(defaultColour);
        wxGetApp().game->answers[i].check->SetValue(false);
        // TODO Undo markAnswerCorrect/Wrong
    }
}

void game_setControlsEnabled(int enable) {
    wxGetApp().createGame();
    for (int i = 0; i < MAX_ANSWERS; i++) {
        wxGetApp().game->answers[i].check->Enable((enable != 0) ? true : false);
    }
    wxGetApp().game->send->Enable((enable != 0) ? true : false);
}

void game_setPlayerName(int position, const char *name) {
    wxGetApp().createGame();
    if ((position >= 0) && (position < MAX_PLAYERS)) {
        // TODO
    } else {
        debugPrint("Invalid game_setPlayerName: %d, %s", position, name);
    }
}

void game_setPlayerScore(int position, unsigned long score) {
    wxGetApp().createGame();
    if ((position >= 0) && (position < MAX_PLAYERS)) {
        // TODO
    } else {
        debugPrint("Invalid game_setPlayerScore: %d, %lu", position, score);
    }
}

void game_highlightPlayer(int position) {
    wxGetApp().createGame();
    if ((position >= 0) && (position < MAX_PLAYERS)) {
        // TODO Color text/score for this player in red
    } else {
        debugPrint("Invalid game_highlightPlayer: %d", position);
    }
}

void game_hideWindow(void) {
    wxGetApp().createGame();
    wxGetApp().game->Show(false);
}

void game_reset(void) {
    wxGetApp().createGame();
    // TODO
}

}


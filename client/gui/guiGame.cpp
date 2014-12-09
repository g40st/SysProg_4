/*
 * guiGame.cpp - Implementation for Game Window
 *
 * Copyright 2014 Thomas Buck <xythobuz@xythobuz.de
 */

#include <pthread.h>
#include <sys/select.h>
#include <sstream>

#include "guiApp.h"
#include "guiGame.h"

#define QUESTION_FACTOR 4
#define SCORE_FACTOR 2
#define WINDOW_DIVISOR 6

#define IMAGE_SIZE 30

static pthread_mutex_t mutexGame = PTHREAD_MUTEX_INITIALIZER;

ScrolledTextPane::ScrolledTextPane(wxWindow* parent, wxWindowID id) : wxScrolledWindow(parent, id) {
    wxBoxSizer *sizer = new wxBoxSizer(wxHORIZONTAL);
    text = new wxStaticText(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL);
    sizer->Add(text, 1, wxALL | wxEXPAND, 0);
    SetSizer(sizer);
    FitInside();
    SetScrollRate(5, 5);
}

extern "C" {
#include "gui_interface.h"
}

wxBEGIN_EVENT_TABLE(GameFrame, wxFrame)
    EVT_CLOSE(GameFrame::OnExit)
    EVT_BUTTON(BUTTON_Send, GameFrame::buttonPress)
wxEND_EVENT_TABLE()

void GameFrame::createImages() {
    imageNone = new wxImage(IMAGE_SIZE, IMAGE_SIZE, false);
    imageNone->SetRGB(wxRect(0, 0, IMAGE_SIZE, IMAGE_SIZE), wxLIGHT_GREY->Red(), wxLIGHT_GREY->Green(), wxLIGHT_GREY->Blue());

    imageOk = new wxImage(IMAGE_SIZE, IMAGE_SIZE, false);
    imageOk->SetRGB(wxRect(0, 0, IMAGE_SIZE, IMAGE_SIZE), wxGREEN->Red(), wxGREEN->Green(), wxGREEN->Blue());

    imageError = new wxImage(IMAGE_SIZE, IMAGE_SIZE, false);
    imageError->SetRGB(wxRect(0, 0, IMAGE_SIZE, IMAGE_SIZE), wxRED->Red(), wxRED->Green(), wxRED->Blue());

    imageTimeout = new wxImage(IMAGE_SIZE, IMAGE_SIZE, false);
    imageTimeout->SetRGB(wxRect(0, 0, IMAGE_SIZE, IMAGE_SIZE), wxYELLOW->Red(), wxYELLOW->Green(), wxYELLOW->Blue());
}

GameFrame::GameFrame(const wxString& title, const wxPoint& pos, const wxSize& size)
        : wxFrame(NULL, wxID_ANY, title, pos, size), maxScore(0) {

    createImages();

    questionBox = new wxStaticBox(this, wxID_ANY, "", wxDefaultPosition,
            wxSize(size.GetWidth() * QUESTION_FACTOR / WINDOW_DIVISOR - 20, size.GetHeight() - 20));
    wxStaticBox* scoreBox = new wxStaticBox(this, wxID_ANY, "", wxDefaultPosition,
            wxSize(size.GetWidth() * SCORE_FACTOR / WINDOW_DIVISOR - 20, size.GetHeight() - 20));

    wxBoxSizer* panelSizer = new wxBoxSizer(wxHORIZONTAL);
    panelSizer->Add(questionBox, 1, wxEXPAND | wxALL, 5);
    panelSizer->Add(scoreBox, 0, wxEXPAND | wxALL, 5);

    wxBoxSizer* questionSizer = new wxBoxSizer(wxVERTICAL);

    wxStaticText* questionText = new wxStaticText(questionBox, wxID_ANY, "Question");
    questionSizer->Add(questionText, 0, wxEXPAND | wxALL, 5);

    question = new ScrolledTextPane(questionBox, wxID_ANY);
    questionSizer->Add(question, 1, wxEXPAND | wxALL | wxALIGN_CENTER, 5);

    for (int i = 0; i < MAX_ANSWERS; i++) {
        wxPanel* answerPanel = new wxPanel(questionBox);
        answers[i].check = new wxCheckBox(answerPanel, wxID_ANY, "");
        answers[i].image = new wxStaticBitmap(answerPanel, wxID_ANY,
                wxBitmap(*imageNone), wxDefaultPosition, wxSize(IMAGE_SIZE, IMAGE_SIZE));

        wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
        sizer->Add(answers[i].image, 0, wxEXPAND | wxALL | wxALIGN_CENTER, 5);
        sizer->Add(answers[i].check, 1, wxEXPAND | wxALL, 5);
        answerPanel->SetSizer(sizer);

        questionSizer->Add(answerPanel, 1, wxEXPAND | wxALL, 5);

        answers[i].check->SetBackgroundColour(*wxLIGHT_GREY);
        answers[i].check->Enable(false);
    }

    send = new wxButton(questionBox, BUTTON_Send, "Send");
    send->Enable(false);
    questionSizer->Add(send, 0, wxEXPAND | wxALL, 5);

    wxStaticBox* statusPanel = new wxStaticBox(questionBox, wxID_ANY, "");
    wxBoxSizer* statusSizer = new wxBoxSizer(wxHORIZONTAL);
    statusIcon = new wxStaticBitmap(statusPanel, wxID_ANY,
            wxBitmap(*imageNone), wxDefaultPosition, wxSize(IMAGE_SIZE, IMAGE_SIZE));
    statusSizer->Add(statusIcon, 0, wxEXPAND | wxALL, 5);
    statusText = new wxStaticText(statusPanel, wxID_ANY, "wxWidgets GUI by xythobuz is ready...");
    statusSizer->Add(statusText, 1, wxEXPAND | wxALL, 5);
    statusPanel->SetSizer(statusSizer);
    questionSizer->Add(statusPanel, 1, wxEXPAND | wxALL, 5);

    questionBox->SetSizer(questionSizer);

    wxBoxSizer* scoreSizer = new wxBoxSizer(wxVERTICAL);
    wxStaticText* scoreText = new wxStaticText(scoreBox, wxID_ANY, "Scores");
    scoreSizer->Add(scoreText, 0, wxEXPAND | wxALL, 5);

    for (int i = 0; i < MAX_PLAYERS; i++) {
        wxStaticBox* playerBox = new wxStaticBox(scoreBox, wxID_ANY, "");
        wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

        wxPanel* textPanel = new wxPanel(playerBox);
        wxBoxSizer* textSizer = new wxBoxSizer(wxHORIZONTAL);

        players[i].text = new wxStaticText(playerBox, wxID_ANY, "");
        textSizer->Add(players[i].text, 1, wxEXPAND | wxALL, 5);

        players[i].score = new wxStaticText(playerBox, wxID_ANY, "",
                wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);
        textSizer->Add(players[i].score, 1, wxEXPAND | wxALL | wxALIGN_RIGHT, 5);

        textPanel->SetSizer(textSizer);
        sizer->Add(textPanel, 1, wxEXPAND | wxALL, 5);

        players[i].bar = new wxGauge(playerBox, wxID_ANY, maxScore);
        sizer->Add(players[i].bar, 1, wxEXPAND | wxALL, 5);

        playerBox->SetSizer(sizer);
        scoreSizer->Add(playerBox, 1, wxEXPAND | wxALL, 5);
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
    pthread_mutex_lock(&mutexGame);
    wxGetApp().createGame();
    wxGetApp().game->Show(true);
    pthread_mutex_unlock(&mutexGame);
}

void game_setStatusText(const char *text) {
    pthread_mutex_lock(&mutexGame);
    wxGetApp().createGame();
    wxGetApp().game->statusText->SetLabel(wxString(text));
    pthread_mutex_unlock(&mutexGame);
}

void game_setStatusIcon(StatusIcon icon) {
    pthread_mutex_lock(&mutexGame);
    wxGetApp().createGame();
    if (icon == STATUS_ICON_CORRECT) {
        wxGetApp().game->statusIcon->SetBitmap(wxBitmap(*wxGetApp().game->imageOk));
    } else if (icon == STATUS_ICON_WRONG) {
        wxGetApp().game->statusIcon->SetBitmap(wxBitmap(*wxGetApp().game->imageError));
    } else if (icon == STATUS_ICON_TIMEOUT) {
        wxGetApp().game->statusIcon->SetBitmap(wxBitmap(*wxGetApp().game->imageTimeout));
    } else {
        wxGetApp().game->statusIcon->SetBitmap(wxBitmap(*wxGetApp().game->imageNone));
    }
    pthread_mutex_unlock(&mutexGame);
}

void game_setQuestion(const char *text) {
    pthread_mutex_lock(&mutexGame);
    wxGetApp().createGame();
    wxGetApp().game->question->text->SetLabel(wxString(text));
    wxGetApp().game->question->text->Wrap(wxGetApp().game->questionBox->GetSize().GetWidth() - 80);
    wxGetApp().game->Layout();
    pthread_mutex_unlock(&mutexGame);
}

void game_setAnswer(int index, const char *text) {
    pthread_mutex_lock(&mutexGame);
    wxGetApp().createGame();
    if ((index >= 0) && (index < MAX_ANSWERS)) {
        wxGetApp().game->answers[index].check->SetLabel(wxString(text));
    } else {
        errorPrint("gui2: Invalid game_setAnswer: %d, %s", index, text);
    }
    pthread_mutex_unlock(&mutexGame);
}

void game_markAnswerCorrect(int index) {
    pthread_mutex_lock(&mutexGame);
    wxGetApp().createGame();
    if ((index >= 0) && (index < MAX_ANSWERS)) {
        wxGetApp().game->answers[index].image->SetBitmap(wxBitmap(*wxGetApp().game->imageOk));
    } else {
        errorPrint("gui2: Invalid game_markAnswerCorrect: %d", index);
    }
    pthread_mutex_unlock(&mutexGame);
}

void game_markAnswerWrong(int index) {
    pthread_mutex_lock(&mutexGame);
    wxGetApp().createGame();
    if ((index >= 0) && (index < MAX_ANSWERS)) {
        wxGetApp().game->answers[index].image->SetBitmap(wxBitmap(*wxGetApp().game->imageError));
    } else {
        errorPrint("gui2: Invalid game_markAnswerWrong: %d", index);
    }
    pthread_mutex_unlock(&mutexGame);
}

void game_highlightMistake(int index) {
    pthread_mutex_lock(&mutexGame);
    wxGetApp().createGame();
    if ((index >= 0) && (index < MAX_ANSWERS)) {
        wxGetApp().game->answers[index].check->SetBackgroundColour(*wxRED);
    } else {
        errorPrint("gui2: Invalid game_highlightMistake: %d", index);
    }
    pthread_mutex_unlock(&mutexGame);
}

void game_clearAnswerMarksAndHighlights(void) {
    pthread_mutex_lock(&mutexGame);
    wxGetApp().createGame();
    for (int i = 0; i < MAX_ANSWERS; i++) {
        wxGetApp().game->answers[i].check->SetBackgroundColour(*wxLIGHT_GREY);
        wxGetApp().game->answers[i].check->SetValue(false);
        wxGetApp().game->answers[i].image->SetBitmap(wxBitmap(*wxGetApp().game->imageNone));
    }
    pthread_mutex_unlock(&mutexGame);
}

void game_setControlsEnabled(int enable) {
    pthread_mutex_lock(&mutexGame);
    wxGetApp().createGame();
    for (int i = 0; i < MAX_ANSWERS; i++) {
        wxGetApp().game->answers[i].check->Enable((enable != 0) ? true : false);
    }
    wxGetApp().game->send->Enable((enable != 0) ? true : false);
    pthread_mutex_unlock(&mutexGame);
}

void game_setPlayerName(int position, const char *name) {
    pthread_mutex_lock(&mutexGame);
    wxGetApp().createGame();
    if (((position - 1) >= 0) && ((position - 1) < MAX_PLAYERS)) {
        wxGetApp().game->players[(position - 1)].text->SetLabel(wxString(name));
        wxGetApp().game->Layout();
    } else {
        errorPrint("gui2: Invalid game_setPlayerName: %d, %s", position, name);
    }
    pthread_mutex_unlock(&mutexGame);
}

void game_setPlayerScore(int position, unsigned long score) {
    pthread_mutex_lock(&mutexGame);
    wxGetApp().createGame();
    if (((position - 1) >= 0) && ((position - 1) < MAX_PLAYERS)) {
        std::stringstream ss;
        ss << score;
        wxGetApp().game->players[(position - 1)].score->SetLabel(wxString(ss.str()));
        if (score > wxGetApp().game->maxScore) {
            wxGetApp().game->maxScore = score;
            for (int i = 0; i < MAX_PLAYERS; i++) {
                wxGetApp().game->players[i].bar->SetRange(wxGetApp().game->maxScore);
            }
        }
        wxGetApp().game->players[(position - 1)].bar->SetValue(score);
        wxGetApp().game->Layout();
    } else {
        errorPrint("gui2: Invalid game_setPlayerScore: %d, %lu", position, score);
    }
    pthread_mutex_unlock(&mutexGame);
}

void game_highlightPlayer(int position) {
    pthread_mutex_lock(&mutexGame);
    wxGetApp().createGame();
    if (((position - 1) >= 0) && ((position - 1) < MAX_PLAYERS)) {
        wxGetApp().game->players[(position - 1)].text->SetForegroundColour(*wxRED);
        wxGetApp().game->players[(position - 1)].score->SetForegroundColour(*wxRED);
    } else {
        errorPrint("gui2: Invalid game_highlightPlayer: %d", position);
    }
    pthread_mutex_unlock(&mutexGame);
}

void game_hideWindow(void) {
    pthread_mutex_lock(&mutexGame);
    wxGetApp().createGame();
    wxGetApp().game->Show(false);
    pthread_mutex_unlock(&mutexGame);
}

void game_reset(void) {
    pthread_mutex_lock(&mutexGame);
    wxGetApp().createGame();
    pthread_mutex_unlock(&mutexGame);

    game_clearAnswerMarksAndHighlights();
    game_setControlsEnabled(0);
    game_setStatusIcon(STATUS_ICON_NONE);
    game_setStatusText("");
    game_setQuestion("");

    for (int i = 0; i < MAX_ANSWERS; i++)
        game_setAnswer(i, "");

    for (int i = 0; i < MAX_PLAYERS; i++) {
        game_setPlayerName(i, "");
        game_setPlayerScore(i, 0);
        pthread_mutex_lock(&mutexGame);
        wxGetApp().game->players[i].bar->SetRange(0);
        wxGetApp().game->players[i].bar->SetValue(0);
        wxGetApp().game->players[i].text->SetForegroundColour(*wxBLACK);
        wxGetApp().game->players[i].score->SetForegroundColour(*wxBLACK);
        pthread_mutex_unlock(&mutexGame);
    }

    pthread_mutex_lock(&mutexGame);
    wxGetApp().game->maxScore = 0;
    pthread_mutex_unlock(&mutexGame);
}

}


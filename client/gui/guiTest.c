/*
 * guiTest.c
 *
 * Copyright 2014 Thomas Buck <xythobuz@xythobuz.de
 */

#include "gui_interface.h"
#include "common/util.h"

int main(int argc, char *argv[]) {
    setProgName(argv[0]);
    debugEnable();
    infoPrint("xythobuz GUI Test");

    debugPrint("--> guiInit(&argc, &argv)");
    guiInit(&argc, &argv);

    debugPrint("--> preparation_showWindow()");
    preparation_showWindow();

    debugPrint("--> preparation_setMode(PREPARATION_MODE_PRIVILEGED)");
    preparation_setMode(PREPARATION_MODE_PRIVILEGED);

    debugPrint("--> preparation_addCatalog(\"Test\")");
    preparation_addCatalog("Test");

    debugPrint("--> preparation_addPlayer(\"Test\")");
    preparation_addPlayer("Test");

    debugPrint("--> guiMain()");
    guiMain();

    debugPrint("--> guiDestroy()");
    guiDestroy();

    return 0;
}

void createGameFrame(void) {
    debugPrint("--> game_showWindow()");
    game_showWindow();

    debugPrint("--> game_reset()");
    game_reset();

    debugPrint("--> game_setControlsEnabled(1)");
    game_setControlsEnabled(1);

    debugPrint("--> game_setQuestion(\"Bla ... bla\")");
    game_setQuestion("Here is the question text ... ?");

    debugPrint("--> game_setAnswer(0, \"Answer 1\")");
    game_setAnswer(0, "Answer 1");

    debugPrint("--> game_setAnswer(1, \"Answer 2\")");
    game_setAnswer(1, "Answer 2");

    debugPrint("--> game_setAnswer(2, \"Answer 3\")");
    game_setAnswer(2, "Answer 3");

    debugPrint("--> game_setAnswer(3, \"Answer 4\")");
    game_setAnswer(3, "Answer 4");

    debugPrint("--> game_markAnswerCorrect(1)");
    game_markAnswerCorrect(1);

    debugPrint("--> game_markAnswerWrong(3)");
    game_markAnswerWrong(3);

    debugPrint("--> game_highlightMistake(2)");
    game_highlightMistake(2);

    debugPrint("--> game_setStatusText(\"xythobuz guiTest StatusText\")");
    game_setStatusText("xythobuz guiTest StatusText");

    debugPrint("--> game_setStatusIcon(STATUS_ICON_TIMEOUT)");
    game_setStatusIcon(STATUS_ICON_TIMEOUT);

    debugPrint("--> game_setPlayerName(0, \"Player1\")");
    game_setPlayerName(1, "Player1");

    debugPrint("--> game_setPlayerScore(0, 42)");
    game_setPlayerScore(1, 42);

    debugPrint("--> game_setPlayerName(1, \"Player2\")");
    game_setPlayerName(2, "Player2");

    debugPrint("--> game_setPlayerScore(1, 23)");
    game_setPlayerScore(2, 23);

    debugPrint("--> game_highlightPlayer(1)");
    game_highlightPlayer(1);

    debugPrint("--> game_setPlayerName(2, \"Player3\")");
    game_setPlayerName(3, "Player3");

    debugPrint("--> game_setPlayerScore(2, 4)");
    game_setPlayerScore(3, 4);
}

void preparation_onCatalogChanged(const char *newSelection) {
    debugPrint("<-- preparation_onCatalogChanged(%s)", newSelection);
}

void preparation_onStartClicked(const char *currentSelection) {
    debugPrint("<-- preparation_onStartClicked(%s)", currentSelection);

    debugPrint("--> preparation_hideWindow()");
    preparation_hideWindow();

    debugPrint("--> guiShowErrorDialog(\"Error Dialog\", 0)");
    guiShowErrorDialog("Error Dialog", 0);

    createGameFrame();
}

void preparation_onWindowClosed(void) {
    debugPrint("<-- preparation_onWindowClosed()");

    createGameFrame();
}

void game_onSubmitClicked(unsigned char selectedAnswers) {
    debugPrint("<-- game_onSubmitClicked(%d)", selectedAnswers);

    debugPrint("--> game_clearAnswerMarksAndHighlights()");
    game_clearAnswerMarksAndHighlights();

    debugPrint("--> guiShowMessageDialog(\"Message Dialog\", 0)");
    guiShowMessageDialog("Message Dialog", 0);

    debugPrint("--> game_reset()");
    game_reset();
}

void game_onWindowClosed(void) {
    debugPrint("<-- game_onWindowClosed()");

    debugPrint("--> guiQuit()");
    guiQuit();
}


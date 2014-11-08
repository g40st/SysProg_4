/*
 * Systemprogrammierung
 * Multiplayer-Quiz
 *
 * Client
 *
 * gui.c: Implementierung des GUI-Threads
 */

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#include "common/rfc.h"
#include "common/util.h"
#include "gui/gui_interface.h"
#include "listener.h"
#include "gui.h"

static int guiSocket = -1;

void *guiThread(void *arg) {
    guiSocket = *((int *)arg);
    while (getRunning()) {
        GamePhase_t phase = getGamePhase();

        if (phase == PHASE_PREPARATION) {

        } else if (phase == PHASE_GAME) {

        } else {

        }

        loopsleep();
    }
    return NULL;
}

void preparation_onCatalogChanged(const char *newSelection) {
    if (guiSocket == -1) {
        errorPrint("preparation_onCatalogChanged: Wrong initialization order?!");
        return;
    }

    if (getGamePhase() == PHASE_PREPARATION) {
        debugPrint("preparation_onCatalogChanged: %s", newSelection);
        rfc response;
        response.main.type[0] = 'C';
        response.main.type[1] = 'C';
        response.main.type[2] = 'H';
        response.main.length = htons(strlen(newSelection));
        strncpy(response.catalogChange.filename, newSelection, strlen(newSelection));
        if (send(guiSocket, &response.main, RFC_BASE_SIZE + strlen(newSelection), 0) == -1) {
            errnoPrint("send");
        }
    } else {
        debugPrint("Got preparation_onCatalogChanged in wrong phase!");
    }
}

void preparation_onStartClicked(const char *currentSelection) {
    if (guiSocket == -1) {
        errorPrint("preparation_onStartClicked: Wrong initialization order?!");
        return;
    }

    if (getGamePhase() == PHASE_PREPARATION) {
        if ((currentSelection == NULL) || (currentSelection[0] == '\0')) {
            debugPrint("Clicked start without selected category!");
            guiShowErrorDialog("Can't start game without selected category!", 0);
            return;
        }
        debugPrint("preparation_onStartClicked: %s", currentSelection);
        rfc response;
        response.main.type[0] = 'S';
        response.main.type[1] = 'T';
        response.main.type[2] = 'G';
        response.main.length = htons(strlen(currentSelection));
        strncpy(response.catalogChange.filename, currentSelection, strlen(currentSelection));
        if (send(guiSocket, &response.main, RFC_BASE_SIZE + strlen(currentSelection), 0) == -1) {
            errnoPrint("send");
        }
        setGamePhase(PHASE_GAME);
    } else {
        debugPrint("Got preparation_onStartClicked in wrong phase!");
    }
}

void preparation_onWindowClosed(void) {
    debugPrint("preparation_onWindowClosed");
    stopThreads();
    guiQuit();
}

void game_onSubmitClicked(unsigned char selectedAnswers) {
    if (guiSocket == -1) {
        errorPrint("game_onSubmitClicked: Wrong initialization order?!");
        return;
    }

    if (getGamePhase() == PHASE_GAME) {
        debugPrint("game_onSubmitClicked: %u", (unsigned)selectedAnswers);
        rfc response;
        response.main.type[0] = 'Q';
        response.main.type[1] = 'A';
        response.main.type[2] = 'N';
        response.main.length = htons(1);
        response.questionAnswered.selection = selectedAnswers;
        if (send(guiSocket, &response.main, RFC_QAN_SIZE, 0) == -1) {
            errnoPrint("send");
        }
    } else {
        debugPrint("Got game_onSubmitClicked in wrong phase!");
    }
}

void game_onWindowClosed(void) {
    debugPrint("game_onWindowClosed");
    stopThreads();
    guiQuit();
}


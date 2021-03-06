/*
 * Systemprogrammierung
 * Multiplayer-Quiz
 *
 * Client
 *
 * gui.c: Implementierung des GUI-Threads
 */

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#include "common/rfc.h"
#include "common/util.h"
#include "fragewechsel.h"
#include "gui/gui_interface.h"
#include "listener.h"
#include "gui.h"

static int guiSocket = -1;

// Place to store the last bitmask of answers given
static unsigned char lastResult = 0;
static pthread_mutex_t guiMutex = PTHREAD_MUTEX_INITIALIZER;

unsigned char guiGetLastResult(void) {
    pthread_mutex_lock(&guiMutex);
    unsigned char r = lastResult;
    pthread_mutex_unlock(&guiMutex);
    return r;
}

/*
 * Properly handle any received error or warning messages.
 */
int handleErrorWarningMessage(rfc response) {
    if (equalLiteral(response.main, "ERR")) {
        int length = ntohs(response.main.length) - RFC_ERR_SIZE;
        if (length >= MAX_STRING_LENGTH)
            length = MAX_STRING_LENGTH;

        char s[length + 1];
        s[length] = '\0';
        memcpy(s, response.errorWarning.message, length);

        // Only print warning messages.
        if (response.errorWarning.subtype == 0) {
            infoPrint("Warning: %s", s);
            guiShowMessageDialog(s, 0);
            return 1;
        // Print error messages and exit afterwards.
        } else if (response.errorWarning.subtype == 1) {
            errorPrint("Error: %s", s);
            guiShowErrorDialog(s, 1);
            exit(1);
            return 1; // Should already have happened, just in case
        } else {
            debugPrint("Got ERR with invalid type (%d): %s", response.errorWarning.subtype, s);
        }
    }

    return 0;
}

void guiSetSocket(int s) {
    guiSocket = s;
}

/*
 * Called by GUI when a new catalog has been selected.
 */
void preparation_onCatalogChanged(const char *newSelection) {
    if (guiSocket == -1) {
        errorPrint("preparation_onCatalogChanged: Wrong initialization order?!");
        return;
    }

    if (getGamePhase() == PHASE_PREPARATION) {
        // Send the name of the newly selected catalog to the server.
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

/*
 * Called by GUI when the Start button has been clicked.
 */
void preparation_onStartClicked(const char *currentSelection) {
    if (guiSocket == -1) {
        errorPrint("preparation_onStartClicked: Wrong initialization order?!");
        return;
    }

    if (getGamePhase() == PHASE_PREPARATION) {
        // Ensure a category has been selected
        if ((currentSelection == NULL) || (currentSelection[0] == '\0')) {
            debugPrint("Clicked start without selected category!");
            guiShowErrorDialog("Can't start game without selected category!", 0);
            return;
        }

        // Ensure there are enough other players to start a game
        if (getPlayerCount() <= 1) {
            debugPrint("Clicked start without another player!");
            guiShowErrorDialog("Can't start game without another player!", 0);
            return;
        }

        // Send the start game message to the server
        debugPrint("preparation_onStartClicked: %s", currentSelection);
        rfc response;
        response.main.type[0] = 'S';
        response.main.type[1] = 'T';
        response.main.type[2] = 'G';
        response.main.length = htons(strlen(currentSelection));
        strncpy(response.catalogChange.filename, currentSelection, strlen(currentSelection));
        if (send(guiSocket, &response.main, RFC_BASE_SIZE + strlen(currentSelection), 0) == -1) {
            errnoPrint("send");
            return;
        }
    } else {
        debugPrint("Got preparation_onStartClicked in wrong phase!");
    }
}

/*
 * Called by GUI when the preparation window has been closed.
 */
void preparation_onWindowClosed(void) {
    debugPrint("preparation_onWindowClosed");
    stopThreads();
    guiQuit();
}

/*
 * Called by GUI when the Submit button has been clicked.
 */
void game_onSubmitClicked(unsigned char selectedAnswers) {
    if (guiSocket == -1) {
        errorPrint("game_onSubmitClicked: Wrong initialization order?!");
        return;
    }

    lastResult = selectedAnswers;

    if (getGamePhase() == PHASE_GAME) {
        /*
         * Disable the controls until we received the result.
         * Then simply send the answers to the server.
         */
        debugPrint("game_onSubmitClicked: %u", (unsigned)selectedAnswers);
        game_setControlsEnabled(0);
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

/*
 * Called by GUI when the Game window has been closed.
 */
void game_onWindowClosed(void) {
    debugPrint("game_onWindowClosed");
    stopThreads();
    guiQuit();
}


/*
 * Systemprogrammierung
 * Multiplayer-Quiz
 *
 * Client
 *
 * listener.c: Implementierung des Listener-Threads
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "common/rfc.h"
#include "common/util.h"
#include "fragewechsel.h"
#include "gui/gui_interface.h"
#include "gui.h"
#include "listener.h"

int getClientID(void); // in main

/*
 * The player count is stored here so we can check if there are enough
 * players to start the game (in gui.c).
 */
static int playerCount;
static pthread_mutex_t playerCountMutex = PTHREAD_MUTEX_INITIALIZER;

void setPlayerCount(int pc) {
    pthread_mutex_lock(&playerCountMutex);
    playerCount = pc;
    pthread_mutex_unlock(&playerCountMutex);
}

int getPlayerCount(void) {
    pthread_mutex_lock(&playerCountMutex);
    int pc = playerCount;
    pthread_mutex_unlock(&playerCountMutex);
    return pc;
}

void *listenerThread(void *arg) {
    int socket = *((int *)arg);
    rfc response;
    debugPrint("ListenerThread is starting its loop...");

    // Listener Threads main loop.
    while (getRunning()) {
        // Receive a message from the server
        int receive = receivePacket(socket, &response);
        if (receive == -1) {
            guiShowErrorDialog("Error reading data from Server!", 0);
            guiQuit();
            stopThreads();
            return NULL;
        } else if (receive == 0) {
            errorPrint("Remote host closed connection");
            guiShowErrorDialog("The Server was closed!", 0);
            guiQuit();
            stopThreads();
            return NULL;
        }

        // Warning/Error messages are always handled here
        if (handleErrorWarningMessage(response))
            continue;

        // Check the message and react accordingly
        if (getGamePhase() == PHASE_PREPARATION) {
            if (equalLiteral(response.main, "LST")) {
                /*
                 * We've got a new list of players. Clear the current
                 * display and send them all to the preparation window.
                 */
                debugPrint("ListenerThread got LST message (%d)", ntohs(response.main.length));
                int count = ntohs(response.main.length) / 37;
                preparation_clearPlayers();
                for (int i = 0; i < count; i++) {
                    preparation_addPlayer(response.playerList.players[i].name);
                    debugPrint("LST: %d %s %d", response.playerList.players[i].id,
                            response.playerList.players[i].name,
                            ntohl(response.playerList.players[i].points));
                }
                setPlayerCount(count);
            } else if (equalLiteral(response.main, "CCH")) {
                /*
                 * A new catalog has been selected, so we send the name
                 * to the preparation window.
                 */
                int len = ntohs(response.main.length);
                char buff[len + 1];
                strncpy(buff, response.catalogChange.filename, len);
                buff[len] = '\0';
                debugPrint("Received CatalogChange: \"%s\"", buff);
                preparation_selectCatalog(buff);
            } else if (equalLiteral(response.main, "CRE")) {
                /*
                 * A new list of available catalogs has been sent.
                 * Display them all in the preparation window.
                 */
                int len = ntohs(response.main.length);
                void *vp = &response.catalogResponse;
                struct rfcCatalog *cat = (struct rfcCatalog *)vp;
                int i = 0;
                if (len == 0)
                    debugPrint("Received completely empty CatalogResponse");
                while (len != 0) {
                    char buff[len + 1];
                    strncpy(buff, cat->filename, len);
                    buff[len] = '\0';
                    debugPrint("Received CatalogResponse %d (%d): %s", i++, len, buff);
                    preparation_addCatalog(buff);
                    vp += RFC_BASE_SIZE + len;
                    if (vp >= (((void *)&response) + receive))
                        break;
                    cat = (struct rfcCatalog *)vp;
                    len = ntohs(cat->main.length);
                }
            } else if (equalLiteral(response.main, "STG")) {
                // The Game Phase has now started.
                int len = ntohs(response.main.length);
                char buff[len + 1];
                strncpy(buff, response.startGame.filename, len);
                buff[len] = '\0';
                debugPrint("The PhaseGame has now started: \"%s\"", buff);
                setGamePhase(PHASE_GAME);

                // Hide the old and show the new window.
                preparation_hideWindow();
                game_showWindow();
                game_setControlsEnabled(0);
                requestNewQuestion(0);
            } else {
                errorPrint("Unexpected response in PhasePreparation: %c%c%c", response.main.type[0],
                        response.main.type[1], response.main.type[2]);
            }
        } else if (getGamePhase() == PHASE_GAME) {
            if (equalLiteral(response.main, "QUE")) {
                if (ntohs(response.main.length) == 769) {
                    // We have received a new question-answer set from the server.
                    debugPrint("Received new question-answer set... Timeout: %d", response.question.question.timeout);

                    // Clear the game window and set the question/answer texts.
                    game_setStatusIcon(STATUS_ICON_NONE);
                    game_clearAnswerMarksAndHighlights();
                    game_setQuestion(response.question.question.question);
                    for (int i = 0; i < NUM_ANSWERS; i++)
                        game_setAnswer(i, response.question.question.answers[i]);
                    char buffer[64];
                    snprintf(buffer, 64, "Time limit: %d seconds!", response.question.question.timeout);
                    game_setStatusText(buffer);
                    game_setControlsEnabled(1);
                } else if (ntohs(response.main.length) == 0) {
                    // We have answered all available questions. Move to the End Phase.
                    debugPrint("The PhaseEnd has now started!");
                    game_setStatusText("Waiting for other players to finish...");
                    setGamePhase(PHASE_END);
                } else {
                    debugPrint("Received Question with nonsense length: %d", ntohs(response.main.length));
                }
            } else if (equalLiteral(response.main, "QRE")) {
                debugPrint("Received QuestionResult: %d %d", response.questionResult.timedOut, response.questionResult.correct);
                game_setControlsEnabled(0);
                for (int i = 0; i < NUM_ANSWERS; i++) {
                    /*
                     * We have received the result of answering the last question.
                     * Mark the answers and mistakes, and request a new question
                     * after the correct timeout for our situation.
                     */
                    if (response.questionResult.correct & (1 << i)) {
                        game_markAnswerCorrect(i);
                    } else {
                        game_markAnswerWrong(i);
                    }

                    if (response.questionResult.timedOut == 0) {
                        if (((guiGetLastResult() & (1 << i))
                                    && (!(response.questionResult.correct & (1 << i))))
                                || ((!(guiGetLastResult() & (1 << i)))
                                    && (response.questionResult.correct & (1 << i)))) {
                            game_highlightMistake(i);
                        }
                    }
                }

                if (response.questionResult.timedOut != 0) {
                    game_setStatusIcon(STATUS_ICON_TIMEOUT);
                    game_setStatusText("Sorry, timed out!");
                } else {
                    if (guiGetLastResult() == response.questionResult.correct) {
                        game_setStatusIcon(STATUS_ICON_CORRECT);
                        game_setStatusText("Your answer was correct!");
                    } else {
                        game_setStatusIcon(STATUS_ICON_WRONG);
                        game_setStatusText("Sorry, that's the wrong answer!");
                    }
                }

                // Request next question
                requestNewQuestion(3);
            } else if (equalLiteral(response.main, "LST")) {
                /*
                 * A new list of player names and scores has been sent.
                 * Update the display in the game window.
                 */
                debugPrint("ListenerThread got LST message (%d)", ntohs(response.main.length));
                int count = ntohs(response.main.length) / 37;
                for (int i = 0; i < count; i++) {
                    game_setPlayerName(i + 1, response.playerList.players[i].name);
                    game_setPlayerScore(i + 1, ntohl(response.playerList.players[i].points));
                    debugPrint("LST %d: %s %d", i, response.playerList.players[i].name, ntohl(response.playerList.players[i].points));
                    if (response.playerList.players[i].id == getClientID()) {
                        game_highlightPlayer(i + 1);
                    }
                }
                for (int i = count; i < MAX_PLAYERS; i++) {
                    game_setPlayerName(i + 1, "");
                    game_setPlayerScore(i + 1, 0);
                }
            } else {
                errorPrint("Unexpected response in PhaseGame: %c%c%c", response.main.type[0],
                        response.main.type[1], response.main.type[2]);
            }
        } else {
            if (equalLiteral(response.main, "GOV")) {
                /*
                 * The Game is Over. Display a message with our rank.
                 */
                debugPrint("Received GameOver message: %d", response.gameOver.rank);
                char buffer[1024];
                buffer[1023] = '\0';
                snprintf(buffer, 1023, "Game Over! You ranked on place %d!", response.gameOver.rank);
                guiShowMessageDialog(buffer, 1);
                stopThreads();
                return NULL;
            } else if (equalLiteral(response.main, "LST")) {
                /*
                 * We're finished but others are still playing. So we still
                 * need to update the score list.
                 */
                debugPrint("ListenerThread got LST message (%d)", ntohs(response.main.length));
                int count = ntohs(response.main.length) / 37;
                for (int i = 0; i < count; i++) {
                    game_setPlayerName(i + 1, response.playerList.players[i].name);
                    game_setPlayerScore(i + 1, ntohl(response.playerList.players[i].points));
                }
                for (int i = count; i < MAX_PLAYERS; i++) {
                    game_setPlayerName(i + 1, "");
                    game_setPlayerScore(i + 1, 0);
                }
            } else {
                errorPrint("Unexpected response in PhaseEnd: %c%c%c", response.main.type[0],
                        response.main.type[1], response.main.type[2]);
            }
        }
    }
    return NULL;
}


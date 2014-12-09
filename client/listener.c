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
    while (getRunning()) {
        // Receive message
        int receive = receivePacket(socket, &response);
        if (receive == -1) {
            // TODO handle error case gracefully?
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

        // Check message and react accordingly
        if (getGamePhase() == PHASE_PREPARATION) {
            if (equalLiteral(response.main, "LST")) {
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
                // Mark selected catalog
                int len = ntohs(response.main.length);
                char buff[len + 1];
                strncpy(buff, response.catalogChange.filename, len);
                buff[len] = '\0';
                debugPrint("Received CatalogChange: \"%s\"", buff);
                preparation_selectCatalog(buff);
            } else if (equalLiteral(response.main, "CRE")) {
                // Display specified catalogs
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
                // Start Game Phase
                int len = ntohs(response.main.length);
                char buff[len + 1];
                strncpy(buff, response.startGame.filename, len);
                buff[len] = '\0';
                debugPrint("The PhaseGame has now started: \"%s\"", buff);
                setGamePhase(PHASE_GAME);
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
                    debugPrint("Received new question-answer set... Timeout: %d", response.question.question.timeout);
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
                    // Move to End Phase
                    debugPrint("The PhaseEnd has now started!");
                    setGamePhase(PHASE_END);
                } else {
                    debugPrint("Received Question with nonsense length: %d", ntohs(response.main.length));
                }
            } else if (equalLiteral(response.main, "QRE")) {
                debugPrint("Received QuestionResult: %d %d", response.questionResult.timedOut, response.questionResult.correct);
                for (int i = 0; i < NUM_ANSWERS; i++) {
                    if (response.questionResult.correct & (1 << i)) {
                        game_markAnswerCorrect(i);
                    } else {
                        game_markAnswerWrong(i);
                    }

                    if (((guiGetLastResult() & (1 << i))
                                && (!(response.questionResult.correct & (1 << i))))
                            || ((!(guiGetLastResult() & (1 << i)))
                                && (response.questionResult.correct & (1 << i)))) {
                        game_highlightMistake(i);
                    }

                    if (guiGetLastResult() == response.questionResult.correct) {
                        game_setStatusIcon(STATUS_ICON_CORRECT);
                    } else {
                        game_setStatusIcon(STATUS_ICON_WRONG);
                    }
                }
                if (response.questionResult.timedOut != 0) {
                    game_setStatusIcon(STATUS_ICON_TIMEOUT);
                    game_setStatusText("Sorry, timed out!");
                    requestNewQuestion(5);
                } else {
                    requestNewQuestion(3);
                }
            } else if (equalLiteral(response.main, "LST")) {
                debugPrint("ListenerThread got LST message (%d)", ntohs(response.main.length));
                int count = ntohs(response.main.length) / 37;
                for (int i = 0; i < count; i++) {
                    game_setPlayerName(i + 1, response.playerList.players[i].name);
                    game_setPlayerScore(i + 1, ntohl(response.playerList.players[i].points));
                    debugPrint("LST %d: %s %d", i, response.playerList.players[i].name, ntohl(response.playerList.players[i].points));
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
                debugPrint("Received GameOver message: %d", response.gameOver.rank);
                char buffer[1024];
                buffer[1023] = '\0';
                snprintf(buffer, 1023, "Game Over! You ranked on place %d!", response.gameOver.rank);
                guiShowMessageDialog(buffer, 1);
                stopThreads();
                return NULL;
            } else if (equalLiteral(response.main, "LST")) {
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


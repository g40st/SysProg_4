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
#include "gui/gui_interface.h"
#include "gui.h"
#include "listener.h"

static int running = 1;
static GamePhase_t gamePhase = PHASE_PREPARATION;
static pthread_mutex_t mutexUsers = PTHREAD_MUTEX_INITIALIZER;

void setGamePhase(GamePhase_t p) {
    pthread_mutex_lock(&mutexUsers);
    gamePhase = p;
    pthread_mutex_unlock(&mutexUsers);
}

GamePhase_t getGamePhase(void) {
    pthread_mutex_lock(&mutexUsers);
    GamePhase_t r = gamePhase;
    pthread_mutex_unlock(&mutexUsers);
    return r;
}

int getRunning(void) {
    pthread_mutex_lock(&mutexUsers);
    int r = running;
    pthread_mutex_unlock(&mutexUsers);
    return r;
}

void stopThreads(void) {
    pthread_mutex_lock(&mutexUsers);
    running = 0;
    pthread_mutex_unlock(&mutexUsers);
}

void *listenerThread(void *arg) {
    int socket = *((int *)arg);
    rfc response;
    debugPrint("ListenerThread is starting its loop...");
    while (getRunning()) {
        // Receive message
        int receive = recv(socket, &response, RFC_MAX_SIZE, 0);
        if (receive == -1) {
            errnoPrint("receive");
            return NULL;
        } else if (receive == 0) {
            errorPrint("Remote host closed connection");
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
                }
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

                // Send QuestionRequest QRQ
                response.main.type[0] = 'Q';
                response.main.type[1] = 'R';
                response.main.type[2] = 'Q';
                response.main.length = htons(0);
                if (send(socket, &response.main, RFC_BASE_SIZE, 0) == -1) {
                    errnoPrint("send");
                    return NULL;
                }
            } else {
                errorPrint("Unexpected response in PhasePreparation: %c%c%c", response.main.type[0],
                        response.main.type[1], response.main.type[2]);
            }
        } else if (getGamePhase() == PHASE_GAME) {
            if (equalLiteral(response.main, "QUE")) {
                if (ntohs(response.main.length) == 769) {
                    debugPrint("Received new question-answer set... Timeout: %d", response.question.timeout);
                    game_clearAnswerMarksAndHighlights();
                    game_setQuestion(response.question.question);
                    for (int i = 0; i < NUM_ANSWERS; i++)
                        game_setAnswer(i, response.question.answers[i]);
                    // TODO do something with timeout!
                } else if (ntohs(response.main.length) == 0) {
                    // Move to End Phase
                    debugPrint("The PhaseEnd has now started!");
                    setGamePhase(PHASE_END);
                } else {
                    debugPrint("Received Question with nonsense length: %d", ntohs(response.main.length));
                }
            } else if (equalLiteral(response.main, "QRE")) {
                // TODO display results to user
                debugPrint("Received QuestionResult: %d %d", response.questionResult.timedOut, response.questionResult.correct);
            } else if (equalLiteral(response.main, "LST")) {
                debugPrint("ListenerThread got LST message (%d)", ntohs(response.main.length));
                int count = ntohs(response.main.length) / 37;
                for (int i = 0; i < count; i++) {
                    game_setPlayerName(i, response.playerList.players[i].name);
                    game_setPlayerScore(i, response.playerList.players[i].points);
                }
                for (int i = count; i < MAX_PLAYERS; i++) {
                    game_setPlayerName(i, "");
                    game_setPlayerScore(i, 0);
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
                guiShowMessageDialog(buffer, 0);
                return NULL;
            } else if (equalLiteral(response.main, "LST")) {
                debugPrint("ListenerThread got LST message (%d)", ntohs(response.main.length));
                int count = ntohs(response.main.length) / 37;
                for (int i = 0; i < count; i++) {
                    game_setPlayerName(i, response.playerList.players[i].name);
                    game_setPlayerScore(i, response.playerList.players[i].points);
                }
                for (int i = count; i < MAX_PLAYERS; i++) {
                    game_setPlayerName(i, "");
                    game_setPlayerScore(i, 0);
                }
            } else {
                errorPrint("Unexpected response in PhaseEnd: %c%c%c", response.main.type[0],
                        response.main.type[1], response.main.type[2]);
            }
        }
    }
    return NULL;
}


/*
 * Systemprogrammierung
 * Multiplayer-Quiz
 *
 * Server
 *
 * clientthread.c: Implementierung des Client-Threads
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <netinet/in.h>
#include <sys/mman.h>
#include <time.h>

#include "catalog.h"
#include "score.h"
#include "user.h"
#include "common/rfc.h"
#include "common/server_loader_protocol.h"
#include "common/util.h"
#include "clientthread.h"

#define MAX_CATEGORIES 64
#define PHASE_PREPARATION 0
#define PHASE_GAME 1
#define PHASE_END 2

static char *categories[MAX_CATEGORIES];
static int numCategories = 0;
static int selectedCategory = -1;
static pthread_mutex_t mutexCategories = PTHREAD_MUTEX_INITIALIZER;
static int countFinished = 0;

static time_t startTime = 0;

void addCategory(const char *c) {
    pthread_mutex_lock(&mutexCategories);
    if (numCategories < (MAX_CATEGORIES - 1)) {
        categories[numCategories] = malloc(strlen(c) + 1);
        strcpy(categories[numCategories], c);
        numCategories++;
    }
    pthread_mutex_unlock(&mutexCategories);
}

int countCategory(void) {
    pthread_mutex_lock(&mutexCategories);
    int c = numCategories;
    pthread_mutex_unlock(&mutexCategories);
    return c;
}

const char *getCategory(int index) {
    const char *r = NULL;
    pthread_mutex_lock(&mutexCategories);
    if ((index >= 0) && (index < numCategories))
        r = categories[index];
    pthread_mutex_unlock(&mutexCategories);
    return r;
}

void selectCategory(int index) {
    pthread_mutex_lock(&mutexCategories);
    if ((index >= 0) && (index < numCategories))
        selectedCategory = index;
    pthread_mutex_unlock(&mutexCategories);
}

void cleanCategories(void) {
    pthread_mutex_lock(&mutexCategories);
    for (int i = 0; i < numCategories; i++)
        free(categories[i]);
    numCategories = 0;
    selectedCategory = -1;
    pthread_mutex_unlock(&mutexCategories);
}

void *clientThread(void *arg) {
    int present = 0;
    int loaded = 0;
    int catalogHasBeenChanged = 0;
    char *catalogThatHasBeenChanged = NULL;

    startTime = time(NULL);
    srand(startTime);

    while (getRunning()) {
        // Send BROWSE command to loader
        if ((!present) && userGetPresent(0)) {
            present = 1;
            sendLoaderCommand(BROWSE_CMD);

            // Read responses from loader
            char buff[BUFFER_SIZE];
            int run = 1;
            while (run) {
                readLineLoader(buff, BUFFER_SIZE);
                if (buff[0] == '\0') {
                    run = 0;
                } else {
                    addCategory(buff);
                    debugPrint("Loader Category: \"%s\"", buff);
                }
            }
        }

        // Send LOAD command to loader
        if (present && (!loaded) && (getGamePhase() == PHASE_GAME)) {
            pthread_mutex_lock(&mutexCategories);
            if ((selectedCategory < 0) || (selectedCategory >= numCategories)) {
                errorPrint("Error: Trying to load while no category is selected!");
                setGamePhase(PHASE_PREPARATION);
                sendWarningMessage(userGetSocket(0), "Please select a category!");
            } else {
                shm_unlink(SHMEM_NAME);
                loaded = 1;
                char buff[BUFFER_SIZE];
                snprintf(buff, BUFFER_SIZE, "%s%s", LOAD_CMD_PREFIX, categories[selectedCategory]);
                sendLoaderCommand(buff);

                // Read response from loader
                readLineLoader(buff, BUFFER_SIZE);
                if (strncmp(buff, LOAD_ERROR_PREFIX, strlen(LOAD_ERROR_PREFIX)) == 0) {
                    errorPrint("Loader: %s", buff);
                    // TODO handle error. Change GamePhase back? Simply exit?
                } else if (strncmp(buff, LOAD_SUCCESS_PREFIX, strlen(LOAD_SUCCESS_PREFIX)) == 0) {
                    int size = 0;
                    if (sscanf(buff, LOAD_SUCCESS_PREFIX "%d", &size) != 1) {
                        errorPrint("Could not parse loader answer: %s", buff);
                        // TODO handle error. Change GamePhase back? Simply exit?
                    }
                    if (!loaderOpenSharedMemory(size)) {
                        // TODO handle error. Change GamePhase back? Simply exit?
                    }
                } else {
                    errorPrint("Unknown response from loader: \"%s\"", buff);
                    // TODO handle error. Change GamePhase back? Simply exit?
                }
            }
            pthread_mutex_unlock(&mutexCategories);
        }

        // Check all sockets for activity
        int result = waitForSockets(SOCKET_TIMEOUT);
        if (result == -2) {
            debugPrint("Error waiting for socket activity...");
            // TODO handle error?
        } else if (result > -1) {
            // Read received message
            int socket = userGetSocket(result);
            rfc response;
            int receive = receivePacket(socket, &response);
            if (receive == -1) {
                // TODO handle error?
            } else if (receive == 0) {
                userSetPresent(result, 0);
                if (result == 0) {
                    debugPrint("Game master closed connection!");
                    for (int i = 1; i < MAX_PLAYERS; i++) {
                        if (userGetPresent(i))
                            sendErrorMessage(userGetSocket(i), "Game master closed connection!");
                    }
                    stopThreads();
                } else {
                    debugPrint("Player closed connection!");
                    scoreMarkForUpdate();
                }
            } else if (getGamePhase() == PHASE_PREPARATION) {
                if (equalLiteral(response.main, "CRQ")) {
                    debugPrint("Got CatalogRequest from ID %d", result);
                    response.main.type[0] = 'C';
                    response.main.type[1] = 'R';
                    response.main.type[2] = 'E';
                    pthread_mutex_lock(&mutexCategories);
                    for (int i = 0; i < (numCategories + 1); i++) {
                        int len = 0;
                        if (i < numCategories) {
                            len = strlen(categories[i]);
                            memcpy(response.catalogResponse.filename, categories[i], len);
                        }
                        response.main.length = htons(len);
                        if (send(socket, &response, RFC_BASE_SIZE + len, 0) == -1) {
                            errnoPrint("ClientThread1 send");
                        }
                    }
                    pthread_mutex_unlock(&mutexCategories);

                    // Relay CCH message to new clients!
                    if (catalogHasBeenChanged && (!userGetLastCCH(result))) {
                        debugPrint("Relaying CatalogChange: %s", catalogThatHasBeenChanged);
                        int len = strlen(catalogThatHasBeenChanged);
                        response.main.type[0] = 'C';
                        response.main.type[1] = 'C';
                        response.main.type[2] = 'H';
                        response.main.length = htons(len);
                        memcpy(response.catalogChange.filename, catalogThatHasBeenChanged, len);
                        if (send(socket, &response, RFC_BASE_SIZE + len, 0) == -1) {
                            errnoPrint("ClientThread2 send");
                        }
                    }
                } else if (equalLiteral(response.main, "CCH")) {
                    debugPrint("Got CatalogChanged from ID %d", result);
                    pthread_mutex_lock(&mutexCategories);
                    char buff[ntohs(response.main.length) + 1];
                    buff[ntohs(response.main.length)] = '\0';
                    memcpy(buff, response.catalogChange.filename, ntohs(response.main.length));
                    for (int i = 0; i < numCategories; i++) {
                        if (strcmp(categories[i], buff) == 0) {
                            selectedCategory = i;
                        }
                    }
                    pthread_mutex_unlock(&mutexCategories);
                    for (int i = 0; i < MAX_PLAYERS; i++) {
                        if ((i != result) && userGetPresent(i)) {
                            if (send(userGetSocket(i), &response,
                                        RFC_BASE_SIZE + ntohs(response.main.length), 0) == -1) {
                                errnoPrint("ClientThread3 send");
                            }
                        }
                        if (userGetPresent(i))
                            userSetLastCCH(i, 1);
                    }
                    if (catalogThatHasBeenChanged != NULL)
                        free(catalogThatHasBeenChanged);
                    catalogThatHasBeenChanged = malloc((ntohs(response.main.length) + 1) * sizeof(char));
                    if (catalogThatHasBeenChanged == NULL) {
                        errorPrint("Not enough memory for CCH relaying!");
                    } else {
                        memcpy(catalogThatHasBeenChanged, buff, ntohs(response.main.length) + 1);
                        catalogHasBeenChanged = 1;
                    }
                } else if (equalLiteral(response.main, "STG")) {
                    if (result != 0) {
                        infoPrint("Received StartGame from unprivileged ID %d", result);
                    } else {
                        debugPrint("Got StartGame game master");
                        setGamePhase(PHASE_GAME);
                        for (int i = 0; i < MAX_PLAYERS; i++) {
                            if ((i != result) && userGetPresent(i)) {
                                if (send(userGetSocket(i), &response,
                                            RFC_BASE_SIZE + ntohs(response.main.length), 0) == -1) {
                                    errnoPrint("ClientThread4 send");
                                }
                            }
                        }
                        scoreMarkForUpdate();

                        // Clear old question answered marks
                        for (int i = 0; i < MAX_QUESTIONS; i++) {
                            userSetQuestion(result, i, 0);
                        }
                        countFinished = 0;
                    }
                } else {
                    errorPrint("Unexpected message in PhasePreparation: %c%c%c", response.main.type[0],
                            response.main.type[1], response.main.type[2]);
                }
            } else if (getGamePhase() == PHASE_GAME) {
                if (equalLiteral(response.main, "QRQ")) {
                    if (userCountQuestionsAnswered(result) < getQuestionCount()) {
                        // Find next question, send to client
                        int qi = rand() % getQuestionCount();
                        while (userGetQuestion(result, qi) != 0) {
                            qi = rand() % getQuestionCount();
                        }
                        debugPrint("Sending question %d to Player %d", qi, result);
                        userSetQuestion(result, qi, 1);
                        Question *q = getQuestion(qi);
                        userSetLastTimeout(result, (time(NULL) - startTime) + q->timeout);
                        for (int i = 0; i < QUESTION_SIZE; i++) {
                            response.question.question.question[i] = q->question[i];
                        }
                        for (int i = 0; i < NUM_ANSWERS; i++) {
                            for (int j = 0; j < ANSWER_SIZE; j++) {
                                response.question.question.answers[i][j] = q->answers[i][j];
                            }
                        }
                        response.question.question.timeout = q->timeout;
                        response.main.type[0] = 'Q';
                        response.main.type[1] = 'U';
                        response.main.type[2] = 'E';
                        response.main.length = htons(RFC_QUESTION_SIZE);
                        if (send(userGetSocket(result), &response,
                                    RFC_BASE_SIZE + RFC_QUESTION_SIZE, 0) == -1) {
                            errnoPrint("ClientThread5 send");
                        }
                    } else {
                        debugPrint("Player %d answered all questions!", result);
                        // Send empty QUE message
                        response.main.type[0] = 'Q';
                        response.main.type[1] = 'U';
                        response.main.type[2] = 'E';
                        response.main.length = htons(0);
                        if (send(userGetSocket(result), &response,
                                    RFC_BASE_SIZE, 0) == -1) {
                            errnoPrint("ClientThread6 send");
                        }
                        countFinished++;

                        // All players finished? Send GOV to all
                        int presentPlayers = 0;
                        for (int i = 0; i < MAX_PLAYERS; i++) {
                            if (userGetPresent(i)) {
                                presentPlayers++;
                            }
                        }

                        // TODO countFinished presentPlayers will break
                        // if users quit while the game is running!
                        // Store data properly in user database...

                        if (countFinished >= presentPlayers) {
                            debugPrint("All players seem to be finished, sending GOV!");
                            response.main.type[0] = 'G';
                            response.main.type[1] = 'O';
                            response.main.type[2] = 'V';
                            response.main.length = htons(1);
                            for (int i = 0; i < MAX_PLAYERS; i++) {
                                if (userGetPresent(i)) {
                                    response.gameOver.rank = userGetRank(i);
                                    if (send(userGetSocket(i), &response,
                                                RFC_BASE_SIZE + 1, 0) == -1) {
                                        errnoPrint("ClientThread7 send");
                                    }
                                }
                            }
                        }
                    }
                } else if (equalLiteral(response.main, "QAN")) {
                    // Check answer
                    int qu = -1;
                    for (int i = 0; i < getQuestionCount(); i++) {
                        if (userGetQuestion(result, i) == 1) {
                            qu = i;
                            break;
                        }
                    }
                    if (qu == -1) {
                        debugPrint("Player %d sent answer without a question?!", result);
                        continue;
                    }
                    int correct = 0, timeout = 0;
                    if (response.questionAnswered.selection == getQuestion(qu)->correct) {
                        correct = 1;
                    }
                    int diff = userGetLastTimeout(result);
                    diff -= time(NULL) - startTime;
                    if (diff < 0) {
                        timeout = 1;
                    }
                    if (correct && (!timeout)) {
                        userAddScore(result, diff, getQuestion(qu)->timeout);
                        scoreMarkForUpdate();
                    }
                    debugPrint("Player %d sent an answer (c%d t%d)...", result, correct, timeout);
                    userSetQuestion(result, qu, 2); // Mark as scored

                    response.main.type[0] = 'Q';
                    response.main.type[1] = 'R';
                    response.main.type[2] = 'E';
                    response.questionResult.timedOut = timeout;
                    response.questionResult.correct = getQuestion(qu)->correct;
                    response.main.length = htons(2);
                    if (send(userGetSocket(result), &response,
                                RFC_QUESTION_RESULT_SIZE, 0) == -1) {
                        errnoPrint("ClientThread8 send");
                    }
                } else {
                    errorPrint("Unexpected message in PhaseGame: %c%c%c", response.main.type[0],
                            response.main.type[1], response.main.type[2]);
                }
            } else {
                errorPrint("Unexpected message: %c%c%c", response.main.type[0],
                        response.main.type[1], response.main.type[2]);
            }
        }

        loopsleep();
    }

    return NULL;
}


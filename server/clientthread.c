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

// Storage for the different questionnaire names
static char *categories[MAX_CATEGORIES];
static int numCategories = 0;
static int selectedCategory = -1;
static pthread_mutex_t mutexCategories = PTHREAD_MUTEX_INITIALIZER;

static time_t startTime = 0;

// Add a new category to the list
void addCategory(const char *c) {
    pthread_mutex_lock(&mutexCategories);
    if (numCategories < (MAX_CATEGORIES - 1)) {
        categories[numCategories] = malloc(strlen(c) + 1);
        strcpy(categories[numCategories], c);
        numCategories++;
    }
    pthread_mutex_unlock(&mutexCategories);
}

// Free all the allocated memory in the category list
void cleanCategories(void) {
    pthread_mutex_lock(&mutexCategories);
    for (int i = 0; i < numCategories; i++)
        free(categories[i]);
    numCategories = 0;
    selectedCategory = -1;
    pthread_mutex_unlock(&mutexCategories);
}

static void sendBrowse(void) {
    sendLoaderCommand(BROWSE_CMD);

    // Read the responses from the loader
    char buff[BUFFER_SIZE];
    int run = 1;
    while (run) {
        readLineLoader(buff, BUFFER_SIZE);
        if (buff[0] == '\0') {
            run = 0;
        } else {
            // Add them to the category list
            addCategory(buff);
            debugPrint("Loader Category: \"%s\"", buff);
        }
    }
}

static void handleQuestionTimeout(int to) {
    // Check if there is a timed out question
    if (to < 0)
        return;

    int ti = time(NULL) - startTime;
    if (ti < to) {
        return;
    }

    // Find out whose question timed out
    int user = -1;
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (userGetPresent(i)) {
            if (userGetLastTimeout(i) == to) {
                user = i;
                break;
            }
        }
    }

    if (user == -1) {
        debugPrint("Timeout but can't find out for which player?!");
        return;
    }

    // Find out which question he answered
    int qu = -1;
    for (int i = 0; i < getQuestionCount(); i++) {
        if (userGetQuestion(user, i) == 1) {
            qu = i;
            break;
        }
    }

    if (qu == -1) {
        debugPrint("Player %d timed out without a question?!", user);
        return;
    }

    debugPrint("Player %d timed out question %d!", user, qu);

    // Mark question as answered
    userSetQuestion(user, qu, 2);
    userSetLastTimeout(user, -1);

    rfc response;
    response.main.type[0] = 'Q';
    response.main.type[1] = 'R';
    response.main.type[2] = 'E';
    response.questionResult.timedOut = 1;
    response.questionResult.correct = getQuestion(qu)->correct;
    response.main.length = htons(2);
    if (send(userGetSocket(user), &response,
                RFC_QUESTION_RESULT_SIZE, 0) == -1) {
        errnoPrint("handleQuestionTimeout send");
    }
}

void *clientThread(void *arg) {
    int loaded = 0;
    int catalogHasBeenChanged = 0;
    char *catalogThatHasBeenChanged = NULL;

    // Seed the random number generator used to select questions
    startTime = time(NULL);
    srand(startTime);

    // Send the BROWSE command to the loader
    sendBrowse();

    // Client Threads main loop
    while (getRunning()) {
        /*
         * Send the LOAD command to the loader if we entered the
         * main game phase, already sent BROWSE and did not yet
         * send the LOAD command.
         */
        if ((!loaded) && (getGamePhase() == PHASE_GAME)) {
            pthread_mutex_lock(&mutexCategories);
            if ((selectedCategory < 0) || (selectedCategory >= numCategories)) {
                errorPrint("Error: Trying to load while no category is selected!");
                setGamePhase(PHASE_PREPARATION);
                sendWarningMessage(userGetSocket(0), "Please select a category!");
            } else {
                // Prepare the shared memory
                shm_unlink(SHMEM_NAME);
                loaded = 1;

                // Send the actual LOAD command
                char buff[BUFFER_SIZE];
                snprintf(buff, BUFFER_SIZE, "%s%s", LOAD_CMD_PREFIX, categories[selectedCategory]);
                sendLoaderCommand(buff);

                // Read the response from the loader
                readLineLoader(buff, BUFFER_SIZE);
                if (strncmp(buff, LOAD_ERROR_PREFIX, strlen(LOAD_ERROR_PREFIX)) == 0) {
                    errorPrint("Loader: %s", buff);
                    stopThreads();
                    return NULL;
                } else if (strncmp(buff, LOAD_SUCCESS_PREFIX, strlen(LOAD_SUCCESS_PREFIX)) == 0) {
                    int size = 0;
                    if (sscanf(buff, LOAD_SUCCESS_PREFIX "%d", &size) != 1) {
                        errorPrint("Could not parse loader answer: %s", buff);
                        stopThreads();
                        return NULL;
                    }

                    // Success! Try to open the shared memory...
                    if (!loaderOpenSharedMemory(size)) {
                        stopThreads();
                        return NULL;
                    }
                } else {
                    errorPrint("Unknown response from loader: \"%s\"", buff);
                    stopThreads();
                    return NULL;
                }
            }
            pthread_mutex_unlock(&mutexCategories);
        }

        int nextTimeout = userGetNextTimeout();
        int socketTimeout = SOCKET_TIMEOUT; // Default timeout
        if (nextTimeout > -1) {
            // Let waitForSockets timeout when the next user question times out
            socketTimeout = (nextTimeout - (time(NULL) - startTime)) * 1000;

            if (socketTimeout <= 0) {
                // We have already _forgot_ to take care of a timed out question
                debugPrint("Missed question timeout?!");
                handleQuestionTimeout(nextTimeout);
                continue;
            }
        }

        // Check all sockets for activity
        int result = waitForSockets(socketTimeout);
        if (result == -2) {
            errorPrint("Error waiting for socket activity...");
            stopThreads();
            return NULL;
        } else if (result == -1) {
            // Timeout, check for question timeouts
            if (nextTimeout > -1) {
                debugPrint("waitForSockets question timeout...");
                handleQuestionTimeout(nextTimeout);
            }
        } else if (result > -1) {
            // Read received message
            int socket = userGetSocket(result);
            rfc response;
            int receive = receivePacket(socket, &response);
            if (receive == -1) {
                errorPrint("Error reading data from client %d...", result);
                receive = 0;
            }

            if (receive == 0) {
                // The user has closed its connection!
                userSetPresent(result, 0);
                if (result == 0) {
                    /*
                     * When the game master closes it's connection, we
                     * have to send an error message to all other remaining
                     * clients and then kill ourselves.
                     */
                    debugPrint("Game master closed connection!");
                    for (int i = 1; i < MAX_PLAYERS; i++) {
                        if (userGetPresent(i))
                            sendErrorMessage(userGetSocket(i), "Game master closed connection!");
                    }
                    stopThreads();
                    return NULL;
                } else {
                    /*
                     * If an ordinary player closes the connection, we only need
                     * to end the game if it was the last player besides the game master.
                     */
                    debugPrint("Player %d closed connection!", result);
                    if ((userCount() > 1) || (getGamePhase() == PHASE_PREPARATION)) {
                        scoreMarkForUpdate();
                    } else {
                        sendErrorMessage(userGetSocket(0), "Last other player closed connection!");
                        stopThreads();
                        return NULL;
                    }
                }
            } else if (getGamePhase() == PHASE_PREPARATION) {
                /*
                 * Check for messages we want to receive in the
                 * game preparation phase.
                 */
                if (equalLiteral(response.main, "CRQ")) {
                    /*
                     * A client has requested a list of available questionnaires.
                     * Prepare and send it to him.
                     */
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

                    /*
                     * Here we need to relay CCH messages, because this is
                     * usually the first message we receive from a new client.
                     * So if we have a cached CCH message, and he did not get it,
                     * send it to him here.
                     */
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
                    /*
                     * A client has selected a new questionnaire. This can only
                     * be done by the game master!
                     */
                    debugPrint("Got CatalogChanged from ID %d", result);
                    pthread_mutex_lock(&mutexCategories);
                    char buff[ntohs(response.main.length) + 1];
                    buff[ntohs(response.main.length)] = '\0';
                    memcpy(buff, response.catalogChange.filename, ntohs(response.main.length));
                    for (int i = 0; i < numCategories; i++) {
                        // Find which category was set and store this index
                        if (strcmp(categories[i], buff) == 0) {
                            selectedCategory = i;
                        }
                    }
                    pthread_mutex_unlock(&mutexCategories);
                    for (int i = 0; i < MAX_PLAYERS; i++) {
                        if ((i != result) && userGetPresent(i)) {
                            // Send the CCH message to all other players
                            if (send(userGetSocket(i), &response,
                                        RFC_BASE_SIZE + ntohs(response.main.length), 0) == -1) {
                                errnoPrint("ClientThread3 send");
                            }
                        }

                        // Store the info that they got this message (for relaying)
                        if (userGetPresent(i))
                            userSetLastCCH(i, 1);
                    }

                    // Store the name using dynamic memory for the CCH relaying
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
                    // Start Game requests can only be executed by the game master!
                    if (result != 0) {
                        infoPrint("Received StartGame from unprivileged ID %d", result);
                    } else {
                        debugPrint("Got StartGame game master");

                        // Set the new phase
                        setGamePhase(PHASE_GAME);
                        for (int i = 0; i < MAX_PLAYERS; i++) {
                            if (userGetPresent(i)) {
                                // Relay the message to all other players
                                if (send(userGetSocket(i), &response,
                                            RFC_BASE_SIZE + ntohs(response.main.length), 0) == -1) {
                                    errnoPrint("ClientThread4 send");
                                }
                            }
                        }

                        // Send the player list to everyone again
                        scoreMarkForUpdate();
                    }
                } else {
                    errorPrint("Unexpected message in PhasePreparation: %c%c%c", response.main.type[0],
                            response.main.type[1], response.main.type[2]);
                }

            /*
             * Check for messages that we want to receive in the main
             * game phase...
             */
            } else if (getGamePhase() == PHASE_GAME) {
                if (equalLiteral(response.main, "QRQ")) {
                    // A player has requested the next question
                    if (userCountQuestionsAnswered(result) < getQuestionCount()) {
                        // If he has not answered all questions
                        int qi = rand() % getQuestionCount();

                        // Find a new question index he did not already answer
                        while (userGetQuestion(result, qi) != 0) {
                            qi = rand() % getQuestionCount();
                        }
                        debugPrint("Sending question %d to Player %d", qi, result);

                        // Mark this question as sent
                        userSetQuestion(result, qi, 1);

                        // Prepare the packet we want to send as response
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

                        // Send the Question, its answers and the timeout.
                        if (send(userGetSocket(result), &response,
                                    RFC_BASE_SIZE + RFC_QUESTION_SIZE, 0) == -1) {
                            errnoPrint("ClientThread5 send");
                        }
                    } else {
                        // This player has finished answering all his questions
                        debugPrint("Player %d answered all questions!", result);
                        // Send an empty QUE message
                        response.main.type[0] = 'Q';
                        response.main.type[1] = 'U';
                        response.main.type[2] = 'E';
                        response.main.length = htons(0);
                        if (send(userGetSocket(result), &response,
                                    RFC_BASE_SIZE, 0) == -1) {
                            errnoPrint("ClientThread6 send");
                        }
                        userSetLastTimeout(result, -1);

                        // Count the amount of finished players
                        int countFinished = 0;
                        for (int i = 0; i < MAX_PLAYERS; i++) {
                            if (userGetPresent(i)) {
                                if (userCountQuestionsAnswered(i) >= getQuestionCount()) {
                                    countFinished++;
                                }
                            }
                        }

                        if (countFinished >= userCount()) {
                            // If everyone is finished, send a Game Over message
                            debugPrint("All players seem to be finished, sending GOV!");
                            response.main.type[0] = 'G';
                            response.main.type[1] = 'O';
                            response.main.type[2] = 'V';
                            response.main.length = htons(1);
                            for (int i = 0; i < MAX_PLAYERS; i++) {
                                if (userGetPresent(i)) {
                                    // Send each player his personal ranking
                                    response.gameOver.rank = userGetRank(i);
                                    if (send(userGetSocket(i), &response,
                                                RFC_BASE_SIZE + 1, 0) == -1) {
                                        errnoPrint("ClientThread7 send");
                                    }

                                    debugPrint("Player %d ranked on %d!", i, response.gameOver.rank);
                                }
                            }

                            // The game is finished
                            debugPrint("Game is now over. Killing server...");
                            stopThreads();
                            return NULL;
                        }
                    }
                } else if (equalLiteral(response.main, "QAN")) {
                    // A player has sent his answer for a question
                    int qu = -1;

                    // Find out which question he answered
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

                    // Check if the answer was correct and fast enough
                    int correct = 0, timeout = 0;
                    if (response.questionAnswered.selection == getQuestion(qu)->correct) {
                        correct = 1;
                    }
                    int diff = userGetLastTimeout(result);
                    diff -= time(NULL) - startTime;
                    if (diff < 0) {
                        timeout = 1;
                    }

                    // If it is correct and not timed out
                    if (correct && (!timeout)) {
                        // Increase the users score
                        userAddScore(result, diff, getQuestion(qu)->timeout);

                        // Send a new score list to everyone
                        scoreMarkForUpdate();
                    }
                    debugPrint("Player %d sent an answer (c%d t%d)...", result, correct, timeout);

                    // Mark the question as scored
                    userSetQuestion(result, qu, 2);
                    userSetLastTimeout(result, -1);

                    if (timeout) {
                        debugPrint("Player %d trying to answer timed out question...", result);
                        continue;
                    }

                    // Send the result to the player
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
    }

    return NULL;
}


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

static int gamePhase = 0;

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

    while (1) {
        // Send BROWSE command to loader
        // TODO should we really BROWSE only if a user is logged in?
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
        if (present && (!loaded) && (gamePhase == PHASE_GAME)) {
            pthread_mutex_lock(&mutexCategories);
            if ((selectedCategory < 0) || (selectedCategory >= numCategories)) {
                errorPrint("Error: Trying to load while no category is selected!");
            } else {
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
                    // TODO send STG messages
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
        } else if (result > -1) {
            // Read received message
            int socket = userGetSocket(result);
            rfc response;
            int receive = recv(socket, &response, RFC_MAX_SIZE, 0);
            if (receive == -1) {
                errnoPrint("receive");
            } else if (receive == 0) {
                debugPrint("Remote host closed connection");
                userSetPresent(result, 0);
                scoreMarkForUpdate();
            } else {
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
                            errnoPrint("send");
                        }
                    }
                    pthread_mutex_unlock(&mutexCategories);
                //} else if (equalLiteral(response.main, "")) {
                } else {
                    errorPrint("Unexpected message: %c%c%c", response.main.type[0],
                            response.main.type[1], response.main.type[2]);
                }
            }
        }

        loopsleep();
    }

    return NULL;
}


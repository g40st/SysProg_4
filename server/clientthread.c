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
#include "user.h"
#include "common/server_loader_protocol.h"
#include "common/util.h"
#include "clientthread.h"

#define MAX_CATEGORIES 64

char *categories[MAX_CATEGORIES];
int numCategories = 0;
pthread_mutex_t mutexCategories = PTHREAD_MUTEX_INITIALIZER;

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

void cleanCategories(void) {
    pthread_mutex_lock(&mutexCategories);
    for (int i = 0; i < numCategories; i++)
        free(categories[i]);
    numCategories = 0;
    pthread_mutex_unlock(&mutexCategories);
}

void *clientThread(void *arg) {
    int present = 0;

    while (1) {
        if ((!present) && userGetPresent(0)) {
            present = 1;
            sendLoaderCommand(BROWSE_CMD);

            char buff[1024];
            while (1) {
                readLineLoader(buff, 1024);
                if (buff[0] == '\0') {
                    // Received empty line
                    break;
                }
                addCategory(buff);
                debugPrint("Loader Category: \"%s\"", buff);
            }
        }

        loopsleep();
    }

    return NULL;
}


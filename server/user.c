/*
 * Systemprogrammierung
 * Multiplayer-Quiz
 *
 * Server
 *
 * user.c: Implementierung der User-Verwaltung
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "common/util.h"
#include "user.h"

static char *users[4] = { NULL, NULL, NULL, NULL };
static int numUsers = 0;
static pthread_mutex_t mutexUsers = PTHREAD_MUTEX_INITIALIZER;

void userCountSet(int c) {
    if ((c >= 0) && (c < MAX_PLAYERS)) {
        pthread_mutex_lock(&mutexUsers);
        numUsers = c;
        pthread_mutex_unlock(&mutexUsers);
    }
}

int userCount(void) {
    pthread_mutex_lock(&mutexUsers);
    int num = numUsers;
    pthread_mutex_unlock(&mutexUsers);
    return num;
}

// Returns NULL on error
const char *userGet(int index) {
    const char *ret = NULL;
    pthread_mutex_lock(&mutexUsers);
    if ((index >= 0) && (index < numUsers)) {
        ret = users[index];
    }
    pthread_mutex_unlock(&mutexUsers);
    return ret;
}

void userSet(const char *name, int index) {
    if ((index >= 0) && (index < MAX_PLAYERS)) {
        pthread_mutex_lock(&mutexUsers);
        users[index] = malloc((strlen(name) + 1) * sizeof(char));
        if (users[index] == NULL) {
            printf("Not enough memory!\n");
        } else {
            strcpy(users[index], name);
        }
        pthread_mutex_unlock(&mutexUsers);
    }
}


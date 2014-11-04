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

#include "user.h"

static char **users = NULL;
static int numUsers = 0;
static pthread_mutex_t mutexUsers = PTHREAD_MUTEX_INITIALIZER;

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

// Returns -1 on error, else new index
int userAdd(const char *name) {
    pthread_mutex_lock(&mutexUsers);
    for (int i = 0; i < numUsers; i++) {
        if (strcmp(users[i], name) == 0) {
            pthread_mutex_unlock(&mutexUsers);
            return -1;
        }
    }

    numUsers++;
    char **newUsers = realloc(users, numUsers * sizeof(const char *));
    if (newUsers == NULL) {
        free(users);
        users = NULL;
        numUsers = 0;
        pthread_mutex_unlock(&mutexUsers);
        printf("Not enough memory!\n");
        return -1;
    }

    users = newUsers;
    int id = numUsers - 1;
    users[id] = malloc((strlen(name) + 1) * sizeof(char));
    if (users[id] == NULL) {
        free(users);
        users = NULL;
        numUsers = 0;
        pthread_mutex_unlock(&mutexUsers);
        printf("Not enough memory!\n");
        return -1;
    }
    strcpy(users[id], name);
    pthread_mutex_unlock(&mutexUsers);
    return id;
}

// Returns 1 on success, 0 on error
int userRemove(int index) {
    pthread_mutex_lock(&mutexUsers);
    if ((index >= 0) && (index < numUsers)) {
        memmove((void *)(users + index), (void *)(users + index + 1), numUsers - index - 1);
        numUsers--;
        char **newUsers = realloc(users, numUsers * sizeof(const char *));
        if (newUsers == NULL) {
            free(users);
            users = NULL;
            numUsers = 0;
            pthread_mutex_unlock(&mutexUsers);
            printf("Not enough memory!\n");
            return 0;
        }
        pthread_mutex_unlock(&mutexUsers);
        return 1;
    } else {
        pthread_mutex_unlock(&mutexUsers);
        return 0;
    }
}


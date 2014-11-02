/*
 * Systemprogrammierung
 * Multiplayer-Quiz
 *
 * Server
 *
 * login.c: Implementierung des Logins
 */

#include "login.h"

#include <stdlib.h>
#include <pthread.h>

void *loginThread(void *arg) {
    struct loginState *state = (struct loginState *)arg;

    pthread_mutex_lock(state->socketMutex);
    int oldCount = *state->socketCount;
    pthread_mutex_unlock(state->socketMutex);

    while (1) {
        pthread_mutex_lock(state->socketMutex);
        int newCount = *state->socketCount;
        if (newCount > oldCount) {
            // We've got a new socket...
            int socket = state->sockets[*state->socketCount - 1];

            // TODO read from socket

            oldCount = newCount;
        }
        pthread_mutex_unlock(state->socketMutex);
    }

    return NULL;
}


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

    //state->sockets[42]
    //&state->socketCount
    //pthread_mutex_lock(state->socketMutex)

    return NULL;
}


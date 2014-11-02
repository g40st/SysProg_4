/*
 * Systemprogrammierung
 * Multiplayer-Quiz
 *
 * Server
 *
 * login.h: Header für das Login
 */

#ifndef LOGIN_H
#define LOGIN_H

#include <pthread.h>

struct loginState {
    int *sockets; // [MAX_SOCKETS]
    int *socketCount;
    pthread_mutex_t *socketMutex;
};

void *loginThread(void *arg);

#endif

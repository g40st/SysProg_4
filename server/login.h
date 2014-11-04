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

void loginAddSocket(int sock);

void *loginThread(void *arg);

#endif

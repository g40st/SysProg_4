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

/*
 * Add a new socket into the login queue.
 */
void loginAddSocket(int sock);

/*
 * Actual login thread main entry point.
 */
void *loginThread(void *arg);

#endif

/*
 * Systemprogrammierung
 * Multiplayer-Quiz
 *
 * Server
 *
 * clientthread.h: Header für den Client-Thread
 */

#ifndef CLIENTTHREAD_H
#define CLIENTTHREAD_H

/*
 * Free any remaining dynamic memory in the client thread.
 */
void cleanCategories(void);

/*
 * Actual client thread main entry point.
 */
void *clientThread(void *arg);

#endif


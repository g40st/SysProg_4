/*
 * Systemprogrammierung
 * Multiplayer-Quiz
 *
 * Client
 *
 * listener.h: Header für den Listener-Thread
 */

#ifndef LISTENER_H
#define LISTENER_H

int getRunning(void);
void stopThreads(void);

void *listenerThread(void *arg);

#endif


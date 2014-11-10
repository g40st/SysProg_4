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

void setPlayerCount(int pc);
int getPlayerCount(void);

void *listenerThread(void *arg);

#endif


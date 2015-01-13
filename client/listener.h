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

/*
 * Get or Set the number of current players.
 */
void setPlayerCount(int pc);
int getPlayerCount(void);

/*
 * Listener Thread main entry point.
 */
void *listenerThread(void *arg);

#endif


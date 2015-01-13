/*
 * Systemprogrammierung
 * Multiplayer-Quiz
 *
 * Server
 *
 * score.h: Header für den Score Agent
 */

#ifndef SCORE_H
#define SCORE_H

/*
 * Instruct the score agent to send a new player and
 * score list to all connected clients.
 */
void scoreMarkForUpdate(void);

/*
 * Main entry point for the score agent thread.
 */
void *scoreThread(void *arg);

#endif


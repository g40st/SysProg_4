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

void scoreMarkForUpdate(void);

void *scoreThread(void *arg);

#endif


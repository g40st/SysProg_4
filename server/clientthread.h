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

typedef enum {
    PHASE_PREPARATION,
    PHASE_GAME,
    PHASE_END
} GamePhase_t;

GamePhase_t getGamePhase(void);

void addCategory(const char *c);
int countCategory(void);
const char *getCategory(int index);
void selectCategory(int index);
void cleanCategories(void);

void *clientThread(void *arg);

#endif


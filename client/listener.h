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

typedef enum {
    PHASE_PREPARATION,
    PHASE_GAME,
    PHASE_END
} GamePhase_t;

void setGamePhase(GamePhase_t p);
GamePhase_t getGamePhase(void);

void *listenerThread(void *arg);

#endif


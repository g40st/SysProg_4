/*
 * Systemprogrammierung
 * Multiplayer-Quiz
 *
 * Server
 *
 * user.h: Header für die User-Verwaltung
 */

#ifndef USER_H
#define USER_H

void userCountSet(int c);

int userCount(void);

// Returns NULL on error
const char *userGet(int index);

void userSet(const char *name, int index);

void scoreSet(int s, int i);

int scoreGet(int i);

void socketSet(int s, int i);

int socketGet(int i);

#endif


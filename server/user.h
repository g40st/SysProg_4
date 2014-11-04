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

#endif


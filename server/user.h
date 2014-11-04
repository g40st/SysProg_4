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

int userCount(void);

// Returns NULL on error
const char *userGet(int index);

// Returns -1 on error, else new index
int userAdd(const char *name);

// Returns 1 on success, 0 on error
int userRemove(int index);

#endif


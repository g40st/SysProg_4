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

void userInit(void);

int userCount(void);
int userFirstFreeSlot(void);

void userSetMainSocket(int s);
int userGetMainSocket(void);

void userSetPresent(int index, int present);
int userGetPresent(int index);

void userSetSocket(int index, int socket);
int userGetSocket(int index);

void userSetName(int index, const char *name);
const char *userGetName(int index);

int userGetScore(int index);
void userAddScore(int index, unsigned long timeLeft, unsigned long timeout);

void userSetLastCCH(int index, int cch);
int userGetLastCCH(int index);

#define MAX_QUESTIONS 128
void userSetQuestion(int index, int question, int q);
int userGetQuestion(int index, int question);
int userCountQuestionsAnswered(int index);

void userSetLastTimeout(int index, int timeout);
int userGetLastTimeout(int index);

// Returns -1 on timeout, -2 on error, else ID
int waitForSockets(int timeout);

#endif


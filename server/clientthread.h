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

void addCategory(const char *c);
int countCategory(void);
const char *getCategory(int index);
void selectCategory(int index);
void cleanCategories(void);

void *clientThread(void *arg);

#endif


/*
 * Systemprogrammierung
 * Multiplayer-Quiz
 *
 * Client
 *
 * fragewechsel.h: Header für den Fragewechsel-Thread
 */

#ifndef FRAGEWECHSEL_H
#define FRAGEWECHSEL_H

void requestNewQuestion(int seconds);

void *questionThread(void *arg);

#endif


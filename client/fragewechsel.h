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

/*
 * Instruct the thread to request a new question after
 * some time passed.
 */
void requestNewQuestion(int seconds);

/*
 * Main entry point for the question change thread.
 */
void *questionThread(void *arg);

#endif


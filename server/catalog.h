/*
 * Systemprogrammierung
 * Multiplayer-Quiz
 *
 * Server
 *
 * catalog.h: Header für die Katalogbehandlung und Loader-Steuerung
 */

#ifndef CATALOG_H
#define CATALOG_H

#include "common/question.h"

/*
 * Creates the pipes used to talk to the loader.
 */
int createPipes(void);

/*
 * Closes any pipes left from the loader.
 */
void closePipes(void);

/*
 * Fork and execute the loader in the child process.
 */
int forkLoader(void);

/*
 * Send a command to the (running!) loader.
 */
void sendLoaderCommand(const char *cmd);

/*
 * Read a single line from the response from
 * the (running!) loader.
 */
void readLineLoader(char *buff, int size);

/*
 * Open the shared memory used with the loader.
 */
int loaderOpenSharedMemory(int size);

/*
 * Properly close the shared memory that was
 * used with the loader.
 */
void loaderCloseSharedMemory(void);

/*
 * Count the questions in the currently loaded questionnaire.
 */
int getQuestionCount(void);

/*
 * Get a question from the currently loaded questionnaire.
 */
Question *getQuestion(int index);

#endif


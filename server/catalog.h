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

int createPipes(void);
int forkLoader(void);

void sendLoaderCommand(const char *cmd);
void readLineLoader(char *buff, int size);

int loaderOpenSharedMemory(int size);
void loaderCloseSharedMemory(void);

int getQuestionCount(void);
Question *getQuestion(int index);

#endif


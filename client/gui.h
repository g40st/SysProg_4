/*
 * Systemprogrammierung
 * Multiplayer-Quiz
 *
 * Client
 *
 * gui.h: Header für den GUI-Thread
 */

#ifndef GUI_H
#define GUI_H

#include "common/rfc.h"

// Returns 1 on success, 0 on error
int handleErrorWarningMessage(rfc response);

void *guiThread(void *arg);

#endif


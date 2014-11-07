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

int createPipes(void);
int forkLoader(void);

int getWritePipe(void);
int getReadPipe(void);

#endif


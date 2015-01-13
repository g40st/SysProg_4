/**
 * \file	common/util.h
 * \author	Stefan Gast
 *
 * \brief	Deklarationen diverser Hilfsfunktionen für Ein-/Ausgabe, Fehlersuche usw.
 */

#ifndef QUIZ_UTIL_H
#define QUIZ_UTIL_H

#include <stdarg.h>
#include <stddef.h>

#define PORT 8111

#define MAX_PLAYERS 4
#define MAX_QUERYS MAX_PLAYERS

#define MAX_CHAR 1024
#define BUFFER_SIZE MAX_CHAR

#define THREAD_TIMEOUT 25
#define SOCKET_TIMEOUT THREAD_TIMEOUT

typedef enum {
    PHASE_PREPARATION,
    PHASE_GAME,
    PHASE_END
} GamePhase_t;

/*
 * Get or Set the current game phase.
 */
void setGamePhase(GamePhase_t p);
GamePhase_t getGamePhase(void);

/*
 * Get or Set the flag used as condition in
 * all Thread main loops. Gives ability to
 * properly kill all running threads.
 */
int getRunning(void);
void stopThreads(void);

/*
 * Helper method to sleep a short amount of time.
 * Used at the end of each Threads main loop to
 * keep CPU utilization to a minimum.
 */
void loopsleep(void);

/* Wir benutzen die __attribute__ Erweiterung von GCC zur Überprüfung
 * der Argumente von debugPrint. Damit andere Compiler sich nicht beschweren,
 * definieren wir auf diesen __attribute__ als leeres Makro. */
#ifndef __GNUC__
#  define  __attribute__(x)  /* leer */
#endif

void setProgName(const char *argv0);
const char *getProgName(void);

void debugEnable(void);
int debugEnabled(void);
void debugDisable(void);

void debugPrint(const char *fmt, ...) __attribute__ ((format(printf, 1, 2)));
void infoPrint(const char *fmt, ...) __attribute__ ((format(printf, 1, 2)));
void errorPrint(const char *fmt, ...) __attribute__ ((format(printf, 1, 2)));
void errnoPrint(const char *prefix);

void debugHexdump(const void *ptr, size_t n, const char *fmt, ...) __attribute__ ((format(printf, 3, 4)));
void hexdump(const void *ptr, size_t n, const char *fmt, ...) __attribute__ ((format(printf, 3, 4)));
void vhexdump(const void *ptr, size_t n, const char *fmt, va_list args);

char *readLine(int fd);

#endif


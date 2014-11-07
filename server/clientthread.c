/*
 * Systemprogrammierung
 * Multiplayer-Quiz
 *
 * Server
 *
 * clientthread.c: Implementierung des Client-Threads
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>

#include "main.h"
#include "user.h"
#include "common/server_loader_protocol.h"
#include "common/util.h"
#include "clientthread.h"

void *clientThread(void *arg) {
    int present = 0;

    while (1) {
        if ((!present) && userGetPresent(0)) {
            present = 1;
            FILE * writePipe = fdopen(getWritePipe(), "w");
            if (writePipe == NULL) {
                errnoPrint("fdopen");
            }
            if (fprintf(writePipe, "%s\n", BROWSE_CMD) <= 0) {
                errnoPrint("fprintf");
            }
            fflush(writePipe);
        }

        loopsleep();
    }

    return NULL;
}


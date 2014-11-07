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

#include "catalog.h"
#include "user.h"
#include "common/server_loader_protocol.h"
#include "common/util.h"
#include "clientthread.h"

void *clientThread(void *arg) {
    int present = 0;

    while (1) {
        if ((!present) && userGetPresent(0)) {
            sendLoaderCommand(BROWSE_CMD);
            present = 1;
        }

        if (present) {
            char buff[1024];
            readLineLoader(buff, 1024);
            debugPrint("Loader: %s", buff);
        }

        loopsleep();
    }

    return NULL;
}


/*
 * Systemprogrammierung
 * Multiplayer-Quiz
 *
 * Server
 *
 * catalog.c: Implementierung der Fragekatalog-Behandlung und Loader-Steuerung
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "common/server_loader_protocol.h"
#include "common/util.h"
#include "catalog.h"

static int stdinPipe[2] = { -1, -1 };
static int stdoutPipe[2] = { -1, -1 };
static FILE *readPipe = NULL;
static FILE *writePipe = NULL;

int createPipes(void) {
    if (pipe(stdinPipe) == -1) {
        errnoPrint("pipe in");
        return 0;
    }

    if (pipe(stdoutPipe) == -1) {
        errnoPrint("pipe out");
        return 0;
    }

    readPipe = fdopen(stdoutPipe[0], "r");
    if (readPipe == NULL) {
        errnoPrint("fdopen");
        return 0;
    }

    writePipe = fdopen(stdinPipe[1], "w");
    if (writePipe == NULL) {
        errnoPrint("fdopen");
        return 0;
    }

    return 1;
}

int forkLoader(void) {
    pid_t forkResult = fork();
    if (forkResult < 0) {
        errnoPrint("fork");
        return 0;
    } else if (forkResult == 0) {
        if (dup2(stdinPipe[0], STDIN_FILENO) == -1) {
            errnoPrint("dup2(stdinPipe[0], STDIN_FILENO)");
            return 0;
        }
        if (dup2(stdoutPipe[1], STDOUT_FILENO) == -1) {
            errnoPrint("dup2(stdoutPipe[1], STDOUT_FILENO)");
            return 0;
        }

        if (debugEnabled())
            execl("bin/loader", "loader", "catalog", "-d", NULL);
        else
            execl("bin/loader", "loader", "catalog", NULL);

        errnoPrint("execl");
        return 0;
    }
    return 1;
}

void sendLoaderCommand(const char *cmd) {
    if (fprintf(writePipe, "%s\n", cmd) <= 0) {
        errnoPrint("fprintf");
    }
    fflush(writePipe);
}

void readLineLoader(char *buff, int size) {
    if (readPipe == NULL) {
        debugPrint("Invalid readLineLoader!");
        return;
    }

    if (fgets(buff, size - 1, readPipe) == NULL)
        errnoPrint("fgets");

    if (buff[strlen(buff) - 1] == '\n')
        buff[strlen(buff) - 1] = '\0';
}


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
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "common/rfc.h"
#include "common/server_loader_protocol.h"
#include "common/util.h"
#include "catalog.h"

/*
 * Pipes used to communicate with the loader.
 */
static int stdinPipe[2] = { -1, -1 };
static int stdoutPipe[2] = { -1, -1 };
static FILE *readPipe = NULL;
static FILE *writePipe = NULL;

int createPipes(void) {
    // Create the first pipe set
    if (pipe(stdinPipe) == -1) {
        errnoPrint("pipe in");
        return 0;
    }

    // Create the second pipe set
    if (pipe(stdoutPipe) == -1) {
        errnoPrint("pipe out");
        return 0;
    }

    // Get File Descriptors for both pipes (on our end)
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
        // Child process!
        // Assign the pipes to stdin/stdout
        if (dup2(stdinPipe[0], STDIN_FILENO) == -1) {
            errnoPrint("dup2(stdinPipe[0], STDIN_FILENO)");
            return 0;
        }
        if (dup2(stdoutPipe[1], STDOUT_FILENO) == -1) {
            errnoPrint("dup2(stdoutPipe[1], STDOUT_FILENO)");
            return 0;
        }

        if (debugEnabled())
            // Run the loader with debug output enabled
            execl("bin/loader", "loader", "catalog", "-d", NULL);
        else
            // Run the loader without debug output
            execl("bin/loader", "loader", "catalog", NULL);

        // This point should never be reached!
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

    // Read a single line from the loaders stdout
    if (fgets(buff, size - 1, readPipe) == NULL)
        errnoPrint("fgets");

    buff[size - 1] = '\0';
    if (buff[strlen(buff) - 1] == '\n')
        buff[strlen(buff) - 1] = '\0';
}

static int shm_size = 0;
static int shm_fd = -1;
static Question *shm_data = NULL;

int loaderOpenSharedMemory(int size) {
    debugPrint("Opening Loader: %d * %d", size, RFC_QUESTION_SHMEM_SIZE);

    // Open the shared memory
    shm_size = size * RFC_QUESTION_SHMEM_SIZE;
    shm_fd = shm_open(SHMEM_NAME, O_RDONLY, 0400);
    if (shm_fd == -1) {
        errnoPrint("shm_open");
        return 0;
    }

    // Map it into our virtual memory
    shm_data = mmap(NULL, shm_size, PROT_READ, MAP_SHARED, shm_fd, 0);
    if (shm_data == MAP_FAILED) {
        errnoPrint("mmap");
        return 0;
    }

    return 1;
}

void loaderCloseSharedMemory(void) {
    if (shm_data != NULL) {
        // Unmap the data
        if (munmap(shm_data, shm_size) == -1) {
            errnoPrint("munmap");
        }
        shm_data = NULL;
        shm_size = 0;

        // And unlink the shared memory
        if (shm_unlink(SHMEM_NAME) == -1) {
            errnoPrint("shm_unlink");
        }
    }

    // Close the file descriptor
    if (shm_fd != -1) {
        close(shm_fd);
        shm_fd = -1;
    }
}

int getQuestionCount(void) {
    // Byte count divided by question size
    return shm_size / RFC_QUESTION_SHMEM_SIZE;
}

Question *getQuestion(int index) {
    // Check if the question actually exists
    if ((index >= 0) && (index < getQuestionCount())) {
        // Return a pointer into the mapped shared memory region
        return shm_data + index;
    } else {
        debugPrint("Invalid getQuestion: %d", index);
        return NULL;
    }
}


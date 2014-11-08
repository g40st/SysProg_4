/*
 * Systemprogrammierung
 * Multiplayer-Quiz
 *
 * Server
 *
 * user.c: Implementierung der User-Verwaltung
 */

#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/select.h>
#include <errno.h>
#include <signal.h>

#include "common/util.h"
#include "clientthread.h"
#include "user.h"

typedef struct {
    int present;
    char name[33];
    int score;
    int socket;
} user_t;

static user_t users[MAX_PLAYERS];
static int mainSocket = -1;
static pthread_mutex_t mutexUsers = PTHREAD_MUTEX_INITIALIZER;

void userInit(void) {
    pthread_mutex_lock(&mutexUsers);
    for (int i = 0; i < MAX_PLAYERS; i++) {
        users[i].present = 0;
        for (int j = 0; j < 33; j++)
            users[i].name[j] = '\0';
        users[i].score = 0;
        users[i].socket = -1;
    }
    mainSocket = -1;
    pthread_mutex_unlock(&mutexUsers);
}

int userCount(void) {
    int c = 0;
    pthread_mutex_lock(&mutexUsers);
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (users[i].present)
            c++;
    }
    pthread_mutex_unlock(&mutexUsers);
    return c;
}

int userFirstFreeSlot(void) {
    int s = -1;
    pthread_mutex_lock(&mutexUsers);
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (!users[i].present) {
            s = i;
            break;
        }
    }
    pthread_mutex_unlock(&mutexUsers);
    return s;
}

void userSetMainSocket(int s) {
    pthread_mutex_lock(&mutexUsers);
    mainSocket = s;
    pthread_mutex_unlock(&mutexUsers);
}

int userGetMainSocket(void) {
    pthread_mutex_lock(&mutexUsers);
    int s = mainSocket;
    pthread_mutex_unlock(&mutexUsers);
    return s;
}

void userSetPresent(int index, int present) {
    pthread_mutex_lock(&mutexUsers);
    if ((index >= 0) && (index < MAX_PLAYERS)) {
        users[index].present = present;
    } else {
        debugPrint("Invalid userSetPresent: %d %d", index, present);
    }
    pthread_mutex_unlock(&mutexUsers);
}

int userGetPresent(int index) {
    int present = 0;
    pthread_mutex_lock(&mutexUsers);
    if ((index >= 0) && (index < MAX_PLAYERS)) {
        present = users[index].present;
    } else {
        debugPrint("Invalid userGetPresent: %d", index);
    }
    pthread_mutex_unlock(&mutexUsers);
    return present;
}

void userSetSocket(int index, int socket) {
    pthread_mutex_lock(&mutexUsers);
    if ((index >= 0) && (index < MAX_PLAYERS)) {
        users[index].socket = socket;
    } else {
        debugPrint("Invalid userSetSocket: %d %d", index, socket);
    }
    pthread_mutex_unlock(&mutexUsers);
}

int userGetSocket(int index) {
    int socket = -1;
    pthread_mutex_lock(&mutexUsers);
    if ((index >= 0) && (index < MAX_PLAYERS)) {
        socket = users[index].socket;
    } else {
        debugPrint("Invalid userGetSocket: %d", index);
    }
    pthread_mutex_unlock(&mutexUsers);
    return socket;
}

void userSetName(int index, const char *name) {
    pthread_mutex_lock(&mutexUsers);
    if ((index >= 0) && (index < MAX_PLAYERS)) {
        strncpy(users[index].name, name, 32);
    } else {
        debugPrint("Invalid userSetName: %d %s", index, name);
    }
    pthread_mutex_unlock(&mutexUsers);
}

const char *userGetName(int index) {
    const char *name = NULL;
    pthread_mutex_lock(&mutexUsers);
    if ((index >= 0) && (index < MAX_PLAYERS)) {
        name = users[index].name;
    } else {
        debugPrint("Invalid userGetName: %d", index);
    }
    pthread_mutex_unlock(&mutexUsers);
    return name;
}

int userGetScore(int index) {
    int score = 0;
    pthread_mutex_lock(&mutexUsers);
    if ((index >= 0) && (index < MAX_PLAYERS)) {
        score = users[index].score;
    } else {
        debugPrint("Invalid userGetScore: %d", index);
    }
    pthread_mutex_unlock(&mutexUsers);
    return score;
}

int waitForSockets(int timeout) {
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = timeout * 1000000;

    sigset_t blockset;
    sigfillset(&blockset);
    sigdelset(&blockset, SIGINT);

    fd_set fds;
    FD_ZERO(&fds);
    int max = -1;
    pthread_mutex_lock(&mutexUsers);
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (users[i].present) {
            if (users[i].socket > max)
                max = users[i].socket;

            FD_SET(users[i].socket, &fds);
        }
    }
    pthread_mutex_unlock(&mutexUsers);

    if (max == -1) {
        return -1;
    }

    int retval = pselect(max + 1, &fds, NULL, NULL, &ts, &blockset);
    if (retval == -1) {
        if (errno == EINTR) {
            // SIGINT was caught
            close(userGetMainSocket());
            cleanCategories();
            return 0;
        } else {
            errnoPrint("select");
            return -2;
        }
    } else if (retval == 0) {
        return -1;
    } else {
        pthread_mutex_lock(&mutexUsers);
        int ret = -2;
        for (int i = 0; i < MAX_PLAYERS; i++) {
            if (users[i].present) {
                if (FD_ISSET(users[i].socket, &fds)) {
                    ret = i;
                }
            }
        }
        pthread_mutex_unlock(&mutexUsers);
        if (ret < 0) {
            errorPrint("Could not find active socket?!");
        }
        return ret;
    }
}


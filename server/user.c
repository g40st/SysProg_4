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
    int cch;
    int question[MAX_QUESTIONS];
    int lastTimeout;
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
        users[i].cch = 0;
        for (int j = 0; j < MAX_QUESTIONS; j++)
            users[i].question[j] = 0;
        users[i].lastTimeout = 0;
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

static unsigned long scoreForTimeLeft(unsigned long timeLeft, unsigned long timeout) {
    unsigned long score = (timeLeft * 1000UL) / timeout;
    score = ((score + 5UL) / 10UL) * 10UL;
    debugPrint("Added Score: %lu & %lu --> %lu", timeLeft, timeout, score);
    return score;
}

void userAddScore(int index, unsigned long timeLeft, unsigned long timeout) {
    pthread_mutex_lock(&mutexUsers);
    if ((index >= 0) && (index < MAX_PLAYERS)) {
        users[index].score += scoreForTimeLeft(timeLeft, timeout);
    } else {
        debugPrint("Invalid userAddScore: %d %lu %lu", index, timeLeft, timeout);
    }
    pthread_mutex_unlock(&mutexUsers);
}

void userSetLastCCH(int index, int cch) {
    pthread_mutex_lock(&mutexUsers);
    if ((index >= 0) && (index < MAX_PLAYERS)) {
        users[index].cch = cch;
    } else {
        debugPrint("Invalid userSetLastCCH: %d", index);
    }
    pthread_mutex_unlock(&mutexUsers);
}

int userGetLastCCH(int index) {
    int cch = 0;
    pthread_mutex_lock(&mutexUsers);
    if ((index >= 0) && (index < MAX_PLAYERS)) {
        cch = users[index].cch;
    } else {
        debugPrint("Invalid userGetLastCCH: %d", index);
    }
    pthread_mutex_unlock(&mutexUsers);
    return cch;
}

int userGetQuestion(int index, int question) {
    int q = 0;
    pthread_mutex_lock(&mutexUsers);
    if ((index >= 0) && (index < MAX_PLAYERS) && (question >= 0) && (question < MAX_QUESTIONS)) {
        q = users[index].question[question];
    } else {
        debugPrint("Invalid userGetQuestion: %d %d", index, question);
    }
    pthread_mutex_unlock(&mutexUsers);
    return q;
}

void userSetQuestion(int index, int question, int q) {
    pthread_mutex_lock(&mutexUsers);
    if ((index >= 0) && (index < MAX_PLAYERS)) {
        users[index].question[question] = q;
    } else {
        debugPrint("Invalid userSetQuestion: %d %d", index, question);
    }
    pthread_mutex_unlock(&mutexUsers);
}

int userCountQuestionsAnswered(int index) {
    int c = 0;
    pthread_mutex_lock(&mutexUsers);
    if ((index >= 0) && (index < MAX_PLAYERS)) {
        for (int i = 0; i < MAX_QUESTIONS; i++) {
            if (users[index].question[i] > 0)
                c++;
        }
    } else {
        debugPrint("Invalid userCountQuestionsAnswered: %d", index);
    }
    pthread_mutex_unlock(&mutexUsers);
    return c;
}

void userSetLastTimeout(int index, int timeout) {
    pthread_mutex_lock(&mutexUsers);
    if ((index >= 0) && (index < MAX_PLAYERS)) {
        users[index].lastTimeout = timeout;
    } else {
        debugPrint("Invalid userSetLastTimeout: %d %d", index, timeout);
    }
    pthread_mutex_unlock(&mutexUsers);
}

int userGetLastTimeout(int index) {
    int t = 0;
    pthread_mutex_lock(&mutexUsers);
    if ((index >= 0) && (index < MAX_PLAYERS)) {
        t = users[index].lastTimeout;
    } else {
        debugPrint("Invalid userGetLastTimeout: %d", index);
    }
    pthread_mutex_unlock(&mutexUsers);
    return t;
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
            return -2;
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


/*
 * Systemprogrammierung
 * Multiplayer-Quiz
 *
 * Server
 *
 * score.h: Implementierung des Score-Agents
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>

#include "common/rfc.h"
#include "common/util.h"
#include "user.h"
#include "score.h"

static int scoreFlag = 0;
static pthread_mutex_t scoreMutex = PTHREAD_MUTEX_INITIALIZER;

static int indexCompare(const void *a1, const void *b1) {
    int a = *((int *)a1);
    int b = *((int *)b1);
    if (userGetScore(a) > userGetScore(b))
        return -1;
    if (userGetScore(a) < userGetScore(b))
        return 1;
    return 0;
}

static void sendPlayerListToAll() {
    int c = userCount();
    if (c <= 0)
        return;

    // Sort list by score
    int indices[MAX_PLAYERS];
    for (int i = 0; i < MAX_PLAYERS; i++) indices[i] = i;
    qsort(indices, MAX_PLAYERS, sizeof(int), indexCompare);

    struct rfcPlayerList list;
    list.main.type[0] = 'L';
    list.main.type[1] = 'S';
    list.main.type[2] = 'T';
    list.main.length = htons(37 * c);
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (userGetPresent(indices[i])) {
            const char *name = userGetName(indices[i]);
            size_t len = strlen(name);
            memcpy(list.players[indices[i]].name, name, len);
            memset(list.players[indices[i]].name + len, 0, 32 - len);
            list.players[indices[i]].points = htonl(userGetScore(indices[i]));
            list.players[indices[i]].id = indices[i];
            debugPrint("LST %d: %d %s %d", i, indices[i], name, userGetScore(indices[i]));
        }
    }

    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (userGetPresent(i)) {
            if (send(userGetSocket(i), &list, RFC_LST_SIZE(c), 0) == -1) {
                errnoPrint("ScoreThread send");
            }
        }
    }
}

void scoreMarkForUpdate(void) {
    pthread_mutex_lock(&scoreMutex);
    scoreFlag++;
    pthread_mutex_unlock(&scoreMutex);
}

void *scoreThread(void *arg) {
    while (getRunning()) {
        pthread_mutex_lock(&scoreMutex);
        int sf = scoreFlag;
        if (sf > 0)
            scoreFlag = 0;
        pthread_mutex_unlock(&scoreMutex);

        if (sf > 0)
            sendPlayerListToAll();

        loopsleep();
    }

    return NULL;
}


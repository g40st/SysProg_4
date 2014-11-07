/*
 * Systemprogrammierung
 * Multiplayer-Quiz
 *
 * Server
 *
 * score.h: Implementierung des Score-Agents
 */

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

#define PLAYER_LIST(x) if (userGetPresent(x)) { \
    memcpy(list.player##x.name, userGetName(x), strlen(userGetName(x))); \
    memset(list.player##x.name + strlen(userGetName(x)), 0, 32 - strlen(userGetName(x))); \
    list.player##x.points = htonl(userGetScore(x)); \
    list.player##x.id = x; \
}

static void sendPlayerListToAll() {
    int c = userCount();
    if (c <= 0)
        return;

    struct rfcPlayerList list;
    list.main.type[0] = 'L';
    list.main.type[1] = 'S';
    list.main.type[2] = 'T';
    list.main.length = htons(37 * c);
    PLAYER_LIST(0)
    PLAYER_LIST(1)
    PLAYER_LIST(2)
    PLAYER_LIST(3)

    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (userGetPresent(i)) {
            if (send(userGetSocket(i), &list, RFC_LST_SIZE(c), 0) == -1) {
                errorPrint("send: %s", strerror(errno));
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
    while (1) {
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


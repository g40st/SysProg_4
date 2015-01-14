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

static int indexCompare(const void *a1, const void *b1) {
    /*
     * Indirect comparison! The two arguments, a and b,
     * are not actually the elements that need to be sorted.
     * Instead, they are indices into the list of elements
     * we want to sort (our user list, in this case).
     */

    // Extract the two (integer) arguments
    int a = *((int *)a1);
    int b = *((int *)b1);

    /*
     * Use them as indices into the user list
     * and compare the users scores
     */
    if (userGetScore(a) > userGetScore(b))
        return -1;
    if (userGetScore(a) < userGetScore(b))
        return 1;
    return 0;
}

static void sendPlayerListToAll() {
    // We only send a list if there really are players
    int c = userCount();
    if (c <= 0)
        return;

    // Sort a list of indices into the user list, by score
    int indices[MAX_PLAYERS];
    for (int i = 0; i < MAX_PLAYERS; i++) indices[i] = i;
    qsort(indices, MAX_PLAYERS, sizeof(int), indexCompare);

    // Prepare the packet we need to send to all users
    struct rfcPlayerList list;
    list.main.type[0] = 'L';
    list.main.type[1] = 'S';
    list.main.type[2] = 'T';
    list.main.length = htons(37 * c);
    int n = 0;
    for (int i = 0; i < MAX_PLAYERS; i++) {
        // Only put actually existing users into the list
        if (userGetPresent(indices[i])) {
            // And access the user list only using our indices array
            const char *name = userGetName(indices[i]);
            size_t len = strlen(name);

            // Copy the username into the packet
            memcpy(list.players[n].name, name, len);
            memset(list.players[n].name + len, 0, 32 - len);

            // Copy score & ID of the user
            list.players[n].points = htonl(userGetScore(indices[i]));
            list.players[n].id = indices[i];

            debugPrint("LST %d: %d %s %d", n, indices[i], name, userGetScore(indices[i]));
            n++;
        }
    }

    // Send the new list to all connected users
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (userGetPresent(i)) {
            if (send(userGetSocket(i), &list, RFC_LST_SIZE(c), 0) == -1) {
                errnoPrint("ScoreThread send");
            }
        }
    }
}

static semaphore_t scoreMutex;

void scoreMarkForUpdate(void) {
    semaphorePost(scoreMutex);
}

void *scoreThread(void *arg) {
    // Prepare semaphore
    scoreMutex = semaphoreNew(0);

    // Score agent main loop
    while (getRunning()) {
        if (!semaphoreTimeout(scoreMutex, SOCKET_TIMEOUT))
            continue;

        sendPlayerListToAll();
    }

    semaphoreRelease(scoreMutex);
    return NULL;
}


/*
 * Systemprogrammierung
 * Multiplayer-Quiz
 *
 * Client
 *
 * listener.c: Implementierung des Listener-Threads
 */

#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>

#include "common/rfc.h"
#include "common/util.h"
#include "gui/gui_interface.h"
#include "listener.h"

#define PLAYER_LIST(x) if (x < count) { \
    preparation_addPlayer(response.playerList.player##x.name); \
}

void *listenerThread(void *arg) {
    int socket = *((int *)arg);
    rfc response;
    debugPrint("ListenerThread is starting its loop...");
    while (1) {
        int receive = recv(socket, &response, RFC_MAX_SIZE, 0);
        if (receive == -1) {
            errorPrint("receive: %s", strerror(errno));
            return NULL;
        } else if (receive == 0) {
            errorPrint("Remote host closed connection");
            return NULL;
        }

        // Check response and react accordingly
        if (equalLiteral(response.main, "LST")) {
            debugPrint("ListenerThread got LST message (%d)", ntohs(response.main.length));
            int count = ntohs(response.main.length) / 37;
            preparation_clearPlayers();
            PLAYER_LIST(0)
            PLAYER_LIST(1)
            PLAYER_LIST(2)
            PLAYER_LIST(3)
        } else if (equalLiteral(response.main, "CRE")) {
            // TODO handle CatalogResponse
        } else {
            errorPrint("Unexpected response: %c%c%c", response.main.type[0],
                    response.main.type[1], response.main.type[2]);
        }
    }

    return NULL;
}


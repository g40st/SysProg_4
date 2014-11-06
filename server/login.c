/*
 * Systemprogrammierung
 * Multiplayer-Quiz
 *
 * Server
 *
 * login.c: Implementierung des Logins
 */

#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>

#include "common/rfc.h"
#include "common/util.h"
#include "score.h"
#include "user.h"
#include "login.h"

static int sockets[MAX_PLAYERS] = { -1, -1, -1, -1 };
static int socketCount = 0;
static pthread_mutex_t socketMutex = PTHREAD_MUTEX_INITIALIZER;

void loginAddSocket(int sock) {
    pthread_mutex_lock(&socketMutex);
    if (socketCount < MAX_PLAYERS) {
        sockets[socketCount] = sock;
        socketCount++;
    }
    pthread_mutex_unlock(&socketMutex);
}

static void loginHandleSocket(int socket) {
    rfc response;
    int receive = recv(socket, &response, RFC_MAX_SIZE, 0);
    if (receive == -1) {
        errnoPrint("receive");
        return;
    } else if (receive == 0) {
        errorPrint("Remote host closed connection");
        return;
    }

    if (equalLiteral(response.main, "LRQ")) {
        // Check RFC version
        if (response.loginRequest.version != RFC_VERSION_NUMBER) {
            sendErrorMessage(socket, "Login Error: Wrong RFC version used");
            infoPrint("Login attempt with wrong RFC version: %d", response.loginRequest.version);
            return;
        }

        // Store username string
        int length = ntohs(response.main.length) - 1;
        char s[length + 1];
        s[length] = '\0';
        memcpy(s, response.loginRequest.name, length);

        // Detect empty name string
        if (length == 0) {
            sendErrorMessage(socket, "Login Error: A username is required");
            infoPrint("Login attempt without a name");
            return;
        }

        // Detect duplicate names
        int id = userCount();
        for (int i = 0; i < id; i++) {
            if (strcmp(userGet(i), s) == 0) {
                sendErrorMessage(socket, "Login error: Name already in use");
                infoPrint("Login attempt with duplicate name: \"%s\"", s);
                return;
            }
        }

        // Detect too many players
        if (id >= MAX_PLAYERS) {
            sendErrorMessage(socket, "Login Error: Server is full");
            infoPrint("Login attempt while server is full: \"%s\"", s);
            return;
        }

        // Write new user data into "database"
        userCountSet(id + 1);
        userSet(s, id);
        socketSet(socket, id);
        scoreSet(0, id);
        scoreMarkForUpdate();

        // Send LOK message
        response.main.type[0] = 'L';
        response.main.type[1] = 'O';
        response.main.type[2] = 'K';
        response.main.length = htons(2);
        response.loginResponseOK.version = RFC_VERSION_NUMBER;
        response.loginResponseOK.clientID = (uint8_t)id;
        if (send(socket, &response, RFC_LOK_SIZE, 0) == -1) {
            errnoPrint("send");
            return;
        }
    } else {
        errorPrint("Unexpected response: %c%c%c", response.main.type[0],
                response.main.type[1], response.main.type[2]);
        return;
    }
}

void *loginThread(void *arg) {
    while (1) {
        pthread_mutex_lock(&socketMutex);
        while (socketCount > 0) {
            int sock = sockets[socketCount - 1];
            sockets[socketCount - 1] = -1;
            socketCount--;
            loginHandleSocket(sock);
        }
        pthread_mutex_unlock(&socketMutex);

        loopsleep();
    }

    return NULL;
}


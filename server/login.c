/*
 * Systemprogrammierung
 * Multiplayer-Quiz
 *
 * Server
 *
 * login.c: Implementierung des Logins
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
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
        printf("receive: %s\n", strerror(errno));
        return;
    } else if (receive == 0) {
        printf("Remote host closed connection\n");
        return;
    }

    if (equalLiteral(response.main, "LRQ")) {
        if (response.loginRequest.version != RFC_VERSION_NUMBER) {
            printf("Wrong RFC version: %d\n", response.loginRequest.version);
            return;
        }

        int length = ntohs(response.main.length) - 1;
        char s[length + 1];
        s[length] = '\0';
        memcpy(s, response.loginRequest.name, length);
        int id = userCount();
        for (int i = 0; i < id; i++) {
            if (strcmp(userGet(i), s) == 0) {
                id = -1;
                break;
            }
        }
        if (id >= 0) {
            // Success
            userCountSet(id + 1);
            userSet(s, id);
            socketSet(socket, id);
            scoreSet(0, id);

            // Send LOK
            response.main.type[0] = 'L';
            response.main.type[1] = 'O';
            response.main.type[2] = 'K';
            response.main.length = htons(2);
            response.loginResponseOK.version = RFC_VERSION_NUMBER;
            response.loginResponseOK.clientID = (uint8_t)id;
            if (send(socket, &response, RFC_LOK_SIZE, 0) == -1) {
                printf("send: %s\n", strerror(errno));
                return;
            }
        } else {
            // Error, send ERR
            response.main.type[0] = 'E';
            response.main.type[1] = 'R';
            response.main.type[2] = 'R';
            response.main.length = htons(2);
            response.errorWarning.subtype = 1; // Error
            memcpy(&response.errorWarning.message, "Login Error", 11);
            if (send(socket, &response, RFC_ERR_SIZE + 11, 0) == -1) {
                printf("send: %s\n", strerror(errno));
                return;
            }
        }
    } else {
        printf("Unexpected response: %c%c%c\n", response.main.type[0],
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
    }

    return NULL;
}


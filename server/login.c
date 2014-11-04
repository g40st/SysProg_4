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
#include "user.h"
#include "login.h"

void *loginThread(void *arg) {
    int socket = *((int *)arg);

    rfc response;
    int receive = recv(socket, &response, RFC_MAX_SIZE, 0);
    if (receive == -1) {
        printf("receive: %s\n", strerror(errno));
        return NULL;
    } else if (receive == 0) {
        printf("Remote host closed connection\n");
        return NULL;
    }

    if (equalLiteral(response.main, "LRQ")) {
        if (response.loginRequest.version != RFC_VERSION_NUMBER) {
            printf("Wrong RFC version: %d\n", response.loginRequest.version);
            return NULL;
        }

        int length = ntohs(response.main.length) - 1;
        char s[length + 1];
        s[length] = '\0';
        memcpy(s, response.loginRequest.name, length);
        int id = userCount();
        userCountSet(id + 1);
        for (int i = 0; i < id; i++) {
            if (strcmp(userGet(i), s) == 0) {
                id = -1;
                break;
            }
        }
        if (id >= 0) {
            // Success, send LOK
            userSet(s, id);
            response.main.type[0] = 'L';
            response.main.type[1] = 'O';
            response.main.type[2] = 'K';
            response.main.length = htons(2);
            response.loginResponseOK.version = RFC_VERSION_NUMBER;
            response.loginResponseOK.clientID = (uint8_t)id;
            if (send(socket, &response, RFC_LOK_SIZE, 0) == -1) {
                printf("send: %s\n", strerror(errno));
                return NULL;
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
                return NULL;
            }
        }
    } else {
        printf("Unexpected response: %c%c%c\n", response.main.type[0],
                response.main.type[1], response.main.type[2]);
        return NULL;
    }

    return NULL;
}


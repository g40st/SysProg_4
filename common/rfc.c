/*
 * Systemprogrammierung
 * Multiplayer-Quiz
 *
 * Gemeinsam verwendete Module
 *
 * rfc.c: Implementierung der Funktionen zum Senden und Empfangen von
 * Datenpaketen gemäß dem RFC
 */

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "util.h"
#include "rfc.h"

int equalLiteral(struct rfcMain m, const char *s) {
    if ((m.type[0] == s[0])
            && (m.type[1] == s[1])
            && (m.type[2] == s[2])) {
        return 1;
    }
    return 0;
}

static int sendErrorWarningMessage(int socket, const char *message, uint8_t subtype) {
    size_t length = strlen(message);
    if (length > MAX_STRING_LENGTH)
        length = MAX_STRING_LENGTH;
    rfc response;
    response.main.type[0] = 'E';
    response.main.type[1] = 'R';
    response.main.type[2] = 'R';
    response.main.length = htons(RFC_ERR_SIZE + length);
    response.errorWarning.subtype = subtype;
    strncpy(response.errorWarning.message, message, length);
    if (send(socket, &response, RFC_ERR_SIZE + length, 0) == -1) {
        errnoPrint("send");
        return 0;
    }
    return 1;
}

int sendErrorMessage(int socket, const char *message) {
    return sendErrorWarningMessage(socket, message, 1);
}

int sendWarningMessage(int socket, const char *message) {
    return sendErrorWarningMessage(socket, message, 0);
}

int receivePacket(int socket, rfc *r) {
    void *rec = r;
    int receive = recv(socket, rec, RFC_BASE_SIZE, 0);
    if (receive == -1) {
        errnoPrint("receive");
        return -1;
    } else if (receive == 0) {
        return 0;
    }

    int len = ntohs(r->main.length);
    if (len > 0) {
        receive = recv(socket, rec + RFC_BASE_SIZE, len, 0);
        if (receive == -1) {
            errnoPrint("receive");
            return -1;
        } else if (receive == 0) {
            return 0;
        }
    }

    return 1;
}


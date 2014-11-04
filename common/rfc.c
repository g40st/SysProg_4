/*
 * Systemprogrammierung
 * Multiplayer-Quiz
 *
 * Gemeinsam verwendete Module
 *
 * rfc.c: Implementierung der Funktionen zum Senden und Empfangen von
 * Datenpaketen gemäß dem RFC
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "rfc.h"

int equalLiteral(struct rfcMain m, const char *s) {
    if ((m.type[0] == s[0])
            && (m.type[1] == s[1])
            && (m.type[2] == s[2])) {
        return 1;
    }
    return 0;
}

int handleErrorWarningMessage(rfc response) {
    if (equalLiteral(response.main, "ERR")) {
        int length = response.main.length - 1;
        char s[length + 1];
        s[length] = '\0';
        memcpy(s, &response.errorWarning.message, length);
        if (response.errorWarning.subtype == 0) {
            printf("Warning: %s\n", s);
            return 1;
        } else if (response.errorWarning.subtype == 1) {
            printf("Error: %s\n", s);
            exit(1);
            return 1;
        } else {
            printf("Unknown message type %d for: %s\n", response.errorWarning.subtype, s);
        }
    }
    return 0;
}


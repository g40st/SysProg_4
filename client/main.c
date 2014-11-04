/*
 * Systemprogrammierung
 * Multiplayer-Quiz
 *
 * Client
 *
 * main.c: Hauptprogramm des Clients
 */

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <errno.h>

#include "common/rfc.h"
#include "common/util.h"
#include "gui/gui_interface.h"

void show_help() {
    printf("Available options:\n");
    printf("    -s --host    specify a host (argument)\n");
    printf("    -p --port    specify a port (argument)\n");
    printf("    -h --help    show this help message\n");
}

int main(int argc, char **argv) {
    setProgName(argv[0]);
    debugEnable();
    infoPrint("Client Gruppe 01");

    // Get name from user
    char username[33];
    printf("Please enter your name:\n");
    fgets(username, 32, stdin);
    for (int i = 0; i < 32; i++)
        if (username[i] == '\n')
            username[i] = '\0';

    // Initialisierung: Verbindungsaufbau, Threads starten usw...
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serv_addr.sin_port = htons(PORT);

    const char* short_options = "hp:s:";
    struct option long_options[] = {
        { "help", no_argument, 0, 'h' },
        { "port", required_argument, 0, 'p' },
        { "host", required_argument, 0, 's' },
        { NULL, 0, NULL, 0 }
    };

    int option_index = 0;
    while (1) {
        int c = getopt_long(argc, argv, short_options, long_options, &option_index);
        if (c == -1) {
            break;
        }

        switch (c) {
            case 'h':
                show_help();
                return 0;

            case 's':
                if (optarg)
                    serv_addr.sin_addr.s_addr = inet_addr(optarg);
                break;

            case 'p':
                if (optarg)
                    serv_addr.sin_port = htons(atoi(optarg));
                break;

            case '?':
            default:
                printf("Option not implemented yet -- %s (%c)\n", argv[optind-1], c);
                show_help();
                break;
        }
    }

    // Anlegen des Sockets
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        printf("socket: %s\n", strerror(errno));
        return 1;
    }

    if (connect(client_socket, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr)) == -1) {
        printf("connect: %s\n", strerror(errno));
        close(client_socket);
        return 1;
    }

    // Ausgabe/Eingabe
    printf("Serveradresse: %s\n", inet_ntoa(serv_addr.sin_addr));
    printf("Serverport: %i\n", ntohs(serv_addr.sin_port));

    // Send LoginRequest
    struct rfcLoginRequest lrq;
    lrq.main.type[0] = 'L';
    lrq.main.type[1] = 'R';
    lrq.main.type[2] = 'Q';
    lrq.main.length = htons(1 + strlen(username));
    lrq.version = RFC_VERSION_NUMBER;
    memcpy(lrq.name, username, strlen(username));
    if (send(client_socket, &lrq, RFC_LRQ_SIZE + strlen(username), 0) == -1) {
        printf("send: %s\n", strerror(errno));
        return 1;
    }

    // Receive response
    rfc response;
    int receive = recv(client_socket, &response, RFC_MAX_SIZE, 0);
    if (receive == -1) {
        printf("receive: %s\n", strerror(errno));
        return 1;
    } else if (receive == 0) {
        printf("Remote host closed connection\n");
        return 1;
    }

    // Check response and react accordingly
    if (equalLiteral(response.main, "LOK")) {
        if (response.loginResponseOK.version != RFC_VERSION_NUMBER) {
            printf("Wrong RFC version: %d\n", response.loginResponseOK.version);
            return 1;
        }

        printf("We've been assigned ID no. %d\n", response.loginResponseOK.clientID);
    } else if (equalLiteral(response.main, "ERR")) {
        handleErrorWarningMessage(response);
        return 1;
    } else {
        printf("Unexpected response: %c%c%c\n", response.main.type[0],
                response.main.type[1], response.main.type[2]);
        return 1;
    }

    // Send CatalogRequest
    response.main.type[0] = 'C';
    response.main.type[1] = 'R';
    response.main.type[2] = 'Q';
    response.main.length = htons(0);
    if (send(client_socket, &response.main, RFC_BASE_SIZE, 0) == -1) {
        printf("send: %s\n", strerror(errno));
        return 1;
    }

    // TODO:
    // - Start GUI Thread
    // - Start Listener Thread
    // - Start Fragewechsel Thread

    guiInit(&argc, &argv);
    preparation_showWindow();
    guiMain();

    // Resourcen freigeben usw...
    guiDestroy();

    return 0;
}

void preparation_onCatalogChanged(const char *newSelection) {
    debugPrint("Katalogauswahl: %s", newSelection);
}

void preparation_onStartClicked(const char *currentSelection) {
    debugPrint("Starte Katalog %s", currentSelection);
}

void preparation_onWindowClosed(void) {
    debugPrint("Vorbereitungsfenster geschlossen");
    guiQuit();
}

void game_onSubmitClicked(unsigned char selectedAnswers) {
    debugPrint("Absende-Button angeklickt, Bitmaske der Antworten: %u", (unsigned)selectedAnswers);
}

void game_onWindowClosed(void) {
    debugPrint("Spielfenster geschlossen");
    guiQuit();
}

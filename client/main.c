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
#include <pthread.h>

#include "common/rfc.h"
#include "common/util.h"
#include "gui/gui_interface.h"
#include "fragewechsel.h"
#include "gui.h"
#include "listener.h"

// Storage for the own player ID
static int clientID = -1;

int getClientID(void) {
    return clientID;
}

static void show_help() {
    printf("Available options:\n");
    printf("    -n --name       speficy a username (argument)\n");
    printf("    -s --host       specify a host (argument)\n");
    printf("    -p --port       specify a port (argument)\n");
    printf("    -v --verbose    enable debug output\n");
    printf("    -h --help       show this help message\n");
}

int main(int argc, char **argv) {
    setProgName(argv[0]);
    debugDisable();
    infoPrint("Client Group 01");

    // Prepare socket structure
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serv_addr.sin_port = htons(PORT);

    // Read command line options
    debugPrint("Parsing command line options...");
    const char* short_options = "hvp:s:n:";
    struct option long_options[] = {
        { "help", no_argument, 0, 'h' },
        { "verbose", no_argument, 0, 'v' },
        { "port", required_argument, 0, 'p' },
        { "host", required_argument, 0, 's' },
        { "name", required_argument, 0, 'n' },
        { NULL, 0, NULL, 0 }
    };

    // Actual parsing of command line options using getopt()
    char username[33];
    username[0] = username[31] = username[32] = '\0';
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

            case 'v':
                debugEnable();
                break;

            case 's':
                if (optarg)
                    serv_addr.sin_addr.s_addr = inet_addr(optarg);
                break;

            case 'p':
                if (optarg)
                    serv_addr.sin_port = htons(atoi(optarg));
                break;

            case 'n':
                if (optarg)
                    strncpy(username, optarg, 31);
                break;

            case '?':
            default:
                errorPrint("Option not implemented yet -- %s (%c)", argv[optind-1], c);
                show_help();
                break;
        }
    }

    // Get name from user via stdin, if required
    if (strlen(username) == 0) {
        debugPrint("Username was not given as argument. Asking...");
        infoPrint("Please enter your name:");
        fgets(username, 32, stdin);
        for (int i = 0; i < 32; i++)
            if (username[i] == '\n')
                username[i] = '\0';
        if (strlen(username) == 0) {
            errorPrint("A username is required!");
            return 1;
        }
    } else {
        debugPrint("Username already given as argument");
    }

    debugPrint("Creating the socket...");
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        errorPrint("socket: %s", strerror(errno));
        return 1;
    }

    // Connect to the server.
    if (connect(client_socket, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr)) == -1) {
        errorPrint("connect: %s", strerror(errno));
        close(client_socket);
        return 1;
    }

    infoPrint("Serveraddress: %s", inet_ntoa(serv_addr.sin_addr));
    infoPrint("Serverport: %i", ntohs(serv_addr.sin_port));

    // Prepare the GUI
    debugPrint("Initializing UI...");
    guiInit(&argc, &argv);
    preparation_setMode(PREPARATION_MODE_BUSY);
    preparation_showWindow();

    // Send the first message to the server, a login request.
    debugPrint("Sending LoginRequest...");
    struct rfcLoginRequest lrq;
    lrq.main.type[0] = 'L';
    lrq.main.type[1] = 'R';
    lrq.main.type[2] = 'Q';
    lrq.main.length = htons(1 + strlen(username));
    lrq.version = RFC_VERSION_NUMBER;
    memcpy(lrq.name, username, strlen(username));
    if (send(client_socket, &lrq, RFC_LRQ_SIZE + strlen(username), 0) == -1) {
        errorPrint("send: %s", strerror(errno));
        return 1;
    }

    debugPrint("Waiting for response...");
    rfc response;
    int receive = receivePacket(client_socket, &response);
    if (receive == -1) {
        return 1;
    } else if (receive == 0) {
        errorPrint("Remote host closed connection");
        return 1;
    }

    // Check response and react accordingly
    if (equalLiteral(response.main, "LOK")) {
        debugPrint("Login successful!");

        if (response.loginResponseOK.version != RFC_VERSION_NUMBER) {
            errorPrint("Wrong RFC version: %d", response.loginResponseOK.version);
            return 1;
        }

        // Successful login, store our ID
        clientID = response.loginResponseOK.clientID;
        infoPrint("We've been assigned ID no. %d", clientID);

        // Give privileges only to the game master (ID == 0)
        preparation_addPlayer(username);
        if (response.loginResponseOK.clientID == 0) {
            preparation_setMode(PREPARATION_MODE_PRIVILEGED);
        } else {
            preparation_setMode(PREPARATION_MODE_NORMAL);
        }
    } else if (equalLiteral(response.main, "ERR")) {
        debugPrint("Error while logging in!");
        handleErrorWarningMessage(response);
        return 1;
    } else {
        errorPrint("Unexpected response: %c%c%c", response.main.type[0],
                response.main.type[1], response.main.type[2]);
        return 1;
    }

    // Request a catalog listing
    debugPrint("Sending CatalogRequest...");
    response.main.type[0] = 'C';
    response.main.type[1] = 'R';
    response.main.type[2] = 'Q';
    response.main.length = htons(0);
    if (send(client_socket, &response.main, RFC_BASE_SIZE, 0) == -1) {
        errnoPrint("send");
        return 1;
    }

    debugPrint("Starting Threads...");
    pthread_t threads[2];

    // Create the listener Thread
    if (pthread_create(&threads[0], NULL, listenerThread, &client_socket) != 0) {
        errnoPrint("pthread_create");
        return 1;
    }

    // Create the question change Thread
    if (pthread_create(&threads[1], NULL, questionThread, &client_socket) != 0) {
        errnoPrint("pthread_create");
        return 1;
    }

    guiSetSocket(client_socket);

    debugPrint("Entering UI main loop...");
    guiMain();

    debugPrint("Destroying UI...");
    guiDestroy();
    close(client_socket);

    debugPrint("Finished!");
    return 0;
}


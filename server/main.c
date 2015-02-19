/*
 * Systemprogrammierung
 * Multiplayer-Quiz
 *
 * Server
 *
 * main.c: Hauptprogramm des Servers
 */

#include "catalog.h"
#include "score.h"
#include "user.h"
#include "clientthread.h"
#include "common/rfc.h"
#include "common/util.h"

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/select.h>
#include <signal.h>

/*
 * Gracefully stop all threads when the user presses
 * Ctrl + C to kill the server.
 */
static void intHandler(int dummy) {
    stopThreads();
    debugPrint("Caught SIGINT, aborting...");
}

/*
 * Singleton Pattern. Taken from the lecture Wiki.
 * Ensures only one server is running at the same time.
 */
#define LOCKFILE "/tmp/serverGroup01"
static int singleton(const char *lockfile) {
    // Open the give file for writing (should not exist)
    int file = open(lockfile, O_WRONLY | O_CREAT, 0644);
    if (file < 0) {
        perror("Cannot create pid file");
        return 1;
    }

    // Prepare putting a write lock on the whole (empty) file
    struct flock lock;
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;

    // Actually set the prepared lock
    if (fcntl(file, F_SETLK, &lock) < 0) {
        perror("Cannot lock pid file");
        return 2;
    }

    // Ensure the file is really empty
    if (ftruncate(file, 0) < 0) {
        perror("ftruncate");
        return 3;
    }

    // Write the Servers Process ID (PID) into the file
    char s[32];
    snprintf(s, sizeof(s), "%d\n", (int)getpid());
    if (write(file, s, strlen(s)) < strlen(s))
        perror("write");

    // Ensure our modifications are actually written to disk
    if (fsync(file) < 0)
        perror("fsync");

    return 0;
}

static pthread_t threads[1 + MAX_PLAYERS];
static int runningThreads = 0;

/*
 * Helper function performing the login procedure.
 */
static void loginHandleSocket(int socket) {
    // Try to receive the first packet from the client
    rfc response;
    int receive = receivePacket(socket, &response);
    if (receive == -1) {
        errnoPrint("receive");
        return;
    } else if (receive == 0) {
        errorPrint("Remote host closed connection");
        return;
    }

    // Check if we received a login request
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

        // Check Game Phase
        if (getGamePhase() != PHASE_PREPARATION) {
            sendErrorMessage(socket, "Login Error: Game has already started");
            infoPrint("Login attempt while game has already started: \"%s\"", s);
            return;
        }

        // Detect empty name string
        if (length == 0) {
            sendErrorMessage(socket, "Login Error: A username is required");
            infoPrint("Login attempt without a name");
            return;
        }

        // Detect duplicate names
        for (int i = 0; i < MAX_PLAYERS; i++) {
            if (userGetPresent(i)) {
                if (strcmp(userGetName(i), s) == 0) {
                    sendErrorMessage(socket, "Login error: Name already in use");
                    infoPrint("Login attempt with duplicate name: \"%s\"", s);
                    return;
                }
            }
        }

        // Detect too many players
        int pos = userFirstFreeSlot();
        if (pos == -1) {
            sendErrorMessage(socket, "Login Error: Server is full");
            infoPrint("Login attempt while server is full: \"%s\"", s);
            return;
        }

        // Write new user data into "database"
        userSetPresent(pos, 1);
        userSetName(pos, s);
        userSetSocket(pos, socket);
        scoreMarkForUpdate();

        // Start client thread for new user
        if (pthread_create(&threads[runningThreads++], NULL, clientThread, (void*)pos) != 0) {
            errnoPrint("pthread_create");
            return;
        }

        // Send LOK message
        response.main.type[0] = 'L';
        response.main.type[1] = 'O';
        response.main.type[2] = 'K';
        response.main.length = htons(2);
        response.loginResponseOK.version = RFC_VERSION_NUMBER;
        response.loginResponseOK.clientID = (uint8_t)pos;
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

// Helper method to list the available command line arguments
static void show_help() {
    printf("Available options:\n");
    printf("    -p --port       specify a port (argument)\n");
    printf("    -v --verbose    enable debug output\n");
    printf("    -h --help       show this help message\n");
}

// Main program entry point
int main(int argc, char **argv) {
    setProgName(argv[0]);
    debugDisable();
    infoPrint("Server Gruppe 01");
    userInit();

    // Create PID lock file or exit if it already exists
    debugPrint("Making sure we're running only once...");
    if (singleton(LOCKFILE) != 0)
        return 1;

    // Socket structure that could be modified by the command line arguments
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(PORT); // Use the default port defined in "common"!

    // Prepare parsing the command line options using getopt()
    debugPrint("Parsing command line options...");
    const char* short_options = "hvp:";
    struct option long_options[] = {
        { "help", no_argument, 0, 'h' },
        { "verbose", no_argument, 0, 'v' },
        { "port", required_argument, 0, 'p' },
        { NULL, 0, NULL, 0 }
    };

    // Actual getopt() loop
    int option_index = 0;
    int loop = 1;
    while (loop != 0) {
        int c = getopt_long(argc, argv, short_options, long_options, &option_index);
        switch (c) {
            // Show help text when using -h
            case 'h':
                show_help();
                exit(1);
                break;

            // Enable verbose (debug) output
            case 'v':
                debugEnable();
                break;

            // Set port to listen on
            case 'p':
                if(optarg) {
                    if (isOnlyDigits(optarg)) {
                        server.sin_port = htons(atoi(optarg));
                    } else {
                        errorPrint("Port has to be a decimal number!");
                        exit(1);
                    }
                }
                break;

            // Unknown option, show error & help message
            default:
            case '?':
                errorPrint("Option not implemented yet -- %s (%c)", argv[optind-1], c);
                show_help();
                exit(1);
                break;

            // All options have been parsed
            case -1:
                loop = 0;
                break;
        }
    }

    infoPrint("Serverport: %i", ntohs(server.sin_port));

    // Create the pipes that will be used to communicate with the loader
    debugPrint("Creating Pipes...");
    if (!createPipes())
        return 1;

    // Actually fork and run the loader as child process
    debugPrint("Forking to run loader...");
    if (!forkLoader())
        return 1;

    debugPrint("Starting Threads...");

    // Start the score agent thread
    if (pthread_create(&threads[runningThreads++], NULL, scoreThread, NULL) != 0) {
        errnoPrint("pthread_create");
        return 1;
    }

    clientInit();

    // Create the listening socket
    debugPrint("Creating socket...");
    int listen_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_socket == -1) {
        errnoPrint("socket");
        return 1;
    }

    // Enable address reusing, so we don't get bind errors
    int on = 1;
    setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    // Register our Interrupt Signal handler
    signal(SIGINT, intHandler);

    // Store the listening socket in our user database
    userSetMainSocket(listen_socket);

    // Actually bind the listening socket
    if (bind(listen_socket, (struct sockaddr*) &server, sizeof(struct sockaddr_in)) == -1) {
        errnoPrint("bind");
        close(listen_socket);
        closePipes();
        loaderCloseSharedMemory();
        return 1;
    }

    // Prepare the pselect() timeout data structure
    fd_set fds;
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = SOCKET_TIMEOUT * 1000000;
    sigset_t blockset;
    sigfillset(&blockset);
    sigdelset(&blockset, SIGINT);

    // Main thread main loop, waiting for new connections
    debugPrint("Waiting for connections...");
    while (getRunning()) {
        // Listen to the listening socket
        if (listen(listen_socket, MAX_QUERYS) == -1) {
            errnoPrint("listen");
            close(listen_socket);
            closePipes();
            loaderCloseSharedMemory();
            cleanCategories();
            return 1;
        }

        // Wait for activity on the listening socket
        FD_ZERO(&fds);
        FD_SET(listen_socket, &fds);
        int retval = pselect(listen_socket + 1, &fds, NULL, NULL, &ts, &blockset);
        if (retval == -1) {
            if (errno == EINTR) {
                // We should exit because the user pressed Ctrl + C
                close(listen_socket);
                closePipes();
                loaderCloseSharedMemory();
                cleanCategories();
                return 0;
            } else {
                // pselect encountered an error!
                errnoPrint("select");
                close(listen_socket);
                closePipes();
                loaderCloseSharedMemory();
                cleanCategories();
                return 1;
            }
        } else if (retval == 0) {
            // Nothing happened and pselect timed out
            continue;
        } else {
            // We have a new connection, accept it
            struct sockaddr_in remote_host;
            socklen_t sin_size = sizeof(struct sockaddr_in);
            int client_socket = accept(listen_socket, (struct sockaddr *) &remote_host, &sin_size);
            if (client_socket == -1) {
                errnoPrint("accept");
                close(listen_socket);
                closePipes();
                loaderCloseSharedMemory();
                cleanCategories();
                return 1;
            }

            debugPrint("Got a new connection! Performing Login...");
            loginHandleSocket(client_socket);
        }
    }

    // Clean up behind ourselves
    close(listen_socket);
    closePipes();
    loaderCloseSharedMemory();
    cleanCategories();

    pthread_exit(NULL);
    return 0;
}


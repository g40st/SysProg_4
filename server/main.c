/*
 * Systemprogrammierung
 * Multiplayer-Quiz
 *
 * Server
 *
 * main.c: Hauptprogramm des Servers
 */

#include "login.h"
#include "score.h"
#include "user.h"
#include "clientthread.h"
#include "common/util.h"
#include "main.h"

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

static int running = 1;
static int stdinPipe[2];
static int stdoutPipe[2];

int getWritePipe() {
    return stdinPipe[1];
}

int getReadPipe() {
    return stdoutPipe[0];
}

static void intHandler(int dummy) {
    running = 0;
    debugPrint("Caught SIGINT, aborting...");
}

#define LOCKFILE "/tmp/serverGroup01"

static int singleton(const char *lockfile) {
    int file = open(lockfile, O_WRONLY | O_CREAT, 0644);
    if (file < 0) {
        perror("Cannot create pid file");
        return 1;
    }

    struct flock lock;
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;

    if (fcntl(file, F_SETLK, &lock) < 0) {
        perror("Cannot lock pid file");
        return 2;
    }

    if (ftruncate(file, 0) < 0) {
        perror("ftruncate");
        return 3;
    }

    char s[32];
    snprintf(s, sizeof(s), "%d\n", (int)getpid());
    if (write(file, s, strlen(s)) < strlen(s))
        perror("write");

    if (fsync(file) < 0)
        perror("fsync");

    return 0;
}

static void show_help() {
    printf("Available options:\n");
    printf("    -p --port       specify a port (argument)\n");
    printf("    -v --verbose    enable debug output\n");
    printf("    -h --help       show this help message\n");
}

int main(int argc, char **argv) {
    setProgName(argv[0]);
    debugDisable();
    infoPrint("Server Gruppe 01");
    userInit();

    debugPrint("Making sure we're running only once...");
    if (singleton(LOCKFILE) != 0)
        return 1;

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(PORT);

    debugPrint("Parsing command line options...");
    const char* short_options = "hvp:";
    struct option long_options[] = {
        { "help", no_argument, 0, 'h' },
        { "verbose", no_argument, 0, 'v' },
        { "port", required_argument, 0, 'p' },
        { NULL, 0, NULL, 0 }
    };

    int option_index = 0;
    int loop = 1;
    while (loop != 0) {
        int c = getopt_long(argc, argv, short_options, long_options, &option_index);
        switch (c) {
            case 'h':
                show_help();
                exit(1);
                break;

            case 'v':
                debugEnable();
                break;

            case 'p':
                if(optarg)
                    server.sin_port = htons(atoi(optarg));
                break;

            default:
            case '?':
                errorPrint("Option not implemented yet -- %s (%c)", argv[optind-1], c);
                show_help();
                break;

            case -1:
                loop = 0;
                break;
        }
    }

    infoPrint("Serverport: %i", ntohs(server.sin_port));

    // Pipes and Forking
    pid_t forkResult;

    debugPrint("Creating Pipes...");
    if (pipe(stdinPipe) == -1) {
        errnoPrint("pipe in");
        return 1;
    }

    if (pipe(stdoutPipe) == -1) {
        errnoPrint("pipe out");
        return 1;
    }

    debugPrint("Forking to run loader...");
    forkResult = fork();
    if (forkResult < 0) {
        errnoPrint("fork");
        return 1;
    } else if (forkResult == 0) {
        if (dup2(stdinPipe[0], STDIN_FILENO) == -1) {
            errnoPrint("dup2(stdinPipe[0], STDIN_FILENO)");
            return 1;
        }

        if (dup2(stdoutPipe[1], STDOUT_FILENO) == -1) {
            errnoPrint("dup2(stdoutPipe[1], STDOUT_FILENO)");
            return 1;
        }

        execl("bin/loader", "loader", "catalog", "-d", NULL);
        errnoPrint("execl");
        return 1;
    }

    debugPrint("Starting Threads...");
    pthread_t threads[3];
    if (pthread_create(&threads[0], NULL, loginThread, NULL) != 0) {
        errnoPrint("pthread_create");
        return 1;
    }
    if (pthread_create(&threads[1], NULL, scoreThread, NULL) != 0) {
        errnoPrint("pthread_create");
        return 1;
    }
    if (pthread_create(&threads[2], NULL, clientThread, NULL) != 0) {
        errnoPrint("pthread_create");
        return 1;
    }

    debugPrint("Creating socket...");
    int listen_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_socket == -1) {
        errnoPrint("socket");
        return 1;
    }
    signal(SIGINT, intHandler);
    if (bind(listen_socket, (struct sockaddr*) &server, sizeof(struct sockaddr_in)) == -1) {
        errnoPrint("bind");
        close(listen_socket);
        return 1;
    }

    fd_set fds;
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 250000000; // 250ms
    sigset_t blockset;
    sigfillset(&blockset);
    sigdelset(&blockset, SIGINT);

    debugPrint("Waiting for connections...");
    while (running) {
        if (listen(listen_socket, MAX_QUERYS) == -1) {
            errnoPrint("listen");
            close(listen_socket);
            return 1;
        }
        FD_ZERO(&fds);
        FD_SET(listen_socket, &fds);
        int retval = pselect(listen_socket + 1, &fds, NULL, NULL, &ts, &blockset);
        if (retval == -1) {
            if (errno == EINTR) {
                // SIGINT was caught
                close(listen_socket);
                return 0;
            } else {
                errnoPrint("select");
                close(listen_socket);
                return 1;
            }
        } else if (retval == 0) {
            continue;
        } else {
            struct sockaddr_in remote_host;
            socklen_t sin_size = sizeof(struct sockaddr_in);
            int client_socket = accept(listen_socket, (struct sockaddr *) &remote_host, &sin_size);
            if (client_socket == -1) {
                errnoPrint("accept");
                close(listen_socket);
                return 1;
            }
            debugPrint("Got a new connection! Sending to LoginThread...");
            loginAddSocket(client_socket);
        }
    }

    close(listen_socket);

    pthread_exit(NULL);
    return 0;
}


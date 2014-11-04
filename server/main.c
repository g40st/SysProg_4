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

#define LOCKFILE "/tmp/serverGroup01"

int singleton(const char *lockfile) {
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

void show_help() {
    printf("Available options:\n");
    printf("    -p --port       specify a port (argument)\n");
    printf("    -v --verbose    enable debug output\n");
    printf("    -h --help       show this help message\n");
}

int main(int argc, char **argv) {
    setProgName(argv[0]);
    debugDisable();
    infoPrint("Server Gruppe 01");

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

    debugPrint("Starting Threads...");
    pthread_t threads[2];
    if (pthread_create(&threads[0], NULL, loginThread, NULL) != 0) {
        errorPrint("pthread_create: %s", strerror(errno));
        return 1;
    }
    if (pthread_create(&threads[1], NULL, scoreThread, NULL) != 0) {
        errorPrint("pthread_create: %s", strerror(errno));
        return 1;
    }

    debugPrint("Creating socket...");
    int listen_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_socket == -1) {
        errorPrint("socket: %s", strerror(errno));
        return 1;
    }
    if (bind(listen_socket, (struct sockaddr*) &server, sizeof(struct sockaddr_in)) == -1) {
        errorPrint("bind: %s", strerror(errno));
        close(listen_socket);
        return 1;
    }

    debugPrint("Waiting for connections...");
    while (1) {
        if (listen(listen_socket, MAX_QUERYS) == -1) {
            errorPrint("listen: %s", strerror(errno));
            close(listen_socket);
            return 1;
        }

        struct sockaddr_in remote_host;
        socklen_t sin_size = sizeof(struct sockaddr_in);
        int client_socket = accept(listen_socket, (struct sockaddr *) &remote_host, &sin_size);
        if (client_socket == -1) {
            errorPrint("accept: %s", strerror(errno));
            close(listen_socket);
            return 1;
        }

        debugPrint("Got a new connection! Sending to LoginThread...");
        loginAddSocket(client_socket);
    }

    pthread_exit(NULL);
    return 0;
}


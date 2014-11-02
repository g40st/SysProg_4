/*
 * Systemprogrammierung
 * Multiplayer-Quiz
 *
 * Server
 *
 * main.c: Hauptprogramm des Servers
 */

#include "server/login.h"
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

#define MAX_SOCKETS 4
int sockets[MAX_SOCKETS];
int socketCount = 0;
pthread_mutex_t socketMutex = PTHREAD_MUTEX_INITIALIZER;

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
    printf("    -p --port    specify a port (argument)\n");
    printf("    -h --help    show this help message\n");
}

int main(int argc, char **argv) {
    setProgName(argv[0]);
    infoPrint("Server Gruppe 01");

    if (singleton(LOCKFILE) != 0)
        return 1;

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(PORT);

    const char* short_options = "hp:";
    struct option long_options[] = {
        { "help", no_argument, 0, 'h' },
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

            case 'p':
                if(optarg)
                    server.sin_port = htons(atoi(optarg));
                break;

            default:
            case '?':
                printf("Option not implemented yet -- %s (%c)\n", argv[optind-1], c);
                show_help();
                break;

            case -1:
                loop = 0;
                break;
        }

    }

    // Create Threads
    pthread_t threads[1];
    if (pthread_create(threads, NULL, loginThread, NULL) != 0) {
        printf("pthread_create: %s\n", strerror(errno));
        return 1;
    }

    printf("Serverport: %i\n", ntohs(server.sin_port));

    int listen_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_socket == -1) {
        printf("socket: %s\n", strerror(errno));
        return 1;
    }

    // IP und Port zuweisen
    if (bind(listen_socket, (struct sockaddr*) &server, sizeof(struct sockaddr_in)) == -1) {
        printf("bind: %s\n", strerror(errno));
        close(listen_socket);
        return 1;
    }

    // Listen for connections from a new client
    while (1) {
        // Wartet auf Verbindungsanfragen
        if (listen(listen_socket, MAX_QUERYS) == -1) {
            printf("listen: %s\n", strerror(errno));
            close(listen_socket);
            return 1;
        }

        struct sockaddr_in remote_host;
        socklen_t sin_size = sizeof(struct sockaddr_in);
        int client_socket = accept(listen_socket, (struct sockaddr *) &remote_host, &sin_size);
        if (client_socket == -1) {
            printf("accept: %s\n", strerror(errno));
            close(listen_socket);
            return 1;
        }

        pthread_mutex_lock(&socketMutex);
        if (socketCount < (MAX_SOCKETS - 1)) {
            sockets[socketCount++] = client_socket;
        } else {
            printf("Too many connections...\n");
            close(listen_socket);
            return 1;
        }
        pthread_mutex_unlock(&socketMutex);

        close(client_socket);
    }

    pthread_exit(NULL);
    return 0;
}


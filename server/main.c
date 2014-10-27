/*
 * Systemprogrammierung
 * Multiplayer-Quiz
 *
 * Server
 *
 * main.c: Hauptprogramm des Servers
 */

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

#define PORT 8111
#define MAX_QUERYS 4
#define MAX_CHAR 1024

void show_help() {
    printf("Usage: getopt [OPTIONS] [EXTRA] ...\n");
    printf("        -p --port       specify a port(argument) \n");
    printf("        -h --help       show this help message \n");
}

int main(int argc, char **argv) {
	setProgName(argv[0]);
	/* debugEnable() */

	infoPrint("Server Gruppe 01");

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(PORT);


    const char* short_options = "hp:";
    struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {"port", required_argument, 0, 'p'},
        {NULL, 0, NULL, 0}
    };

    int option_index = 0;
    int c;

    while(1) {
        c = getopt_long(argc, argv, short_options, long_options, &option_index);
        if(c == -1) {
            break;
        }
        switch(c) {
            case 'h':
                show_help();
                exit(1);
                break;
            case 'p':
                if(optarg) {
                    server.sin_port = htons(atoi(optarg));
                    break;
                }
            case '?':
                    printf("Option not implemented yet -- %s (%c)\n", argv[optind-1], c);
                    show_help();
                    break;
        }

    }

    printf("Serverport: %i\n", ntohs(server.sin_port));

    int listen_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_socket == -1) {
        printf("socket: %s\n", strerror(errno));
        return 1;
    }

    // IP und Port zweisen
    if (bind(listen_socket, (struct sockaddr*) &server, sizeof(struct sockaddr_in)) == -1) {
        printf("bind: %s\n", strerror(errno));
        close(listen_socket);
        return 1;
    }

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
	return 0;
}

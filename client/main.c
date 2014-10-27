/*
 * Systemprogrammierung
 * Multiplayer-Quiz
 *
 * Client
 *
 * main.c: Hauptprogramm des Clients
 */

#include "common/util.h"
#include "gui/gui_interface.h"
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

void show_help() {
    printf("Available options:\n");
	printf("        -s --host       specify a host(argument) [required] \n");
    printf("        -p --port       specify a port(argument) \n");
    printf("        -h --help       show this help message \n");
}

int main(int argc, char **argv) {
	setProgName(argv[0]);
	debugEnable();

	guiInit(&argc, &argv);
	infoPrint("Client Gruppe 01");

	/* Initialisierung: Verbindungsaufbau, Threads starten usw... */
	struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serv_addr.sin_port = htons(PORT);

    const char* short_options = "hp:s:";
    struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {"port", required_argument, 0, 'p'},
        {"host", required_argument, 0, 's'},
        {NULL, 0, NULL, 0}
    };

    int option_index = 0;
    int c;

    int argument = 0;
    for(int i = 0; i < argc; i++) {
        if((strcmp(argv[i], "-s") == 0) || (strcmp(argv[i], "--host")  == 0)) {
            argument = 1;
        }
        else{
        }
    }
    if(argument == 0) {
        printf("host argument must be set!\n");
        show_help();
        exit(0);
    }

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
            case 's':
                if(optarg) {
                    serv_addr.sin_addr.s_addr = inet_addr(optarg);
                    break;
                }
            case 'p':
                if(optarg) {
                    serv_addr.sin_port = htons(atoi(optarg));
                    break;
                }
            case '?':
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

	preparation_showWindow();
	guiMain();

	/* Resourcen freigeben usw... */
	guiDestroy();

	return 0;
}

void preparation_onCatalogChanged(const char *newSelection)
{
	debugPrint("Katalogauswahl: %s", newSelection);
}

void preparation_onStartClicked(const char *currentSelection)
{
	debugPrint("Starte Katalog %s", currentSelection);
}

void preparation_onWindowClosed(void)
{
	debugPrint("Vorbereitungsfenster geschlossen");
	guiQuit();
}

void game_onSubmitClicked(unsigned char selectedAnswers)
{
	debugPrint("Absende-Button angeklickt, Bitmaske der Antworten: %u",
			(unsigned)selectedAnswers);
}

void game_onWindowClosed(void)
{
	debugPrint("Spielfenster geschlossen");
	guiQuit();
}

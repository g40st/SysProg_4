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

void show_help() {
    printf("Usage: getopt [OPTIONS] [EXTRA] ...\n");
    printf("        -f --flag       flag without argument \n");
    printf("        -p --port       specify a server(argument) \n");
    printf("\n");
    printf("        -h --help       show this help message \n");
    printf("\n");
    printf("Examples:\n");

}

int main(int argc, char **argv)
{
	setProgName(argv[0]);
	/* debugEnable() */

	infoPrint("Server Gruppe xy");

    const char* short_options = "";

	return 0;
}

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

int main(int argc, char **argv)
{
	setProgName(argv[0]);
	debugEnable();

	guiInit(&argc, &argv);
	infoPrint("Client Gruppe xy");

	/* Initialisierung: Verbindungsaufbau, Threads starten usw... */

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

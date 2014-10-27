/*
 * guiMain.cpp - Main GUI method implementations
 *
 * Copyright 2014 Thomas Buck <xythobuz@xythobuz.de
 */

#include "guiApp.h"

static wxApp* ourApp = NULL;
static int* gui_argc = NULL;
static char ***gui_argv = NULL;

extern "C" {

#include "../gui/gui_interface.h"

void guiInit(int *argc, char ***argv) {
    if (ourApp == NULL) {
        ourApp = new ClientApp();
        wxApp::SetInstance(ourApp);
        gui_argc = argc;
        gui_argv = argv;
    }
}

void guiMain(void) {
    wxEntry(*gui_argc, *gui_argv);
}

void guiQuit(void) {

}

void guiDestroy(void) {
    wxEntryCleanup();
}

void guiShowErrorDialog(const char *message, int quitOnClose) {

}

void guiShowMessageDialog(const char *message, int quitOnClose) {

}

}


/*
 * guiMain.cpp - Main GUI method implementations
 *
 * Copyright 2014 Thomas Buck <xythobuz@xythobuz.de
 */

#include "guiApp.h"

extern "C" {

#include "../gui/gui_interface.h"

void guiInit(int *argc, char ***argv) {
    wxApp::SetInstance(new ClientApp());
    wxEntryStart(*argc, *argv);
    wxGetApp().CallOnInit();
}

void guiMain(void) {
    wxGetApp().OnRun();
}

void guiQuit(void) {
    wxGetApp().ExitMainLoop();
}

void guiDestroy(void) {
    wxEntryCleanup();
    //wxExit();
}

void guiShowErrorDialog(const char *message, int quitOnClose) {

}

void guiShowMessageDialog(const char *message, int quitOnClose) {

}

}


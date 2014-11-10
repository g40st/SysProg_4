/*
 * guiMain.cpp - Main GUI method implementations
 *
 * Copyright 2014 Thomas Buck <xythobuz@xythobuz.de
 */

#include "guiApp.h"

extern "C" {

#include "gui_interface.h"

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
    wxMessageDialog dialog(NULL, wxString(message), wxString("Error"));
    dialog.ShowModal();

    if (quitOnClose) {
        if (wxGetApp().preparation != NULL)
            wxGetApp().preparation->Destroy();
        if (wxGetApp().game != NULL)
            wxGetApp().game->Destroy();
    }
}

void guiShowMessageDialog(const char *message, int quitOnClose) {
    wxMessageDialog dialog(NULL, wxString(message));
    dialog.ShowModal();

    if (quitOnClose) {
        if (wxGetApp().preparation != NULL)
            wxGetApp().preparation->Destroy();
        if (wxGetApp().game != NULL)
            wxGetApp().game->Destroy();
    }
}

}


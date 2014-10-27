/*
 * guiPreparation.cpp - Implementation for Preparation Window
 *
 * Copyright 2014 Thomas Buck <xythobuz@xythobuz.de
 */

#include "guiPreparation.h"

PreparationFrame::PreparationFrame(const wxString& title, const wxPoint& pos, const wxSize& size)
        : wxFrame(NULL, wxID_ANY, title, pos, size) {

    CreateStatusBar();
    SetStatusText("Preparation Window initialized!");
}

extern "C" {

#include "../gui/gui_interface.h"

void preparation_setMode(PreparationMode mode) {

}

void preparation_showWindow(void) {

}

void preparation_addCatalog(const char *name) {

}

int preparation_selectCatalog(const char *name) {

}

void preparation_addPlayer(const char *name) {

}

int preparation_removePlayer(const char *name) {

}

void preparation_clearPlayers(void) {

}

void preparation_hideWindow(void) {

}

void preparation_reset(void) {

}

}


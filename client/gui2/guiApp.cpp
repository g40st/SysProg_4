/*
 * guiApp.cpp - WxWidgets App Class
 *
 * Copyright 2014 Thomas Buck <xythobuz@xythobuz.de
 */

#include "guiApp.h"

IMPLEMENT_APP_NO_MAIN(ClientApp);

bool ClientApp::OnInit() {
    preparation = NULL;
    game = NULL;
    return true;
}

void ClientApp::createPreparation() {
    if (!preparation)
        preparation = new PreparationFrame("Preparation", wxPoint(50, 50), wxSize(600, 400));
}

void ClientApp::createGame() {
    if (!game)
        game = new GameFrame("Game", wxPoint(50, 50), wxSize(600, 400));
}


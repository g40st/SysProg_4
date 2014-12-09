/*
 * guiApp.cpp - WxWidgets App Class
 *
 * Copyright 2014 Thomas Buck <xythobuz@xythobuz.de
 */

#include <pthread.h>
#include <sys/select.h>

#include "guiApp.h"

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

IMPLEMENT_APP_NO_MAIN(ClientApp);

bool ClientApp::OnInit() {
    pthread_mutex_lock(&mutex);
    preparation = NULL;
    game = NULL;
    pthread_mutex_unlock(&mutex);
    return true;
}

void ClientApp::createPreparation() {
    pthread_mutex_lock(&mutex);
    if (!preparation)
        preparation = new PreparationFrame("Preparation", wxPoint(50, 50), wxSize(600, 400));
    pthread_mutex_unlock(&mutex);
}

void ClientApp::createGame() {
    pthread_mutex_lock(&mutex);
    if (!game)
        game = new GameFrame("Game", wxPoint(50, 50), wxSize(600, 400));
    pthread_mutex_unlock(&mutex);
}


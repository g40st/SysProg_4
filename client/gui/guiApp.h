/*
 * guiApp.h - WxWidgets App Class
 *
 * Copyright 2014 Thomas Buck <xythobuz@xythobuz.de
 */

#ifndef _GUI_APP_H
#define _GUI_APP_H

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include "guiPreparation.h"
#include "guiGame.h"

class ClientApp : public wxApp {
public:
    virtual bool OnInit();

    void createPreparation();
    void createGame();

    PreparationFrame* preparation;
    GameFrame* game;
};

DECLARE_APP(ClientApp);

enum {
    // PreparationFrame
    BUTTON_Start = wxID_HIGHEST + 1,
    LIST_Questions = wxID_HIGHEST + 2,

    // GameFrame
    BUTTON_Send = wxID_HIGHEST + 3,
};

#endif


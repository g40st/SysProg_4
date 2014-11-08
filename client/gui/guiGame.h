/*
 * guiGame.h - Implementation for Game Window
 *
 * Copyright 2014 Thomas Buck <xythobuz@xythobuz.de
 */

#ifndef _GUI_GAME_H
#define _GUI_GAME_H

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

class GameFrame: public wxFrame {
public:
    GameFrame(const wxString& title, const wxPoint& pos, const wxSize& size);
};

#endif


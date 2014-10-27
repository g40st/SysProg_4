/*
 * guiPreparation.h - Implementation for Preparation Window
 *
 * Copyright 2014 Thomas Buck <xythobuz@xythobuz.de
 */

#ifndef _GUI_PREPARATION_H
#define _GUI_PREPARATION_H

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

class PreparationFrame: public wxFrame {
public:
    PreparationFrame(const wxString& title, const wxPoint& pos, const wxSize& size);
};

#endif


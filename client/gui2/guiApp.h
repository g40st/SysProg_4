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

class ClientApp : public wxApp {
public:
    virtual bool OnInit();
};

#endif


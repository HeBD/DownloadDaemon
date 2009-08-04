/***************************************************************
 * Name:      ddclient-wx.cpp
 * Purpose:   Code for Application Class
 * Author:    ko ()
 * Created:   2009-08-04
 * Copyright: ko ()
 * License:
 **************************************************************/

#include "ddclient-wx_main.h"
#include "ddclient-wx.h"

IMPLEMENT_APP(myapp);

bool myapp::OnInit(){
    wxInitAllImageHandlers(); // so that pictures can be seen

    myframe* frame = new myframe(0, wxT("DownloadDaemon"));
    frame->Show();
    SetTopWindow(frame);

    return true;
}

/***************************************************************
 * Name:	  ddclient-wx.cpp
 * Purpose:   Code for Application Class
 * Author:	ko ()
 * Created:   2009-08-04
 * Copyright: ko ()
 * License:
 **************************************************************/

#include "ddclient-wx_main.h"
#include "ddclient-wx.h"

IMPLEMENT_APP(myapp);

bool myapp::OnInit(){
	wxInitAllImageHandlers(); // so that pictures can be seen

	if(argc > 0){
		myframe* frame = new myframe(argv[0], 0, wxT("DownloadDaemon-ClientWX"));
		frame->Show();
		SetTopWindow(frame);
	}
	return true;
}

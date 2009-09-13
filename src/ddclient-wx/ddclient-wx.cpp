/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

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

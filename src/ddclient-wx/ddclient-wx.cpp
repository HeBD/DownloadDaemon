#include <wx/wxprec.h>

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include <wx/app.h>
#endif

#include "ddclient-wx_main.h"
#include "connect_dlg.h"
#include "../lib/netpptk.h"
#include "dlg_add.h"
#include <boost/thread.hpp>
#include <string>
#include <sstream>
#include <vector>

#include <wx/msgdlg.h>
//(*AppHeaders
#include <wx/image.h>
//*)

void list_manager(wxListCtrl *list);
// Global Variable declaration.. There are some thing we need through the whole program, so let's just make them global..
tkSock srvConnection;
bool connected_successfully = false;
bool enable_poll = true;

class MyApp : public wxApp
{
	public:
		virtual bool OnInit();
};

IMPLEMENT_APP(MyApp);

bool MyApp::OnInit()
{
	ddclient_main* Frame;
    //(*AppInitialize
    bool wxsOK = true;
    wxInitAllImageHandlers();
    if ( wxsOK )
    {
	Frame = new ddclient_main(0);
    Frame->Show();
    SetTopWindow(Frame);
    }
    //*)
    boost::thread list_mgmt(list_manager, Frame->DownloadList);
    return wxsOK;
}



void list_manager(wxListCtrl *list) {
	// Create the columns

	list->InsertColumn( 0, _("ID"), wxLIST_AUTOSIZE_USEHEADER, -1);
	list->InsertColumn( 1, _("Added"), wxLIST_AUTOSIZE_USEHEADER, -1);
	list->InsertColumn( 2, _("Title"), wxLIST_AUTOSIZE_USEHEADER, -1);
	list->InsertColumn( 3, _("\tURL\t"), wxLIST_AUTOSIZE_USEHEADER, -1);
	list->InsertColumn( 4, _("Status"), wxLIST_AUTOSIZE_USEHEADER, -1);
	list->InsertColumn( 5, _("Bytes Done"), wxLIST_AUTOSIZE_USEHEADER, -1);
	list->InsertColumn( 6, _("Bytes Total"), wxLIST_AUTOSIZE_USEHEADER, -1);
	list->InsertColumn( 7, _("Wait"), wxLIST_AUTOSIZE_USEHEADER, -1);
	list->InsertColumn( 8, _("Error"), wxLIST_AUTOSIZE_USEHEADER, -1);

	list->Show();

	std::string answer;
	std::stringstream ss;
	while(true) {
		if(!connected_successfully || !srvConnection || !enable_poll) {
			if(!srvConnection) {
				connected_successfully = false;
			}
			sleep(1);
			continue;
		}
		srvConnection << "DDP GET LIST";
		srvConnection >> answer;
		ss << answer;

		list->ClearAll();
		while(getline(ss, answer)) {
			answer.erase(answer.length());
			int index = 0;

			size_t start = 0;
			if(answer.find('|') == std::string::npos) {
				connected_successfully = false;
				continue;
			}

			size_t pos = answer.find('|') + 1;
			wxString id(answer.substr(1, pos).c_str(), wxConvUTF8);
			long itemIndex = list->InsertItem( 0 , id );
			while((pos = answer.find('|', pos)) != std::string::npos) {
				std::string curr = answer.substr(start, pos - start);
				wxString data(curr.c_str(), wxConvUTF8);
				++pos;
				start = pos;
				list->SetItem(itemIndex, index, data);
				++index;
			}
		}
		sleep(1);
	}




//1|Tue Jul 21 21:02:35 2009|testdownload|http://rapidshare.com/files/144286726/worldshift_Ka0oS.part02.rar|DOWNLOAD_RUNNING|280800|200000000|113|NO_ERROR
}


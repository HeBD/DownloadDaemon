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
//(*AppHeaders
#include <wx/image.h>
//*)

// Global Variable declaration.. There are some thing we need through the whole program, so let's just make them global..
tkSock srvConnection;
bool connected_successfully = false;

class MyApp : public wxApp
{
	public:
		virtual bool OnInit();
};

IMPLEMENT_APP(MyApp);

bool MyApp::OnInit()
{
    //(*AppInitialize
    bool wxsOK = true;
    wxInitAllImageHandlers();
    if ( wxsOK )
    {
    ddclient_main* Frame = new ddclient_main(0);
    Frame->Show();
    SetTopWindow(Frame);
    }
    //*)
    return wxsOK;
}


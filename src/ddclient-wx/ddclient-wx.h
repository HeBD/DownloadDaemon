/***************************************************************
 * Name:	  ddclient-wx.h
 * Purpose:   Header for Application Class
 * Author:	ko ()
 * Created:   2009-08-04
 * Copyright: ko ()
 * License:
 **************************************************************/

#ifndef DDCLIENT_WX_APP_H
#define DDCLIENT_WX_APP_H

#include <wx/wxprec.h>

#ifdef __BORLANDC__
	#pragma hdrstop
#endif

#ifndef WX_PRECOMP
	#include <wx/wx.h>
#endif

class myapp: public wxApp{
	    public:
			virtual bool OnInit();
};

#endif //DDCLIENT_WX_APP_H

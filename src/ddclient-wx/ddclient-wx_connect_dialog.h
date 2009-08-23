/***************************************************************
 * Name:	  ddclient-wx_connect_dialog.h
 * Purpose:   Header for Connect Dialog Class
 * Author:	ko ()
 * Created:   2009-08-05
 * Copyright: ko ()
 * License:
 **************************************************************/

#ifndef DDCLIENT_WX_CONNECT_DIALOG_H
#define DDCLIENT_WX_CONNECT_DIALOG_H

#include <wx/msgdlg.h> // for wxmessagebox
#include <wx/dialog.h>
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/textctrl.h>

#include "../lib/netpptk/netpptk.h"
#if defined(__WXMSW__)
    #include <wx/msw/winundef.h> // because of conflicting wxWidgets and windows.h
#endif
#include "ddclient-wx_main.h"


/** Connection Dialog Class. Shows a Dialog to connect to a DownloadDaemon Server. */
class connect_dialog : public wxDialog{
	public:

		/** Constructor
		*	@param parent Parent wxWindow
		*/
		connect_dialog(wxWindow *parent);

	private:
		wxStaticText *message_text;
		wxStaticText *host_text;
		wxStaticText *port_text;
		wxStaticText *pass_text;
		wxTextCtrl *host_input;
		wxTextCtrl *port_input;
		wxTextCtrl *pass_input;
		wxButton *connect_button;
		wxButton *cancel_button;

		wxBoxSizer *dialog_sizer;
		wxBoxSizer *text_sizer;
		wxStaticBoxSizer* outer_input_sizer;
		wxFlexGridSizer *input_sizer;
		wxBoxSizer *button_sizer;

		// event handle methods
		void on_connect(wxCommandEvent &event);
		void on_cancel(wxCommandEvent &event);

		DECLARE_EVENT_TABLE()
};


#endif // DDCLIENT_WX_CONNECT_DIALOG_H

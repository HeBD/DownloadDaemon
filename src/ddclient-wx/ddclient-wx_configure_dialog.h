/***************************************************************
 * Name:	  ddclient-wx_configure_dialog.h
 * Purpose:   Header for Configure Dialog Class
 * Author:	ko ()
 * Created:   2009-08-26
 * Copyright: ko ()
 * License:
 **************************************************************/

#ifndef DDCLIENT_WX_CONFIGURE_DIALOG_H
#define DDCLIENT_WX_CONFIGURE_DIALOG_H

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


/** Configure Dialog Class. Shows a Dialog to configure to DownloadDaemon Server. */
class configure_dialog : public wxDialog{
	public:

		/** Constructor
		*	@param parent Parent wxWindow
		*/
		configure_dialog(wxWindow *parent);

	private:
		wxStaticText *old_pass_text;
		wxStaticText *new_pass_text;
		wxTextCtrl *old_pass_input;
		wxTextCtrl *new_pass_input;
		wxButton *pass_button;
		wxButton *cancel_button;

		wxBoxSizer *dialog_sizer;
		wxStaticBoxSizer* outer_pass_sizer;
		wxFlexGridSizer *pass_sizer;
		wxBoxSizer *button_sizer;

		// element IDs
		static const long id_pass_change;

		// event handle methods
		void on_pass_change(wxCommandEvent &event);
		void on_cancel(wxCommandEvent &event);

		DECLARE_EVENT_TABLE()
};



#endif // DDCLIENT_WX_CONFIGURE_DIALOG_H

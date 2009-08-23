/***************************************************************
 * Name:	  ddclient-wx_add_dialog.h
 * Purpose:   Header for Add Dialog Class
 * Author:	ko ()
 * Created:   2009-08-18
 * Copyright: ko ()
 * License:
 **************************************************************/

#ifndef DDCLIENT_WX_ADD_DIALOG_H
#define DDCLIENT_WX_ADD_DIALOG_H

//#include <wx/msgdlg.h> // for wxmessagebox
#include <wx/dialog.h>
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/textctrl.h>

#include "../lib/netpptk/netpptk.h"
#if defined(__WXMSW__)
    #include <wx/msw/winundef.h> // Because of conflicting wxWidgets and windows.h
#endif
#include "ddclient-wx_main.h"


/** Add Dialog Class. Shows a Dialog to add Downloads. */
class add_dialog : public wxDialog{
	public:

		/** Constructor
		*	@param parent Parent wxWindow
		*/
		add_dialog(wxWindow *parent);

	private:
		wxStaticText *add_one_text;
		wxStaticText *add_many_text;
		wxStaticText *title_text;
		wxStaticText *url_text;
		wxStaticText *add_many_inner_text;
		wxTextCtrl *title_input;
		wxTextCtrl *url_input;
		wxTextCtrl *many_input;

		wxButton *add_one_button;
		wxButton *add_many_button;
		wxButton *cancel_button;

		wxBoxSizer *dialog_sizer;
		wxStaticBoxSizer* outer_add_one_sizer;
		wxStaticBoxSizer* outer_add_many_sizer;
		wxFlexGridSizer *inner_add_one_sizer;

		// element IDs
		static const long id_add_one;
		static const long id_add_many;

		// event handle methods
		void on_add_one(wxCommandEvent &event);
		void on_add_many(wxCommandEvent &event);
		void on_cancel(wxCommandEvent &event);

		DECLARE_EVENT_TABLE()
};


#endif // DDCLIENT_WX_ADD_DIALOG_H


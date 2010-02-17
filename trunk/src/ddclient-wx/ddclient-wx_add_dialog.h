/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef DDCLIENT_WX_ADD_DIALOG_H
#define DDCLIENT_WX_ADD_DIALOG_H

#include <wx/dialog.h>
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/textctrl.h>

#if defined(__WXMSW__)
	#include <wx/msw/winundef.h> // Because of conflicting wxWidgets and windows.h
#endif


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

		wxButton *add_button;
		wxButton *cancel_button;

		wxBoxSizer *dialog_sizer;
		wxStaticBoxSizer* outer_add_one_sizer;
		wxStaticBoxSizer* outer_add_many_sizer;
		wxFlexGridSizer *inner_add_one_sizer;
		wxBoxSizer *button_sizer;

		// event handle methods
		void on_add(wxCommandEvent &event);
		void on_cancel(wxCommandEvent &event);

		DECLARE_EVENT_TABLE()
};


#endif // DDCLIENT_WX_ADD_DIALOG_H


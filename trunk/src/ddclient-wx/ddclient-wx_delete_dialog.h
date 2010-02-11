/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef DDCLIENT_WX_DELETE_DIALOG_H
#define DDCLIENT_WX_DELETE_DIALOG_H

#include <wx/dialog.h>
#include <wx/panel.h>
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/string.h>

#include "ddclient-wx_main.h"


/** Delete Dialog Class. Shows a Dialog which asks the User if a File should be deleted. */
class delete_dialog: public wxDialog{
	public:

		/** Constructor
		*	@param answer Button clicked to exit the Dialog
		*	@param id ID of the File to be deleted
		*	@param parent Parent wxWindow
		*/
		delete_dialog(int *answer, std::string id, wxWindow *parent);

	private:
		int *answer;
		wxStaticText *question_text;
		wxButton *yes_all_button;
		wxButton *yes_button;
		wxButton *no_button;
		wxButton *no_all_button;

		wxBoxSizer *overall_sizer;
		wxBoxSizer *button_sizer;

		// element IDs
		static const long id_yes_all;
		static const long id_no_all;

		// event handle methods
		void on_yes_all(wxCommandEvent &event);
		void on_yes(wxCommandEvent &event);
		void on_no(wxCommandEvent &event);
		void on_no_all(wxCommandEvent &event);

		DECLARE_EVENT_TABLE()
};

#endif // DDCLIENT_WX_DELETE_DIALOG_H



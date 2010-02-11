/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef DDCLIENT_WX_ABOUT_DIALOG_H
#define DDCLIENT_WX_ABOUT_DIALOG_H

#include <wx/dialog.h>
#include <wx/panel.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/string.h>
#include <wx/hyperlink.h>

#include "ddclient-wx_picture.h"
#include "ddclient-wx_main.h"


/** About Dialog Class. Shows a Dialog with Information about the Program. */
class about_dialog: public wxDialog{
	public:

		/** Constructor
		*	@param working_dir Working Directory of the Program
		*	@param parent Parent wxWindow
		*/
		about_dialog(wxString working_dir, wxWindow *parent);

	private:
		wxString working_dir;
		wxStaticText *name_text;
		wxStaticText *build_text;
		wxHyperlinkCtrl *website_text;
		wxButton *ok_button;
		picture *pic;

		wxBoxSizer *overall_sizer;
		wxBoxSizer *info_sizer;
		wxBoxSizer *text_sizer;

		// event handle methods
		void on_ok(wxCommandEvent &event);

		DECLARE_EVENT_TABLE()
};

#endif // DDCLIENT_WX_ABOUT_DIALOG_H


/***************************************************************
 * Name:      ddclient-wx_about_dialog.h
 * Purpose:   Header for About Dialog Class
 * Author:    ko ()
 * Created:   2009-08-09
 * Copyright: ko ()
 * License:
 **************************************************************/

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

class about_dialog: public wxDialog{
	public:
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
		void on_link_click(wxHyperlinkEvent &event);

		DECLARE_EVENT_TABLE()
};

#endif // DDCLIENT_WX_ABOUT_DIALOG_H


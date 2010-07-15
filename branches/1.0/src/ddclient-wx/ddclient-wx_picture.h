/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef DDCLIENT_WX_PICTURE_H
#define DDCLIENT_WX_PICTURE_H

#include <wx/panel.h>
#include <wx/bitmap.h>
#include <wx/image.h>


/** Picture Class. Paints a PNG Image. */
class picture: public wxPanel{
	public:

		/** Constructor with nearly standard wxDialog Parameters
		*	@param image Image Path
		*/
		picture(wxString image, wxWindow *parent, const wxSize &size, const wxWindowID &id=wxID_ANY, const wxPoint &pos=wxDefaultPosition, const long &style=0, const wxString &name=wxPanelNameStr);

		/** Paints the Panel.*/
		void OnPaint(wxPaintEvent &evt);

	private:

		wxBitmap t_bmp;
		DECLARE_EVENT_TABLE()
};

#endif // DDCLIENT_WX_PICTURE_H


/***************************************************************
 * Name:      ddclient-wx_picture.h
 * Purpose:   Header for Picture Class
 * Author:    ko ()
 * Created:   2009-08-09
 * Copyright: ko ()
 * License:
 **************************************************************/

#ifndef DDCLIENT_WX_PICTURE_H
#define DDCLIENT_WX_PICTURE_H

#include <wx/panel.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/dcclient.h>


/** Picture Class. Paints a PNG Image. */
class picture: public wxPanel{
    public:

		/** Constructor with nearly standard wxDialog Parameters
		*	@param image Image Path
		*/
        picture(wxString image, wxWindow *parent, const wxSize &size, const wxWindowID &id=wxID_ANY, const wxPoint &pos=wxDefaultPosition, const long &style=0, const wxString &name=wxPanelNameStr);


        void OnPaint(wxPaintEvent &evt);

    private:

        wxBitmap t_bmp;
        DECLARE_EVENT_TABLE()
};

#endif // DDCLIENT_WX_PICTURE_H


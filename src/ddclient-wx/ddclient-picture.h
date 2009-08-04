/***************************************************************
 * Name:      picture.h
 * Purpose:   Code for Picture Class
 * Author:    ko ()
 * Created:   2009-08-04
 * Copyright: ko ()
 * License:
 **************************************************************/

#ifndef PICTURE_H
#define PICTURE_H

#include <wx/panel.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/dcclient.h>

class picture : public wxPanel{
    public:
        picture(wxWindow *parent, const wxWindowID &id, const wxPoint &pos, const wxSize &size, const long &style, const wxString &name, wxString image);
        void OnPaint(wxPaintEvent &evt);

    private:
        wxBitmap t_bmp;
        DECLARE_EVENT_TABLE()
};

#endif //PICTURE_H


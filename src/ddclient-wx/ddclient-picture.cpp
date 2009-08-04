/***************************************************************
 * Name:      ddclient-picture.cpp
 * Purpose:   Code for Picture Class
 * Author:    ko ()
 * Created:   2009-08-04
 * Copyright: ko ()
 * License:
 **************************************************************/

#include "ddclient-picture.h"

BEGIN_EVENT_TABLE (picture, wxPanel)
    EVT_PAINT(picture::OnPaint)
END_EVENT_TABLE()


picture::picture(wxWindow *parent,const wxWindowID &id, const wxPoint &pos, const wxSize &size, const long &style, const wxString &name, wxString image)
: wxPanel(parent, id, pos, size, style, name){
   t_bmp = wxBitmap(wxImage(image, wxBITMAP_TYPE_JPEG));
}

void picture::OnPaint(wxPaintEvent &evt){
    wxPaintDC dc(this);
    dc.DrawBitmap(t_bmp, 0, 0, true);
}



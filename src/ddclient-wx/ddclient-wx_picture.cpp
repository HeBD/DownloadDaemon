/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "ddclient-wx_picture.h"

BEGIN_EVENT_TABLE (picture, wxPanel)
    EVT_PAINT(picture::OnPaint)
END_EVENT_TABLE()


picture::picture(wxString image, wxWindow *parent, const wxSize &size, const wxWindowID &id, const wxPoint &pos, const long &style, const wxString &name)
: wxPanel(parent, id, pos, size, style, name){
   t_bmp = wxBitmap(wxImage(image, wxBITMAP_TYPE_PNG));
}

void picture::OnPaint(wxPaintEvent &event){
    wxPaintDC dc(this);
    dc.DrawBitmap(t_bmp, 0, 0, true);
}



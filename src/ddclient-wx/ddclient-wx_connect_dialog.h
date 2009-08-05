/***************************************************************
 * Name:      ddclient-wx_connect_dialog.h
 * Purpose:   Header for Connect Dialog Class
 * Author:    ko ()
 * Created:   2009-08-05
 * Copyright: ko ()
 * License:
 **************************************************************/

#ifndef DDCLIENT_WX_CONNECT_DIALOG_H
#define DDCLIENT_WX_CONNECT_DIALOG_H

#include <wx/dialog.h>
#include <wx/stattext.h>
#include <wx/button.h>

class connect_dialog : public wxDialog{
    public:
        connect_dialog(wxWindow *parent);

    private:
        wxStaticText *message;
        wxButton *connect_button;

        // event handle methods
        void on_connect(wxCloseEvent &event);

        DECLARE_EVENT_TABLE()
};


#endif //DDCLIENT_WX_CONNECT_DIALOG_H

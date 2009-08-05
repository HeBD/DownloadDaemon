/***************************************************************
 * Name:      ddclient-wx_connect_dialog.cpp
 * Purpose:   Code for Connect Dialog Class
 * Author:    ko ()
 * Created:   2009-08-04
 * Copyright: ko ()
 * License:
 **************************************************************/

#include "ddclient-wx_connect_dialog.h"

BEGIN_EVENT_TABLE(connect_dialog, wxDialog)
    EVT_CLOSE(connect_dialog::on_connect)
END_EVENT_TABLE()

connect_dialog::connect_dialog(wxWindow *parent) : wxDialog(parent, -1, wxT("Connect to Host")){

    message = new wxStaticText(this, -1, wxT("Host IP/URL"), wxPoint(5,5));
    connect_button = new wxButton(this, wxID_ANY, wxT("Connect"), wxPoint(5, 40));

    return;
}


// event handle methods
void connect_dialog::on_connect(wxCloseEvent &event){
    // give the data to parent (myframe)
    Destroy();
}

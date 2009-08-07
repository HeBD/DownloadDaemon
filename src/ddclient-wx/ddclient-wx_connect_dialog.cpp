/***************************************************************
 * Name:      ddclient-wx_connect_dialog.cpp
 * Purpose:   Code for Connect Dialog Class
 * Author:    ko ()
 * Created:   2009-08-04
 * Copyright: ko ()
 * License:
 **************************************************************/

#include "ddclient-wx_connect_dialog.h"


// event table
BEGIN_EVENT_TABLE(connect_dialog, wxDialog)
    EVT_BUTTON(wxID_OK, connect_dialog::on_connect)
    EVT_BUTTON(wxID_CANCEL, connect_dialog::on_cancel)
END_EVENT_TABLE()


connect_dialog::connect_dialog(wxWindow *parent) : wxDialog(parent, -1, wxT("Connect to Host")){

    // create elements
    dialog_sizer = new wxBoxSizer(wxVERTICAL);
    text_sizer = new wxBoxSizer(wxHORIZONTAL);
    outer_input_sizer = new wxStaticBoxSizer(wxHORIZONTAL,this,wxT("Data"));
    input_sizer = new wxFlexGridSizer(3, 2, 10, 10);
    button_sizer = new wxBoxSizer(wxHORIZONTAL);

    message_text = new wxStaticText(this, -1, wxT("Please insert Host Data"));
    host_text = new wxStaticText(this, -1, wxT("IP/URL"));
    port_text = new wxStaticText(this, -1, wxT("Port"));
    pass_text = new wxStaticText(this, -1, wxT("Password"));

    host_input = new wxTextCtrl(this,-1,wxEmptyString, wxDefaultPosition, wxSize(300, 25));
    port_input = new wxTextCtrl(this,-1,wxT("56789"), wxDefaultPosition, wxSize(50, 25));
    pass_input = new wxTextCtrl(this,-1,wxEmptyString, wxDefaultPosition, wxSize(150, 25), wxTE_PASSWORD);

    connect_button = new wxButton(this, wxID_OK, wxT("Connect"));
    cancel_button = new wxButton(this, wxID_CANCEL, wxT("Cancel"));


    // filling sizers
    text_sizer->Add(message_text, 1, wxALL, 20);

    input_sizer->Add(host_text, 0, wxALIGN_LEFT);
    input_sizer->Add(host_input, 0, wxALIGN_LEFT);
    input_sizer->Add(port_text, 0, wxALIGN_LEFT);
    input_sizer->Add(port_input, 0, wxALIGN_LEFT);
    input_sizer->Add(pass_text, 0, wxALIGN_LEFT);
    input_sizer->Add(pass_input, 0, wxALIGN_LEFT);

    button_sizer->Add(connect_button, 1, wxALL, 20);
    button_sizer->Add(cancel_button, 1, wxALL, 20);

    outer_input_sizer->Add(input_sizer,0,wxALL|wxEXPAND,10);

    dialog_sizer->Add(text_sizer, 0, wxGROW);
    dialog_sizer->Add(outer_input_sizer, 0, wxGROW|wxLEFT|wxRIGHT, 20);
    dialog_sizer->Add(button_sizer, 0, wxGROW);

    SetSizer(dialog_sizer);
    Layout();
    Fit();

    return;
}


// event handle methods
void connect_dialog::on_connect(wxCommandEvent &event){ //TODO needs testing

    std::string host = std::string(host_input->GetValue().mb_str());
    int port = wxAtoi(port_input->GetValue());
    std::string pass = std::string(pass_input->GetValue().mb_str());

    tkSock *mysock = new tkSock();
    bool connection = false;

    try{
       connection = mysock->connect(host, port);
    }catch(...){} // no code needed here due to boolean connection


    if(connection){
        wxMessageBox(wxT("Connection succeeded."), wxT("Delete me when finished.")); //TODO delete

        myframe *myparent = (myframe *) GetParent();

        if(myparent->get_connection_attributes() != NULL) //if there is already a connection, delete the old one
            delete myparent->get_connection_attributes();

        myparent->set_connection_attributes(mysock, pass);

    }else // failed
        wxMessageBox(wxT("\nConnection failed.\t\t\nPlease try again."), wxT("Connection failed"));

    // TODO testing if you need a password has to be done somewhere => maybe here

    Destroy();
}

void connect_dialog::on_cancel(wxCommandEvent &event){
    Destroy();
}

/***************************************************************
 * Name:	  ddclient-wx_configure_dialog.cpp
 * Purpose:   Code for Configure Dialog Class
 * Author:	ko ()
 * Created:   2009-08-26
 * Copyright: ko ()
 * License:
 **************************************************************/

#include "ddclient-wx_configure_dialog.h"


// IDs
const long configure_dialog::id_pass_change = wxNewId();


// event table
BEGIN_EVENT_TABLE(configure_dialog, wxDialog)
	EVT_BUTTON(id_pass_change, configure_dialog::on_pass_change)
	EVT_BUTTON(wxID_CANCEL, configure_dialog::on_cancel)
END_EVENT_TABLE()


configure_dialog::configure_dialog(wxWindow *parent) : wxDialog(parent, -1, wxString(wxT("Configure DownloadDaemon Server"))){

	// create elements
	dialog_sizer = new wxBoxSizer(wxVERTICAL);
	outer_pass_sizer = new wxStaticBoxSizer(wxHORIZONTAL,this,wxT("Change Password"));
	pass_sizer = new wxFlexGridSizer(3, 2, 10, 10);
	button_sizer = new wxBoxSizer(wxHORIZONTAL);

	old_pass_text = new wxStaticText(this, -1, wxT("Old Password"));
	new_pass_text = new wxStaticText(this, -1, wxT("New Password"));

	old_pass_input = new wxTextCtrl(this,-1,wxT(""), wxDefaultPosition, wxSize(200, 25), wxTE_PASSWORD);
	old_pass_input->SetSelection(-1, -1);
	old_pass_input->SetFocus();
	new_pass_input = new wxTextCtrl(this,-1,wxT(""), wxDefaultPosition, wxSize(200, 25), wxTE_PASSWORD);

	pass_button = new wxButton(this, id_pass_change, wxT("Change Password"));
	pass_button->SetDefault();
	cancel_button = new wxButton(this, wxID_CANCEL, wxT("Cancel"));

	// filling sizers
	pass_sizer->Add(old_pass_text, 0, wxALIGN_LEFT);
	pass_sizer->Add(old_pass_input, 0, wxALIGN_LEFT);
	pass_sizer->Add(new_pass_text, 0, wxALIGN_LEFT);
	pass_sizer->Add(new_pass_input, 0, wxALIGN_LEFT);
	pass_sizer->Add(pass_button, 0, wxALIGN_LEFT);

	button_sizer->Add(cancel_button, 1, wxALL, 20);

	outer_pass_sizer->Add(pass_sizer, 0,wxALL|wxEXPAND,10);

	dialog_sizer->Add(outer_pass_sizer, 0, wxGROW|wxALL, 20);
	dialog_sizer->Add(button_sizer, 0, wxALL);

	SetSizer(dialog_sizer);
	Layout();
	Fit();

	return;
}


// event handle methods
void configure_dialog::on_pass_change(wxCommandEvent &event){

	// getting user input
	std::string old_pass = std::string(old_pass_input->GetValue().mb_str());
	std::string new_pass = std::string(new_pass_input->GetValue().mb_str());

	// check connection
	myframe *myparent = (myframe *) GetParent();
	tkSock *mysock = myparent->get_connection_attributes();

	if(mysock == NULL || !*mysock){ // if there is no active connection
		wxMessageBox(wxT("Please connect before configurating the DownloadDaemon Server."), wxT("No Connection to Server"));

	}else{ // we have a connection
		string answer;

		boost::mutex *mx = myparent->get_mutex();
		mx->lock();

		mysock->send("DDP VAR SET mgmt_password = " + old_pass + " ; " + new_pass);
		mysock->recv(answer);

		if(answer.find("102") == 0) // 102 AUTHENTICATION	<-- Authentication failed
			wxMessageBox(wxT("Failed to reset the Password."), wxT("Password Error"));


		mx->unlock();
	}

	Destroy();
	return;
}


void configure_dialog::on_cancel(wxCommandEvent &event){
	Destroy();
	return;
}

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "ddclient-wx_connect_dialog.h"
#include "../lib/crypt/md5.h"


// event table
BEGIN_EVENT_TABLE(connect_dialog, wxDialog)
	EVT_BUTTON(wxID_OK, connect_dialog::on_connect)
	EVT_BUTTON(wxID_CANCEL, connect_dialog::on_cancel)
END_EVENT_TABLE()


connect_dialog::connect_dialog(wxWindow *parent):
	wxDialog(parent, -1, wxString(wxT("Connect to Host"))){

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

	host_input = new wxTextCtrl(this,-1,wxT("127.0.0.1"), wxDefaultPosition, wxSize(300, 25));
	host_input->SetSelection(-1, -1);
	host_input->SetFocus();
	port_input = new wxTextCtrl(this,-1,wxT("56789"), wxDefaultPosition, wxSize(50, 25));
	pass_input = new wxTextCtrl(this,-1,wxEmptyString, wxDefaultPosition, wxSize(150, 25), wxTE_PASSWORD);

	connect_button = new wxButton(this, wxID_OK);
	connect_button->SetDefault();
	cancel_button = new wxButton(this, wxID_CANCEL);


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

	outer_input_sizer->Add(input_sizer, 0,wxALL|wxEXPAND,10);

	dialog_sizer->Add(text_sizer, 0, wxGROW);
	dialog_sizer->Add(outer_input_sizer, 0, wxGROW|wxLEFT|wxRIGHT, 20);
	dialog_sizer->Add(button_sizer, 0, wxLEFT|wxRIGHT, 0);

	SetSizer(dialog_sizer);
	Layout();
	Fit();
}


// event handle methods
void connect_dialog::on_connect(wxCommandEvent &event){

	// getting user input
	std::string host = std::string(host_input->GetValue().mb_str());
	int port = wxAtoi(port_input->GetValue());
	std::string pass = std::string(pass_input->GetValue().mb_str());

	tkSock *mysock = new tkSock();
	bool connection = false, error_occured = false;

	try{
	   connection = mysock->connect(host, port);
	}catch(...){} // no code needed here due to boolean connection


	if(connection){ // connection succeeded,  host (IP/URL or port) is ok

		// authentification
		std::string snd;
		mysock->recv(snd);

		if(snd.find("100") == 0){ // 100 SUCCESS <-- Operation succeeded
			// nothing to do here if you reach this
		}else if(snd.find("102") == 0){ // 102 AUTHENTICATION <-- Authentication failed

			// try md5 authentication
			mysock->send("ENCRYPT");
			std::string rnd;
			mysock->recv(rnd); // random bytes

			if(rnd.find("102") != 0) { // encryption permitted
				rnd += pass;

				MD5_CTX md5;
				MD5_Init(&md5);

				unsigned char *enc_data = new unsigned char[rnd.length()];
				for(size_t i = 0; i < rnd.length(); ++i){ // copy random bytes from string to cstring
					enc_data[i] = rnd[i];
				}

				MD5_Update(&md5, enc_data, rnd.length());
				unsigned char result[16];
				MD5_Final(result, &md5); // put md5hash in result
				std::string enc_passwd((char*)result, 16);
				delete [] enc_data;

				mysock->send(enc_passwd);
				mysock->recv(snd);

			}else{ // encryption not permitted
				wxMessageDialog dialog(this, wxT("Encrypted authentication not supported by server.\nDo you want to try unsecure plain-text authentication?"),
									   wxT("No Encryption"), wxYES_NO|wxYES_DEFAULT|wxICON_EXCLAMATION);
				int del = dialog.ShowModal();

				if(del == wxID_YES){ // user clicked yes
					// reconnect
					try{
						connection = mysock->connect(host, port);
					}catch(...){} // no code needed here due to boolean connection

					if(connection){
						mysock->recv(snd);
						mysock->send(pass);
						mysock->recv(snd);
					}else{
						wxMessageBox(wxT("Unknown Error while Authentication."), wxT("Authentification Error"));
						error_occured = true;
					}
				}else{
					snd = "99"; // user doesn't want to connect without encryption => own error code
					error_occured = true;
				}
			}

			if(snd.find("100") == 0 && connection){
				// nothing to do here if you reach this

			}else if(snd.find("102") == 0 && connection){
				wxMessageBox(wxT("Wrong Password, Authentification failed."), wxT("Authentification Error"));
				error_occured = true;

			}else if(snd.find("99") == 0 && connection){
				wxMessageBox(wxT("No connection established."), wxT("Authentification Error"));
				error_occured = true;

			}else{
				wxMessageBox(wxT("Unknown Error while Authentication."), wxT("Authentification Error"));
				error_occured = true;
			}
		}else{
			wxMessageBox(wxT("Unknown Error while Authentication."), wxT("Authentification Error"));
			error_occured = true;
		}


		if(!error_occured){

			// give socket and password to parent (myframe)
			myframe *myparent = (myframe *) GetParent();
			tkSock *frame_socket = myparent->get_connection_attributes();

			boost::mutex *mx = myparent->get_mutex();
			mx->lock();

			if(frame_socket != NULL){ //if there is already a connection, delete the old one
				delete frame_socket;
				frame_socket = NULL;
			}

			myparent->set_connection_attributes(mysock, pass);
			mx->unlock();
			myparent->update_status();

			Destroy();

		}else{
			delete mysock;
		}

	}else{ // connection failed due to host (IP/URL or port)
		wxMessageBox(wxT("\nConnection failed (wrong IP/URL or port).\t\t\nPlease try again."), wxT("Connection failed"));
		delete mysock;
	}
	// the dialog doesn't close with an error (so you don't have to type everything again), you have to get an connection or press cancel to do so
}

void connect_dialog::on_cancel(wxCommandEvent &event){
	Destroy();
}

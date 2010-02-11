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


connect_dialog::connect_dialog(wxString config_dir, wxWindow *parent) :
	wxDialog(parent, -1, wxString(wxEmptyString)){

	this->config_dir = config_dir;
	myframe *p = (myframe *)parent;

	// read saved data if it exists
	string file_name = string(config_dir.mb_str()) + "save.dat";
	ifstream ifs(file_name.c_str(), fstream::in | fstream::binary); // open file

	login_data last_data =  { "", 0, ""};
	if((ifs.rdstate() & ifstream::failbit) == 0){ // file successfully opened

		ifs.read((char *) &last_data,sizeof(login_data));
	}
	ifs.close();

	if(strcmp(last_data.host, "") == 0){ // data wasn't read from file => default
		snprintf(last_data.host, 256, "127.0.0.1");
		last_data.port = 56789;
		last_data.pass[0] = '\0';
		snprintf(last_data.lang, 128, "English");
	}

	if(last_data.lang[0] != '\0') // older versions of save.dat won't have lang, so we have to check
		p->set_language(last_data.lang); // set program language

	SetTitle(p->tsl("Connect to Host"));

	// create elements
	dialog_sizer = new wxBoxSizer(wxVERTICAL);
	text_sizer = new wxBoxSizer(wxHORIZONTAL);
	outer_input_sizer = new wxStaticBoxSizer(wxHORIZONTAL,this,p->tsl("Data"));
	input_sizer = new wxFlexGridSizer(5, 2, 10, 10);
	button_sizer = new wxBoxSizer(wxHORIZONTAL);

	message_text = new wxStaticText(this, -1, p->tsl("Please insert Host Data"));
	host_text = new wxStaticText(this, -1, p->tsl("IP/URL"));
	port_text = new wxStaticText(this, -1, p->tsl("Port"));
	pass_text = new wxStaticText(this, -1, p->tsl("Password"));
	lang_text = new wxStaticText(this, -1, p->tsl("Language"));

	host_input = new wxTextCtrl(this, -1, wxString(last_data.host, wxConvUTF8), wxDefaultPosition, wxSize(300, 25));
	host_input->SetSelection(-1, -1);
	host_input->SetFocus();
	port_input = new wxTextCtrl(this, -1, wxString::Format(wxT("%i"),last_data.port), wxDefaultPosition, wxSize(50, 25));
	pass_input = new wxTextCtrl(this, -1, wxString(last_data.pass, wxConvUTF8), wxDefaultPosition, wxSize(150, 25), wxTE_PASSWORD);

	// create wxChoice lang_choice
	wxArrayString lang_choice_text;
	int selection;
	lang_choice_text.Add(wxT("English"));
	lang_choice_text.Add(wxT("Deutsch"));

	if(strcmp(last_data.lang, "Deutsch") == 0)
		selection = 1;
	else // English is default
		selection = 0;

	lang_choice = new wxChoice(this, -1, wxDefaultPosition, wxSize(100, 30), lang_choice_text);
	lang_choice->SetSelection(selection);

	save_data_check = new wxCheckBox(this, -1, p->tsl("Save Data"));
	save_data_check->SetValue(true);

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
	input_sizer->Add(lang_text, 0, wxALIGN_LEFT);
	input_sizer->Add(lang_choice, 0, wxALIGN_LEFT);
	input_sizer->Add(save_data_check, 0, wxALIGN_LEFT);

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

	string lang("English");
	int selection = lang_choice->GetCurrentSelection();

	switch (selection){
		case 0: 	lang = "English";
					break;
		case 1: 	lang = "Deutsch";
					break;
		default:	break;
	}

	myframe *p = ((myframe *) GetParent());
	p->set_language(lang);

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
				wxMessageDialog dialog(this, p->tsl("Encrypted authentication not supported by server.")
										+ wxT("/n") + p->tsl("Do you want to try unsecure plain-text authentication?"),
										p->tsl("No Encryption"), wxYES_NO|wxYES_DEFAULT|wxICON_EXCLAMATION);
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
						wxMessageBox(p->tsl("Unknown Error while Authentication."), p->tsl("Authentification Error"));
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
				wxMessageBox(p->tsl("Wrong Password, Authentification failed."), p->tsl("Authentification Error"));
				error_occured = true;

			}else if(snd.find("99") == 0 && connection){
				wxMessageBox(p->tsl("No connection established."), p->tsl("Authentification Error"));
				error_occured = true;

			}else{
				wxMessageBox(p->tsl("Unknown Error while Authentication."), p->tsl("Authentification Error"));
				error_occured = true;
			}
		}else{
			wxMessageBox(p->tsl("Unknown Error while Authentication."), p->tsl("Authentification Error"));
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
			//p->set_language(lang);
			myparent->update_status(host_input->GetValue());

			// write login_data to a file
			if(save_data_check->GetValue()){ // user clicked checkbox to save data

				string file_name = string(config_dir.mb_str()) + "save.dat";
				ofstream ofs(file_name.c_str(), fstream::out | fstream::binary | fstream::trunc); // open file

				login_data last_data;
				snprintf(last_data.host, 256, "%s", host.c_str());
				last_data.port = port;
				snprintf(last_data.pass, 256, "%s", pass.c_str());
				snprintf(last_data.lang, 128, "%s", lang.c_str());

				if(ofs.good()){ // file successfully opened

					ofs.write((char *) &last_data, sizeof(login_data));
				}
				ofs.close();
			}

			Destroy();

		}else{
			delete mysock;
		}

	}else{ // connection failed due to host (IP/URL or port)
		wxMessageBox(wxT("\n") + p->tsl("Connection failed (wrong IP/URL or Port).") + wxT("\t\t\n")
					+ p->tsl("Please try again."), p->tsl("Connection failed"));
		delete mysock;
	}
	// the dialog doesn't close with an error (so you don't have to type everything again), you have to get an connection or press cancel to do so
}

void connect_dialog::on_cancel(wxCommandEvent &event){
	Destroy();
}

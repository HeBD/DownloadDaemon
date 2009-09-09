/***************************************************************
 * Name:	  ddclient-wx_add_dialog.cpp
 * Purpose:   Code for Add Dialog Class
 * Author:	ko ()
 * Created:   2009-08-18
 * Copyright: ko ()
 * License:
 **************************************************************/

#include "ddclient-wx_add_dialog.h"


// event table
BEGIN_EVENT_TABLE(add_dialog, wxDialog)
	EVT_BUTTON(wxID_ADD, add_dialog::on_add)
	EVT_BUTTON(wxID_CANCEL, add_dialog::on_cancel)
END_EVENT_TABLE()


add_dialog::add_dialog(wxWindow *parent) : wxDialog(parent, -1, wxString(wxT("Add Downloads"))){

	// create elements
	dialog_sizer = new wxBoxSizer(wxVERTICAL);
	outer_add_one_sizer = new wxStaticBoxSizer(wxVERTICAL,this,wxT("single Download"));
	outer_add_many_sizer = new wxStaticBoxSizer(wxVERTICAL,this,wxT("many Downloads"));
	inner_add_one_sizer = new wxFlexGridSizer(2, 2, 10, 10);
	button_sizer = new wxBoxSizer(wxHORIZONTAL);

	add_one_text = new wxStaticText(this, -1, wxT("Add single Download (no Title necessary)"));
	add_many_text = new wxStaticText(this, -1, wxT("\nAdd many Downloads (one per Line, no Title necessary)"));
	title_text = new wxStaticText(this, -1, wxT("Title"));
	url_text = new wxStaticText(this, -1, wxT("URL"));
	add_many_inner_text = new wxStaticText(this, -1, wxT("Separate URL and Title like this: http://something.aa/bb|a fancy Title"));

	title_input = new wxTextCtrl(this,-1,wxT(""), wxDefaultPosition, wxSize(410, 25));
	url_input = new wxTextCtrl(this,-1,wxT(""), wxDefaultPosition, wxSize(410, 25));
	url_input->SetFocus();
	many_input = new wxTextCtrl(this,-1,wxT(""), wxDefaultPosition, wxSize(410, 100), wxTE_MULTILINE);

	add_button = new wxButton(this, wxID_ADD);
	add_button->SetDefault();
	cancel_button = new wxButton(this, wxID_CANCEL);


	// filling sizers
	inner_add_one_sizer->Add(title_text, 0, wxALIGN_LEFT);
	inner_add_one_sizer->Add(title_input, 0, wxALIGN_LEFT);
	inner_add_one_sizer->Add(url_text, 0, wxALIGN_LEFT);
	inner_add_one_sizer->Add(url_input, 0, wxALIGN_LEFT);

	outer_add_one_sizer->Add(inner_add_one_sizer, 0,wxALL|wxEXPAND,10);

	outer_add_many_sizer->Add(add_many_inner_text, 0,wxALL|wxEXPAND,10);
	outer_add_many_sizer->Add(many_input, 0,wxALL|wxEXPAND,10);

	button_sizer->Add(add_button, 1, wxALL, 20);
	button_sizer->Add(cancel_button, 1, wxALL, 20);

	dialog_sizer->Add(add_one_text, 0, wxALL|wxGROW, 10);
	dialog_sizer->Add(outer_add_one_sizer, 0, wxALL|wxGROW, 10);
	dialog_sizer->Add(add_many_text, 0, wxALL|wxGROW, 10);
	dialog_sizer->Add(outer_add_many_sizer, 0, wxALL|wxGROW, 10);
	dialog_sizer->Add(button_sizer, 0, wxLEFT|wxRIGHT, 0);

	SetSizer(dialog_sizer);
	Layout();
	Fit();
}


// event handle methods
void add_dialog::on_add(wxCommandEvent &event){

	// extracting the data
	string title = string((title_input->GetValue()).mb_str());
	string url = string((url_input->GetValue()).mb_str());

	 // exchange every | inside the title with a blank
	size_t title_find;
	title_find = title.find("|");

	while(title_find != string::npos){
		title.at(title_find) = ' ';
		title_find = title.find("|");
	}

	// check connection
	myframe *myparent = (myframe *) GetParent();
	tkSock *mysock = myparent->get_connection_attributes();

	if(mysock == NULL || !*mysock || mysock->get_peer_name() == "") // if there is no active connection
		wxMessageBox(wxT("Please connect before adding Downloads."), wxT("No Connection to Server"));

	else{ // send data to server

		boost::mutex *mx = myparent->get_mutex();
		mx->lock();

		vector<string> splitted_line;
		string line, snd, answer, many;
		size_t lineend = 1, urlend;
		bool error_occured = false;

		if(!url.empty()){ // add single download

			mysock->send("DDP DL ADD " + url + " " + title);
			mysock->recv(answer);

			if(answer.find("103") == 0) // 103 URL <-- Invalid URL
				error_occured = true;
		}

		// add many downloads
		many = string((many_input->GetValue()).mb_str());

		// parse lines
		while(many.length() > 0 && lineend != string::npos){
			lineend = many.find("\n"); // termination character for line

			if(lineend == string::npos){ // this is the last line (which ends without \n)
				line = many.substr(0, many.length());
				many = "";

			}else{ // there is still another line after this one
				line = many.substr(0, lineend);
				many = many.substr(lineend+1);
			}

			// parse url and title
			urlend = line.find("|");
			while(urlend != string::npos){ // exchange every | with a blank (there can be some | inside the title too)
				line.at(urlend) = ' ';
				urlend = line.find("|");
			}

			// send a single download
			mysock->send("DDP DL ADD " + line);
			mysock->recv(answer);

			if(answer.find("103") == 0) // 103 URL <-- Invalid URL
				error_occured = true;
		}

		mx->unlock();

		if(error_occured)
			wxMessageBox(wxT("At least one inserted URL was invalid."), wxT("Invalid URL"));
	}

	Destroy();
}


void add_dialog::on_cancel(wxCommandEvent &event){
	Destroy();
}

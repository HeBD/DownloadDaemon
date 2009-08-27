/***************************************************************
 * Name:	  ddclient-wx_add_dialog.cpp
 * Purpose:   Code for Add Dialog Class
 * Author:	ko ()
 * Created:   2009-08-18
 * Copyright: ko ()
 * License:
 **************************************************************/

#include "ddclient-wx_add_dialog.h"


// IDs
const long add_dialog::id_add_one = wxNewId();
const long add_dialog::id_add_many = wxNewId();


// event table
BEGIN_EVENT_TABLE(add_dialog, wxDialog)
	EVT_BUTTON(id_add_one, add_dialog::on_add_one)
	EVT_BUTTON(id_add_many, add_dialog::on_add_many)
	EVT_BUTTON(wxID_CANCEL, add_dialog::on_cancel)
END_EVENT_TABLE()


add_dialog::add_dialog(wxWindow *parent) : wxDialog(parent, -1, wxString(wxT("Add Downloads"))){

	// create elements
	dialog_sizer = new wxBoxSizer(wxVERTICAL);
	outer_add_one_sizer = new wxStaticBoxSizer(wxVERTICAL,this,wxT("single Download"));
	outer_add_many_sizer = new wxStaticBoxSizer(wxVERTICAL,this,wxT("many Downloads"));
	inner_add_one_sizer = new wxFlexGridSizer(2, 2, 10, 10);

	add_one_text = new wxStaticText(this, -1, wxT("Add single Download (no Title necessary)"));
	add_many_text = new wxStaticText(this, -1, wxT("\nAdd many Downloads (one per Line, no Title necessary)"));
	title_text = new wxStaticText(this, -1, wxT("Title"));
	url_text = new wxStaticText(this, -1, wxT("URL"));
	add_many_inner_text = new wxStaticText(this, -1, wxT("Separate URL and Title like this: http://something.aa/bb|a fancy Title"));

	title_input = new wxTextCtrl(this,-1,wxT(""), wxDefaultPosition, wxSize(410, 25));
	url_input = new wxTextCtrl(this,-1,wxT(""), wxDefaultPosition, wxSize(410, 25));
	url_input->SetFocus();
	many_input = new wxTextCtrl(this,-1,wxT(""), wxDefaultPosition, wxSize(410, 100), wxTE_MULTILINE);

	add_one_button = new wxButton(this, id_add_one, wxT("Add one"));
	add_one_button->SetDefault();
	add_many_button = new wxButton(this, id_add_many, wxT("Add many"));
	cancel_button = new wxButton(this, wxID_CANCEL, wxT("Cancel"));


	// filling sizers
	inner_add_one_sizer->Add(title_text, 0, wxALIGN_LEFT);
	inner_add_one_sizer->Add(title_input, 0, wxALIGN_LEFT);
	inner_add_one_sizer->Add(url_text, 0, wxALIGN_LEFT);
	inner_add_one_sizer->Add(url_input, 0, wxALIGN_LEFT);

	outer_add_one_sizer->Add(inner_add_one_sizer, 0,wxALL|wxEXPAND,10);
	outer_add_one_sizer->Add(add_one_button, 0,wxALL,10);

	outer_add_many_sizer->Add(add_many_inner_text, 0,wxALL|wxEXPAND,10);
	outer_add_many_sizer->Add(many_input, 0,wxALL|wxEXPAND,10);
	outer_add_many_sizer->Add(add_many_button, 0,wxALL,10);

	dialog_sizer->Add(add_one_text, 0, wxALL|wxGROW, 10);
	dialog_sizer->Add(outer_add_one_sizer, 0, wxALL|wxGROW, 10);
	dialog_sizer->Add(add_many_text, 0, wxALL|wxGROW, 10);
	dialog_sizer->Add(outer_add_many_sizer, 0, wxALL|wxGROW, 10);
	dialog_sizer->Add(cancel_button, 0, wxALL, 10);


	SetSizer(dialog_sizer);
	Layout();
	Fit();
}


// event handle methods
void add_dialog::on_add_one(wxCommandEvent &event){

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

		string answer;

		mysock->send("DDP DL ADD " + url + " " + title);
		mysock->recv(answer);

		if(answer.find("103") == 0) // 103 URL <-- Invalid URL
			wxMessageBox(wxT("The inserted URL was invalid."), wxT("Invalid URL"));

		mx->unlock();
	}

	Destroy();
}


void add_dialog::on_add_many(wxCommandEvent &event){

	// check connection
	myframe *myparent = (myframe *) GetParent();
	tkSock *mysock = myparent->get_connection_attributes();

	if(mysock == NULL || !*mysock || mysock->get_peer_name() == ""){ // if there is no active connection
		wxMessageBox(wxT("Please connect before adding Downloads."), wxT("No Connection to Server"));

	}else{ // we have a connection

		boost::mutex *mx = myparent->get_mutex();
		mx->lock();

		string many = string((many_input->GetValue()).mb_str());

		vector<string> splitted_line;
		string line, snd, answer;
		size_t lineend = 1, urlend;
		bool error_occured = false;

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

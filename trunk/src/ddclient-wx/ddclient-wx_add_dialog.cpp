/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "ddclient-wx_add_dialog.h"
#include "ddclient-wx_main.h"
#include <wx/msgdlg.h> // for wxmessagebox


// event table
BEGIN_EVENT_TABLE(add_dialog, wxDialog)
	EVT_BUTTON(wxID_ADD, add_dialog::on_add)
	EVT_BUTTON(wxID_CANCEL, add_dialog::on_cancel)
END_EVENT_TABLE()


add_dialog::add_dialog(wxWindow *parent):
	wxDialog(parent, -1, wxString(wxEmptyString)){
	myframe *p = (myframe *)parent;
	SetTitle(p->tsl("Add Downloads"));

	// create elements
	dialog_sizer = new wxBoxSizer(wxVERTICAL);
	outer_add_one_sizer = new wxStaticBoxSizer(wxVERTICAL,this,p->tsl("single Download"));
	outer_add_many_sizer = new wxStaticBoxSizer(wxVERTICAL,this,p->tsl("many Downloads"));
	inner_add_one_sizer = new wxFlexGridSizer(3, 2, 10, 10);
	inner_add_many_sizer = new wxFlexGridSizer(1, 2, 10, 10);
	button_sizer = new wxBoxSizer(wxHORIZONTAL);

	add_one_text = new wxStaticText(this, -1, p->tsl("Add single Download (no Title necessary)"));
	add_many_text = new wxStaticText(this, -1, wxT("\n") + p->tsl("Add many Downloads (one per Line, no Title necessary)"));
	title_text = new wxStaticText(this, -1, p->tsl("Title"));
	url_text = new wxStaticText(this, -1, p->tsl("URL"));
	package_text = new wxStaticText(this, -1, p->tsl("Package"));
	add_many_inner_text = new wxStaticText(this, -1, p->tsl("Separate URL and Title like this: http://something.aa/bb|a fancy Title"));
	package_text_many = new wxStaticText(this, -1, p->tsl("Package"));

	package_input = new wxTextCtrl(this,-1, wxT("0"), wxDefaultPosition, wxSize(50, 25));
	title_input = new wxTextCtrl(this,-1, wxEmptyString, wxDefaultPosition, wxSize(410, 25));
	url_input = new wxTextCtrl(this,-1, wxEmptyString, wxDefaultPosition, wxSize(410, 25));
	url_input->SetFocus();
	package_input_many = new wxTextCtrl(this,-1, wxT("0"), wxDefaultPosition, wxSize(50, 25));
	many_input = new wxTextCtrl(this,-1, wxEmptyString, wxDefaultPosition, wxSize(410, 100), wxTE_MULTILINE);

	add_button = new wxButton(this, wxID_ADD);
	add_button->SetDefault();
	cancel_button = new wxButton(this, wxID_CANCEL);


	// filling sizers
	inner_add_one_sizer->Add(package_text, 0, wxALIGN_LEFT);
	inner_add_one_sizer->Add(package_input, 0, wxALIGN_LEFT);
	inner_add_one_sizer->Add(title_text, 0, wxALIGN_LEFT);
	inner_add_one_sizer->Add(title_input, 0, wxALIGN_LEFT);
	inner_add_one_sizer->Add(url_text, 0, wxALIGN_LEFT);
	inner_add_one_sizer->Add(url_input, 0, wxALIGN_LEFT);

	outer_add_one_sizer->Add(inner_add_one_sizer, 0, wxALL|wxEXPAND, 10);

	inner_add_many_sizer->Add(package_text_many, 0, wxALIGN_LEFT, 10);
	inner_add_many_sizer->Add(package_input_many, 0, wxALIGN_LEFT);

	outer_add_many_sizer->Add(add_many_inner_text, 0, wxALL|wxEXPAND, 10);
	outer_add_many_sizer->Add(inner_add_many_sizer, 0, wxLEFT|wxEXPAND, 10);
	outer_add_many_sizer->Add(many_input, 0, wxALL|wxEXPAND, 10);

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
	myframe *p = (myframe *)GetParent();
	downloadc *dclient = p->get_connection();

	// extracting the data
	string title = string((title_input->GetValue()).mb_str());
	string url = string((url_input->GetValue()).mb_str());
	int package = wxAtoi(package_input->GetValue());
	int package_many = wxAtoi(package_input_many->GetValue());

	 // exchange every | inside the title with a blank
	size_t title_find;
	title_find = title.find("|");

	while(title_find != string::npos){
		title.at(title_find) = ' ';
		title_find = title.find("|");
	}


	bool error_occured = false;
	int error = 0;
	vector<string> splitted_line;
	string line, many;
	size_t lineend = 1, urlend;

	// add single download
	boost::mutex *mx = p->get_mutex();
	mx->lock();
	if(!url.empty()){
		try{
			dclient->add_download(package, url, title);
		}catch(client_exception &e){
			error_occured = true;
			error = e.get_id();
		}
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

		if(urlend != string::npos){ // we have a title
			url = line.substr(0, urlend);
			line = line.substr(urlend+1);

			urlend = line.find("|");
			while(urlend != string::npos){ // exchange every | with a blank (there can be some | inside the title too)
				line.at(urlend) = ' ';
				urlend = line.find("|");
			}
			title = line;

		}else{ // no title
			url = line;
			title = "";
		}

		// send a single download
		try{
			dclient->add_download(package_many, url, title);
		}catch(client_exception &e){
			error_occured = true;
			error = e.get_id();
		}
	}

	if(error_occured){
		if(error == 6)
			wxMessageBox(p->tsl("Failed to create Package."), p->tsl("Error"));
		else if(error == 13)
			wxMessageBox(p->tsl("At least one inserted URL was invalid."), p->tsl("Invalid URL"));
	}
	mx->unlock();
	Destroy();
}


void add_dialog::on_cancel(wxCommandEvent &event){
	Destroy();
}

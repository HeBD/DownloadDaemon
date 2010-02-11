/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "ddclient-wx_delete_dialog.h"


// IDs
const long delete_dialog::id_yes_all = wxNewId();
const long delete_dialog::id_no_all = wxNewId();


// event table
BEGIN_EVENT_TABLE(delete_dialog, wxDialog)
	EVT_BUTTON(id_yes_all, delete_dialog::on_yes_all)
	EVT_BUTTON(wxID_YES, delete_dialog::on_yes)
	EVT_BUTTON(wxID_NO, delete_dialog::on_no)
	EVT_BUTTON(id_no_all, delete_dialog::on_no_all)
END_EVENT_TABLE()


delete_dialog::delete_dialog(int *answer, std::string id, wxWindow *parent):
	wxDialog(parent, -1, wxString(wxEmptyString)){
	myframe *p = (myframe *)parent;
	SetTitle(p->tsl("Delete File"));

	this->answer = answer;

	// creating elements
	overall_sizer = new wxBoxSizer(wxVERTICAL);
	button_sizer = new wxBoxSizer(wxHORIZONTAL);


	question_text = new wxStaticText(this, -1, p->tsl("Do you want to delete the downloaded File for Download ID")
									+ wxT(" ") +  wxString(id.c_str(), wxConvUTF8) + wxT("?"));

	yes_all_button = new wxButton(this, id_yes_all, p->tsl("Always yes"));
	yes_button = new wxButton(this, wxID_YES);
	yes_button->SetDefault();
	no_button = new wxButton(this, wxID_NO);
	no_all_button = new wxButton(this, id_no_all, p->tsl("Always no"));


	// filling sizers
	button_sizer->Add(yes_all_button, 1, wxALL, 20);
	button_sizer->Add(yes_button, 1, wxALL, 20);
	button_sizer->Add(no_button, 1, wxALL, 20);
	button_sizer->Add(no_all_button, 1, wxALL, 20);

	overall_sizer->Add(question_text, 0, wxALL|wxGROW|wxALIGN_LEFT, 10);
	overall_sizer->Add(button_sizer, 0, wxLEFT|wxRIGHT, 0);

	SetSizer(overall_sizer);
	Layout();
	Fit();
}


void delete_dialog::on_yes_all(wxCommandEvent &event){
	*answer = 0;
	Destroy();
}


void delete_dialog::on_yes(wxCommandEvent &event){
	*answer = 1;
	Destroy();
}


void delete_dialog::on_no(wxCommandEvent &event){
	*answer = 2;
	Destroy();
}


void delete_dialog::on_no_all(wxCommandEvent &event){
	*answer = 3;
	Destroy();
}

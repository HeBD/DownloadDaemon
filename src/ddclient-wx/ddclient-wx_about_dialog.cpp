/***************************************************************
 * Name:      ddclient-wx_about_dialog.cpp
 * Purpose:   Code for About Dialog Class
 * Author:    ko ()
 * Created:   2009-08-09
 * Copyright: ko ()
 * License:
 **************************************************************/

#include "ddclient-wx_about_dialog.h"


// event table
BEGIN_EVENT_TABLE(about_dialog, wxDialog)
	EVT_BUTTON(wxID_OK, about_dialog::on_ok)
END_EVENT_TABLE()


about_dialog::about_dialog(wxString working_dir, wxWindow *parent) : wxDialog(parent, -1, wxT("About...")){

	this->working_dir = working_dir;

	// creating elements
	overall_sizer = new wxBoxSizer(wxVERTICAL);
	info_sizer = new wxBoxSizer(wxHORIZONTAL);
	text_sizer = new wxBoxSizer(wxVERTICAL);

	name_text = new wxStaticText(this, -1, wxT("DownloadDaemon-ClientWX"));
	build_text = new wxStaticText(this, -1, wxT("(insert build info)"));
	website_text = new wxStaticText(this, -1, wxT("(create link) \nhttp://board.gulli.com/thread/1419174-downloaddaemon\n---och-downloader-fr-server-nas-o/"));

	ok_button = new wxButton(this, wxID_OK, wxT("Ok"));
	ok_button->SetDefault();

	wxString url = working_dir + wxT("ddclient-wx_data/img/teufelchen_about.png");
	pic = new picture(this, -1, wxPoint(0,0), wxSize(150,150), 0, _T(""), url);


	// filling sizers
	text_sizer->Add(name_text,0,wxALL|wxEXPAND,10);
	text_sizer->Add(build_text,0,wxALL|wxEXPAND,10);
	text_sizer->Add(website_text,0,wxALL|wxEXPAND,10);

	info_sizer->Add(pic, 0, wxALL, 20);
	info_sizer->Add(text_sizer, 0, wxALL, 20);

	overall_sizer->Add(info_sizer, 0, wxALL, 20);
	overall_sizer->Add(ok_button, 0, wxALIGN_CENTER);


	SetSizer(overall_sizer);
	Layout();
	Fit();

	return;
}

void about_dialog::on_ok(wxCommandEvent &event){
	Destroy();
}



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


//helper function to generate build info
wxString wxbuildinfo(){
	wxString wxbuild (wxString(wxT("Build: ")));

	wxbuild << wxT(__DATE__) << wxT(", ") << wxT(__TIME__) << wxT("\n(") << wxVERSION_STRING;

	#if defined(__WXMSW__)
		wxbuild << wxT("-Windows");
	#elif defined(__UNIX__)
		wxbuild << wxT("-Linux");
	#endif // defined(__WXMSW__)

	#if wxUSE_UNICODE
		wxbuild << wxT("-unicode)");
	#else
		wxbuild << wxT("-ANSI)");
	#endif // wxUSE_UNICODE

    return wxbuild;
}


about_dialog::about_dialog(wxString working_dir, wxWindow *parent) : wxDialog(parent, -1, wxString(wxT("About..."))){

	this->working_dir = working_dir;

	// creating elements
	overall_sizer = new wxBoxSizer(wxVERTICAL);
	info_sizer = new wxBoxSizer(wxHORIZONTAL);
	text_sizer = new wxBoxSizer(wxVERTICAL);

	name_text = new wxStaticText(this, -1, wxT("DownloadDaemon-ClientWX"));
	build_text = new wxStaticText(this, -1, wxbuildinfo());
	website_text = new wxHyperlinkCtrl(this, -1, wxT("Project Website"), wxT("http://board.gulli.com/thread/1419174-downloaddaemon---och-downloader-fr-server-nas-o"), wxDefaultPosition, wxDefaultSize, wxBORDER_NONE|wxHL_ALIGN_LEFT|wxHL_CONTEXTMENU);
	website_text->SetNormalColour(wxT("NAVY"));
	website_text->SetHoverColour(wxT("MEDIUM SLATE BLUE"));

	ok_button = new wxButton(this, wxID_OK);
	ok_button->SetDefault();

	wxString url = working_dir + wxT("img/teufelchen_about.png");
	pic = new picture(url, this, wxSize(150,150));


	// filling sizers
	text_sizer->Add(name_text,0,wxALL|wxEXPAND,10);
	text_sizer->Add(build_text,0,wxALL|wxEXPAND,10);
	text_sizer->Add(website_text,0,wxALL|wxEXPAND,10);

	info_sizer->Add(pic, 0, wxALL, 0);
	info_sizer->Add(text_sizer, 0, wxALL, 0);

	overall_sizer->Add(info_sizer, 0, wxALL, 0);
	overall_sizer->Add(ok_button, 0, wxALIGN_CENTER);


	SetSizer(overall_sizer);
	Layout();
	Fit();
}


void about_dialog::on_ok(wxCommandEvent &event){
	Destroy();
}

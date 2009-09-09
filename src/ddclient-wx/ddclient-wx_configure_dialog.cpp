/***************************************************************
 * Name:	  ddclient-wx_configure_dialog.cpp
 * Purpose:   Code for Configure Dialog Class
 * Author:	ko ()
 * Created:   2009-08-26
 * Copyright: ko ()
 * License:
 **************************************************************/

#include "ddclient-wx_configure_dialog.h"


// event table
BEGIN_EVENT_TABLE(configure_dialog, wxDialog)
	EVT_BUTTON(wxID_APPLY, configure_dialog::on_apply)
	EVT_BUTTON(wxID_SAVE, configure_dialog::on_pass_change)
	EVT_BUTTON(wxID_CANCEL, configure_dialog::on_cancel)
END_EVENT_TABLE()


configure_dialog::configure_dialog(wxWindow *parent) : wxDialog(parent, -1, wxString(wxT("Configure DownloadDaemon Server"))){

	notebook = new wxNotebook(this, -1, wxDefaultPosition, wxSize(300, 200));

	create_download_panel();
	create_pass_panel();
	create_log_panel();

	notebook->AddPage(download_panel, wxT("Download"), true);
	notebook->AddPage(pass_panel, wxT("Password"), false);
	notebook->AddPage(log_panel, wxT("Logging"), false);

	notebook->Fit();
	Layout();
	Fit();
	CenterOnScreen();
}


void configure_dialog::create_download_panel(){
	// creating elements
	download_panel = new wxPanel(notebook, -1, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);

	overall_download_sizer = new wxBoxSizer(wxVERTICAL);
	outer_time_sizer = new wxStaticBoxSizer(wxHORIZONTAL,download_panel,wxT("Download Time"));
	outer_save_dir_sizer = new wxStaticBoxSizer(wxHORIZONTAL,download_panel,wxT("Download Folder"));
	outer_count_sizer = new wxStaticBoxSizer(wxHORIZONTAL,download_panel,wxT("Download Count"));
	time_sizer = new wxFlexGridSizer(2, 2, 10, 10);
	save_dir_sizer = new wxFlexGridSizer(1, 2, 10, 10);
	count_sizer = new wxFlexGridSizer(1, 2, 10, 10);
	download_button_sizer = new wxBoxSizer(wxHORIZONTAL);

	exp_time_text = new wxStaticText(download_panel, -1, wxT("You can force DownloadDaemon to only download at specific times by entering a start and\nend time in the format hours:minutes.\nLeave these fields empty if you want to allow DownloadDaemon to download permanently."));
	start_time_text = new wxStaticText(download_panel, -1, wxT("Start Time"));
	end_time_text = new wxStaticText(download_panel, -1, wxT("End Time"));
	exp_save_dir_text = new wxStaticText(download_panel, -1, wxT("This option specifies where finished downloads should be safed on the server."));
	save_dir_text = new wxStaticText(download_panel, -1, wxT("Download Folder"));
	exp_count_text = new wxStaticText(download_panel, -1, wxT("Here you can specify how many downloads may run at the same time."));
	count_text = new wxStaticText(download_panel, -1, wxT("Simultaneous downloads"));

	start_time_input = new wxTextCtrl(download_panel,-1, get_var("download_timing_start"), wxDefaultPosition, wxSize(100, 25));
	start_time_input->SetFocus();
	end_time_input = new wxTextCtrl(download_panel, -1, get_var("download_timing_end"), wxDefaultPosition, wxSize(100, 25));
	save_dir_input = new wxTextCtrl(download_panel, -1, get_var("download_folder"), wxDefaultPosition, wxSize(400, 25));
	count_input = new wxTextCtrl(download_panel, -1, get_var("simultaneous_downloads"), wxDefaultPosition, wxSize(100, 25));

//	download_button = new wxButton(download_panel, id_download_change, wxT("Apply"));
	download_button = new wxButton(download_panel, wxID_APPLY);
	download_cancel_button = new wxButton(download_panel, wxID_CANCEL);

	// filling sizers
	time_sizer->Add(start_time_text, 0, wxALIGN_LEFT);
	time_sizer->Add(start_time_input, 0, wxALIGN_LEFT);
	time_sizer->Add(end_time_text, 0, wxALIGN_LEFT);
	time_sizer->Add(end_time_input, 0, wxALIGN_LEFT);

	save_dir_sizer->Add(save_dir_text, 0, wxALIGN_LEFT);
	save_dir_sizer->Add(save_dir_input, 0, wxALIGN_LEFT);

	count_sizer->Add(count_text, 0, wxALIGN_LEFT);
	count_sizer->Add(count_input, 0, wxALIGN_LEFT);

	download_button_sizer->Add(download_button, 1, wxLEFT|wxRIGHT, 20);
	download_button_sizer->Add(download_cancel_button, 1, wxLEFT|wxRIGHT, 10);

	outer_time_sizer->Add(time_sizer, 0,wxALL|wxEXPAND,10);
	outer_save_dir_sizer->Add(save_dir_sizer, 0,wxALL|wxEXPAND,10);
	outer_count_sizer->Add(count_sizer, 0,wxALL|wxEXPAND,10);

	overall_download_sizer->Add(exp_time_text, 0, wxGROW|wxLEFT|wxRIGHT|wxTOP, 20);
	overall_download_sizer->Add(outer_time_sizer, 0, wxGROW|wxALL, 20);
	overall_download_sizer->Add(exp_save_dir_text, 0, wxGROW|wxLEFT|wxRIGHT|wxTOP, 20);
	overall_download_sizer->Add(outer_save_dir_sizer, 0, wxGROW|wxALL, 20);
	overall_download_sizer->Add(exp_count_text, 0, wxGROW|wxLEFT|wxRIGHT|wxTOP, 20);
	overall_download_sizer->Add(outer_count_sizer, 0, wxGROW|wxALL, 20);
	overall_download_sizer->Add(download_button_sizer, 0, wxLEFT|wxRIGHT, 0);

	download_panel->SetSizerAndFit(overall_download_sizer);
}


void configure_dialog::create_pass_panel(){
	// creating elements
	pass_panel = new wxPanel(notebook, -1, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);

	overall_pass_sizer = new wxBoxSizer(wxVERTICAL);
	outer_pass_sizer = new wxStaticBoxSizer(wxHORIZONTAL,pass_panel,wxT("Change Password"));
	pass_sizer = new wxFlexGridSizer(3, 2, 10, 10);
	pass_button_sizer = new wxBoxSizer(wxHORIZONTAL);

	old_pass_text = new wxStaticText(pass_panel, -1, wxT("Old Password"));
	new_pass_text = new wxStaticText(pass_panel, -1, wxT("New Password"));

	old_pass_input = new wxTextCtrl(pass_panel, -1, wxT(""), wxDefaultPosition, wxSize(200, 25), wxTE_PASSWORD);
	new_pass_input = new wxTextCtrl(pass_panel, -1, wxT(""), wxDefaultPosition, wxSize(200, 25), wxTE_PASSWORD);

	pass_button = new wxButton(pass_panel, wxID_SAVE);
	pass_cancel_button = new wxButton(pass_panel, wxID_CANCEL);

	// filling sizers
	pass_sizer->Add(old_pass_text, 0, wxALIGN_LEFT);
	pass_sizer->Add(old_pass_input, 0, wxALIGN_LEFT);
	pass_sizer->Add(new_pass_text, 0, wxALIGN_LEFT);
	pass_sizer->Add(new_pass_input, 0, wxALIGN_LEFT);
	pass_sizer->Add(pass_button, 0, wxALIGN_LEFT);

	pass_button_sizer->Add(pass_cancel_button, 1, wxLEFT|wxRIGHT, 20);

	outer_pass_sizer->Add(pass_sizer, 0,wxALL|wxEXPAND,10);
	overall_pass_sizer->Add(outer_pass_sizer, 0, wxGROW|wxALL, 20);
	overall_pass_sizer->Add(pass_button_sizer, 0, wxALL);

	pass_panel->SetSizerAndFit(overall_pass_sizer);
}


void configure_dialog::create_log_panel(){
	// creating elements
	log_panel = new wxPanel(notebook, -1, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);

	overall_log_sizer = new wxBoxSizer(wxVERTICAL);
	outer_log_activity_sizer = new wxStaticBoxSizer(wxVERTICAL,log_panel,wxT("Logging Activity"));
	outer_log_output_sizer = new wxStaticBoxSizer(wxHORIZONTAL,log_panel,wxT("Log Output"));
	log_output_sizer = new wxFlexGridSizer(1, 2, 10, 10);
	log_button_sizer = new wxBoxSizer(wxHORIZONTAL);

	log_activity_text = new wxStaticText(log_panel, -1, wxT("This option specifies DownloadDaemons logging activity."));
	exp_log_output_text = new wxStaticText(log_panel, -1, wxT("This option specifies the destination for log-output and can either be a filename, stderr\nor stdout for local console output"));
	log_output_text = new wxStaticText(log_panel, -1, wxT("Log File"));

	log_output_input = new wxTextCtrl(log_panel, -1, get_var("log_file"), wxDefaultPosition, wxSize(400, 25));

	// wxChoice log_activity
	wxString old_activity = get_var("log_level");
	int selection = 0;

	if(old_activity == wxT("DEBUG"))
		selection = 0;
	else if(old_activity == wxT("WARNING"))
		selection = 1;
	else if(old_activity == wxT("SEVERE"))
		selection = 2;
	else if(old_activity == wxT("OFF"))
		selection = 3;

	wxArrayString activity;
	activity.Add(wxT("Debug"));
	activity.Add(wxT("Warning"));
	activity.Add(wxT("Severe"));
	activity.Add(wxT("Off"));
	log_activity_choice = new wxChoice(log_panel, -1, wxDefaultPosition, wxSize(100, 30), activity);
	log_activity_choice->SetSelection(selection);

	log_button = new wxButton(log_panel, wxID_APPLY);
	log_cancel_button = new wxButton(log_panel, wxID_CANCEL);

	// filling sizers
	log_output_sizer->Add(log_output_text, 0, wxALIGN_LEFT);
	log_output_sizer->Add(log_output_input, 0, wxALIGN_LEFT);

	outer_log_output_sizer->Add(log_output_sizer, 0,wxALL|wxEXPAND,10);
	outer_log_activity_sizer->Add(log_activity_text, 0,wxALL|wxEXPAND,10);
	outer_log_activity_sizer->Add(log_activity_choice, 0,wxALL,10);

	log_button_sizer->Add(log_button, 1, wxLEFT|wxRIGHT, 20);
	log_button_sizer->Add(log_cancel_button, 1, wxLEFT|wxRIGHT, 10);

	overall_log_sizer->Add(exp_log_output_text, 0, wxGROW|wxLEFT|wxRIGHT|wxTOP, 20);
	overall_log_sizer->Add(outer_log_output_sizer, 0, wxGROW|wxALL, 20);
	overall_log_sizer->Add(outer_log_activity_sizer, 0, wxGROW|wxALL, 20);
	overall_log_sizer->Add(log_button_sizer, 0, wxALL);

	log_panel->SetSizerAndFit(overall_log_sizer);
}


wxString configure_dialog::get_var(const string &var){
	string answer;

	// check connection
	myframe *myparent = (myframe *) GetParent();
	tkSock *mysock = myparent->get_connection_attributes();

	if(mysock == NULL || !*mysock || mysock->get_peer_name() == ""){ // if there is no active connection
		wxMessageBox(wxT("Please connect before configurating the DownloadDaemon Server."), wxT("No Connection to Server"));

	}else{ // we have a connection
		boost::mutex *mx = myparent->get_mutex();
		mx->lock();

		mysock->send("DDP VAR GET " + var);
		mysock->recv(answer);

		mx->unlock();
	}

	return wxString(answer.c_str(), wxConvUTF8);
}


// event handle methods
void configure_dialog::on_apply(wxCommandEvent &event){

	// getting user input
	string start_time = string(start_time_input->GetValue().mb_str());
	string end_time = string(end_time_input->GetValue().mb_str());
	string save_dir = string(save_dir_input->GetValue().mb_str());
	string count = string(count_input->GetValue().mb_str());

	string log_output = string(log_output_input->GetValue().mb_str());
	string activity_level;
	int selection = log_activity_choice->GetCurrentSelection();

	switch (selection){
		case 0: 	activity_level = "DEBUG";
					break;
		case 1: 	activity_level = "WARNING";
					break;
		case 2: 	activity_level = "SEVERE";
					break;
		case 3: 	activity_level = "OFF";
					break;
	}


	// check connection
	myframe *myparent = (myframe *) GetParent();
	tkSock *mysock = myparent->get_connection_attributes();

	if(mysock == NULL || !*mysock || mysock->get_peer_name() == ""){ // if there is no active connection
		wxMessageBox(wxT("Please connect before configurating the DownloadDaemon Server."), wxT("No Connection to Server"));

	}else{ // we have a connection
		string answer;

		boost::mutex *mx = myparent->get_mutex();
		mx->lock();

		// download times
		mysock->send("DDP VAR SET download_timing_start = " + start_time);
		mysock->recv(answer);

		mysock->send("DDP VAR SET download_timing_end = " + end_time);
		mysock->recv(answer);

		// download folder
		mysock->send("DDP VAR SET download_folder = " + save_dir);
		mysock->recv(answer);

		// download count
		mysock->send("DDP VAR SET simultaneous_downloads = " + count);
		mysock->recv(answer);

		// log output
		mysock->send("DDP VAR SET log_file = " + log_output);
		mysock->recv(answer);

		if(answer.find("102") == 0) // 102 AUTHENTICATION	<-- Authentication failed
			wxMessageBox(wxT("Failed to change Log Output."), wxT("Password Error"));

		// log activity
		mysock->send("DDP VAR SET log_level = " + activity_level);
		mysock->recv(answer);

		mx->unlock();
	}

	Destroy();
}


void configure_dialog::on_pass_change(wxCommandEvent &event){

	// getting user input
	string old_pass = string(old_pass_input->GetValue().mb_str());
	string new_pass = string(new_pass_input->GetValue().mb_str());

	// check connection
	myframe *myparent = (myframe *) GetParent();
	tkSock *mysock = myparent->get_connection_attributes();

	if(mysock == NULL || !*mysock || mysock->get_peer_name() == ""){ // if there is no active connection
		wxMessageBox(wxT("Please connect before configurating the DownloadDaemon Server."), wxT("No Connection to Server"));

	}else{ // we have a connection
		string answer;

		boost::mutex *mx = myparent->get_mutex();
		mx->lock();

		// password
		mysock->send("DDP VAR SET mgmt_password = " + old_pass + " ; " + new_pass);
		mysock->recv(answer);

		if(answer.find("102") == 0) // 102 AUTHENTICATION	<-- Authentication failed
			wxMessageBox(wxT("Failed to reset the Password."), wxT("Password Error"));

		mx->unlock();
	}

	Destroy();
}


void configure_dialog::on_cancel(wxCommandEvent &event){
	Destroy();
}

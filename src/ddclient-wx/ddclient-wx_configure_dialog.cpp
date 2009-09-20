/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "ddclient-wx_configure_dialog.h"


// event table
BEGIN_EVENT_TABLE(configure_dialog, wxDialog)
	EVT_BUTTON(wxID_APPLY, configure_dialog::on_apply)
	EVT_BUTTON(wxID_SAVE, configure_dialog::on_pass_change)
	EVT_BUTTON(wxID_CANCEL, configure_dialog::on_cancel)
	EVT_CHECKBOX(wxID_ANY, configure_dialog::on_checkbox_change)
END_EVENT_TABLE()


configure_dialog::configure_dialog(wxWindow *parent) : wxDialog(parent, -1, wxString(wxT("Configure DownloadDaemon Server"))){

	notebook = new wxNotebook(this, -1);

	create_download_panel();
	create_pass_panel();
	create_log_panel();
	create_reconnect_panel();

	notebook->AddPage(download_panel, wxT("Download"), true);
	notebook->AddPage(pass_panel, wxT("Password"), false);
	notebook->AddPage(log_panel, wxT("Logging"), false);
	notebook->AddPage(reconnect_panel, wxT("Reconnect"), false);

	Layout();
	notebook->Fit();
	Fit();
	CenterOnScreen();
}


void configure_dialog::create_download_panel(){
	// creating elements
	download_panel = new wxPanel(notebook, -1, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);

	overall_download_sizer = new wxBoxSizer(wxVERTICAL);
	outer_time_sizer = new wxStaticBoxSizer(wxHORIZONTAL, download_panel, wxT("Download Time"));
	outer_save_dir_sizer = new wxStaticBoxSizer(wxHORIZONTAL, download_panel, wxT("Download Folder"));
	outer_count_sizer = new wxStaticBoxSizer(wxHORIZONTAL, download_panel, wxT("Download Count"));
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
	outer_pass_sizer = new wxStaticBoxSizer(wxHORIZONTAL, pass_panel, wxT("Change Password"));
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
	outer_log_activity_sizer = new wxStaticBoxSizer(wxVERTICAL, log_panel, wxT("Logging Activity"));
	outer_log_output_sizer = new wxStaticBoxSizer(wxHORIZONTAL, log_panel, wxT("Log Output"));
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


void configure_dialog::create_reconnect_panel(){
	// creating elements
	reconnect_panel = new wxPanel(notebook, -1, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);

	overall_reconnect_sizer = new wxBoxSizer(wxVERTICAL);
	outer_policy_sizer = new wxStaticBoxSizer(wxVERTICAL, reconnect_panel, wxT("Policy"));
	outer_router_sizer = new wxStaticBoxSizer(wxHORIZONTAL, reconnect_panel, wxT("Router Data"));
	router_sizer = new wxFlexGridSizer(4, 2, 10, 10);
	policy_sizer = new wxFlexGridSizer(1, 2, 10, 10);
	reconnect_button_sizer = new wxBoxSizer(wxHORIZONTAL);

	// is reconnecting enabled?
	bool enable = false;
	if(get_var("enable_reconnect") == wxT("1"))
		enable = true;

	enable_reconnecting_check = new wxCheckBox(reconnect_panel, -1, wxT("enable reconnecting"));
	enable_reconnecting_check->SetValue(enable);

	policy_text = new wxStaticText(reconnect_panel, -1, wxT("Reconnect Policy"));
	router_model_text = new wxStaticText(reconnect_panel, -1, wxT("Model"));
	router_ip_text = new wxStaticText(reconnect_panel, -1, wxT("IP"));
	router_user_text = new wxStaticText(reconnect_panel, -1, wxT("Username"));
	router_pass_text = new wxStaticText(reconnect_panel, -1, wxT("Password"));

	router_ip_input = new wxTextCtrl(reconnect_panel, -1,  wxT(""), wxDefaultPosition, wxSize(200, 25));
	router_user_input = new wxTextCtrl(reconnect_panel, -1,  wxT(""), wxDefaultPosition, wxSize(200, 25));
	router_pass_input = new wxTextCtrl(reconnect_panel, -1, wxT(""), wxDefaultPosition, wxSize(200, 25), wxTE_PASSWORD);

	// wxChoice policy
	wxArrayString policy;
	policy_choice = new wxChoice(reconnect_panel, -1, wxDefaultPosition, wxSize(100, 30), policy);
	policy_choice->SetSelection(-1);

	// wxChoice router_model
	wxArrayString model;
	router_model_choice = new wxChoice(reconnect_panel, -1, wxDefaultPosition, wxSize(200, 30), model);

	reconnect_button = new wxButton(reconnect_panel, wxID_APPLY);
	reconnect_cancel_button = new wxButton(reconnect_panel, wxID_CANCEL);

	// filling sizers
	policy_sizer->Add(policy_text, 0, wxALIGN_LEFT);
	policy_sizer->Add(policy_choice, 0, wxALIGN_LEFT);

	router_sizer->Add(router_model_text, 0, wxALIGN_LEFT);
	router_sizer->Add(router_model_choice, 0, wxALIGN_LEFT);
	router_sizer->Add(router_ip_text, 0, wxALIGN_LEFT);
	router_sizer->Add(router_ip_input, 0, wxALIGN_LEFT);
	router_sizer->Add(router_user_text, 0, wxALIGN_LEFT);
	router_sizer->Add(router_user_input, 0, wxALIGN_LEFT);
	router_sizer->Add(router_pass_text, 0, wxALIGN_LEFT);
	router_sizer->Add(router_pass_input, 0, wxALIGN_LEFT);

	outer_policy_sizer->Add(policy_sizer, 0,wxALL|wxEXPAND,10);
	outer_router_sizer->Add(router_sizer, 0,wxALL|wxEXPAND,10);

	reconnect_button_sizer->Add(reconnect_button, 1, wxLEFT|wxRIGHT, 20);
	reconnect_button_sizer->Add(reconnect_cancel_button, 1, wxLEFT|wxRIGHT, 10);

	overall_reconnect_sizer->Add(enable_reconnecting_check, 0, wxLEFT|wxRIGHT|wxTOP, 20);
	overall_reconnect_sizer->Add(outer_policy_sizer, 0, wxGROW|wxALL, 20);
	overall_reconnect_sizer->Add(outer_router_sizer, 0, wxGROW|wxALL, 20);
	overall_reconnect_sizer->Add(reconnect_button_sizer, 0, wxALL);

	log_panel->SetSizerAndFit(overall_reconnect_sizer);

	if(enable) // reconnecting enabled
		enable_reconnect_panel();
	else
		disable_reconnect_panel();
}


void configure_dialog::enable_reconnect_panel(){

	// fill router_model_choice
	router_model_choice->Clear();
	wxArrayString model;
	size_t lineend = 1;
	string line = "", model_list, old_model;
	int selection = 0, i = 0;

	old_model = (get_var("router_model", true)).mb_str();
	model.Add(wxT(""));

	// check connection
	myframe *myparent = (myframe *) GetParent();
	tkSock *mysock = myparent->get_connection_attributes();

	if(mysock == NULL || !*mysock || mysock->get_peer_name() == ""){ // if there is no active connection
		wxMessageBox(wxT("Please connect before configurating the DownloadDaemon Server."), wxT("No Connection to Server"));

	}else{ // we have a connection
		boost::mutex *mx = myparent->get_mutex();
		mx->lock();

		mysock->send("DDP ROUTER LIST");
		mysock->recv(model_list);

		mx->unlock();
	}

	// parse lines
	while(model_list.length() > 0 && lineend != string::npos){
		lineend = model_list.find("\n"); // termination character for line

		if(lineend == string::npos){ // this is the last line (which ends without \n)
			line = model_list.substr(0, model_list.length());
			model_list = "";

		}else{ // there is still another line after this one
			line = model_list.substr(0, lineend);
			model_list = model_list.substr(lineend+1);
		}

		model.Add(wxString(line.c_str(), wxConvUTF8));
		if(line == old_model) // is this the selected value?
			selection = i;
		i++;

	}
	router_model_choice->Append(model);
	router_model_choice->SetSelection(selection);


	// fill policy_choice
	policy_choice->Clear();
	wxArrayString policy;

	policy.Add(wxT("Hard"));
	policy.Add(wxT("Continue"));
	policy.Add(wxT("Soft"));
	policy.Add(wxT("Pussy"));

	policy_choice->Append(policy);

	wxString old_policy = get_var("reconnect_policy", true);
	string bla = string(old_policy.mb_str());

	selection = -1;

	if(old_policy == wxT("HARD"))
		selection = 0;
	else if(old_policy == wxT("CONTINUE"))
		selection = 1;
	else if(old_policy == wxT("SOFT"))
		selection = 2;
	else if(old_policy == wxT("PUSSY"))
		selection = 3;

	policy_choice->SetSelection(selection);


	// fill router_ip_input and router_user_input
	router_ip_input->Clear();
	router_ip_input->AppendText(get_var("router_ip"));
	router_user_input->Clear();
	router_user_input->AppendText(get_var("router_username"));


	// enabling all
	router_ip_input->Enable(true);
	router_pass_input->Enable(true);
	router_user_input->Enable(true);
	policy_choice->Enable(true);
	router_model_choice->Enable(true);
}


void configure_dialog::disable_reconnect_panel(){
	// disabling all
	router_ip_input->Enable(false);
	router_pass_input->Enable(false);
	router_user_input->Enable(false);
	policy_choice->Enable(false);
	router_model_choice->Enable(false);
}


wxString configure_dialog::get_var(const string &var, bool router){
	string answer;

	// check connection
	myframe *myparent = (myframe *) GetParent();
	tkSock *mysock = myparent->get_connection_attributes();

	if(mysock == NULL || !*mysock || mysock->get_peer_name() == ""){ // if there is no active connection
		wxMessageBox(wxT("Please connect before configurating the DownloadDaemon Server."), wxT("No Connection to Server"));

	}else{ // we have a connection
		boost::mutex *mx = myparent->get_mutex();
		mx->lock();

		if(router) // get router var
			mysock->send("DDP ROUTER GET " + var);
		else // get normal var
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


void configure_dialog::on_checkbox_change(wxCommandEvent &event){
	if(enable_reconnecting_check->GetValue())
		enable_reconnect_panel();
	else
		disable_reconnect_panel();
}

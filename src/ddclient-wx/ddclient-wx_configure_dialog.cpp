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
#include "ddclient-wx_main.h"

// IDs
const long configure_dialog::id_premium_host_choice = wxNewId();
const long configure_dialog::id_enable_reconnect_check = wxNewId();
const long configure_dialog::id_help_button = wxNewId();
const long configure_dialog::id_router_model_input = wxNewId();

// event table
BEGIN_EVENT_TABLE(configure_dialog, wxDialog)
	EVT_BUTTON(wxID_APPLY, configure_dialog::on_apply)
	EVT_BUTTON(wxID_SAVE, configure_dialog::on_pass_change)
	EVT_BUTTON(wxID_OK, configure_dialog::on_premium_change)
	EVT_BUTTON(wxID_CANCEL, configure_dialog::on_cancel)
	EVT_BUTTON(id_help_button, configure_dialog::on_help)
	EVT_CHOICE(configure_dialog::id_premium_host_choice, configure_dialog::on_select_premium_host)
	EVT_CHECKBOX(configure_dialog::id_enable_reconnect_check, configure_dialog::on_checkbox_change)
	EVT_TEXT(configure_dialog::id_router_model_input, configure_dialog::on_model_input_change)
END_EVENT_TABLE()


configure_dialog::configure_dialog(wxWindow *parent):
	wxDialog(parent, -1, wxString(wxEmptyString)){

	myframe *p = (myframe *)parent;
	SetTitle(p->tsl("Configure DownloadDaemon"));

	notebook = new wxNotebook(this, -1);

	create_general_panel();
	create_download_panel();
	create_pass_panel();
	create_log_panel();
	create_reconnect_panel();

	notebook->AddPage(general_panel, p->tsl("General"), true);
	notebook->AddPage(download_panel, p->tsl("Download"), false);
	notebook->AddPage(pass_panel, p->tsl("Password"), false);
	notebook->AddPage(log_panel, p->tsl("Logging"), false);
	notebook->AddPage(reconnect_panel, p->tsl("Reconnect"), false);

	Layout();
	notebook->Fit();
	Fit();
	CenterOnScreen();
}


void configure_dialog::create_general_panel(){
	myframe *p = (myframe *)GetParent();

	// creating elements
	general_panel = new wxPanel(notebook, -1, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);

	overall_general_sizer = new wxBoxSizer(wxVERTICAL);
	outer_premium_sizer = new wxStaticBoxSizer(wxVERTICAL, general_panel, p->tsl("Premium Account"));
	outer_general_sizer = new wxStaticBoxSizer(wxVERTICAL, general_panel, p->tsl("General Options"));
	premium_sizer = new wxFlexGridSizer(4, 2, 10, 10);
	general_button_sizer = new wxBoxSizer(wxHORIZONTAL);

	premium_host_text = new wxStaticText(general_panel, -1, p->tsl("Host"));
	premium_user_text = new wxStaticText(general_panel, -1, p->tsl("Username"));
	premium_pass_text = new wxStaticText(general_panel, -1, p->tsl("Password"));

	premium_user_input = new wxTextCtrl(general_panel, -1, wxEmptyString, wxDefaultPosition, wxSize(200, 25));
	premium_pass_input = new wxTextCtrl(general_panel, -1, wxEmptyString, wxDefaultPosition, wxSize(200, 25), wxTE_PASSWORD);


	// fill premium_host_choice
	wxArrayString host;
	size_t lineend = 1;
	string line = "", host_list;

	host.Add(wxEmptyString);
	premium_host_list.clear();
	premium_host_list.push_back("");
/*
	// check connection
	myframe *myparent = (myframe *) GetParent();
	tkSock *mysock = myparent->get_connection_attributes();

	if(mysock == NULL || !*mysock || mysock->get_peer_name() == ""){ // if there is no active connection
		wxMessageBox(p->tsl("Please connect before configurating DownloadDaemon."), p->tsl("No Connection to Downloaddaemon"));

	}else{ // we have a connection
		boost::mutex *mx = myparent->get_mutex();
		mx->lock();

		mysock->send("DDP PREMIUM LIST");
		mysock->recv(host_list);

		mx->unlock();
	}

	// parse lines
	while(host_list.length() > 0 && lineend != string::npos){
		lineend = host_list.find("\n"); // termination character for line

		if(lineend == string::npos){ // this is the last line (which ends without \n)
			line = host_list.substr(0, host_list.length());
			host_list = "";

		}else{ // there is still another line after this one
			line = host_list.substr(0, lineend);
			host_list = host_list.substr(lineend+1);
		}

		host.Add(wxString(line.c_str(), wxConvUTF8));
		premium_host_list.push_back(line); // save premium host for later use


	}
	premium_host_choice = new wxChoice(general_panel, id_premium_host_choice, wxDefaultPosition, wxSize(200, 30), host);


	bool enable = false;
	if(get_var("overwrite_files") == wxT("1"))
		enable = true;

	overwrite_check = new wxCheckBox(general_panel, -1, p->tsl("overwrite existing Files"));
	overwrite_check->SetValue(enable);

	enable = false;
	if(get_var("refuse_existing_links") == wxT("1"))
		enable = true;

	refuse_existing_check = new wxCheckBox(general_panel, -1, p->tsl("refuse existing Links"));
	refuse_existing_check->SetValue(enable);

	premium_button = new wxButton(general_panel, wxID_OK);
	general_button = new wxButton(general_panel, wxID_APPLY);
	general_cancel_button = new wxButton(general_panel, wxID_CANCEL);


	// filling sizers
	premium_sizer->Add(premium_host_text, 0, wxALIGN_LEFT);
	premium_sizer->Add(premium_host_choice, 0, wxALIGN_LEFT);
	premium_sizer->Add(premium_user_text, 0, wxALIGN_LEFT);
	premium_sizer->Add(premium_user_input, 0, wxALIGN_LEFT);
	premium_sizer->Add(premium_pass_text, 0, wxALIGN_LEFT);
	premium_sizer->Add(premium_pass_input, 0, wxALIGN_LEFT);
	premium_sizer->Add(premium_button, 0, wxALIGN_LEFT);

	outer_premium_sizer->Add(premium_sizer, 0,wxALL|wxEXPAND,10);

	outer_general_sizer->Add(overwrite_check, 0,wxALL|wxEXPAND,10);
	outer_general_sizer->Add(refuse_existing_check, 0,wxALL|wxEXPAND,10);

	general_button_sizer->Add(general_button, 1, wxLEFT|wxRIGHT, 20);
	general_button_sizer->Add(general_cancel_button, 1, wxLEFT|wxRIGHT, 10);

	overall_general_sizer->Add(outer_premium_sizer, 0, wxGROW|wxALL, 20);
	overall_general_sizer->Add(outer_general_sizer, 0, wxGROW|wxALL, 20);
	overall_general_sizer->Add(general_button_sizer, 0, wxALL);

	general_panel->SetSizerAndFit(overall_general_sizer);*/
}


void configure_dialog::create_download_panel(){/*
	myframe *p = (myframe *)GetParent();

	// creating elements
	download_panel = new wxPanel(notebook, -1, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);

	overall_download_sizer = new wxBoxSizer(wxVERTICAL);
	outer_time_sizer = new wxStaticBoxSizer(wxVERTICAL, download_panel, p->tsl("Download Time"));
	outer_save_dir_sizer = new wxStaticBoxSizer(wxVERTICAL, download_panel, p->tsl("Download Folder"));
	outer_count_sizer = new wxStaticBoxSizer(wxVERTICAL, download_panel, p->tsl("Additional Download Options"));
	time_sizer = new wxFlexGridSizer(1, 4, 10, 10);
	count_sizer = new wxFlexGridSizer(1, 4, 10, 10);
	download_button_sizer = new wxBoxSizer(wxHORIZONTAL);

	exp_time_text = new wxStaticText(download_panel, -1, p->tsl("You can force DownloadDaemon to only download at specific times by entering a start and"
									"\nend time in the format hours:minutes."
									"\nLeave these fields empty if you want to allow DownloadDaemon to download permanently."));
	start_time_text = new wxStaticText(download_panel, -1, p->tsl("Start Time"));
	end_time_text = new wxStaticText(download_panel, -1, p->tsl("End Time"));
	save_dir_text = new wxStaticText(download_panel, -1, p->tsl("This option specifies where finished downloads should be safed on the server."));
	exp_count_text = new wxStaticText(download_panel, -1, p->tsl("Here you can specify how many downloads may run at the same time and regulate the"
									"\ndownload speed for each download (overall max speed is download number * max speed)."));
	count_text = new wxStaticText(download_panel, -1, p->tsl("Simultaneous Downloads"));
	speed_text = new wxStaticText(download_panel, -1, p->tsl("Maximal Speed in kb/s"));

	start_time_input = new wxTextCtrl(download_panel,-1, get_var("download_timing_start"), wxDefaultPosition, wxSize(100, 25));
	start_time_input->SetFocus();
	end_time_input = new wxTextCtrl(download_panel, -1, get_var("download_timing_end"), wxDefaultPosition, wxSize(100, 25));
	save_dir_input = new wxTextCtrl(download_panel, -1, get_var("download_folder"), wxDefaultPosition, wxSize(400, 25));
	count_input = new wxTextCtrl(download_panel, -1, get_var("simultaneous_downloads"), wxDefaultPosition, wxSize(100, 25));
	speed_input = new wxTextCtrl(download_panel, -1, get_var("max_dl_speed"), wxDefaultPosition, wxSize(100, 25));

	download_button = new wxButton(download_panel, wxID_APPLY);
	download_cancel_button = new wxButton(download_panel, wxID_CANCEL);

	// filling sizers
	time_sizer->Add(start_time_text, 0, wxALIGN_LEFT);
	time_sizer->Add(start_time_input, 0, wxALIGN_LEFT);
	time_sizer->Add(end_time_text, 0, wxALIGN_LEFT);
	time_sizer->Add(end_time_input, 0, wxALIGN_LEFT);

	count_sizer->Add(count_text, 0, wxALIGN_LEFT);
	count_sizer->Add(count_input, 0, wxALIGN_LEFT);
	count_sizer->Add(speed_text, 0, wxALIGN_LEFT);
	count_sizer->Add(speed_input, 0, wxALIGN_LEFT);

	download_button_sizer->Add(download_button, 1, wxLEFT|wxRIGHT, 20);
	download_button_sizer->Add(download_cancel_button, 1, wxLEFT|wxRIGHT, 10);

	outer_time_sizer->Add(exp_time_text, 0,wxALL|wxEXPAND,10);
	outer_time_sizer->Add(time_sizer, 0,wxALL|wxEXPAND,10);
	outer_save_dir_sizer->Add(save_dir_text, 0,wxALL|wxEXPAND,10);
	outer_save_dir_sizer->Add(save_dir_input, 0,wxALL|wxEXPAND,10);
	outer_count_sizer->Add(exp_count_text, 0,wxALL|wxEXPAND,10);
	outer_count_sizer->Add(count_sizer, 0,wxALL|wxEXPAND,10);

	overall_download_sizer->Add(outer_time_sizer, 0, wxGROW|wxALL, 20);
	overall_download_sizer->Add(outer_save_dir_sizer, 0, wxGROW|wxALL, 20);
	overall_download_sizer->Add(outer_count_sizer, 0, wxGROW|wxALL, 20);
	overall_download_sizer->Add(download_button_sizer, 0, wxLEFT|wxRIGHT, 0);

	download_panel->SetSizerAndFit(overall_download_sizer);*/
}


void configure_dialog::create_pass_panel(){/*
	myframe *p = (myframe *)GetParent();

	// creating elements
	pass_panel = new wxPanel(notebook, -1, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);

	overall_pass_sizer = new wxBoxSizer(wxVERTICAL);
	outer_pass_sizer = new wxStaticBoxSizer(wxHORIZONTAL, pass_panel, p->tsl("Change Password"));
	pass_sizer = new wxFlexGridSizer(3, 2, 10, 10);
	pass_button_sizer = new wxBoxSizer(wxHORIZONTAL);

	old_pass_text = new wxStaticText(pass_panel, -1, p->tsl("Old Password"));
	new_pass_text = new wxStaticText(pass_panel, -1, p->tsl("New Password"));

	old_pass_input = new wxTextCtrl(pass_panel, -1, wxEmptyString, wxDefaultPosition, wxSize(200, 25), wxTE_PASSWORD);
	new_pass_input = new wxTextCtrl(pass_panel, -1, wxEmptyString, wxDefaultPosition, wxSize(200, 25), wxTE_PASSWORD);

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

	pass_panel->SetSizerAndFit(overall_pass_sizer);*/
}


void configure_dialog::create_log_panel(){/*
	myframe *p = (myframe *)GetParent();

	// creating elements
	log_panel = new wxPanel(notebook, -1, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);

	overall_log_sizer = new wxBoxSizer(wxVERTICAL);
	outer_log_activity_sizer = new wxStaticBoxSizer(wxVERTICAL, log_panel, p->tsl("Logging Activity"));
	outer_log_output_sizer = new wxStaticBoxSizer(wxVERTICAL, log_panel, p->tsl("Log Procedure"));
	log_button_sizer = new wxBoxSizer(wxHORIZONTAL);

	log_activity_text = new wxStaticText(log_panel, -1, p->tsl("This option specifies DownloadDaemons logging activity."));
	log_output_text = new wxStaticText(log_panel, -1, p->tsl("This option specifies how logging should be done (Standard output, Standard error"
															"\noutput, Syslog-daemon)."));

	// wxChoice log_output
	wxString old_output = get_var("log_procedure");
	int selection = 0;

	if(old_output == wxT("stdout"))
		selection = 0;
	else if(old_output == wxT("stderr"))
		selection = 1;
	else if(old_output == wxT("syslog"))
		selection = 2;

	wxArrayString output;
	output.Add(p->tsl("stdout - Standard output"));
	output.Add(p->tsl("stderr - Standard error output"));
	output.Add(p->tsl("syslog - Syslog-daemon"));
	log_output_choice = new wxChoice(log_panel, -1, wxDefaultPosition, wxSize(250, 30), output);
	log_output_choice->SetSelection(selection);

	// wxChoice log_activity
	wxString old_activity = get_var("log_level");
	selection = 0;

	if(old_activity == wxT("DEBUG"))
		selection = 0;
	else if(old_activity == wxT("WARNING"))
		selection = 1;
	else if(old_activity == wxT("SEVERE"))
		selection = 2;
	else if(old_activity == wxT("OFF"))
		selection = 3;

	wxArrayString activity;
	activity.Add(p->tsl("Debug"));
	activity.Add(p->tsl("Warning"));
	activity.Add(p->tsl("Severe"));
	activity.Add(p->tsl("Off"));
	log_activity_choice = new wxChoice(log_panel, -1, wxDefaultPosition, wxSize(100, 30), activity);
	log_activity_choice->SetSelection(selection);

	log_button = new wxButton(log_panel, wxID_APPLY);
	log_cancel_button = new wxButton(log_panel, wxID_CANCEL);

	// filling sizers
	outer_log_output_sizer->Add(log_output_text, 0,wxALL|wxEXPAND,10);
	outer_log_output_sizer->Add(log_output_choice, 0,wxALL,10);
	outer_log_activity_sizer->Add(log_activity_text, 0,wxALL|wxEXPAND,10);
	outer_log_activity_sizer->Add(log_activity_choice, 0,wxALL,10);

	log_button_sizer->Add(log_button, 1, wxLEFT|wxRIGHT, 20);
	log_button_sizer->Add(log_cancel_button, 1, wxLEFT|wxRIGHT, 10);

	overall_log_sizer->Add(outer_log_output_sizer, 0, wxGROW|wxALL, 20);
	overall_log_sizer->Add(outer_log_activity_sizer, 0, wxGROW|wxALL, 20);
	overall_log_sizer->Add(log_button_sizer, 0, wxALL);

	log_panel->SetSizerAndFit(overall_log_sizer);*/
}


void configure_dialog::create_reconnect_panel(){/*
	myframe *p = (myframe *)GetParent();

	// creating elements
	reconnect_panel = new wxPanel(notebook, -1, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);

	overall_reconnect_sizer = new wxBoxSizer(wxVERTICAL);
	outer_policy_sizer = new wxStaticBoxSizer(wxVERTICAL, reconnect_panel, p->tsl("Policy"));
	outer_router_sizer = new wxStaticBoxSizer(wxHORIZONTAL, reconnect_panel, p->tsl("Router Data"));
	router_sizer = new wxFlexGridSizer(4, 2, 10, 10);
	policy_sizer = new wxFlexGridSizer(1, 3, 10, 10);
	reconnect_button_sizer = new wxBoxSizer(wxHORIZONTAL);

	// is reconnecting enabled?
	bool enable = false;
	if(get_var("enable_reconnect") == wxT("1"))
		enable = true;

	enable_reconnecting_check = new wxCheckBox(reconnect_panel, id_enable_reconnect_check, p->tsl("enable reconnecting"));
	enable_reconnecting_check->SetValue(enable);

	policy_text = new wxStaticText(reconnect_panel, -1, p->tsl("Reconnect Policy"));
	router_model_text = new wxStaticText(reconnect_panel, -1, p->tsl("Model"));
	router_model_blank_text = new wxStaticText(reconnect_panel, -1, wxT(""));
	router_ip_text = new wxStaticText(reconnect_panel, -1, p->tsl("IP"));
	router_user_text = new wxStaticText(reconnect_panel, -1, p->tsl("Username"));
	router_pass_text = new wxStaticText(reconnect_panel, -1, p->tsl("Password"));

	router_model_input = new wxTextCtrl(reconnect_panel, id_router_model_input, wxEmptyString, wxDefaultPosition, wxSize(200, 25));
	router_ip_input = new wxTextCtrl(reconnect_panel, -1, wxEmptyString, wxDefaultPosition, wxSize(200, 25));
	router_user_input = new wxTextCtrl(reconnect_panel, -1, wxEmptyString, wxDefaultPosition, wxSize(200, 25));
	router_pass_input = new wxTextCtrl(reconnect_panel, -1, wxEmptyString, wxDefaultPosition, wxSize(200, 25), wxTE_PASSWORD);

	router_model_box = new wxListBox(reconnect_panel, -1, wxDefaultPosition, wxSize(400, 100));

	// wxChoice policy
	wxArrayString policy;
	policy_choice = new wxChoice(reconnect_panel, -1, wxDefaultPosition, wxSize(150, 30), policy);
	policy_choice->SetSelection(-1);

	help_button = new wxButton(reconnect_panel, id_help_button, wxT("?"), wxDefaultPosition, wxSize(30, 30));
	reconnect_button = new wxButton(reconnect_panel, wxID_APPLY);
	reconnect_cancel_button = new wxButton(reconnect_panel, wxID_CANCEL);

	// filling sizers
	policy_sizer->Add(policy_text, 0, wxALIGN_LEFT);
	policy_sizer->Add(policy_choice, 0, wxALIGN_LEFT);
	policy_sizer->Add(help_button, 0, wxALIGN_LEFT);

	router_sizer->Add(router_model_text, 0, wxALIGN_LEFT);
	router_sizer->Add(router_model_input, 0, wxALIGN_LEFT);
	router_sizer->Add(router_model_blank_text, 0, wxALIGN_LEFT);
	router_sizer->Add(router_model_box, 0, wxALIGN_LEFT);
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

	reconnect_panel->SetSizerAndFit(overall_reconnect_sizer);

	if(enable) // reconnecting enabled
		enable_reconnect_panel();
	else
		disable_reconnect_panel();*/
}


void configure_dialog::enable_reconnect_panel(){/*
	myframe *p = (myframe *)GetParent();


	// fill router_model_choice
	router_model_box->Clear();
	wxArrayString model;
	size_t lineend = 1;
	string line = "", model_list, old_model;
	int selection = 0, i = 1;

	old_model = (get_var("router_model", ROUTER_T)).mb_str();
	model.Add(wxEmptyString);
	router_model_list.clear();
	router_model_list.push_back("");

	// check connection
	myframe *myparent = (myframe *) GetParent();
	tkSock *mysock = myparent->get_connection_attributes();

	if(mysock == NULL || !*mysock || mysock->get_peer_name() == ""){ // if there is no active connection
		wxMessageBox(p->tsl("Please connect before configurating DownloadDaemon."), p->tsl("No Connection to Downloaddaemon"));

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
		router_model_list.push_back(line); // save router model for later use

		if(line == old_model) // is this the selected value?
			selection = i;
		i++;

	}
	router_model_box->Append(model);
	router_model_box->SetSelection(selection);


	// fill policy_choice
	policy_choice->Clear();
	wxArrayString policy;

	policy.Add(p->tsl("Hard"));
	policy.Add(p->tsl("Continue"));
	policy.Add(p->tsl("Soft"));
	policy.Add(p->tsl("Pussy"));

	policy_choice->Append(policy);

	wxString old_policy = get_var("reconnect_policy", ROUTER_T);

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
	router_ip_input->AppendText(get_var("router_ip", ROUTER_T));
	router_user_input->Clear();
	router_user_input->AppendText(get_var("router_username", ROUTER_T));


	// enabling all
	router_ip_input->Enable(true);
	router_pass_input->Enable(true);
	router_user_input->Enable(true);
	policy_choice->Enable(true);
	router_model_input->Enable(true);
	router_model_box->Enable(true);*/
}


void configure_dialog::disable_reconnect_panel(){
	// disabling all
	router_ip_input->Enable(false);
	router_pass_input->Enable(false);
	router_user_input->Enable(false);
	policy_choice->Enable(false);
	router_model_input->Enable(false);
	router_model_box->Enable(false);
}


wxString configure_dialog::get_var(const string &var, var_type typ){/*
	myframe *p = (myframe *)GetParent();
	string answer;

	// check connection
	myframe *myparent = (myframe *) GetParent();
	tkSock *mysock = myparent->get_connection_attributes();

	if(mysock == NULL || !*mysock || mysock->get_peer_name() == ""){ // if there is no active connection
		wxMessageBox(p->tsl("Please connect before configurating DownloadDaemon."), p->tsl("No Connection to Downloaddaemon"));

	}else{ // we have a connection
		boost::mutex *mx = myparent->get_mutex();
		mx->lock();

		if(typ == NORMAL_T) // get normal var
			mysock->send("DDP VAR GET " + var);
		else if(typ == ROUTER_T)  // get router var
			mysock->send("DDP ROUTER GET " + var);
		else // get premium var
			mysock->send("DDP PREMIUM GET " + var);
		mysock->recv(answer);

		mx->unlock();
	}

	return wxString(answer.c_str(), wxConvUTF8);*/
}


// event handle methods
void configure_dialog::on_apply(wxCommandEvent &event){/*
	myframe *p = (myframe *)GetParent();

	// getting user input
	bool overwrite = overwrite_check->GetValue();
	bool refuse_existing = refuse_existing_check->GetValue();

	string start_time = string(start_time_input->GetValue().mb_str());
	string end_time = string(end_time_input->GetValue().mb_str());
	string save_dir = string(save_dir_input->GetValue().mb_str());
	string count = string(count_input->GetValue().mb_str());
	string speed = string(speed_input->GetValue().mb_str());

	string log_output;
	int selection = log_output_choice->GetCurrentSelection();

	switch (selection){
		case 0: 	log_output = "stdout";
					break;
		case 1: 	log_output = "stderr";
					break;
		case 2: 	log_output = "syslog";
					break;
	}

	string activity_level;
	selection = log_activity_choice->GetCurrentSelection();

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

	bool reconnect_enable = enable_reconnecting_check->GetValue();
	string policy, router_model, router_ip, router_user, router_pass;

	if(reconnect_enable){ // other contents of reconnect panel are only saved if reconnecting is enabled
		selection = policy_choice->GetCurrentSelection();

		switch (selection){
			case 0: 	policy = "HARD";
						break;
			case 1: 	policy = "CONTINUE";
						break;
			case 2: 	policy = "SOFT";
						break;
			case 3: 	policy = "PUSSY";
						break;
		}

		router_model = wxString(router_model_box->GetStringSelection()).mb_str();

		router_ip = string(router_ip_input->GetValue().mb_str());
		router_user = string(router_user_input->GetValue().mb_str());
		router_pass = string(router_pass_input->GetValue().mb_str());
	}


	// check connection
	myframe *myparent = (myframe *) GetParent();
	tkSock *mysock = myparent->get_connection_attributes();

	boost::mutex *mx = myparent->get_mutex();
	mx->lock();

	if(mysock == NULL || !*mysock || mysock->get_peer_name() == ""){ // if there is no active connection
		mx->unlock();
		wxMessageBox(p->tsl("Please connect before configurating DownloadDaemon."), p->tsl("No Connection to Downloaddaemon"));
	}else{ // we have a connection
		string answer;

		// overwrite files
		if(overwrite)
			mysock->send("DDP VAR SET overwrite_files = 1");
		else
			mysock->send("DDP VAR SET overwrite_files = 0");
		mysock->recv(answer);

		// refuse existing links
		if(refuse_existing)
			mysock->send("DDP VAR SET refuse_existing_links = 1");
		else
			mysock->send("DDP VAR SET refuse_existing_links = 0");
		mysock->recv(answer);

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

		// download speed
		mysock->send("DDP VAR SET max_dl_speed = " + speed);
		mysock->recv(answer);

		// log output
		mysock->send("DDP VAR SET log_procedure = " + log_output);
		mysock->recv(answer);

		if(answer.find("102") == 0) // 102 AUTHENTICATION	<-- Authentication failed
			wxMessageBox(p->tsl("Failed to change Log Procedure."), p->tsl("Password Error"));

		// log activity
		mysock->send("DDP VAR SET log_level = " + activity_level);
		mysock->recv(answer);

		// reconnect enable
		if(reconnect_enable){
			mysock->send("DDP VAR SET enable_reconnect = 1");
			mysock->recv(answer);

			// policy
			mysock->send("DDP ROUTER SET reconnect_policy = " + policy);
			mysock->recv(answer);

			// router model
			mysock->send("DDP ROUTER SETMODEL " + router_model);
			mysock->recv(answer);

			// router ip
			mysock->send("DDP ROUTER SET router_ip = " + router_ip);
			mysock->recv(answer);

			// router user
			mysock->send("DDP ROUTER SET router_username = " + router_user);
			mysock->recv(answer);

			// user pass
			mysock->send("DDP ROUTER SET router_password = " + router_pass);
			mysock->recv(answer);

		}else{
			mysock->send("DDP VAR SET enable_reconnect = 0");
			mysock->recv(answer);

		}

		mx->unlock();
	}

	Destroy();*/
}


void configure_dialog::on_pass_change(wxCommandEvent &event){/*
	myframe *p = (myframe *)GetParent();

	// getting user input
	string old_pass = string(old_pass_input->GetValue().mb_str());
	string new_pass = string(new_pass_input->GetValue().mb_str());

	// check connection
	myframe *myparent = (myframe *) GetParent();
	tkSock *mysock = myparent->get_connection_attributes();

	boost::mutex *mx = myparent->get_mutex();
	mx->lock();

	if(mysock == NULL || !*mysock || mysock->get_peer_name() == ""){ // if there is no active connection
		mx->unlock();
		wxMessageBox(p->tsl("Please connect before configurating DownloadDaemon."), p->tsl("No Connection to Downloaddaemon"));

	}else{ // we have a connection
		string answer;

		// password
		mysock->send("DDP VAR SET mgmt_password = " + old_pass + " ; " + new_pass);
		mysock->recv(answer);

		mx->unlock();

		if(answer.find("102") == 0) // 102 AUTHENTICATION	<-- Authentication failed
			wxMessageBox(p->tsl("Failed to change the Password."), p->tsl("Password Error"));
	}

	Destroy();*/
}


void configure_dialog::on_premium_change(wxCommandEvent &event){/*
	myframe *p = (myframe *)GetParent();

	int selection = premium_host_choice->GetCurrentSelection();

	if(selection != 0){ // selected a valid host
		string premium_host =  premium_host_list.at(selection);
		string premium_user = string(premium_user_input->GetValue().mb_str());
		string premium_pass = string(premium_pass_input->GetValue().mb_str());

		// check connection
		myframe *myparent = (myframe *) GetParent();
		tkSock *mysock = myparent->get_connection_attributes();

		boost::mutex *mx = myparent->get_mutex();
		mx->lock();

		if(mysock == NULL || !*mysock || mysock->get_peer_name() == ""){ // if there is no active connection
			mx->unlock();
			wxMessageBox(p->tsl("Please connect before configurating DownloadDaemon."), p->tsl("No Connection to Downloaddaemon"));

		}else{ // we have a connection
			string answer;

			// host, user and password together
			mysock->send("DDP PREMIUM SET " + premium_host + " " + premium_user + " ; " + premium_pass);
			mysock->recv(answer);

			mx->unlock();
		}
	}*/
}


void configure_dialog::on_cancel(wxCommandEvent &event){
	Destroy();
}


void configure_dialog::on_help(wxCommandEvent &event){
	myframe *p = (myframe *) GetParent();

	string help("HARD cancels all downloads if a reconnect is needed\n"
				"CONTINUE only cancels downloads that can be continued after the reconnect\n"
				"SOFT will wait until all other downloads are finished\n"
				"PUSSY will only reconnect if there is no other choice \n\t(no other download can be started without a reconnect)");

	wxMessageBox(p->tsl(help), p->tsl("Help"));
}


void configure_dialog::on_select_premium_host(wxCommandEvent &event){
	int selection = premium_host_choice->GetCurrentSelection();

	premium_user_input->Clear();
	premium_pass_input->Clear();

	if(selection != 0){ // selected a host
		string premium_host =  premium_host_list.at(selection);
		premium_user_input->AppendText(get_var(premium_host, PREMIUM_T));

	}

}


void configure_dialog::on_checkbox_change(wxCommandEvent &event){
	if(enable_reconnecting_check->GetValue())
		enable_reconnect_panel();
	else
		disable_reconnect_panel();
}


void configure_dialog::on_model_input_change(wxCommandEvent &event){
	router_model_box->Clear();
	wxArrayString model;

	string search_input = string(router_model_input->GetValue().mb_str());

	for(size_t i=0; i<search_input.length(); i++) // lower everything for case insensitivity
		search_input[i] = tolower(search_input[i]);

	vector<string>::iterator it;
	for(it = router_model_list.begin(); it != router_model_list.end(); it++){
		string lower = *it;

		for(size_t i=0; i<lower.length(); i++) // lower everything for case insensitivity
			lower[i] = tolower(lower[i]);

		if(lower.find(search_input) != string::npos)
			model.Add(wxString(it->c_str(), wxConvUTF8));
	}

	router_model_box->Append(model);
}

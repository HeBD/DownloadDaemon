/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "ddclient-wx_main.h"
#ifdef _WIN32
	#define sleep(x) Sleep(x * 1000)
#endif

// IDs
const long myframe::id_menu_about = wxNewId();
const long myframe::id_menu_select_all_lines = wxNewId();
const long myframe::id_toolbar_connect = wxNewId();
const long myframe::id_toolbar_add = wxNewId();
const long myframe::id_toolbar_delete = wxNewId();
const long myframe::id_toolbar_activate = wxNewId();
const long myframe::id_toolbar_deactivate = wxNewId();
const long myframe::id_toolbar_priority_up = wxNewId();
const long myframe::id_toolbar_priority_down = wxNewId();
const long myframe::id_toolbar_configure = wxNewId();
const long myframe::id_toolbar_download_activate = wxNewId();
const long myframe::id_toolbar_download_deactivate = wxNewId();


// for custom event
BEGIN_DECLARE_EVENT_TYPES()
	DECLARE_LOCAL_EVENT_TYPE(wxEVT_reload_list, wxNewEventType())
END_DECLARE_EVENT_TYPES()

DEFINE_LOCAL_EVENT_TYPE(wxEVT_reload_list)


// event table (has to be after creating IDs, won't work otherwise)
BEGIN_EVENT_TABLE(myframe, wxFrame)
	EVT_MENU(wxID_EXIT, myframe::on_quit)
	EVT_MENU(id_menu_about, myframe::on_about)
	EVT_MENU(id_menu_select_all_lines, myframe::on_select_all_lines)
	EVT_MENU(id_toolbar_connect, myframe::on_connect)
	EVT_MENU(id_toolbar_add, myframe::on_add)
	EVT_MENU(id_toolbar_delete, myframe::on_delete)
	EVT_MENU(id_toolbar_activate, myframe::on_activate)
	EVT_MENU(id_toolbar_deactivate, myframe::on_deactivate)
	EVT_MENU(id_toolbar_priority_up, myframe::on_priority_up)
	EVT_MENU(id_toolbar_priority_down, myframe::on_priority_down)
	EVT_MENU(id_toolbar_configure, myframe::on_configure)
	EVT_MENU(id_toolbar_download_activate, myframe::on_download_activate)
	EVT_MENU(id_toolbar_download_deactivate, myframe::on_download_deactivate)
	EVT_SIZE(myframe::on_resize)
	EVT_CUSTOM(wxEVT_reload_list, wxID_ANY, myframe::on_reload )
END_EVENT_TABLE()


myframe::myframe(wxChar *parameter, wxWindow *parent, const wxString &title, wxWindowID id):
	wxFrame(parent, id, title){

	// getting working dir
	working_dir = parameter;

	if(working_dir.find_first_of(wxT("/\\")) == wxString::npos) {
		// no / in argv[0]? this means that it's in the path or in ./ (windows) let's find out where.
		wxString env_path;
		wxGetEnv(wxT("PATH"), &env_path);
		wxString curr_path;
		size_t curr_pos = 0, last_pos = 0;
		bool found = false;

		while((curr_pos = env_path.find_first_of(wxT(";:"), curr_pos)) != wxString::npos) {
			curr_path = env_path.substr(last_pos, curr_pos -  last_pos);
			curr_path += wxT("/ddclient-wx");

			if(wxFile::Exists(curr_path)) {
				found = true;
				break;
			}

			last_pos = ++curr_pos;
		}

		if(!found) {
			// search the last folder of $PATH which is not included in the while loop
			curr_path = env_path.substr(last_pos, curr_pos -  last_pos);
			curr_path += wxT("/ddclient-wx");
			if(wxFile::Exists(curr_path)) {
				found = true;
			}
		}

		if(found) {
			// successfully located the file..
			working_dir = curr_path;
		}

	}

	working_dir = working_dir.substr(0, working_dir.find_last_of(wxT("/\\")));
	if(working_dir.find_last_of(wxT("/\\")) != wxString::npos) {
		working_dir = working_dir.substr(0, working_dir.find_last_of(wxT("/\\")));
	} else {
		// needed if it's started with ./ddclient-wx, so we become ./../share/
		working_dir += wxT("/..");
	}
	working_dir += wxT("/share/ddclient-wx/");
	#ifndef _WIN32
		char* abs_path = realpath(working_dir.mb_str(), NULL);
		wxString tmp(abs_path, wxConvUTF8);
		working_dir = tmp;
		working_dir += wxT("/");
		free(abs_path);
	#endif

	wxFileName fn(working_dir);
	fn.SetCwd();

	SetClientSize(wxSize(750,500));
	SetMinSize(wxSize(750,500));
	CenterOnScreen();
	SetIcon(wxIcon(wxT("img/logoDD.png")));

	add_bars();
	add_components();

	Layout();
	Fit();

	mysock = new tkSock();
	boost::thread(boost::bind(&myframe::update_list, this));
}


myframe::~myframe(){
		delete mysock;
}


void myframe::update_status(){

	if(mysock == NULL || !*mysock || mysock->get_peer_name() == ""){ // if there is no active connection
		wxMessageBox(wxT("Please reconnect."), wxT("No Connection to Server"));
		SetStatusText(wxT("Not connected"),1);

	}else{
		string answer;

		mx.lock();
		mysock->send("DDP VAR GET downloading_active");
		mysock->recv(answer);

		// removing both icons, even when maybe only one is shown
		toolbar->RemoveTool(id_toolbar_download_activate);
		toolbar->RemoveTool(id_toolbar_download_deactivate);

		if(answer == "1"){ // downloading active
			toolbar->AddTool(download_deactivate);
			file_menu->Enable(id_toolbar_download_deactivate, true);
		}else if(answer =="0"){ // downloadin not active
			toolbar->AddTool(download_activate);
			file_menu->Enable(id_toolbar_download_activate, true);
		}else{
			// should never be reached
		}

		toolbar->Realize();

		SetStatusText(wxT("Connected"),1);

		mx.unlock();
	}
}

void myframe::add_bars(){

	// menubar
	menu = new wxMenuBar();
	SetMenuBar(menu);

	file_menu = new wxMenu(_T(""));

	file_menu->Append(id_toolbar_connect, wxT("&Connect..\tAlt-C"), wxT("Connect"));
	file_menu->Append(id_toolbar_configure, wxT("&Configure..\tAlt-P"), wxT("Configure"));
	file_menu->Append(id_toolbar_download_activate, wxT("&Activate Downloading\tF2"), wxT("Activate Downloading"));
	file_menu->Append(id_toolbar_download_deactivate, wxT("&Deactivate Downloading\tF3"), wxT("Deactivate Downloading"));
	file_menu->Enable(id_toolbar_download_activate, false); // those two are not enabled from the start
	file_menu->Enable(id_toolbar_download_deactivate, false);
	file_menu->AppendSeparator();

	file_menu->Append(id_toolbar_activate, wxT("&Activate Download\tAlt-A"), wxT("Activate Download"));
	file_menu->Append(id_toolbar_deactivate, wxT("&Deactivate Download\tAlt-D"), wxT("Deactivate Download"));
	file_menu->AppendSeparator();

	file_menu->Append(id_toolbar_add, wxT("&Add Download..\tAlt-I"), wxT("Add Download"));
	file_menu->Append(id_toolbar_delete, wxT("&Delete Download\tDEL"), wxT("Delete Download"));
	file_menu->Append(id_menu_select_all_lines, wxT("&Select all\tCtrl-A"), wxT("Select all"));
	file_menu->AppendSeparator();

	file_menu->Append(wxID_EXIT, wxT("&Quit\tAlt-F4"), wxT("Quit"));
	menu->Append(file_menu, wxT("&File"));

	help_menu = new wxMenu(_T(""));
	help_menu->Append(id_menu_about, wxT("&About..\tF1"), wxT("About"));
	menu->Append(help_menu, wxT("&Help"));


	// toolbar with icons
	toolbar = new wxToolBar(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTB_HORIZONTAL|wxTB_FLAT);

	toolbar->AddTool(id_toolbar_connect, wxT("Connect"), wxBitmap(working_dir + wxT("img/1_connect.png"), wxBITMAP_TYPE_PNG), wxNullBitmap, wxITEM_NORMAL, wxT("Connect to a DownloadDaemon Server"), wxT("Connect to a DownloadDaemon Server"));
	toolbar->AddSeparator();

	toolbar->AddTool(id_toolbar_add, wxT("Add"), wxBitmap(working_dir + wxT("img/2_add.png"), wxBITMAP_TYPE_PNG), wxNullBitmap, wxITEM_NORMAL, wxT("Add a new Download"), wxT("Add a new Download"));
	toolbar->AddTool(id_toolbar_delete, wxT("Delete"), wxBitmap(working_dir + wxT("img/3_delete.png"), wxBITMAP_TYPE_PNG), wxNullBitmap, wxITEM_NORMAL, wxT("Delete the selected Download"), wxT("Delete the selected Download"));
	toolbar->AddSeparator();

	toolbar->AddTool(id_toolbar_activate, wxT("Activate"), wxBitmap(working_dir + wxT("img/5_start.png"), wxBITMAP_TYPE_PNG), wxNullBitmap, wxITEM_NORMAL, wxT("Activate the selected Download"), wxT("Activate the selected Download"));
	toolbar->AddTool(id_toolbar_deactivate, wxT("Deactivate"), wxBitmap(working_dir + wxT("img/4_stop.png"), wxBITMAP_TYPE_PNG), wxNullBitmap, wxITEM_NORMAL, wxT("Deactivate the selected Download"), wxT("Deactivate the selected Download"));
	toolbar->AddSeparator();

	toolbar->AddTool(id_toolbar_priority_up, wxT("Increase Priority"), wxBitmap(working_dir + wxT("img/6_up.png"), wxBITMAP_TYPE_PNG), wxNullBitmap, wxITEM_NORMAL, wxT("Increase Priority of the selected Download"), wxT("Increase Priority of the selected Download"));
	toolbar->AddTool(id_toolbar_priority_down, wxT("Decrease Priority"), wxBitmap(working_dir + wxT("img/7_down.png"), wxBITMAP_TYPE_PNG), wxNullBitmap, wxITEM_NORMAL, wxT("Decrease Priority of the selected Download"), wxT("Decrease Priority of the selected Download"));
	toolbar->AddSeparator();

	toolbar->AddTool(id_toolbar_configure, wxT("Configure"), wxBitmap(working_dir + wxT("img/8_configure.png"), wxBITMAP_TYPE_PNG), wxNullBitmap, wxITEM_NORMAL, wxT("Configure DownloadDaemon Server"), wxT("Configure DownloadDaemon Server"));
	toolbar->AddSeparator();

	download_activate = toolbar->AddTool(id_toolbar_download_activate, wxT("Activate Downloading"), wxBitmap(working_dir + wxT("img/9_activate.png"), wxBITMAP_TYPE_PNG), wxNullBitmap, wxITEM_NORMAL, wxT("Activate Downloading"), wxT("Activate Downloading"));
	download_deactivate = toolbar->AddTool(id_toolbar_download_deactivate, wxT("Deactivate Downloading"), wxBitmap(working_dir + wxT("img/9_deactivate.png"), wxBITMAP_TYPE_PNG), wxNullBitmap, wxITEM_NORMAL, wxT("Deactivate Downloading"), wxT("Deactivate Downloading"));

	toolbar->Realize();

	toolbar->RemoveTool(id_toolbar_download_activate); // these two toolbar icons are created for later use
	toolbar->RemoveTool(id_toolbar_download_deactivate);

	SetToolBar(toolbar);


	// statusbar
	CreateStatusBar(2);
	SetStatusText(wxT("DownloadDaemon-ClientWX"),0);
	SetStatusText(wxT("Not connected"),1);
}


void myframe::add_components(){

	panel_downloads = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
	sizer_downloads = new wxBoxSizer(wxHORIZONTAL); // lines/rows, colums, vgap, hgap
	panel_downloads->SetSizer(sizer_downloads);

	// all download lists
	list = new wxListCtrl(panel_downloads, wxID_ANY, wxPoint(120, -46), wxSize(0,0), wxLC_REPORT);
	sizer_downloads->Add(list , 1, wxALL|wxEXPAND|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);


	// columns
	list->InsertColumn(0, wxT("ID"), wxLIST_AUTOSIZE_USEHEADER, 50);
	list->InsertColumn(1, wxT("Added"), wxLIST_AUTOSIZE_USEHEADER, 190);
	list->InsertColumn(2, wxT("Title"), wxLIST_AUTOSIZE_USEHEADER, 76);
	list->InsertColumn(3, wxT("URL"), wxLIST_AUTOSIZE_USEHEADER, 178);
	list->InsertColumn(4, wxT("Status"), wxLIST_AUTOSIZE_USEHEADER, 126);
}


void myframe::update_list(){
	while(true){ // for boost::thread
		if(mysock == NULL || !*mysock || mysock->get_peer_name() == ""){
			SetStatusText(wxT("Not connected"),1);
			sleep(2);
			continue;
		}

		get_content();
		sleep(2); // reload every two seconds
	}
}


void myframe::get_content(){

	if(mysock == NULL || !*mysock || mysock->get_peer_name() == ""){
		SetStatusText(wxT("Not connected"),1);
	}else{

		SetStatusText(wxT("Connected"),1);

		vector<string> splitted_line;
		string answer, line, tab;
		size_t lineend = 1, tabend = 1, column_nr, line_nr = 0;

		mx.lock();
		new_content.clear();

		mysock->send("DDP DL LIST");
		mysock->recv(answer);


		// parse lines
		while(answer.length() > 0 && lineend != string::npos){
			lineend = answer.find("\n"); // termination character for line
			line = answer.substr(0, lineend);
			answer = answer.substr(lineend+1);

			// parse columns
			column_nr = 0;
			tabend = 0;
			while(line.length() > 0 && tabend != string::npos){
				tabend = line.find("|"); // termination character for column

				if(tabend == string::npos){ // no | found, so it is the last column
					tab = line;
					line = "";
				}else{
					if(tabend != 0 && line.at(tabend-1) == '\\') // because titles can have | inside (will be escaped with \)
						tabend = line.find("|", tabend+1);

					tab = line.substr(0, tabend);
					line = line.substr(tabend+1);

				}
				splitted_line.push_back(tab); // save all tabs per line for later use
			}


			line_nr++;
			new_content.push_back(splitted_line);
			splitted_line.clear();
		}

		mx.unlock();

		// send event to reload list
		wxCommandEvent event(wxEVT_reload_list, GetId());
		wxPostEvent(this, event);

	}
}


string myframe::build_status(string &status_text, vector<string> &splitted_line){
	string color = "WHITE";

	if(splitted_line[4] == "DOWNLOAD_RUNNING"){
		color = "LIME GREEN";

		if(atol(splitted_line[7].c_str()) > 0 && splitted_line[8] == "PLUGIN_SUCCESS"){ // waiting time > 0
			status_text = "Download running. Waiting " + splitted_line[7] + " seconds.";

		}else if(atol(splitted_line[7].c_str()) > 0 && splitted_line[8] != "PLUGIN_SUCCESS") {
			color = "RED";
			status_text = "Error: " + splitted_line[8] + " Retrying in " + splitted_line[7] + " seconds.";
		}else{ // no waiting time
			stringstream stream_buffer;
			stream_buffer << "Running";

			if(splitted_line[9] != "0" || splitted_line[9] != "-1") // download speed known
				stream_buffer << "@" << setprecision(1) << fixed << (float)atol(splitted_line[9].c_str()) / 1024 << " kb/s";

			stream_buffer << ": ";

			if(splitted_line[6] == "0" || splitted_line[6] == "1"){ // download size unknown
				stream_buffer << "0.00% - ";

				if(splitted_line[5] == "0" || splitted_line[5] == "1") // nothing downloaded yet
					stream_buffer << "0.00 MB/ 0.00 MB";
				else // something downloaded
					stream_buffer << setprecision(1) << fixed << (float)atol(splitted_line[5].c_str()) / 1048576 << " MB/ 0.00 MB";

			}else{ // download size known
				if(splitted_line[5] == "0" || splitted_line[5] == "1") // nothing downloaded yet
					stream_buffer << "0.00% - 0.00 MB/ " << fixed << (float)atol(splitted_line[6].c_str()) / 1048576 << " MB";
				else{ // download size known and something downloaded
					stream_buffer << setprecision(1) << fixed << (float)atol(splitted_line[5].c_str()) / (float)atol(splitted_line[6].c_str()) * 100 << "% - ";
					stream_buffer << setprecision(1) << fixed << (float)atol(splitted_line[5].c_str()) / 1048576 << " MB/ ";
					stream_buffer << setprecision(1) << fixed << (float)atol(splitted_line[6].c_str()) / 1048576 << " MB";
				}
			}
			status_text = stream_buffer.str();
		}

	}else if(splitted_line[4] == "DOWNLOAD_INACTIVE"){ // TODO: test the rest of the status stuff!
		if(splitted_line[8] == "PLUGIN_SUCCESS"){
			color = "YELLOW";
			status_text = "Download Inactive.";

		}else{ // error occured
			color = "RED";
			status_text = "Inactive. Error: " + splitted_line[8];
		}

	}else if(splitted_line[4] == "DOWNLOAD_PENDING"){
		if(splitted_line[8] == "PLUGIN_SUCCESS"){
			status_text = "Download Pending.";

		}else{ //error occured
			color = "RED";
			status_text = "Error: " + splitted_line[8];
		}

	}else if(splitted_line[4] == "DOWNLOAD_WAITING"){
		color = "YELLOW";
		status_text = "Have to wait " + splitted_line[7] + " seconds.";

	}else if(splitted_line[4] == "DOWNLOAD_FINISHED"){
		color = "GREEN";
		status_text = "Download Finished.";

	}else if(splitted_line[4] == "DOWNLOAD_RECONNECTING") {
		color = "YELLOW";
		status_text = "Reconnecting...";
	}else{ // default, column 4 has unknown input
		status_text = "Status not detected.";
	}

	return color;
}


void myframe::find_selected_lines(){
	long item_index = -1;

	selected_lines.clear();

	while(true){
		item_index = list->GetNextItem(item_index, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED); // gets the next selected item

		if(item_index == -1) // no selected ones left => leave loop
			break;
		else // found a selected one
			selected_lines.push_back(item_index);
	  }
}


void myframe::select_lines(){
	long item_index = -1;

	while(true){
		item_index = list->GetNextItem(item_index, wxLIST_NEXT_ALL, wxLIST_STATE_DONTCARE); // gets the next item

		if(item_index == -1) // no item left => leave loop
			break;
		else
			list->SetItemState(item_index, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
	  }
}


void myframe::select_line_by_id(string id){
	long item_index = -1;

	item_index = list->FindItem(-1, wxString(id.c_str(), wxConvUTF8));

	if(item_index != -1)
		list->SetItemState(item_index, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
}


void myframe::deselect_lines(){
	long item_index = -1;

	while(true){
		item_index = list->GetNextItem(item_index, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED); // gets the next selected item

		if(item_index == -1) // no selected ones left => leave loop
			break;
		else // found a selected one
			list->SetItemState(item_index, 0, wxLIST_STATE_SELECTED);
	  }
}


// methods for comparing and actualizing content if necessary
void myframe::compare_vectorvector(){

	vector<vector<string> >::iterator old_content_it = content.begin();
	vector<vector<string> >::iterator new_content_it = new_content.begin();

	size_t line_nr = 0, line_index;
	string status_text, color;


	// compare the i-th vector in content with the i-th vector in new_content
	while((new_content_it < new_content.end()) && (old_content_it < content.end())){
		compare_vector(line_nr, *new_content_it, *old_content_it);

		new_content_it++;
		old_content_it++;
		line_nr++;
	}


	if(new_content_it < new_content.end()){ // there are more new lines then old ones
		while(new_content_it < new_content.end()){

			// insert content
			line_index = list->InsertItem(line_nr, wxString(new_content_it->at(0).c_str(), wxConvUTF8));

			for(int i=1; i<4; i++) // column 1 to 3
				list->SetItem(line_index, i, wxString(new_content_it->at(i).c_str(), wxConvUTF8));

			// status column
			color = build_status(status_text, *new_content_it);
			list->SetItemBackgroundColour(line_index, wxString(color.c_str(), wxConvUTF8));
			list->SetItem(line_index, 4, wxString(status_text.c_str(), wxConvUTF8));

			new_content_it++;
			line_nr++;
		}

	}else if(old_content_it < content.end()){ // there are more old lines than new ones
		while(old_content_it < content.end()){

			// delete content
			list->DeleteItem(line_nr);

			old_content_it++;
			// line_nr stays the same!
		}
	}
}


void myframe::compare_vector(size_t line_nr, vector<string> &splitted_line_new, vector<string> &splitted_line_old){

	vector<string>::iterator it_old = splitted_line_old.begin();
	vector<string>::iterator end_old = splitted_line_old.end();
	vector<string>::iterator it_new = splitted_line_new.begin();
	vector<string>::iterator end_new = splitted_line_new.end();

	size_t column_nr = 0;
	bool status_change = false;
	string status_text, color;

	// compare every column
	while((it_new < end_new) && (it_old < end_old)){

		if(*it_new != *it_old){ // content of column_new != content of column_old

			if(column_nr < 4) // direct input for columns 0 to 3
				list->SetItem(line_nr, column_nr, wxString(it_new->c_str(), wxConvUTF8));

			else // status column
				status_change = true;
		}

		it_new++;
		it_old++;
		column_nr++;
	}

	if(status_change){
		color = build_status(status_text, splitted_line_new);
		list->SetItemBackgroundColour(line_nr, wxString(color.c_str(), wxConvUTF8));
		list->SetItem(line_nr, 4, wxString(status_text.c_str(), wxConvUTF8));
	}
}


// event handle methods
void myframe::on_quit(wxCommandEvent &event){
	Close();
}


void myframe::on_about(wxCommandEvent &event){
	about_dialog dialog(working_dir, this);
	dialog.ShowModal();
}


void myframe::on_select_all_lines(wxCommandEvent &event){

	mx.lock();
	select_lines();
	mx.unlock();
}


 void myframe::on_connect(wxCommandEvent &event){
	connect_dialog dialog(this);
	dialog.ShowModal();
	get_content();
 }


void myframe::on_add(wxCommandEvent &event){
	if(mysock == NULL || !*mysock || mysock->get_peer_name() == ""){ // if there is no active connection
		wxMessageBox(wxT("Please connect before adding Downloads."), wxT("No Connection to Server"));
		SetStatusText(wxT("Not connected"),1);

	}else{
		add_dialog dialog(this);
		dialog.ShowModal();
		get_content();
	}
 }


void myframe::on_delete(wxCommandEvent &event){
	vector<int>::iterator it;
	string id, answer;
	bool error_occured = false;

	if(mysock == NULL || !*mysock || mysock->get_peer_name() == ""){ // if there is no active connection
		wxMessageBox(wxT("Please connect before deleting Downloads."), wxT("No Connection to Server"));
		SetStatusText(wxT("Not connected"),1);

	}else{ // we have a connection

		mx.lock();

		find_selected_lines(); // save selection into selected_lines
		if(!selected_lines.empty()){

			// make sure user wants to delete downloads
			wxMessageDialog dialog(this, wxT("Do you really want to delete\nthe selected Download(s)?"), wxT("Delete Downloads"), wxYES_NO|wxYES_DEFAULT|wxICON_EXCLAMATION);
			int del = dialog.ShowModal();

			if(del == wxID_YES){ // user clicked yes to delete
				int dialog_answer = 2; // possible answers are 0 = yes all, 1 = yes, 2 = no, 3 = no all

				for(it = selected_lines.begin(); it<selected_lines.end(); it++){
					id = (content[*it])[0]; // gets the id of the line, which index is stored in selected_lines

					// test if there is a file on the server
					mysock->send("DDP FILE GETPATH " + id);
					mysock->recv(answer);

					if(!answer.empty()){ // file exists

						// only show dialog if user didn't choose yes_all (0) or no_all (3) before
						if((dialog_answer != 0) && (dialog_answer != 3)){
							delete_dialog dialog(&dialog_answer, id, this);
							dialog.ShowModal();
						}


						if((dialog_answer == 0) || (dialog_answer == 1)){ // user clicked yes all (0) or yes (1) to delete
							mysock->send("DDP DL DEACTIVATE " + id);
							mysock->recv(answer);

							mysock->send("DDP FILE DEL " + id);
							mysock->recv(answer);

							if(answer.find("109") == 0){ // 109 FILE <-- file operation on a file that does not exist
								string message = "Error occured at deleting File from ID " + id;
								wxMessageBox(wxString(message.c_str(), wxConvUTF8), wxT("Error"));
							}

						}
					}

					mysock->send("DDP DL DEL " + id);
					mysock->recv(answer);

					if(answer.find("104") == 0) // 104 ID <-- Entered a not-existing ID
						error_occured = true;
				}
			}
		}

		deselect_lines();
		mx.unlock();

		if(error_occured)
			wxMessageBox(wxT("Error occured at deleting Download(s)."), wxT("Error"));

		get_content();
	}
 }


void myframe::on_activate(wxCommandEvent &event){
	vector<int>::iterator it;
	string id, answer;

	if(mysock == NULL || !*mysock || mysock->get_peer_name() == ""){ // if there is no active connection
		wxMessageBox(wxT("Please connect before activating Downloads."), wxT("No Connection to Server"));
		SetStatusText(wxT("Not connected"),1);

	}else{ // we have a connection

		mx.lock();

		find_selected_lines(); // save selection into selected_lines
		if(!selected_lines.empty()){

			for(it = selected_lines.begin(); it<selected_lines.end(); it++){
				id = (content[*it])[0]; // gets the id of the line, which index is stored in selected_lines

				mysock->send("DDP DL ACTIVATE " + id);
				mysock->recv(answer); // might receive error 106 ACTIVATE, but that doesn't matter
			}

		}

		mx.unlock();
		get_content();
	}
 }


void myframe::on_deactivate(wxCommandEvent &event){
	vector<int>::iterator it;
	string id, answer;

	if(mysock == NULL || !*mysock || mysock->get_peer_name() == ""){ // if there is no active connection
		wxMessageBox(wxT("Please connect before deactivating Downloads."), wxT("No Connection to Server"));

	}else{ // we have a connection

		mx.lock();

		find_selected_lines(); // save selection into selected_lines
		if(!selected_lines.empty()){

			for(it = selected_lines.begin(); it<selected_lines.end(); it++){
				id = (content[*it])[0]; // gets the id of the line, which index is stored in selected_lines

				mysock->send("DDP DL DEACTIVATE " + id);
				mysock->recv(answer); // might receive error 107 DEACTIVATE, but that doesn't matter
			}

		}

		mx.unlock();
		get_content();
	}
 }


 void myframe::on_priority_up(wxCommandEvent &event){
	vector<int>::iterator it;
	string id, answer;

	if(mysock == NULL || !*mysock || mysock->get_peer_name() == ""){ // if there is no active connection
		wxMessageBox(wxT("Please connect before increasing Priority."), wxT("No Connection to Server"));
		SetStatusText(wxT("Not connected"),1);

	}else{ // we have a connection

		mx.lock();
		reselect_lines.clear();

		find_selected_lines(); // save selection into selected_lines
		if(!selected_lines.empty()){

			for(it = selected_lines.begin(); it<selected_lines.end(); it++){
				id = (content[*it])[0]; // gets the id of the line, which index is stored in selected_lines
				reselect_lines.push_back(id);

				mysock->send("DDP DL UP " + id);
				mysock->recv(answer);

				if(answer.find("104") == 0){ // 104 ID <-- tried to move the top download up
					reselect_lines.clear();
					break;
				}
			}

		}

		deselect_lines();
		mx.unlock();
		get_content();
	}
}


void myframe::on_priority_down(wxCommandEvent &event){
	vector<int>::reverse_iterator rit;
	string id, answer;

	if(mysock == NULL || !*mysock || mysock->get_peer_name() == ""){ // if there is no active connection
		wxMessageBox(wxT("Please connect before decreasing Priority."), wxT("No Connection to Server"));
		SetStatusText(wxT("Not connected"),1);

	}else{ // we have a connection

		mx.lock();
		reselect_lines.clear();

		find_selected_lines(); // save selection into selected_lines
		if(!selected_lines.empty()){

			for(rit = selected_lines.rbegin(); rit<selected_lines.rend(); rit++){ // has to be done from the last to the first line
				id = (content[*rit])[0]; // gets the id of the line, which index is stored in selected_lines
				reselect_lines.push_back(id);

				mysock->send("DDP DL DOWN " + id);
				mysock->recv(answer);

				if(answer.find("104") == 0){ // 104 ID <-- tried to move the bottom download down
					reselect_lines.clear();
					break;
				}
			}

		}

		deselect_lines();
		mx.unlock();
		get_content();
	}
}


void myframe::on_configure(wxCommandEvent &event){
	if(mysock == NULL || !*mysock || mysock->get_peer_name() == ""){ // if there is no active connection
		wxMessageBox(wxT("Please connect before configurating\nDownloadDaemon Server."), wxT("No Connection to Server"));
		SetStatusText(wxT("Not connected"),1);

	}else{
		configure_dialog dialog(this);
		dialog.ShowModal();
		get_content();
	}
}


void myframe::on_download_activate(wxCommandEvent &event){
	if(mysock == NULL || !*mysock || mysock->get_peer_name() == ""){ // if there is no active connection
		wxMessageBox(wxT("Please connect before activate Downloading."), wxT("No Connection to Server"));
		SetStatusText(wxT("Not connected"),1);

	}else{
		string answer;

		mx.lock();

		mysock->send("DDP VAR SET downloading_active = 1");
		mysock->recv(answer);

		mx.unlock();
		get_content();

		// update toolbar
		toolbar->RemoveTool(id_toolbar_download_activate);
		toolbar->RemoveTool(id_toolbar_download_deactivate);
		toolbar->AddTool(download_deactivate);
		toolbar->Realize();

		file_menu->Enable(id_toolbar_download_deactivate, true);
		file_menu->Enable(id_toolbar_download_activate, false);
	}
}


void myframe::on_download_deactivate(wxCommandEvent &event){
	if(mysock == NULL || !*mysock || mysock->get_peer_name() == ""){ // if there is no active connection
		wxMessageBox(wxT("Please connect before deactivate Downloading."), wxT("No Connection to Server"));
		SetStatusText(wxT("Not connected"),1);

	}else{
		string answer;

		mx.lock();

		mysock->send("DDP VAR SET downloading_active = 0");
		mysock->recv(answer);

		mx.unlock();
		get_content();

		// update toolbar
		toolbar->RemoveTool(id_toolbar_download_activate);
		toolbar->RemoveTool(id_toolbar_download_deactivate);
		toolbar->AddTool(download_activate);
		toolbar->Realize();

		file_menu->Enable(id_toolbar_download_activate, true);
		file_menu->Enable(id_toolbar_download_deactivate, false);
	}
}


 void myframe::on_resize(wxSizeEvent &event){
	myframe::OnSize(event); // call default evt_size handler
	Refresh();

 	if(list != NULL){

		// change column widths on resize
		int width = GetClientSize().GetWidth();
		width -= 255; // minus width of fix sized columns

		#if defined(__WXMSW__)
			width -= 10;
		#endif // defined(__WXMSW__)

		list->SetColumnWidth(2, width*0.2); // only column 2 to 4, because 0 and 1 have fix sizes
		list->SetColumnWidth(3, width*0.45);
		list->SetColumnWidth(4, width*0.35);
 	}
 }


void myframe::on_reload(wxEvent &event){

	mx.lock();

	compare_vectorvector();
	content.clear();
	content = new_content;

	if(!reselect_lines.empty()){ // update selection
		vector<string>::iterator sit;

		for(sit = reselect_lines.begin(); sit<reselect_lines.end(); sit++){
			select_line_by_id(*sit);
		}

		reselect_lines.clear();
	}

	mx.unlock();
}


// getter and setter
void myframe::set_connection_attributes(tkSock *mysock, string password){
	this->mysock = mysock;
}


tkSock *myframe::get_connection_attributes(){
	return mysock;
}


boost::mutex *myframe::get_mutex(){
	return &mx;
}

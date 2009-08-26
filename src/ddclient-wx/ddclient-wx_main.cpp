/***************************************************************
 * Name:	  ddclient-wx_main.cpp
 * Purpose:   Code for Frame Class
 * Author:	ko ()
 * Created:   2009-08-04
 * Copyright: ko ()
 * License:
 **************************************************************/

#include "ddclient-wx_main.h"
#ifdef _WIN32
    #define sleep(x) Sleep(x)
#endif

// IDs
const long myframe::id_menu_quit = wxNewId();
const long myframe::id_menu_about = wxNewId();
const long myframe::id_toolbar_connect = wxNewId();
const long myframe::id_toolbar_add = wxNewId();
const long myframe::id_toolbar_delete = wxNewId();
const long myframe::id_toolbar_deactivate = wxNewId();
const long myframe::id_toolbar_activate = wxNewId();
const long myframe::id_toolbar_configure = wxNewId();
const long myframe::id_toolbar_download_activate = wxNewId();
const long myframe::id_toolbar_download_deactivate = wxNewId();


// event table (has to be after creating IDs, won't work otherwise)
BEGIN_EVENT_TABLE(myframe, wxFrame)
	EVT_MENU(id_menu_quit, myframe::on_quit)
	EVT_MENU(id_menu_about, myframe::on_about)
	EVT_MENU(id_toolbar_connect, myframe::on_connect)
	EVT_MENU(id_toolbar_add, myframe::on_add)
	EVT_MENU(id_toolbar_delete, myframe::on_delete)
	EVT_MENU(id_toolbar_deactivate, myframe::on_deactivate)
	EVT_MENU(id_toolbar_activate, myframe::on_activate)
	EVT_MENU(id_toolbar_configure, myframe::on_configure)
	EVT_MENU(id_toolbar_download_activate, myframe::on_download_activate)
	EVT_MENU(id_toolbar_download_deactivate, myframe::on_download_deactivate)
	EVT_SIZE(myframe::on_resize)
END_EVENT_TABLE()


myframe::myframe(wxChar *parameter, wxWindow *parent, const wxString &title, wxWindowID id):
	wxFrame(parent, id, title){

	working_dir = parameter;
	working_dir = working_dir.substr(0, working_dir.find_last_of(wxT("/\\")));
	working_dir = working_dir.substr(0, working_dir.find_last_of(wxT("/\\")));


	working_dir += wxT("/");

	SetClientSize(wxSize(750,500));
	SetMinSize(wxSize(750,500));
	CenterOnScreen();

	add_bars();
	add_content();

	Layout();
	Fit();

	mysock = new tkSock();
	boost::thread(boost::bind(&myframe::fill_list, this));

}


myframe::~myframe(){
		delete mysock;
}


void myframe::update_status(){

	if(mysock == NULL || !*mysock){ // if there is no active connection
		wxMessageBox(wxT("Please connect before activate Downloading."), wxT("No Connection to Server"));
		SetStatusText(wxT("Not connected"),1);

	}else{
		string answer;

		mx.lock();
		mysock->send("DDP VAR GET downloading_active");
		mysock->recv(answer);

		if(answer == "1"){ // downloading active
			toolbar->RemoveTool(id_toolbar_download_activate);
		}else if(answer =="0"){ // downloadin not active
			toolbar->RemoveTool(id_toolbar_download_deactivate);
		}else{
			// should never be reached
		}

		SetStatusText(wxT("Connected"),1);

		mx.unlock();
	}

	return;
}

void myframe::add_bars(){

	// menubar
	menu = new wxMenuBar();
	SetMenuBar(menu);

	file_menu = new wxMenu(_T(""));
	file_menu->Append(id_menu_quit, wxT("&Quit\tAlt-F4"), wxT("Quit"));
	menu->Append(file_menu, wxT("&File"));

	help_menu = new wxMenu(_T(""));
	help_menu->Append(id_menu_about, wxT("&About\tF1"), wxT("About"));
	menu->Append(help_menu, wxT("&Help"));


	// toolbar with icons
	toolbar = new wxToolBar(this, wxID_ANY);

	toolbar->AddTool(id_toolbar_connect, wxT("Connect"), wxArtProvider::GetBitmap(wxART_MAKE_ART_ID_FROM_STR(_T("wxART_EXECUTABLE_FILE")),wxART_TOOLBAR), wxNullBitmap, wxITEM_NORMAL, wxT("Connect to a DownloadDaemon Server"), wxEmptyString);
	toolbar->AddSeparator();

	toolbar->AddTool(id_toolbar_add, wxT("Add"), wxArtProvider::GetBitmap(wxART_MAKE_ART_ID_FROM_STR(_T("wxART_ADD_BOOKMARK")),wxART_TOOLBAR), wxNullBitmap, wxITEM_NORMAL, wxT("Add a new Download"), wxEmptyString);
	toolbar->AddTool(id_toolbar_delete, wxT("Delete"), wxArtProvider::GetBitmap(wxART_MAKE_ART_ID_FROM_STR(_T("wxART_DEL_BOOKMARK")),wxART_TOOLBAR), wxNullBitmap, wxITEM_NORMAL, wxT("Delete the selected Download"), wxEmptyString);
	toolbar->AddSeparator();

	toolbar->AddTool(id_toolbar_deactivate, wxT("Deactivate"), wxArtProvider::GetBitmap(wxART_MAKE_ART_ID_FROM_STR(_T("wxART_CROSS_MARK")),wxART_TOOLBAR), wxNullBitmap, wxITEM_NORMAL, wxT("Deactivate the selected Download"), wxEmptyString);
	toolbar->AddTool(id_toolbar_activate, wxT("Activate"), wxArtProvider::GetBitmap(wxART_MAKE_ART_ID_FROM_STR(_T("wxART_TICK_MARK")),wxART_TOOLBAR), wxNullBitmap, wxITEM_NORMAL, wxT("Activate the selected Download"), wxEmptyString);

	toolbar->AddSeparator();
	toolbar->AddTool(id_toolbar_configure, wxT("Configure"), wxArtProvider::GetBitmap(wxART_MAKE_ART_ID_FROM_STR(_T("wxART_HELP_SETTINGS")),wxART_TOOLBAR), wxNullBitmap, wxITEM_NORMAL, wxT("Configure DownloadDaemon Server"), wxEmptyString);
	toolbar->AddSeparator();

	download_activate = toolbar->AddTool(id_toolbar_download_activate, wxT("Activate Downloading"), wxArtProvider::GetBitmap(wxART_MAKE_ART_ID_FROM_STR(_T("wxART_NEW")),wxART_TOOLBAR), wxNullBitmap, wxITEM_NORMAL, wxT("Activate Downloading"), wxEmptyString);
	download_deactivate = toolbar->AddTool(id_toolbar_download_deactivate, wxT("Deactivate Downloading"), wxArtProvider::GetBitmap(wxART_MAKE_ART_ID_FROM_STR(_T("wxART_CUT")),wxART_TOOLBAR), wxNullBitmap, wxITEM_NORMAL, wxT("Deactivate Downloading"), wxEmptyString);


	toolbar->Realize();
	SetToolBar(toolbar);


	// statusbar
	CreateStatusBar(2);
	SetStatusText(wxT("DownloadDaemon-ClientWX"),0);
	SetStatusText(wxT("Not connected"),1);

	return;
}


void myframe::add_content(){

	panel_downloads = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
	sizer_downloads = new wxBoxSizer(wxHORIZONTAL); // lines/rows, colums, vgap, hgap
	panel_downloads->SetSizer(sizer_downloads);

	// all download lists
	list = new wxListCtrl(panel_downloads, wxID_ANY, wxPoint(120, -46), wxDefaultSize, wxLC_REPORT);
	sizer_downloads->Add(list , 1, wxALL|wxEXPAND|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);


	// columns
	list->InsertColumn(0, wxT("ID"), 0, 50);
	list->InsertColumn(1, wxT("Added"), 0, 190);
	list->InsertColumn(2, wxT("Title"), 0, 100);
	list->InsertColumn(3, wxT("\tURL\t"), 0, 200);
	list->InsertColumn(4, wxT("Status"), 0, 80);

	return;
}


void myframe::fill_list(){
	while(true){ // for boost::thread
		if(mysock == NULL || !*mysock){
			SetStatusText(wxT("Not connected"),1);
			sleep(2);
			continue;
		}

		SetStatusText(wxT("Connected"),1);

		vector<vector<string> > new_content;
		vector<string> splitted_line;
		string answer, line, tab;
		size_t lineend = 1, tabend = 1, column_nr, line_nr = 0;

		mx.lock();

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

		compare_vectorvector(new_content.begin(), new_content.end());
		content = new_content;

		mx.unlock();

		sleep(2); // reload every two seconds
	}
	return;
}


string myframe::build_status(string &status_text, vector<string> &splitted_line){
	string color = "WHITE";

	if(splitted_line[4] == "DOWNLOAD_RUNNING"){
		color = "LIME GREEN";

		if(atoi(splitted_line[7].c_str()) > 0){ // waiting time > 0
			status_text = "Download running. Waiting " + splitted_line[7] + " seconds.";

		}else{ // no waiting time
			stringstream stream_buffer;
			stream_buffer << "Download Running: ";

			if(splitted_line[6] == "0"){ // download size unknown
				stream_buffer << "0.00% - ";

				if(splitted_line[5] == "0") // nothing downloaded yet
					stream_buffer << "0.00 MB/ 0.00 MB";
				else // something downloaded
					stream_buffer << setprecision(3) << (float)atoi(splitted_line[5].c_str()) / 1048576 << " MB/ 0.00 MB";

			}else{ // download size known
				if(splitted_line[5] == "0") // nothing downloaded yet
					stream_buffer << "0.00% - 0.00 MB/ " << (float)atoi(splitted_line[6].c_str()) / 1048576 << " MB";

				else{ // download size known and something downloaded
					stream_buffer << setprecision(3) << (float)atoi(splitted_line[5].c_str()) / (float)atoi(splitted_line[6].c_str()) * 100 << "% - ";
					stream_buffer << setprecision(3) << (float)atoi(splitted_line[5].c_str()) / 1048576 << " MB/ ";
					stream_buffer << setprecision(3) << (float)atoi(splitted_line[6].c_str()) / 1048576 << " MB";
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

		if (item_index == -1) // no selected ones left => leave loop
			break;
		else // found a selected one
			selected_lines.push_back(item_index);
	  }

	return;
}


// methods for comparing and actualizing content if necessary
void myframe::compare_vectorvector(vector<vector<string> >::iterator new_content_it, vector<vector<string> >::iterator new_content_end){

	vector<vector<string> >::iterator old_content_it = content.begin();
	size_t line_nr = 0, line_index;
	string status_text, color;


	// compare the i-th vector in content with the i-th vector in new_content
	while((new_content_it < new_content_end) && (old_content_it < content.end())){
		compare_vector(line_nr, *new_content_it, old_content_it->begin(), old_content_it->end());

		new_content_it++;
		old_content_it++;
		line_nr++;
	}


	if(new_content_it < new_content_end){ // there are more new lines then old ones

		while(new_content_it < new_content_end){

			// insert content
			line_index = list->InsertItem(line_nr, wxString((*new_content_it)[0].c_str(), wxConvUTF8));

			for(int i=1; i<4; i++) // column 1 to 3
				list->SetItem(line_index, i, wxString((*new_content_it)[i].c_str(), wxConvUTF8));

			// status column
			color = build_status(status_text, *new_content_it);
			list->SetItemBackgroundColour(line_index, wxString(color.c_str(), wxConvUTF8));
			list->SetItem(line_index, 4, wxString(status_text.c_str(), wxConvUTF8));

			new_content_it++;
			line_nr++;
		}

	}else if(old_content_it < content.end()){ // there are more old lines then new ones
		while(old_content_it < content.end()){

			// delete content
			list->DeleteItem(line_nr);

			old_content_it++;
			// line_nr stays the same!
		}
	}

	return;
}


void myframe::compare_vector(size_t line_nr, vector<string> &splitted_line_new, vector<string>::iterator it_old, vector<string>::iterator end_old){
	vector<string>::iterator it_new = splitted_line_new.begin();
	vector<string>::iterator end_new = splitted_line_new.end();

	size_t column_nr = 0;
	bool status_change = false;
	string status_text, color;

	// compare every column
	while((it_new < end_new) && (it_old < end_old)){

		if(*it_new != *it_old){ // content of column_new != content of column_old

			if(column_nr < 4) // direct input for columns 0 to 3
				list->SetItem(line_nr, column_nr, wxString((*it_new).c_str(), wxConvUTF8));

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

	return;
}


// event handle methods
void myframe::on_quit(wxCommandEvent &event){
	Close();
	return;
}


void myframe::on_about(wxCommandEvent &event){
	about_dialog dialog(working_dir, this);
	dialog.ShowModal();

	return;
}


 void myframe::on_connect(wxCommandEvent &event){
	connect_dialog dialog(this);
	dialog.ShowModal();

	return;
 }


void myframe::on_add(wxCommandEvent &event){
	if(mysock == NULL || !*mysock){ // if there is no active connection
		wxMessageBox(wxT("Please connect before adding Downloads."), wxT("No Connection to Server"));
		SetStatusText(wxT("Not connected"),1);

	}else{
		add_dialog dialog(this);
		dialog.ShowModal();
	}

	return;
 }


void myframe::on_delete(wxCommandEvent &event){
	vector<int>::iterator it;
	string id, answer;
	bool error_occured = false;

	if(mysock == NULL || !*mysock){ // if there is no active connection
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

				for(it = selected_lines.begin(); it<selected_lines.end(); it++){
					id = (content[*it])[0]; // gets the id of the line, which index is stored in selected_lines

					// test if there is a file on the server
					mysock->send("DDP FILE GETPATH " + id);
					mysock->recv(answer);

					if(!answer.empty()){ // file exists

						string question = "Do you want to delete the downloaded File for Download ID " + id + "?";
						wxMessageDialog file_dialog(this, wxString(question.c_str(), wxConvUTF8), wxT("Delete File"), wxYES_NO|wxYES_DEFAULT|wxICON_EXCLAMATION);
						int del_file = file_dialog.ShowModal();

						if(del_file == wxID_YES){ // user clicked yes to delete
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

		mx.unlock();

		if(error_occured)
			wxMessageBox(wxT("Error occured at deleting Download(s)."), wxT("Error"));
	}

	return;
 }


void myframe::on_deactivate(wxCommandEvent &event){
	vector<int>::iterator it;
	string id, answer;

	if(mysock == NULL || !*mysock){ // if there is no active connection
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
	}

	return;
 }


void myframe::on_activate(wxCommandEvent &event){
	vector<int>::iterator it;
	string id, answer;

	if(mysock == NULL || !*mysock){ // if there is no active connection
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
	}

	return;
 }


void myframe::on_configure(wxCommandEvent &event){
	if(mysock == NULL || !*mysock){ // if there is no active connection
		wxMessageBox(wxT("Please connect before configurating\nDownloadDaemon Server."), wxT("No Connection to Server"));
		SetStatusText(wxT("Not connected"),1);

	}else{
		configure_dialog dialog(this);
		dialog.ShowModal();
	}

	return;
}


void myframe::on_download_activate(wxCommandEvent &event){
	if(mysock == NULL || !*mysock){ // if there is no active connection
		wxMessageBox(wxT("Please connect before activate Downloading."), wxT("No Connection to Server"));
		SetStatusText(wxT("Not connected"),1);

	}else{
		string answer;

		mx.lock();

		mysock->send("DDP VAR SET downloading_active = 1");
		mysock->recv(answer);

		mx.unlock();

		// update toolbar
		toolbar->RemoveTool(id_toolbar_download_activate);
		toolbar->RemoveTool(id_toolbar_download_deactivate);
		toolbar->AddTool(download_deactivate);
	}

	return;
}


void myframe::on_download_deactivate(wxCommandEvent &event){
	if(mysock == NULL || !*mysock){ // if there is no active connection
		wxMessageBox(wxT("Please connect before deactivate Downloading."), wxT("No Connection to Server"));
		SetStatusText(wxT("Not connected"),1);

	}else{
		string answer;

		mx.lock();

		mysock->send("DDP VAR SET downloading_active = 0");
		mysock->recv(answer);

		mx.unlock();

		// update toolbar
		toolbar->RemoveTool(id_toolbar_download_activate);
		toolbar->RemoveTool(id_toolbar_download_deactivate);
		toolbar->AddTool(download_activate);


	}

	return;
}


 void myframe::on_resize(wxSizeEvent &event){
    myframe::OnSize(event); // call default evt_size handler
    Refresh();

 	if(list != NULL){

		// change column widths on resize
		int width = GetClientSize().GetWidth();
		width -= 255; // minus width of fix sized columns

		list->SetColumnWidth(2, width/4); // only column 2 to 4, because 0 and 1 have fix sizes
		list->SetColumnWidth(3, width/2);
		list->SetColumnWidth(4, width/4);
 	}
	return;
 }


// getter and setter methods
void myframe::set_connection_attributes(tkSock *mysock, string password){
		this->mysock = mysock;
}


tkSock *myframe::get_connection_attributes(){
	return mysock;
}


boost::mutex *myframe::get_mutex(){
	return &mx;
}

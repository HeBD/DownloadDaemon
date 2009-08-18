/***************************************************************
 * Name:	  ddclient-wx_main.cpp
 * Purpose:   Code for Frame Class
 * Author:	ko ()
 * Created:   2009-08-04
 * Copyright: ko ()
 * License:
 **************************************************************/

#include "ddclient-wx_main.h"


// IDs
const long myframe::id_menu_quit = wxNewId();
const long myframe::id_menu_about = wxNewId();
const long myframe::id_toolbar_connect = wxNewId();
const long myframe::id_toolbar_add = wxNewId();
const long myframe::id_toolbar_delete = wxNewId();
const long myframe::id_toolbar_stop = wxNewId();
const long myframe::id_toolbar_start = wxNewId();


// event table (has to be after creating IDs, won't work otherwise)
BEGIN_EVENT_TABLE(myframe, wxFrame)
	EVT_MENU(id_menu_quit, myframe::on_quit)
	EVT_MENU(id_menu_about, myframe::on_about)
	EVT_MENU(id_toolbar_connect, myframe::on_connect)
	EVT_MENU(id_toolbar_add, myframe::on_add)
	EVT_MENU(id_toolbar_delete, myframe::on_delete)
	EVT_MENU(id_toolbar_stop, myframe::on_stop)
	EVT_MENU(id_toolbar_start, myframe::on_start)
	EVT_SIZE(myframe::on_resize)
END_EVENT_TABLE()


myframe::myframe(wxChar *parameter, wxWindow *parent, const wxString &title, wxWindowID id):
	wxFrame(parent, id, title){

	working_dir = parameter;
	working_dir = working_dir.substr(0, working_dir.find_last_of(wxT("/")));
	working_dir = working_dir.substr(0, working_dir.find_last_of(wxT("/")));
	working_dir += wxT("/");

	SetClientSize(wxSize(750,500));
	SetMinSize(wxSize(750,500));
	CenterOnScreen();
	//SetIcon(); // later

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


void myframe::add_bars(){

	// menubar
	menu = new wxMenuBar();
	SetMenuBar(menu);

	file_menu = new wxMenu(_T(""));
	file_menu->Append(id_menu_quit, wxT("&Quit\tAlt-F4"), wxT("Quit"));
	menu->Append(file_menu, wxT("&File"));

	wxMenu *help_menu = new wxMenu(_T(""));
	help_menu->Append(id_menu_about, wxT("&About\tF1"), wxT("About"));
	menu->Append(help_menu, wxT("&Help"));


	// toolbar with icons
	toolbar = new wxToolBar(this, wxID_ANY);

	// wxToolBarToolBase *connect_item = // not needed
	toolbar->AddTool(id_toolbar_connect, wxT("Connect"), wxArtProvider::GetBitmap(wxART_MAKE_ART_ID_FROM_STR(_T("wxART_EXECUTABLE_FILE")),wxART_TOOLBAR), wxNullBitmap, wxITEM_NORMAL, wxT("Connect to a DownloadDaemon"), wxEmptyString);
	toolbar->AddSeparator();

	toolbar->AddTool(id_toolbar_add, wxT("Add"), wxArtProvider::GetBitmap(wxART_MAKE_ART_ID_FROM_STR(_T("wxART_ADD_BOOKMARK")),wxART_TOOLBAR), wxNullBitmap, wxITEM_NORMAL, wxT("Add a new Download"), wxEmptyString);
	toolbar->AddTool(id_toolbar_delete, wxT("Delete"), wxArtProvider::GetBitmap(wxART_MAKE_ART_ID_FROM_STR(_T("wxART_DEL_BOOKMARK")),wxART_TOOLBAR), wxNullBitmap, wxITEM_NORMAL, wxT("Delete the selected Download"), wxEmptyString);
	toolbar->AddSeparator();

	toolbar->AddTool(id_toolbar_stop, wxT("Stop"), wxArtProvider::GetBitmap(wxART_MAKE_ART_ID_FROM_STR(_T("wxART_CROSS_MARK")),wxART_TOOLBAR), wxNullBitmap, wxITEM_NORMAL, wxT("Stop downloading files"), wxEmptyString);
	toolbar->AddTool(id_toolbar_start, wxT("Start"), wxArtProvider::GetBitmap(wxART_MAKE_ART_ID_FROM_STR(_T("wxART_TICK_MARK")),wxART_TOOLBAR), wxNullBitmap, wxITEM_NORMAL, wxT("Start downloading files"), wxEmptyString);

	toolbar->Realize();
	SetToolBar(toolbar);


	// statusbar
	CreateStatusBar(2);
	SetStatusText(wxT("DownloadDaemon-ClientWX"),0);
	SetStatusText(wxT(".."),1);

	return;
}


void myframe::add_content(){

	panel_downloads = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
	sizer_downloads = new wxBoxSizer(wxHORIZONTAL); // lines/rows, colums, vgap, hgap
	panel_downloads->SetSizer(sizer_downloads);

	// all download lists
	list = new wxListCtrl(panel_downloads, wxID_ANY, wxPoint(120, -46), wxSize(0,0), wxLC_REPORT);
	sizer_downloads->Add(list , 1, wxALL|wxEXPAND|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);


	// columns
	list->InsertColumn(0, wxT("ID"), wxLIST_AUTOSIZE_USEHEADER, 50);
	list->InsertColumn(1, wxT("Added"), wxLIST_AUTOSIZE_USEHEADER, 190);
	list->InsertColumn(2, wxT("Title"), wxLIST_AUTOSIZE_USEHEADER, 100);
	list->InsertColumn(3, wxT("\tURL\t"), wxLIST_AUTOSIZE_USEHEADER, 200);
	list->InsertColumn(4, wxT("Status"), wxLIST_AUTOSIZE_USEHEADER, 80);

	return;
}


void myframe::fill_list(){
	while(true){ // for boost::thread
		if(mysock == NULL || !*mysock){
			sleep(2);
			continue;
		}

		vector<vector<string> > new_content;
		vector<string> splitted_line;
		string answer, line, tab;
		size_t lineend = 1, tabend = 1, column_nr, line_nr = 0;

		while(mx.Lock() != wxMUTEX_NO_ERROR){ // wait while the mutex can't be locked
			sleep(2);
		}

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

		mx.Unlock();

		sleep(2); // reload every two seconds
	}
	return;
}


string myframe::build_status(string &status_text, vector<string> &splitted_line){ // TODO: test every possible status
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
		if(splitted_line[8] == "NO_ERROR"){
			color = "YELLOW";
			status_text = "Download Inactive.";

		}else{ // error occured
			color = "RED";
			status_text = "Inactive. Error: " + splitted_line[8];
		}

	}else if(splitted_line[4] == "DOWNLOAD_PENDING"){
		if(splitted_line[8] == "NO_ERROR"){
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
		compare_vector(line_nr, *new_content_it, (*old_content_it).begin(), (*old_content_it).end());

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
			color = build_status(status_text, (*new_content_it));
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
	add_dialog dialog(this);
	dialog.ShowModal();

	return;
 }


void myframe::on_delete(wxCommandEvent &event){ // TODO: ERROR inside + add file delete popup
	vector<int>::iterator it;
	string id, answer;
	bool error_occured = false;

	if(mysock == NULL || !*mysock){ // if there is no active connection
		wxMessageBox(wxT("Please connect before deleting Downloads."), wxT("No Connection to Server"));

	}else{ // we have a connection

		while(mx.Lock() != wxMUTEX_NO_ERROR){ // while the mutex can't be locked
			sleep(1);
		}

		find_selected_lines(); // save selection into selected_lines

		for(it = selected_lines.begin(); it<selected_lines.end(); it++){
			id = (content[*it])[0]; // gets the id of the line, which index is stored in selected_lines

			// TODO : ask, if there should be a file delete (if there is a file)

			mysock->send("DDP DL DEL " + id);
			mysock->recv(answer);


			if(answer.find("104") == 0) // 104 ID <-- Entered a not-existing ID
				error_occured = true;

		}

		mx.Unlock();

		if(error_occured)
			wxMessageBox(wxT("Tried to delete an nonexisting ID."), wxT("Error"));
	}

	return;
 }


void myframe::on_stop(wxCommandEvent &event){ // TODO: realize
	wxMessageBox(wxT("\nDummy Dialog"), wxT("Dummy"));
	return;
 }


void myframe::on_start(wxCommandEvent &event){ // TODO: realize
	wxMessageBox(wxT("\nDummy Dialog"), wxT("Dummy"));
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


wxMutex *myframe::get_mutex(){
	return &mx;
}

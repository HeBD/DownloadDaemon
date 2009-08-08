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
END_EVENT_TABLE()


myframe::myframe(wxWindow *parent, const wxString &title, wxWindowID id,const wxPoint &pos,const wxSize &size): wxFrame(parent, -1, title){

	SetClientSize(wxSize(700,500));
	SetMinSize(wxSize(700,500));
	CenterOnScreen();
	//SetIcon(); // later

	add_bars();
	add_content();

	Layout();
	Fit();
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
	SetStatusText(wxT("I'm not useful atm..."),0);
	SetStatusText(wxT(".."),1);

	return;
}


void myframe::add_content(){

	notebook = new wxNotebook(this, wxID_ANY, wxPoint(88,48));
	panel_all = new wxPanel(notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
	panel_running = new wxPanel(notebook, wxNewId(), wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
	panel_finished = new wxPanel(notebook, wxNewId(), wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
	panel_all->SetFocus();

	sizer_all = new wxBoxSizer(wxHORIZONTAL); // lines/rows, colums, vgap, hgap
	sizer_running = new wxBoxSizer(wxHORIZONTAL);
	sizer_finished = new wxBoxSizer(wxHORIZONTAL);

	panel_all->SetSizer(sizer_all);
	panel_running->SetSizer(sizer_running);
	panel_finished->SetSizer(sizer_finished);

	notebook->AddPage(panel_all, wxT("All Downloads"), false);
	notebook->AddPage(panel_running, wxT("Running Downloads"), false);
	notebook->AddPage(panel_finished, wxT("Finished Downloads"), false);


	// all download lists
	list[0] = new wxListCtrl(panel_all, wxID_ANY, wxPoint(120, -46), wxSize(0,0), wxLC_REPORT);
	sizer_all->Add(list[0] , 1, wxALL|wxEXPAND|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
	list[1] = new wxListCtrl(panel_running, wxID_ANY, wxPoint(120, -46), wxSize(0,0), wxLC_REPORT);
	sizer_running->Add(list[1] , 1, wxALL|wxEXPAND|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
	list[2] = new wxListCtrl(panel_finished, wxID_ANY, wxPoint(120, -46), wxSize(0,0), wxLC_REPORT);
	sizer_finished->Add(list[2] , 1, wxALL|wxEXPAND|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);


	int num = 0; // for dummy input

	for(int i=0; i<3; i++){
		// columns
		list[i]->InsertColumn(0, wxT("ID"), wxLIST_AUTOSIZE_USEHEADER, -1);
		list[i]->InsertColumn(1, wxT("Added"), wxLIST_AUTOSIZE_USEHEADER, 50);
		list[i]->InsertColumn(2, wxT("Title"), wxLIST_AUTOSIZE_USEHEADER, 150);
		list[i]->InsertColumn(3, wxT("\tURL\t"), wxLIST_AUTOSIZE_USEHEADER, 150);
		list[i]->InsertColumn(4, wxT("Status"), wxLIST_AUTOSIZE_USEHEADER, 100);

		// dummy input
		wxString buffer;

		int index;

		for(int j=0; j<3; j++){
				buffer.Printf(wxT("Item %d"), ++num);
				index = list[i]->InsertItem(0, buffer);

				for(int k=1; k<5; k++){

						buffer.Printf(wxT("Col %d, item %d"), k, num);
						list[i]->SetItem(index, k, buffer);
				}
		}
	}

	return;
}


void myframe::fill_lists(){ //TODO: not finished
	std::string answer, line, tab;
	size_t lineend = 1, tabend = 1, column_nr, line_index = 0, line_nr = 0;

	mysock->send("DDP DL LIST");
	mysock->recv(answer);

	// delete old input
	for(int i=0; i<3; i++)
		list[i]->DeleteAllItems();

	// parse lines
	while(answer.length() > 0 && lineend != std::string::npos){
		lineend = answer.find("\n"); // termination character for line
		line = answer.substr(0, lineend);
		answer = answer.substr(lineend+1);

		// parse columns
		column_nr = 0;
		tabend = 0;
		while(line.length() > 0 && tabend != std::string::npos){
			tabend = line.find("|"); // termination character for column

			if(tabend == std::string::npos){ // no | found, so it is the last column
				tab = line;
				line = "";
			}else{
				if(tabend != 0 && line.at(tabend-1) == '\\') // because titles can have | inside (will be escaped with \)
					tabend = line.find("|", tabend+1);

				tab = line.substr(0, tabend);

				line = line.substr(tabend+1);
			}

			// fill columns
			if(column_nr == 0){
				line_index = list[0]->InsertItem(line_nr, wxString(tab.c_str(), wxConvUTF8)); //TODO: just list 0 atm (all downloads) //careful: has to be inserted in the bottom line!!!
			}else if(column_nr < 4){
				list[0]->SetItem(line_index, column_nr, wxString(tab.c_str(), wxConvUTF8)); //TODO: just list 0 atm (all downloads)
			}else{ continue; }
			column_nr++;
		}
		line_nr++;
	} //kills the programm, find mistake!

	return;
}


// event handle methods
void myframe::on_quit(wxCommandEvent &event){
	Close();
	return;
}


void myframe::on_about(wxCommandEvent &event){ // TODO: insert more meaningful info
	wxMessageBox(wxT("\nDownloadDaemon-ClientWX\n\nVersion 0.00000001~"), wxT("About"));
	return;
}


 void myframe::on_connect(wxCommandEvent &event){
	connect_dialog dialog(this);
	dialog.ShowModal();

	return;
 }


void myframe::on_add(wxCommandEvent &event){ // TODO: realize
	wxMessageBox(wxT("\nDummy Dialog"), wxT("Dummy"));
	return;
 }

void myframe::on_delete(wxCommandEvent &event){ // TODO: realize
	wxMessageBox(wxT("\nDummy Dialog"), wxT("Dummy"));
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


// getter and setter methods
void myframe::set_connection_attributes(tkSock *mysock, std::string password){
		this->mysock = mysock;
}

tkSock *myframe::get_connection_attributes(){
	return mysock;
}

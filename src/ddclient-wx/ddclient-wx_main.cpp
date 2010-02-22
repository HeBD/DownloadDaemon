/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 * GNU General Public License for more details.
 */

#include <crypt/md5.h>
#include <downloadc/client_exception.h>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <iomanip>
#include <cstdlib>
#include <climits>
#include <fstream>
#include <cstdarg>

#include "ddclient-wx_main.h"
#include "ddclient-wx_connect_dialog.h"
#include "ddclient-wx_about_dialog.h"
#include "ddclient-wx_add_dialog.h"
#include "ddclient-wx_configure_dialog.h"
#include "ddclient-wx_delete_dialog.h"

#include <wx/msgdlg.h> // for wxmessagebox
#include <wx/stattext.h>
#include <wx/menu.h>
#include <wx/artprov.h>
#include <wx/string.h>
#include <wx/gdicmn.h> // color database
#include <wx/string.h>
#include <wx/filename.h>
#include <wx/file.h>
#include <wx/clipbrd.h>
#include <wx/dataobj.h>
#include <wx/statusbr.h>

#ifdef _WIN32
	#define sleep(x) Sleep(x * 1000)
#endif


// IDs
const long myframe::id_menu_about = wxNewId();
const long myframe::id_menu_select_all_lines = wxNewId();
const long myframe::id_toolbar_connect = wxNewId();
const long myframe::id_toolbar_add = wxNewId();
const long myframe::id_toolbar_delete = wxNewId();
const long myframe::id_toolbar_delete_finished = wxNewId();
const long myframe::id_menu_delete_file = wxNewId();
const long myframe::id_toolbar_activate = wxNewId();
const long myframe::id_toolbar_deactivate = wxNewId();
const long myframe::id_toolbar_priority_up = wxNewId();
const long myframe::id_toolbar_priority_down = wxNewId();
const long myframe::id_toolbar_configure = wxNewId();
const long myframe::id_toolbar_download_activate = wxNewId();
const long myframe::id_toolbar_download_deactivate = wxNewId();
const long myframe::id_toolbar_copy_url = wxNewId();
const long myframe::id_right_click = wxNewId();


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
	EVT_MENU(id_toolbar_delete_finished, myframe::on_delete_finished)
	EVT_MENU(id_menu_delete_file, myframe::on_delete_file)
	EVT_MENU(id_toolbar_activate, myframe::on_activate)
	EVT_MENU(id_toolbar_deactivate, myframe::on_deactivate)
	EVT_MENU(id_toolbar_priority_up, myframe::on_priority_up)
	EVT_MENU(id_toolbar_priority_down, myframe::on_priority_down)
	EVT_MENU(id_toolbar_configure, myframe::on_configure)
	EVT_MENU(id_toolbar_download_activate, myframe::on_download_activate)
	EVT_MENU(id_toolbar_download_deactivate, myframe::on_download_deactivate)
	EVT_MENU(id_toolbar_copy_url, myframe::on_copy_url)
	EVT_SIZE(myframe::on_resize)
	EVT_CUSTOM(wxEVT_reload_list, wxID_ANY, myframe::on_reload)
	EVT_CONTEXT_MENU(myframe::on_right_click)
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
			curr_path = env_path.substr(last_pos, curr_pos - last_pos);
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

	// getting config dir
	#ifdef _WIN32
		wxGetEnv(wxT("APPDATA"), &config_dir);
		config_dir += wxT("/ddclient-wx/");
	#else
		wxGetEnv(wxT("HOME"), &config_dir);
		config_dir += wxT("/.ddclient-wx/");
	#endif
	if(!wxFileName::DirExists(config_dir))
		wxFileName::Mkdir(config_dir, 0755);


	SetClientSize(wxSize(750,500));
	SetMinSize(wxSize(750,500));
	CenterOnScreen();
	SetIcon(wxIcon(wxT("img/logoDD.png")));

	add_bars();
	add_components();
	Layout();
	Fit();

	lang.set_working_dir(std::string(working_dir.mb_str()) + "lang/");

	mysock = new tkSock();

	// connect if logindata was saved
	string file_name = string(config_dir.mb_str()) + "save.dat";
	ifstream ifs(file_name.c_str(), fstream::in | fstream::binary); // open file

	if(ifs.good()){ // file successfully opened
		login_data last_data =  { "", 0, ""};

		ifs.read((char *) &last_data, sizeof(login_data));

		if(last_data.lang[0] != '\0') // older versions of save.dat won't have lang, so we have to check
			set_language(last_data.lang); // set program language


		// try to connect with the data read from file
		tkSock *mysock_tmp = new tkSock();
		bool connection = false, error_occured = false;

		try{
		   connection = mysock_tmp->connect(last_data.host, last_data.port);
		}catch(...){} // no code needed here due to boolean connection


		if(connection){ // connection succeeded,  host (IP/URL or port) is ok

			// authentification
			std::string snd;
			mysock_tmp->recv(snd);

			if(snd.find("100") == 0){ // 100 SUCCESS <-- Operation succeeded
				// nothing to do here if you reach this
			}else if(snd.find("102") == 0){ // 102 AUTHENTICATION <-- Authentication failed

				// try md5 authentication
				mysock_tmp->send("ENCRYPT");
				std::string rnd;
				mysock_tmp->recv(rnd); // random bytes

				if(rnd.find("102") != 0) { // encryption permitted
					rnd += last_data.pass;

					MD5_CTX md5;
					MD5_Init(&md5);

					unsigned char *enc_data = new unsigned char[rnd.length()];
					for(size_t i = 0; i < rnd.length(); ++i){ // copy random bytes from string to cstring
						enc_data[i] = rnd[i];
					}

					MD5_Update(&md5, enc_data, rnd.length());
					unsigned char result[16];
					MD5_Final(result, &md5); // put md5hash in result
					std::string enc_passwd((char*)result, 16);
					delete [] enc_data;

					mysock_tmp->send(enc_passwd);
					mysock_tmp->recv(snd);

				}else{ // encryption not permitted
					wxMessageDialog dialog(this, tsl("Encrypted authentication not supported by server.")
											+ wxT("/n") + tsl("Do you want to try unsecure plain-text authentication?"),
										   tsl("Auto Connection: No Encryption Supported"), wxYES_NO|wxYES_DEFAULT|wxICON_EXCLAMATION);
					int del = dialog.ShowModal();

					if(del == wxID_YES){ // user clicked yes
						// reconnect
						try{
							connection = mysock_tmp->connect(last_data.host, last_data.port);
						}catch(...){} // no code needed here due to boolean connection

						if(connection){
							mysock_tmp->recv(snd);
							mysock_tmp->send(last_data.pass);
							mysock_tmp->recv(snd);
						}else{
							error_occured = true;
						}
					}else{
						snd = "99"; // user doesn't want to connect without encryption => own error code
						error_occured = true;
					}
				}

				if(snd.find("100") == 0 && connection){
					// nothing to do here if you reach this

				}else if(snd.find("102") == 0 && connection){
					error_occured = true;

				}else if(snd.find("99") == 0 && connection){
					error_occured = true;

				}else{
					error_occured = true;
				}
			}else{
				error_occured = true;
			}


			if(!error_occured){
				// save socket and password
				mx.lock();

				if(mysock != NULL){ //if there is already a connection, delete the old one
					delete mysock;
					mysock = NULL;
				}

				mysock = mysock_tmp;
				password = last_data.pass;
				mx.unlock();
				update_status(wxString(last_data.host, wxConvUTF8));

			}else{
				delete mysock_tmp;
			}

		}else{ // connection failed due to host (IP/URL or port)
			delete mysock_tmp;
		}
	}

	boost::thread(boost::bind(&myframe::update_list, this));
}


myframe::~myframe(){
		mx.lock();
		if(mysock != NULL)
			delete mysock;
		mysock = NULL;
		mx.unlock();
}


void myframe::update_status(wxString server){
	mx.lock();

	if(mysock == NULL || !*mysock || mysock->get_peer_name() == ""){ // if there is no active connection
		// make sure mysock doesn't crash the program
		if(mysock != NULL)
			delete mysock;
		mysock = NULL;

		if(server != wxT("")){
			wxMessageBox(tsl("Please reconnect."), tsl("No Connection to Server"));
		}

	}else{
		string answer;

		mysock->send("DDP VAR GET downloading_active");
		mysock->recv(answer);

		// removing both icons, even when maybe only one is shown
		toolbar->RemoveTool(id_toolbar_download_activate);
		toolbar->RemoveTool(id_toolbar_download_deactivate);

		if(answer == "1"){ // downloading active
			toolbar->AddTool(download_deactivate);
			file_menu->Enable(id_toolbar_download_deactivate, true);
		}else if(answer =="0"){ // downloading not active
			toolbar->AddTool(download_activate);
			file_menu->Enable(id_toolbar_download_activate, true);
		}else{
			// should never be reached
		}

		toolbar->Realize();

		if(server != wxT("")){ // sometimes the function is called to update the toolbar, not the status text => server is empty
			wxString status_text = tsl("Connected to");
			status_text += wxT(" ");
			status_text += server;
			SetStatusText(status_text, 1);
		}
	}
	mx.unlock();
}

void myframe::add_bars(){
	menu = new wxMenuBar();
	SetMenuBar(menu);

	file_menu = new wxMenu(wxT(""));

	file_menu->Append(id_toolbar_connect, wxT("&") + tsl("Connect") + wxT("..\tAlt-C"), tsl("Connect"));
	file_menu->Append(id_toolbar_configure, wxT("&") + tsl("Configure") + wxT("..\tAlt-P"), tsl("Configure"));
	file_menu->Append(id_toolbar_download_activate, wxT("&") + tsl("Activate Downloading") + wxT("\tF2"), tsl("Activate Downloading"));
	file_menu->Append(id_toolbar_download_deactivate, wxT("&") + tsl("Deactivate Downloading") + wxT("\tF3"), tsl("Deactivate Downloading"));
	file_menu->Enable(id_toolbar_download_activate, false); // those two are not enabled from the start
	file_menu->Enable(id_toolbar_download_deactivate, false);
	file_menu->AppendSeparator();

	file_menu->Append(id_toolbar_activate, wxT("&") + tsl("Activate Download") + wxT("\tAlt-A"), tsl("Activate Download"));
	file_menu->Append(id_toolbar_deactivate, wxT("&") + tsl("Deactivate Download") + wxT("\tAlt-D"), tsl("Deactivate Download"));
	file_menu->AppendSeparator();

	file_menu->Append(id_toolbar_add, wxT("&") + tsl("Add Download") + wxT("..\tAlt-I"), tsl("Add Download"));
	file_menu->Append(id_toolbar_delete, wxT("&") + tsl("Delete Download") + wxT("\tDEL"), tsl("Delete Download"));
	file_menu->Append(id_toolbar_delete_finished, wxT("&") + tsl("Delete finished Downloads") + wxT("\tCtrl-DEL"), tsl("Delete finished Downloads"));
	file_menu->AppendSeparator();

	file_menu->Append(id_menu_select_all_lines, wxT("&") + tsl("Select all") + wxT("\tCtrl-A"), tsl("Select all"));
	file_menu->Append(id_toolbar_copy_url, wxT("&") + tsl("Copy URL") + wxT("\tCtrl-C"), tsl("Copy URL"));
	file_menu->AppendSeparator();

	file_menu->Append(wxID_EXIT, wxT("&") + tsl("Quit") + wxT("\tAlt-F4"), tsl("Quit"));
	menu->Append(file_menu, wxT("&") + tsl("File"));

	help_menu = new wxMenu(_T(""));
	help_menu->Append(id_menu_about, wxT("&") + tsl("About") + wxT("..\tF1"), tsl("About"));
	menu->Append(help_menu, wxT("&") + tsl("Help"));


	// toolbar with icons
	toolbar = new wxToolBar(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTB_HORIZONTAL|wxTB_FLAT);

	toolbar->AddTool(id_toolbar_connect, tsl("Connect"), wxBitmap(working_dir + wxT("img/1_connect.png"), wxBITMAP_TYPE_PNG), wxNullBitmap, wxITEM_NORMAL,
					tsl("Connect to a DownloadDaemon Server"), tsl("Connect to a DownloadDaemon Server"));
	toolbar->AddSeparator();

	toolbar->AddTool(id_toolbar_add, tsl("Add"), wxBitmap(working_dir + wxT("img/2_add.png"), wxBITMAP_TYPE_PNG), wxNullBitmap, wxITEM_NORMAL,
					tsl("Add a new Download"), tsl("Add a new Download"));
	toolbar->AddTool(id_toolbar_delete, tsl("Delete"), wxBitmap(working_dir + wxT("img/3_delete.png"), wxBITMAP_TYPE_PNG), wxNullBitmap, wxITEM_NORMAL,
					tsl("Delete the selected Download"), tsl("Delete the selected Download"));
	toolbar->AddTool(id_toolbar_delete_finished, tsl("Delete Finished Downloads"), wxBitmap(working_dir + wxT("img/10_delete_finished.png"), wxBITMAP_TYPE_PNG), wxNullBitmap,
					wxITEM_NORMAL, tsl("Delete all finished Downloads"), tsl("Delete all finished Downloads"));
	toolbar->AddSeparator();

	toolbar->AddTool(id_toolbar_activate, tsl("Activate"), wxBitmap(working_dir + wxT("img/5_start.png"), wxBITMAP_TYPE_PNG), wxNullBitmap, wxITEM_NORMAL,
					tsl("Activate the selected Download"), tsl("Activate the selected Download"));
	toolbar->AddTool(id_toolbar_deactivate, tsl("Deactivate"), wxBitmap(working_dir + wxT("img/4_stop.png"), wxBITMAP_TYPE_PNG), wxNullBitmap, wxITEM_NORMAL,
					tsl("Deactivate the selected Download"), tsl("Deactivate the selected Download"));
	toolbar->AddSeparator();

	toolbar->AddTool(id_toolbar_priority_up, tsl("Increase Priority"), wxBitmap(working_dir + wxT("img/6_up.png"), wxBITMAP_TYPE_PNG), wxNullBitmap, wxITEM_NORMAL,
					tsl("Increase Priority of the selected Download"), tsl("Increase Priority of the selected Download"));
	toolbar->AddTool(id_toolbar_priority_down, tsl("Decrease Priority"), wxBitmap(working_dir + wxT("img/7_down.png"), wxBITMAP_TYPE_PNG), wxNullBitmap, wxITEM_NORMAL,
					tsl("Decrease Priority of the selected Download"), tsl("Decrease Priority of the selected Download"));
	toolbar->AddSeparator();

	toolbar->AddTool(id_toolbar_configure, tsl("Configure"), wxBitmap(working_dir + wxT("img/8_configure.png"), wxBITMAP_TYPE_PNG), wxNullBitmap, wxITEM_NORMAL,
					tsl("Configure DownloadDaemon Server"), tsl("Configure DownloadDaemon Server"));
	toolbar->AddSeparator();

	download_activate = toolbar->AddTool(id_toolbar_download_activate, tsl("Activate Downloading"), wxBitmap(working_dir + wxT("img/9_activate.png"), wxBITMAP_TYPE_PNG),
										wxNullBitmap, wxITEM_NORMAL, tsl("Activate Downloading"), tsl("Activate Downloading"));
	download_deactivate = toolbar->AddTool(id_toolbar_download_deactivate, tsl("Deactivate Downloading"), wxBitmap(working_dir + wxT("img/9_deactivate.png"), wxBITMAP_TYPE_PNG),
											wxNullBitmap, wxITEM_NORMAL, tsl("Deactivate Downloading"), tsl("Deactivate Downloading"));

	toolbar->Realize();

	toolbar->RemoveTool(id_toolbar_download_activate); // these two toolbar icons are created for later use
	toolbar->RemoveTool(id_toolbar_download_deactivate);

	SetToolBar(toolbar);

	// statusbar
	status_bar = new wxStatusBar(this);
	status_bar->SetFieldsCount(2);
	status_bar->SetStatusText(tsl("DownloadDaemon Client-wx"), 0);
	status_bar->SetStatusText(tsl("Not connected"), 1);
	SetStatusBar(status_bar);
	update_status(wxT(""));
}


void myframe::update_bars(){
	//mx.lock();

	// update menu entrys
	file_menu->SetLabel(id_toolbar_connect, wxT("&") + tsl("Connect") + wxT("..\tAlt-C"));
	file_menu->SetHelpString(id_toolbar_connect, tsl("Connect"));

	file_menu->SetLabel(id_toolbar_configure, wxT("&") + tsl("Configure") + wxT("..\tAlt-P"));
	file_menu->SetHelpString(id_toolbar_configure, tsl("Configure"));

	file_menu->SetLabel(id_toolbar_download_activate, wxT("&") + tsl("Activate Downloading") + wxT("\tF2"));
	file_menu->SetHelpString(id_toolbar_download_activate, tsl("Activate Downloading"));
	file_menu->SetLabel(id_toolbar_download_deactivate, wxT("&") + tsl("Deactivate Downloading") + wxT("\tF3"));
	file_menu->SetHelpString(id_toolbar_download_deactivate, tsl("Deactivate Downloading"));

	file_menu->SetLabel(id_toolbar_activate, wxT("&") + tsl("Activate Download") + wxT("\tAlt-A"));
	file_menu->SetHelpString(id_toolbar_activate, tsl("Activate Download"));
	file_menu->SetLabel(id_toolbar_deactivate, wxT("&") + tsl("Deactivate Download") + wxT("\tAlt-D"));
	file_menu->SetHelpString(id_toolbar_deactivate, tsl("Deactivate Download"));

	file_menu->SetLabel(id_toolbar_add, wxT("&") + tsl("Add Download") + wxT("..\tAlt-I"));
	file_menu->SetHelpString(id_toolbar_add, tsl("Add Download"));
	file_menu->SetLabel(id_toolbar_delete, wxT("&") + tsl("Delete Download") + wxT("\tDEL"));
	file_menu->SetHelpString(id_toolbar_delete, tsl("Delete Download"));
	file_menu->SetLabel(id_toolbar_delete_finished, wxT("&") + tsl("Delete finished Downloads") + wxT("\tCtrl-DEL"));
	file_menu->SetHelpString(id_toolbar_delete_finished, tsl("Delete finished Downloads"));

	file_menu->SetLabel(id_menu_select_all_lines, wxT("&") + tsl("Select all") + wxT("\tCtrl-A"));
	file_menu->SetHelpString(id_menu_select_all_lines, tsl("Select all"));
	file_menu->SetLabel(id_toolbar_copy_url, wxT("&") + tsl("Copy URL") + wxT("\tCtrl-C"));
	file_menu->SetHelpString(id_toolbar_copy_url, tsl("Copy URL"));

	file_menu->SetLabel(wxID_EXIT, wxT("&") + tsl("Quit") + wxT("\tAlt-F4"));
	file_menu->SetHelpString(wxID_EXIT, tsl("Quit"));

	help_menu->SetLabel(id_menu_about, wxT("&") + tsl("About") + wxT("..\tF1"));
	help_menu->SetHelpString(id_menu_about, tsl("About"))	;

	menu->SetLabelTop(0, wxT("&") + tsl("File"));
	menu->SetLabelTop(1, wxT("&") + tsl("Help"));


	// recreate icons
	toolbar->ClearTools();

	toolbar->AddTool(id_toolbar_connect, tsl("Connect"), wxBitmap(working_dir + wxT("img/1_connect.png"), wxBITMAP_TYPE_PNG), wxNullBitmap, wxITEM_NORMAL,
					tsl("Connect to a DownloadDaemon Server"), tsl("Connect to a DownloadDaemon Server"));
	toolbar->AddSeparator();

	toolbar->AddTool(id_toolbar_add, tsl("Add"), wxBitmap(working_dir + wxT("img/2_add.png"), wxBITMAP_TYPE_PNG), wxNullBitmap, wxITEM_NORMAL,
					tsl("Add a new Download"), tsl("Add a new Download"));
	toolbar->AddTool(id_toolbar_delete, tsl("Delete"), wxBitmap(working_dir + wxT("img/3_delete.png"), wxBITMAP_TYPE_PNG), wxNullBitmap, wxITEM_NORMAL,
					tsl("Delete the selected Download"), tsl("Delete the selected Download"));
	toolbar->AddTool(id_toolbar_delete_finished, tsl("Delete Finished Downloads"), wxBitmap(working_dir + wxT("img/10_delete_finished.png"), wxBITMAP_TYPE_PNG), wxNullBitmap,
					wxITEM_NORMAL, tsl("Delete all finished Downloads"), tsl("Delete all finished Downloads"));
	toolbar->AddSeparator();

	toolbar->AddTool(id_toolbar_activate, tsl("Activate"), wxBitmap(working_dir + wxT("img/5_start.png"), wxBITMAP_TYPE_PNG), wxNullBitmap, wxITEM_NORMAL,
					tsl("Activate the selected Download"), tsl("Activate the selected Download"));
	toolbar->AddTool(id_toolbar_deactivate, tsl("Deactivate"), wxBitmap(working_dir + wxT("img/4_stop.png"), wxBITMAP_TYPE_PNG), wxNullBitmap, wxITEM_NORMAL,
					tsl("Deactivate the selected Download"), tsl("Deactivate the selected Download"));
	toolbar->AddSeparator();

	toolbar->AddTool(id_toolbar_priority_up, tsl("Increase Priority"), wxBitmap(working_dir + wxT("img/6_up.png"), wxBITMAP_TYPE_PNG), wxNullBitmap, wxITEM_NORMAL,
					tsl("Increase Priority of the selected Download"), tsl("Increase Priority of the selected Download"));
	toolbar->AddTool(id_toolbar_priority_down, tsl("Decrease Priority"), wxBitmap(working_dir + wxT("img/7_down.png"), wxBITMAP_TYPE_PNG), wxNullBitmap, wxITEM_NORMAL,
					tsl("Decrease Priority of the selected Download"), tsl("Decrease Priority of the selected Download"));
	toolbar->AddSeparator();

	toolbar->AddTool(id_toolbar_configure, tsl("Configure"), wxBitmap(working_dir + wxT("img/8_configure.png"), wxBITMAP_TYPE_PNG), wxNullBitmap, wxITEM_NORMAL,
					tsl("Configure DownloadDaemon Server"), tsl("Configure DownloadDaemon Server"));
	toolbar->AddSeparator();

	download_activate = toolbar->AddTool(id_toolbar_download_activate, tsl("Activate Downloading"), wxBitmap(working_dir + wxT("img/9_activate.png"), wxBITMAP_TYPE_PNG),
										wxNullBitmap, wxITEM_NORMAL, tsl("Activate Downloading"), tsl("Activate Downloading"));
	download_deactivate = toolbar->AddTool(id_toolbar_download_deactivate, tsl("Deactivate Downloading"), wxBitmap(working_dir + wxT("img/9_deactivate.png"), wxBITMAP_TYPE_PNG),
											wxNullBitmap, wxITEM_NORMAL, tsl("Deactivate Downloading"), tsl("Deactivate Downloading"));

	toolbar->Realize();

	toolbar->RemoveTool(id_toolbar_download_activate); // these two toolbar icons are created for later use
	toolbar->RemoveTool(id_toolbar_download_deactivate);

	//mx.unlock();
	update_status(wxT(""));

}


void myframe::add_components(){
	panel_downloads = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
	sizer_downloads = new wxBoxSizer(wxHORIZONTAL);
	panel_downloads->SetSizer(sizer_downloads);

	// all download lists
	list = new wxListCtrl(panel_downloads, wxID_ANY, wxPoint(120, -46), wxSize(0,0), wxLC_REPORT);
	sizer_downloads->Add(list , 1, wxALL|wxEXPAND|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);

	// columns
	list->InsertColumn(0, tsl("ID"), wxLIST_AUTOSIZE_USEHEADER, 50);
	list->InsertColumn(1, tsl("Title"), wxLIST_AUTOSIZE_USEHEADER, 76);
	list->InsertColumn(2, tsl("URL"), wxLIST_AUTOSIZE_USEHEADER, 170);
	list->InsertColumn(3, tsl("Time left"), wxLIST_AUTOSIZE_USEHEADER, 100);
	list->InsertColumn(4, tsl("Status"), wxLIST_AUTOSIZE_USEHEADER, 150);
}


void myframe::update_components(){

	if(list != NULL){

		// change column widths on resize
		int width = GetClientSize().GetWidth();
		width -= 160; // minus width of fix sized columns

		#if defined(__WXMSW__)
			width -= 10;
		#endif // defined(__WXMSW__)

		/*list->DeleteColumn(4);
		list->DeleteColumn(3);
		list->DeleteColumn(2);
		list->DeleteColumn(1);
		list->DeleteColumn(0);*/
		list->ClearAll();

		list->InsertColumn(0, tsl("ID"), wxLIST_AUTOSIZE_USEHEADER, 50);
		list->InsertColumn(1, tsl("Title"), wxLIST_AUTOSIZE_USEHEADER, width*0.25);
		list->InsertColumn(2, tsl("URL"), wxLIST_AUTOSIZE_USEHEADER, width*0.3);
		list->InsertColumn(3, tsl("Time left"), wxLIST_AUTOSIZE_USEHEADER, 100);
		list->InsertColumn(4, tsl("Status"), wxLIST_AUTOSIZE_USEHEADER, width*0.45);

 	}

	mx.lock();
	content.clear(); // delete old content to force reload of list
	mx.unlock();
}


void myframe::update_list(){
	while(true){ // for boost::thread
		mx.lock();

		if(mysock == NULL || !*mysock || mysock->get_peer_name() == ""){
			// make sure mysock doesn't crash the program
			if(mysock != NULL)
				delete mysock;
			mysock = NULL;
			mx.unlock();

			sleep(2);
			continue;
		}else{

			mx.unlock();

			get_content();
			sleep(2); // reload every two seconds
		}
	}
}


void myframe::get_content(){
	mx.lock();

	if(mysock == NULL || !*mysock || mysock->get_peer_name() == ""){
		// make sure mysock doesn't crash the program
		if(mysock != NULL)
			delete mysock;
		mysock = NULL;

		mx.unlock();

	}else{

		vector<string> splitted_line;
		string answer, line, tab;
		size_t lineend = 1, tabend = 1;

		new_content.clear();

		mysock->send("DDP DL LIST");
		mysock->recv(answer);


		// parse lines
		while(answer.length() > 0 && lineend != string::npos){
			lineend = answer.find("\n"); // termination character for line
			line = answer.substr(0, lineend);
			answer = answer.substr(lineend+1);

			// parse columns
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

			new_content.push_back(splitted_line);
			splitted_line.clear();
		}

		mx.unlock();

		// send event to reload list
		wxCommandEvent event(wxEVT_reload_list, GetId());
		wxPostEvent(this, event);

	}
}


void myframe::cut_time(string &time_left){
	long time_span = atol(time_left.c_str());
	int hours = 0, mins = 0, secs = 0;
	stringstream stream_buffer;

	secs = time_span % 60;
	if(time_span >= 60) // cut time_span down to minutes
		time_span /= 60;
	else { // we don't have minutes
		stream_buffer << secs << " s";
		time_left = stream_buffer.str();
		return;
	}

	mins = time_span % 60;
	if(time_span >= 60) // cut time_span down to hours
		time_span /= 60;
	else { // we don't have hours
		stream_buffer << mins << ":";
		if(secs < 10)
			stream_buffer << "0";
		stream_buffer << secs << "m";
		time_left = stream_buffer.str();
		return;
	}

	hours = time_span;
	stream_buffer << hours << "h, ";
	if(mins < 10)
		stream_buffer << "0";
	stream_buffer << mins << ":";
	if(secs < 10)
		stream_buffer << "0";
	stream_buffer << secs << "m";

	time_left = stream_buffer.str();
	return;
}

string myframe::build_status(string &status_text, string &time_left, vector<string> &splitted_line){
	string color = "WHITE";

	if(splitted_line[4] == "DOWNLOAD_RUNNING"){
		color = "LIME GREEN";

		if(atol(splitted_line[7].c_str()) > 0 && splitted_line[8] == "PLUGIN_SUCCESS"){ // waiting time > 0
			status_text = lang["Download running. Waiting."];
			time_left =  splitted_line[7];
			cut_time(time_left);

		}else if(atol(splitted_line[7].c_str()) > 0 && splitted_line[8] != "PLUGIN_SUCCESS") {
			color = "RED";
			status_text = lang["Error"] + ": " + lang[splitted_line[8]] + " " + lang["Retrying soon."];
			time_left =  splitted_line[7];
			cut_time(time_left);

		}else{ // no waiting time
			stringstream stream_buffer, time_buffer;
			stream_buffer << lang["Running"];

			if(splitted_line[9] != "0" && splitted_line[9] != "-1") // download speed known
				stream_buffer << "@" << setprecision(1) << fixed << (float)atol(splitted_line[9].c_str()) / 1024 << " kb/s";

			stream_buffer << ": ";

			if(splitted_line[6] == "0" || splitted_line[6] == "1"){ // download size unknown
				stream_buffer << "0.00% - ";
				time_left = "";

				if(splitted_line[5] == "0" || splitted_line[5] == "1") // nothing downloaded yet
					stream_buffer << "0.00 MB/ 0.00 MB";
				else // something downloaded
					stream_buffer << setprecision(1) << fixed << (float)atol(splitted_line[5].c_str()) / 1048576 << " MB/ 0.00 MB";

			}else{ // download size known
				if(splitted_line[5] == "0" || splitted_line[5] == "1"){ // nothing downloaded yet
					stream_buffer << "0.00% - 0.00 MB/ " << fixed << (float)atol(splitted_line[6].c_str()) / 1048576 << " MB";

					if(splitted_line[9] != "0" && splitted_line[9] != "-1"){ // download speed known => calc time left
						time_buffer << (int)(atol(splitted_line[6].c_str()) / atol(splitted_line[9].c_str()));
						time_left = time_buffer.str();
						cut_time(time_left);
					}else
						time_left = "";

				}else{ // download size known and something downloaded
					stream_buffer << setprecision(1) << fixed << (float)atol(splitted_line[5].c_str()) / (float)atol(splitted_line[6].c_str()) * 100 << "% - ";
					stream_buffer << setprecision(1) << fixed << (float)atol(splitted_line[5].c_str()) / 1048576 << " MB/ ";
					stream_buffer << setprecision(1) << fixed << (float)atol(splitted_line[6].c_str()) / 1048576 << " MB";

					if(splitted_line[9] != "0" && splitted_line[9] != "-1"){ // download speed known => calc time left
						time_buffer << (int)((atol(splitted_line[6].c_str()) - atol(splitted_line[5].c_str())) / atol(splitted_line[9].c_str()));
						time_left = time_buffer.str();
						cut_time(time_left);
					}else
						time_left = "";
				}
			}
			status_text = stream_buffer.str();
		}

	}else if(splitted_line[4] == "DOWNLOAD_INACTIVE"){
		if(splitted_line[8] == "PLUGIN_SUCCESS"){
			color = "YELLOW";
			status_text = lang["Download Inactive."];
			time_left = "";

		}else{ // error occured
			color = "RED";
			status_text = lang["Inactive. Error"] + ": " + lang[splitted_line[8]];
			time_left = "";
		}

	}else if(splitted_line[4] == "DOWNLOAD_PENDING"){
		time_left = "";

		if(splitted_line[8] == "PLUGIN_SUCCESS"){
			status_text = lang["Download Pending."];

		}else{ //error occured
			color = "RED";
			status_text = "Error: " + lang[splitted_line[8]];
		}

	}else if(splitted_line[4] == "DOWNLOAD_WAITING"){
		color = "YELLOW";
		status_text = lang["Have to wait."];
		time_left = splitted_line[7];
		cut_time(time_left);

	}else if(splitted_line[4] == "DOWNLOAD_FINISHED"){
		color = "GREEN";
		status_text = lang["Download Finished."];
		time_left = "";

	}else if(splitted_line[4] == "DOWNLOAD_RECONNECTING") {
		color = "YELLOW";
		status_text = lang["Reconnecting..."];
		time_left = "";
	}else{ // default, column 4 has unknown input
		status_text = lang["Status not detected."];
		time_left = "";
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


wxString myframe::tsl(string text, ...){
	string translated = lang[text];

	size_t n;
	int i = 1;
	va_list vl;
	va_start(vl,text);
	string search("p1");


	while((n = translated.find(search)) != string::npos){
		translated.replace(n-1, 3, va_arg(vl, char *));

		i++;
		if(i>9) // maximal 9 additional Parameters
			break;

		stringstream sb;
		sb << "p" << i;
		search = sb.str();
	}

	//return wxString(lang[text].c_str(), wxConvUTF8);
	return wxString(translated.c_str(), wxConvUTF8);
}


// methods for comparing and actualizing content if necessary
void myframe::compare_vectorvector(){

	vector<vector<string> >::iterator old_content_it = content.begin();
	vector<vector<string> >::iterator new_content_it = new_content.begin();

	size_t line_nr = 0, line_index;
	string status_text, time_left, color;


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

			for(int i=1; i<3; i++){ // column 1 to 2
				list->SetItem(line_index, i, wxString(new_content_it->at(i+1).c_str(), wxConvUTF8));
			}

			// status column
			color = build_status(status_text, time_left, *new_content_it);
			list->SetItemBackgroundColour(line_index, wxString(color.c_str(), wxConvUTF8));
			list->SetItem(line_index, 3, wxString(time_left.c_str(), wxConvUTF8));
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
	string status_text, time_left, color;

	// compare every column
	while((it_new < end_new) && (it_old < end_old)){

		if(*it_new != *it_old){ // content of column_new != content of column_old

			if(column_nr == 0) // id
				list->SetItem(line_nr, column_nr, wxString(it_new->c_str(), wxConvUTF8));

			else if(column_nr == 2 || column_nr == 3) // content of column 2 and 3 goes into gui column 1 and 2! (content column 1 is no needed)
				list->SetItem(line_nr, column_nr-1, wxString(it_new->c_str(), wxConvUTF8));

			else // status column
				status_change = true;
		}

		it_new++;
		it_old++;
		column_nr++;
	}

	if(status_change){
		color = build_status(status_text, time_left, splitted_line_new);
		list->SetItemBackgroundColour(line_nr, wxString(color.c_str(), wxConvUTF8));
		list->SetItem(line_nr, 3, wxString(time_left.c_str(), wxConvUTF8));
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
 	status_bar->SetStatusText(tsl("DownloadDaemon Client-wx"), 0); // program should do that automatically, but doesn't somehow...
	connect_dialog dialog(config_dir, this);
	dialog.ShowModal();
	get_content();
 }


void myframe::on_add(wxCommandEvent &event){
	mx.lock();

	if(mysock == NULL || !*mysock || mysock->get_peer_name() == ""){ // if there is no active connection
		// make sure mysock doesn't crash the program
		if(mysock != NULL)
			delete mysock;
		mysock = NULL;
		mx.unlock();

		wxMessageBox(tsl("Please connect before adding Downloads."), tsl("No Connection to Server"));


	}else{
		mx.unlock();
		add_dialog dialog(this);
		dialog.ShowModal();
		get_content();
	}


 }


void myframe::on_delete(wxCommandEvent &event){
	vector<int>::iterator it;
	string id, answer;
	bool error_occured = false;

	mx.lock();

	if(mysock == NULL || !*mysock || mysock->get_peer_name() == ""){ // if there is no active connection
		// make sure mysock doesn't crash the program
		if(mysock != NULL)
			delete mysock;
		mysock = NULL;
		mx.unlock();

		wxMessageBox(tsl("Please connect before deleting Downloads."), tsl("No Connection to Server"));

	}else{ // we have a connection

		find_selected_lines(); // save selection into selected_lines
		if(!selected_lines.empty()){

			// make sure user wants to delete downloads
			wxMessageDialog dialog(this, tsl("Do you really want to delete\nthe selected Download(s)?"), tsl("Delete Downloads"),
									wxYES_NO|wxYES_DEFAULT|wxICON_EXCLAMATION);
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
								string message = lang["Error occured at deleting File of ID"] + " " + id;
								wxMessageBox(wxString(message.c_str(), wxConvUTF8), tsl("Error"));
							}

						}
					}

					mysock->send("DDP DL DEL " + id);
					mysock->recv(answer);

					if(answer.find("104") == 0) // 104 ID <-- Entered a not-existing ID
						error_occured = true;
				}
			}
		}else
			wxMessageBox(tsl("At least one Row should be selected."), tsl("Error"));

		deselect_lines();
		mx.unlock();

		if(error_occured)
			wxMessageBox(tsl("Error occured at deleting Download(s)."), tsl("Error"));

		get_content();
	}
 }


void myframe::on_delete_finished(wxCommandEvent &event){
	vector<string>::iterator it;
	vector<string> finished_ids;
	vector<vector<string> >::iterator content_it;
	string answer;
	bool error_occured = false;

	mx.lock();

	if(mysock == NULL || !*mysock || mysock->get_peer_name() == ""){ // if there is no active connection
		// make sure mysock doesn't crash the program
		if(mysock != NULL)
			delete mysock;
		mysock = NULL;
		mx.unlock();

		wxMessageBox(tsl("Please connect before deleting Downloads."), tsl("No Connection to Server"));

	}else{ // we have a connection

		// find all finished downloads
		for(content_it = content.begin(); content_it<content.end(); content_it++){
			if((*content_it)[4] == "DOWNLOAD_FINISHED")
				finished_ids.push_back((*content_it)[0]);
		}

		if(!finished_ids.empty()){

			// make sure user wants to delete downloads
			wxMessageDialog dialog(this, tsl("Do you really want to delete\nall finished Download(s)?"), tsl("Delete Downloads"),
									wxYES_NO|wxYES_DEFAULT|wxICON_EXCLAMATION);
			int del = dialog.ShowModal();

			if(del == wxID_YES){ // user clicked yes to delete
				int dialog_answer = 2; // possible answers are 0 = yes all, 1 = yes, 2 = no, 3 = no all

				for(it = finished_ids.begin(); it<finished_ids.end(); it++){
					//id = (content[*it])[0]; // gets the id of the line, which index is stored in finished_ids

					// test if there is a file on the server
					mysock->send("DDP FILE GETPATH " + *it);
					mysock->recv(answer);

					if(!answer.empty()){ // file exists

						// only show dialog if user didn't choose yes_all (0) or no_all (3) before
						if((dialog_answer != 0) && (dialog_answer != 3)){
							delete_dialog dialog(&dialog_answer, *it, this);
							dialog.ShowModal();
						}


						if((dialog_answer == 0) || (dialog_answer == 1)){ // user clicked yes all (0) or yes (1) to delete
							mysock->send("DDP DL DEACTIVATE " + *it);
							mysock->recv(answer);

							mysock->send("DDP FILE DEL " + *it);
							mysock->recv(answer);

							if(answer.find("109") == 0){ // 109 FILE <-- file operation on a file that does not exist
								wxMessageBox(tsl("Error occured at deleting File of Download %p1.", it->c_str()), tsl("Error"));
							}

						}
					}

					mysock->send("DDP DL DEL " + *it);
					mysock->recv(answer);

					if(answer.find("104") == 0) // 104 ID <-- Entered a not-existing ID
						error_occured = true;
				}
			}
		}

		deselect_lines();
		mx.unlock();

		if(error_occured)
			wxMessageBox(tsl("Error occured at deleting Download(s)."), tsl("Error"));

		get_content();
	}
}


 void myframe::on_delete_file(wxCommandEvent &event){
	vector<int>::iterator it;
	string id, answer;
	bool error_occured = false;

	mx.lock();

	if(mysock == NULL || !*mysock || mysock->get_peer_name() == ""){ // if there is no active connection
		// make sure mysock doesn't crash the program
		if(mysock != NULL)
			delete mysock;
		mysock = NULL;
		mx.unlock();

		wxMessageBox(tsl("Please connect before deleting Files."), tsl("No Connection to Server"));

	}else{ // we have a connection

		find_selected_lines(); // save selection into selected_lines
		if(!selected_lines.empty()){

			// make sure user wants to delete downloads
			wxMessageDialog dialog(this, tsl("Do you really want to delete\nthe selected File(s)?"), tsl("Delete Files"), wxYES_NO|wxYES_DEFAULT|wxICON_EXCLAMATION);
			int del = dialog.ShowModal();

			if(del == wxID_YES){ // user clicked yes to delete

				for(it = selected_lines.begin(); it<selected_lines.end(); it++){
					id = (content[*it])[0]; // gets the id of the line, which index is stored in selected_lines

					// test if there is a file on the server
					mysock->send("DDP FILE GETPATH " + id);
					mysock->recv(answer);

					if(!answer.empty()){ // file exists

						mysock->send("DDP DL DEACTIVATE " + id);
						mysock->recv(answer);

						mysock->send("DDP FILE DEL " + id);
						mysock->recv(answer);

						if(answer.find("109") == 0){ // 109 FILE <-- file operation on a file that does not exist
							string message = lang["Error occured at deleting File from ID"] + " " + id;
							wxMessageBox(wxString(message.c_str(), wxConvUTF8), tsl("Error"));
						}

					}
				}
			}
		}else
			wxMessageBox(tsl("At least one Row should be selected."), tsl("Error"));

		deselect_lines();
		mx.unlock();

		if(error_occured)
			wxMessageBox(tsl("Error occured at deleting Files(s)."), tsl("Error"));

		get_content();
	}
 }


void myframe::on_activate(wxCommandEvent &event){
	vector<int>::iterator it;
	string id, answer;

	mx.lock();

	if(mysock == NULL || !*mysock || mysock->get_peer_name() == ""){ // if there is no active connection
		// make sure mysock doesn't crash the program
		if(mysock != NULL)
			delete mysock;
		mysock = NULL;
		mx.unlock();

		wxMessageBox(tsl("Please connect before activating Downloads."), tsl("No Connection to Server"));

	}else{ // we have a connection

		find_selected_lines(); // save selection into selected_lines
		if(!selected_lines.empty()){

			for(it = selected_lines.begin(); it<selected_lines.end(); it++){
				id = (content[*it])[0]; // gets the id of the line, which index is stored in selected_lines

				mysock->send("DDP DL ACTIVATE " + id);
				mysock->recv(answer); // might receive error 106 ACTIVATE, but that doesn't matter
			}

		}else
			wxMessageBox(tsl("At least one Row should be selected."), tsl("Error"));

		mx.unlock();
		get_content();
	}
 }


void myframe::on_deactivate(wxCommandEvent &event){
	vector<int>::iterator it;
	string id, answer;

	mx.lock();
	if(mysock == NULL || !*mysock || mysock->get_peer_name() == ""){ // if there is no active connection
		mx.unlock();
		wxMessageBox(tsl("Please connect before deactivating Downloads."), tsl("No Connection to Server"));

	}else{ // we have a connection

		find_selected_lines(); // save selection into selected_lines
		if(!selected_lines.empty()){

			for(it = selected_lines.begin(); it<selected_lines.end(); it++){
				id = (content[*it])[0]; // gets the id of the line, which index is stored in selected_lines

				mysock->send("DDP DL DEACTIVATE " + id);
				mysock->recv(answer); // might receive error 107 DEACTIVATE, but that doesn't matter
			}

		}else
			wxMessageBox(tsl("At least one Row should be selected."), tsl("Error"));

		mx.unlock();
		get_content();
	}
 }


 void myframe::on_priority_up(wxCommandEvent &event){
	vector<int>::iterator it;
	string id, answer;

	mx.lock();

	if(mysock == NULL || !*mysock || mysock->get_peer_name() == ""){ // if there is no active connection
		// make sure mysock doesn't crash the program
		if(mysock != NULL)
			delete mysock;
		mysock = NULL;
		mx.unlock();

		wxMessageBox(tsl("Please connect before increasing Priority."), tsl("No Connection to Server"));

	}else{ // we have a connection

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

		}else
			wxMessageBox(tsl("At least one Row should be selected."), tsl("Error"));

		deselect_lines();
		mx.unlock();
		get_content();
	}
}


void myframe::on_priority_down(wxCommandEvent &event){
	vector<int>::reverse_iterator rit;
	string id, answer;

	mx.lock();

	if(mysock == NULL || !*mysock || mysock->get_peer_name() == ""){ // if there is no active connection
		// make sure mysock doesn't crash the program
		if(mysock != NULL)
			delete mysock;
		mysock = NULL;
		mx.unlock();

		wxMessageBox(tsl("Please connect before decreasing Priority."), tsl("No Connection to Server"));

	}else{ // we have a connection

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

		}else
			wxMessageBox(tsl("At least one Row should be selected."), tsl("Error"));

		deselect_lines();
		mx.unlock();
		get_content();
	}
}


void myframe::on_configure(wxCommandEvent &event){
	mx.lock();

	if(mysock == NULL || !*mysock || mysock->get_peer_name() == ""){ // if there is no active connection
		// make sure mysock doesn't crash the program
		if(mysock != NULL)
			delete mysock;
		mysock = NULL;
		mx.unlock();

		wxMessageBox(tsl("Please connect before configurating DownloadDaemon."),tsl("No Connection to Server"));

	}else{
		mx.unlock();
		configure_dialog dialog(this);
		dialog.ShowModal();
		get_content();
	}
}


void myframe::on_download_activate(wxCommandEvent &event){
	mx.lock();

	if(mysock == NULL || !*mysock || mysock->get_peer_name() == ""){ // if there is no active connection
		// make sure mysock doesn't crash the program
		if(mysock != NULL)
			delete mysock;
		mysock = NULL;
		mx.unlock();

		wxMessageBox(tsl("Please connect before activate Downloading."), tsl("No Connection to Server"));

	}else{
		string answer;

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
	mx.lock();

	if(mysock == NULL || !*mysock || mysock->get_peer_name() == ""){ // if there is no active connection
		// make sure mysock doesn't crash the program
		if(mysock != NULL)
			delete mysock;
		mysock = NULL;
		mx.unlock();

		wxMessageBox(tsl("Please connect before deactivate Downloading."), tsl("No Connection to Server"));

	}else{
		string answer;

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


void myframe::on_copy_url(wxCommandEvent &event){
	vector<int>::iterator it;
	string url, answer;
	wxString clipboard_data = wxT("");

	mx.lock();

	if(mysock == NULL || !*mysock || mysock->get_peer_name() == ""){ // if there is no active connection
		// make sure mysock doesn't crash the program
		if(mysock != NULL)
			delete mysock;
		mysock = NULL;
		mx.unlock();

		wxMessageBox(tsl("Please connect before copying URLs."), tsl("No Connection to Server"));

	}else{ // we have a connection

		find_selected_lines(); // save selection into selected_lines
		if(!selected_lines.empty()){

			for(it = selected_lines.begin(); it<selected_lines.end(); it++){
				url = (content[*it])[3]; // gets the url of the line, which index is stored in selected_lines
				url += "\n";
				clipboard_data += wxString(url.c_str(), wxConvUTF8);
			}

			if(wxTheClipboard->Open()){
				wxTheClipboard->SetData(new wxTextDataObject(clipboard_data));
				wxTheClipboard->Close();
			}else
				wxMessageBox(tsl("Couldn't write into clipboard."), tsl("Clipboard Error"));
		}else
			wxMessageBox(tsl("At least one Row should be selected."), tsl("Error"));

		mx.unlock();
	}
 }


 void myframe::on_resize(wxSizeEvent &event){
	myframe::OnSize(event); // call default evt_size handler
	Refresh();

 	if(list != NULL){
		mx.lock();

		// change column widths on resize
		int width = GetClientSize().GetWidth();
		width -= 160; // minus width of fix sized columns

		#if defined(__WXMSW__)
			width -= 10;
		#endif // defined(__WXMSW__)

		list->SetColumnWidth(1, width*0.25);  // only column 1, 2 and 4, because 0 and 3 have fix sizes
		list->SetColumnWidth(2, width*0.3);
		list->SetColumnWidth(4, width*0.45);

		mx.unlock();
 	}
 }


void myframe::on_reload(wxEvent &event){

	mx.lock();

	if(mysock == NULL || !*mysock || mysock->get_peer_name() == ""){ // if there is no active connection
		status_bar->SetStatusText(tsl("Not connected"), 1);
	}

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


void myframe::on_right_click(wxContextMenuEvent &event){
	wxMenu popup_menu;

	popup_menu.Append(id_toolbar_activate, wxT("&") + tsl("Activate Download") + wxT("\tAlt-A"), tsl("Activate Download"));
	popup_menu.Append(id_toolbar_deactivate, wxT("&") + tsl("Deactivate Download") + wxT("\tAlt-D"), tsl("Deactivate Download"));
	popup_menu.AppendSeparator();

	popup_menu.Append(id_toolbar_delete, wxT("&") + tsl("Delete Download") + wxT("\tDEL"), tsl("Delete Download"));
	popup_menu.Append(id_menu_delete_file, wxT("&") + tsl("Delete File") + wxT("\tCtrl-F"), tsl("Delete File"));
	popup_menu.Append(id_toolbar_copy_url, wxT("&") + tsl("Copy URL") + wxT("\tCtrl-C"), tsl("Copy URL"));
	popup_menu.AppendSeparator();

	PopupMenu(&popup_menu);
}


// getter and setter
void myframe::set_connection_attributes(tkSock *mysock, string password){
	this->mysock = mysock;
	this->password = password;
}


tkSock *myframe::get_connection_attributes(){
	return mysock;
}


boost::mutex *myframe::get_mutex(){
	return &mx;
}


void myframe::set_language(std::string lang_to_set){
	lang.set_language(lang_to_set);

	update_bars();
	update_components();
	Layout();
	Fit();
}

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

	dclient = new downloadc();

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

	// connect if logindata was saved
	string file_name = string(config_dir.mb_str()) + "save.dat";
	ifstream ifs(file_name.c_str(), fstream::in | fstream::binary); // open file

	if(ifs.good()){ // file successfully opened
		login_data last_data =  { "", 0, ""};
		ifs.read((char *) &last_data, sizeof(login_data));

		if(last_data.lang[0] != '\0') // older versions of save.dat won't have lang, so we have to check
			set_language(last_data.lang); // set program language

		try{
			dclient->connect(last_data.host, last_data.port, last_data.pass, true);
			update_status(wxString(last_data.host, wxConvUTF8));

		}catch(client_exception &e){
			if(e.get_id() == 2){ // daemon doesn't allow encryption

				wxMessageDialog dialog(this, tsl("Encrypted authentication not supported by server.") + wxT("\n")
											+ tsl("Do you want to try unsecure plain-text authentication?"),
										   tsl("Auto Connection: No Encryption Supported"), wxYES_NO|wxYES_DEFAULT|wxICON_EXCLAMATION);
				int del = dialog.ShowModal();

				if(del == wxID_YES){ // connect again
					try{
						dclient->connect(last_data.host, last_data.port, last_data.pass, false);
						update_status(wxString(last_data.host, wxConvUTF8));
					}catch(client_exception &e){}
				}
			} // we don't have error message here because it's an auto fuction
		}
	}

	boost::thread(boost::bind(&myframe::update_list, this));
}


myframe::~myframe(){
		mx.lock();
		if(dclient != NULL)
			delete dclient;
		dclient = NULL;
		mx.unlock();
}


void myframe::update_status(wxString server){
	// removing both icons/deactivating both menuentrys, even when maybe only one is shown
	toolbar->RemoveTool(id_toolbar_download_activate);
	toolbar->RemoveTool(id_toolbar_download_deactivate);
	file_menu->Enable(id_toolbar_download_activate, false);
	file_menu->Enable(id_toolbar_download_deactivate, false);

	if(!check_connection()){
		return;
	}
	string answer;
	mx.lock();
	try{
		answer = dclient->get_var("downloading_active");

	}catch(client_exception &e){}
	mx.unlock();

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

	update_status(wxT(""));
}


void myframe::add_components(){
	panel_downloads = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
	sizer_downloads = new wxBoxSizer(wxHORIZONTAL);
	panel_downloads->SetSizer(sizer_downloads);

	// all download lists
	list = new wxListCtrl(panel_downloads, wxID_ANY, wxPoint(120, -46), wxSize(0,0), wxLC_REPORT);
	sizer_downloads->Add(list, 1, wxALL|wxEXPAND|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);

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

		if(check_connection())
			get_content();
		sleep(2); // reload every two seconds
	}
}


void myframe::get_content(){
	mx.lock();
	new_content.clear();

	try{
		new_content = dclient->get_list();

	}catch(client_exception &e){}
	mx.unlock();

	// send event to reload list
	wxCommandEvent event(wxEVT_reload_list, GetId());
	wxPostEvent(this, event);
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

string myframe::build_status(string &status_text, string &time_left, download &dl){
	string color = "WHITE";

	if(dl.status == "DOWNLOAD_RUNNING"){
		color = "LIME GREEN";

		if(dl.wait > 0 && dl.error == "PLUGIN_SUCCESS"){ // waiting time > 0
			status_text = lang["Download running. Waiting."];
			stringstream time;
			time << dl.wait;
			time_left =  time.str();
			cut_time(time_left);

		}else if(dl.wait > 0 && dl.error != "PLUGIN_SUCCESS"){
			color = "RED";
			status_text = lang["Error"] + ": " + lang[dl.error] + " " + lang["Retrying soon."];
			stringstream time;
			time << dl.wait;
			time_left =  time.str();
			cut_time(time_left);

		}else{ // no waiting time
			stringstream stream_buffer, time_buffer;
			stream_buffer << lang["Running"];

			if(dl.speed != 0 && dl.speed != -1) // download speed known
				stream_buffer << "@" << setprecision(1) << fixed << (float)dl.speed / 1024 << " kb/s";

			stream_buffer << ": ";

			if(dl.size == 0 || dl.size == 1){ // download size unknown
				stream_buffer << "0.00% - ";
				time_left = "";

				if(dl.downloaded == 0 || dl.downloaded == 1) // nothing downloaded yet
					stream_buffer << "0.00 MB/ 0.00 MB";
				else // something downloaded
					stream_buffer << setprecision(1) << fixed << (float)dl.downloaded / 1048576 << " MB/ 0.00 MB";

			}else{ // download size known
				if(dl.downloaded == 0 || dl.downloaded == 1){ // nothing downloaded yet
					stream_buffer << "0.00% - 0.00 MB/ " << fixed << (float)dl.size / 1048576 << " MB";

					if(dl.speed != 0 && dl.speed != -1){ // download speed known => calc time left
						time_buffer << (int)(dl.size / dl.speed);
						time_left = time_buffer.str();
						cut_time(time_left);
					}else
						time_left = "";

				}else{ // download size known and something downloaded
					stream_buffer << setprecision(1) << fixed << (float)dl.downloaded / (float)dl.size * 100 << "% - ";
					stream_buffer << setprecision(1) << fixed << (float)dl.downloaded / 1048576 << " MB/ ";
					stream_buffer << setprecision(1) << fixed << (float)dl.size / 1048576 << " MB";

					if(dl.speed != 0 && dl.speed != -1){ // download speed known => calc time left
						time_buffer << (int)((dl.size - dl.downloaded) / dl.speed);
						time_left = time_buffer.str();
						cut_time(time_left);
					}else
						time_left = "";
				}
			}
			status_text = stream_buffer.str();
		}

	}else if(dl.status == "DOWNLOAD_INACTIVE"){
		if(dl.error == "PLUGIN_SUCCESS"){
			color = "YELLOW";
			status_text = lang["Download Inactive."];
			time_left = "";

		}else{ // error occured
			color = "RED";
			status_text = lang["Inactive. Error"] + ": " + lang[dl.error];
			time_left = "";
		}

	}else if(dl.status == "DOWNLOAD_PENDING"){
		time_left = "";

		if(dl.error == "PLUGIN_SUCCESS"){
			status_text = lang["Download Pending."];

		}else{ //error occured
			color = "RED";
			status_text = lang["Error"] + ": " + lang[dl.error];
		}

	}else if(dl.status == "DOWNLOAD_WAITING"){
		color = "YELLOW";
		status_text = lang["Have to wait."];
		stringstream time;
		time << dl.wait;
		time_left =  time.str();
		cut_time(time_left);

	}else if(dl.status == "DOWNLOAD_FINISHED"){
		color = "GREEN";
		status_text = lang["Download Finished."];
		time_left = "";

	}else if(dl.status == "DOWNLOAD_RECONNECTING") {
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
	std::string id;

	selected_lines.clear();

	while(true){
		item_index = list->GetNextItem(item_index, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED); // gets the next selected item

		if(item_index == -1) // no selected ones left => leave loop
			break;
		else{ // found a selected one
			id = std::string(list->GetItemText(item_index).mb_str());
			selected_lines.push_back(id);
		}
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
	va_start(vl, text);
	string search("p1");


	while((n = translated.find(search)) != string::npos){
		stringstream id;
		id << va_arg(vl, int);
		translated.replace(n-1, 3, id.str());

		i++;
		if(i>9) // maximal 9 additional Parameters
			break;

		stringstream sb;
		sb << "p" << i;
		search = sb.str();
	}

	return wxString(translated.c_str(), wxConvUTF8);
}


bool myframe::check_connection(bool tell_user, string individual_message){
	boost::mutex::scoped_lock lock(mx);

	try{
		dclient->check_connection();

	}catch(client_exception &e){
		if(e.get_id() == 10){ //connection lost

			if(tell_user)
				wxMessageBox(tsl(individual_message), tsl("No Connection to Server"));

			return false;
		}
	}
	return true;
}


// methods for comparing and actualizing content if necessary
void myframe::compare_all_packages(){
	vector<package>::iterator old_content_it = content.begin();
	vector<package>::iterator new_content_it = new_content.begin();

	int line_nr = 0, line_index;
	string status_text, time_left, color;
	bool change;

	// compare the i-th package in content with the i-th package in new_content
	while((new_content_it < new_content.end()) && (old_content_it < content.end())){
		change = compare_package(line_nr, *new_content_it, *old_content_it);

		new_content_it++;
		old_content_it++;

		if(change) // a package changed it's download count => every line coming now changed
			break;
	}


	if(old_content_it < content.end()){ // there are more old lines than new ones or change was true
		line_nr++;
		while(list->GetItemCount() >= line_nr)
			list->DeleteItem(list->GetItemCount()-1);
	}

	if(new_content_it < new_content.end()){ // there are more new packages then old ones  or change was true
		while(new_content_it < new_content.end()){

			stringstream id; // package ID
			id << "PKG " << new_content_it->id;
			line_index = list->InsertItem(line_nr, wxString(id.str().c_str(), wxConvUTF8));

			list->SetItem(line_index, 1, wxString(new_content_it->name.c_str(), wxConvUTF8)); // package name
			list->SetItemBackgroundColour(line_index, wxString(wxT("WHITE")));
			line_nr++;


			// insert all downloads of the new package
			vector<download>::iterator it_new = new_content_it->dls.begin();
			vector<download>::iterator end_new = new_content_it->dls.end();

			for(; it_new < end_new; it_new++){

				// insert content
				line_index = list->InsertItem(line_nr, wxString::Format(wxT("%i"), it_new->id));
				list->SetItem(line_index, 1, wxString(it_new->title.c_str(), wxConvUTF8));
				list->SetItem(line_index, 2, wxString(it_new->url.c_str(), wxConvUTF8));

				// status column
				color = build_status(status_text, time_left, *it_new);
				list->SetItemBackgroundColour(line_index, wxString(color.c_str(), wxConvUTF8));
				list->SetItem(line_index, 3, wxString(time_left.c_str(), wxConvUTF8));
				list->SetItem(line_index, 4, wxString(status_text.c_str(), wxConvUTF8));

				line_nr++;
			}

			new_content_it++;
		}
	}
}

bool myframe::compare_package(int &line_nr, package &pkg_new, package &pkg_old){
	int line_index;
	string status_text, time_left, color;
	bool change = false;

	if(pkg_new.id != pkg_old.id){ // package ID
		stringstream id;
		id << "PKG " << pkg_new.id;
		list->SetItem(line_nr, 0, wxString(id.str().c_str(), wxConvUTF8));
	}
	if(pkg_new.name != pkg_old.name){ // package name
		list->SetItem(line_nr, 1, wxString(pkg_new.name.c_str(), wxConvUTF8));
	}

	line_nr++;


	// compare all downloads
	vector<download>::iterator it_old = pkg_old.dls.begin();
	vector<download>::iterator end_old = pkg_old.dls.end();
	vector<download>::iterator it_new = pkg_new.dls.begin();
	vector<download>::iterator end_new = pkg_new.dls.end();

	while((it_new < end_new) && (it_old < end_old)){
		compare_download(line_nr, *it_new, *it_old);
		it_new++;
		it_old++;
		line_nr++;
	}


	if(it_new < end_new){ // there are more new downloads in the package then old ones
		change = true;
		while(it_new < end_new){

			line_nr++;
			while(list->GetItemCount() >= line_nr) // delete all following items, because they will all be changed anyway
				list->DeleteItem(list->GetItemCount()-1);

			// insert content
			line_index = list->InsertItem(line_nr, wxString::Format(wxT("%i"), it_new->id));
			list->SetItem(line_index, 1, wxString(it_new->title.c_str(), wxConvUTF8));
			list->SetItem(line_index, 2, wxString(it_new->url.c_str(), wxConvUTF8));

			// status column
			color = build_status(status_text, time_left, *it_new);
			list->SetItemBackgroundColour(line_index, wxString(color.c_str(), wxConvUTF8));
			list->SetItem(line_index, 3, wxString(time_left.c_str(), wxConvUTF8));
			list->SetItem(line_index, 4, wxString(status_text.c_str(), wxConvUTF8));

			line_nr++;
			it_new++;
		}

	}else if(it_old < end_old){ // there are more old downloads than new ones
		change = true;
		while(it_old < end_old){

			// delete content
			list->DeleteItem(line_nr);

			it_old++;
			// line_nr stays the same!
		}
	}
	return change;
}

void myframe::compare_download(int &line_nr, download &new_dl, download &old_dl){
	string status_text, time_left, color;

	// compare every column
	if(new_dl.id != old_dl.id) // ID
		list->SetItem(line_nr, 0, wxString::Format(wxT("%i"), new_dl.id));

	if(new_dl.title != old_dl.title) // title
		list->SetItem(line_nr, 1,wxString(new_dl.title.c_str(), wxConvUTF8));

	if(new_dl.url != old_dl.url) // url
		list->SetItem(line_nr, 2,wxString(new_dl.url.c_str(), wxConvUTF8));

	if((new_dl.status != old_dl.status) || (new_dl.status != old_dl.status) || (new_dl.status != old_dl.status) ||
	   (new_dl.status != old_dl.status) || (new_dl.status != old_dl.status) || (new_dl.status != old_dl.status)){

		color = build_status(status_text, time_left, new_dl);
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
	if(!check_connection(false))
		return;

	select_lines();
}


 void myframe::on_connect(wxCommandEvent &event){
 	status_bar->SetStatusText(tsl("DownloadDaemon Client-wx"), 0); // program should do that automatically, but doesn't somehow...
	connect_dialog dialog(config_dir, this);
	dialog.ShowModal();
	get_content();
 }


void myframe::on_add(wxCommandEvent &event){
	if(!check_connection(true, "Please connect before adding Downloads."))
		return;

	add_dialog dialog(this);
	dialog.ShowModal();
	get_content();
 }


void myframe::on_delete(wxCommandEvent &event){
	if(!check_connection(true, "Please connect before deleting Downloads."))
		return;

	vector<string>::iterator it;
	int id;

	find_selected_lines(); // save selection into selected_lines

	if(selected_lines.empty()){
		wxMessageBox(tsl("At least one Row should be selected."), tsl("Error"));

		deselect_lines();
		get_content();

		return;
	}


	// make sure user wants to delete downloads
	wxMessageDialog dialog(this, tsl("Do you really want to delete\nthe selected Download(s)?"), tsl("Delete Downloads"),
							wxYES_NO|wxYES_DEFAULT|wxICON_EXCLAMATION);
	int del = dialog.ShowModal();

	if(del != wxID_YES) // user clicked no to delete
		return;


	int dialog_answer = 2; // possible answers are 0 = yes all, 1 = yes, 2 = no, 3 = no all
	int package_id = -1;
	vector<int> package_deletes; // saves how many dowloads of each package have been deleted to see if the package is empty afterwards
	unsigned int package_count = 0;

	mx.lock();
	for(it = selected_lines.begin(); it < selected_lines.end(); it++){

		if((*it)[0] == 'P'){ // we have a package! => only save number and count downloads deleted

			if(package_id != -1){ // so we don't delete the default value
				package_deletes.push_back(package_count);
				package_count = 0;
			}

			string package = it->substr(4);
			package_id = atol(package.c_str());
			id = -1;
			package_deletes.push_back(package_id);

		}else{ // we have a real download
			id = atol(it->c_str());
			package_count++;
		}

		try{
			if(id != -1) // we have a real download, not a package
				dclient->delete_download(id, dont_know);

		}catch(client_exception &e){
			if(e.get_id() == 7){

				if((dialog_answer != 0) && (dialog_answer != 3)){ // file exists and user didn't choose yes_all (0) or no_all (3) before
					delete_dialog dialog(&dialog_answer, id, this);
					dialog.ShowModal();
				}

				if((dialog_answer == 0) || (dialog_answer == 1)){ // user clicked yes all (0) or yes (1) to delete file
					try{
						dclient->delete_download(id, del_file);

					}catch(client_exception &e){
						wxMessageBox(tsl("Error occured at deleting File of Download %p1.", id), tsl("Error"));
						package_count--; // download delete failed
					}

				}else{ // don't delete file
					try{
						dclient->delete_download(id, dont_delete);

					}catch(client_exception &e){
							wxMessageBox(tsl("Error occured at deleting Download(s)."), tsl("Error"));
							package_count--; // download delete failed
					}

				}
			}else{ // some error occured
				wxMessageBox(tsl("Error occured at deleting Download(s)."), tsl("Error"));
				package_count--; // download delete failed
			}
		}
	}
	package_deletes.push_back(package_count);

	// see if we can delete the packages (count == dls.size)
	vector<int>::iterator pit;
	vector<package>::iterator contentpackage_it;
	unsigned int size;

	for(pit = package_deletes.begin(); pit < package_deletes.end(); pit++){
		package_id = *pit;
		pit++;
		package_count = *pit;

		for(contentpackage_it = content.begin(); contentpackage_it < content.end(); contentpackage_it++){
			if(contentpackage_it->id == package_id){
				size = contentpackage_it->dls.size();
				break;
			}
		}

		if(size <= package_count){ // we deleted all downloads of that package
			try{
				dclient->delete_package(package_id);
			}catch(client_exception &e){}
		}
	}

	mx.unlock();
	//get_content(); // I make deadlocks, whatever reason that could be for
 }


void myframe::on_delete_finished(wxCommandEvent &event){
	if(!check_connection(true, "Please connect before deleting Downloads."))
		return;

	vector<int>::iterator it;
	vector<int> finished_ids;
	vector<package>::iterator content_it;
	vector<download>::iterator download_it;
	int id;
	int package_count = 0;

	mx.lock();

	vector<int> package_deletes; // prepare structure to save how many downloads of which package will be deleted
	for(unsigned int i = 0; i < content.size(); i++)
		package_deletes.push_back(0);

	// delete all empty packages
	for(content_it = content.begin(); content_it < content.end(); content_it++){
		if(content_it->dls.size() == 0){
			try{
				dclient->delete_package(content_it->id);
			}catch(client_exception &e){}
		}
	}

	// find all finished downloads
	for(content_it = content.begin(); content_it < content.end(); content_it++){
		for(download_it = content_it->dls.begin(); download_it < content_it->dls.end(); download_it++){
			if(download_it->status == "DOWNLOAD_FINISHED"){
				finished_ids.push_back(download_it->id);
				package_deletes[package_count]++;
			}
		}
		package_count++;
	}


	if(!finished_ids.empty()){
		// make sure user wants to delete downloads
		wxMessageDialog dialog(this, tsl("Do you really want to delete\nall finished Download(s)?"), tsl("Delete Downloads"),
								wxYES_NO|wxYES_DEFAULT|wxICON_EXCLAMATION);
		int del = dialog.ShowModal();

		if(del != wxID_YES){ // user clicked no to delete
			mx.unlock();
			return;
		}

		int dialog_answer = 2; // possible answers are 0 = yes all, 1 = yes, 2 = no, 3 = no all

		for(it = finished_ids.begin(); it < finished_ids.end(); it++){
				id = *it;

			try{
				dclient->delete_download(id, dont_know);

			}catch(client_exception &e){
				if(e.get_id() == 7){

					if((dialog_answer != 0) && (dialog_answer != 3)){ // file exists and user didn't choose yes_all (0) or no_all (3) before
						delete_dialog dialog(&dialog_answer, id, this);
						dialog.ShowModal();
					}

					if((dialog_answer == 0) || (dialog_answer == 1)){ // user clicked yes all (0) or yes (1) to delete file
						try{
							dclient->delete_download(id, del_file);

						}catch(client_exception &e){
							wxMessageBox(tsl("Error occured at deleting File of Download %p1.", id), tsl("Error"));
						}

					}else{ // don't delete file
						try{
							dclient->delete_download(id, dont_delete);

						}catch(client_exception &e){
								wxMessageBox(tsl("Error occured at deleting Download(s)."), tsl("Error"));
						}

					}
				}else{ // some error occured
					wxMessageBox(tsl("Error occured at deleting Download(s)."), tsl("Error"));
				}
			}
		}

	}
	deselect_lines();

	// delete all empty packages
	int i = 0;
	for(it = package_deletes.begin(); it < package_deletes.end(); it++){
		unsigned int package_size = *it;

		if(content[i].dls.size() <= package_size){ // if we deleted every download inside a package
			try{
				dclient->delete_package(content[i].id);
			}catch(client_exception &e){}
		}
		i++;
	}

	mx.unlock();
	//get_content(); // I make deadlocks, whatever reason that could be for
}


 void myframe::on_delete_file(wxCommandEvent &event){
 	if(!check_connection(true, "Please connect before deleting Files."))
		return;

	vector<string>::iterator it;
	string answer;
	int id;

	find_selected_lines(); // save selection into selected_lines
	if(selected_lines.empty()){
		wxMessageBox(tsl("At least one Row should be selected."), tsl("Error"));
		return;
	}

	// make sure user wants to delete downloads
	wxMessageDialog dialog(this, tsl("Do you really want to delete\nthe selected File(s)?"), tsl("Delete Files"), wxYES_NO|wxYES_DEFAULT|wxICON_EXCLAMATION);
	int del = dialog.ShowModal();

	if(del != wxID_YES) // user clicked yes to delete
		return

	mx.lock();
	for(it = selected_lines.begin(); it < selected_lines.end(); it++){

		if((*it)[0] != 'P') // we have a real download
			id = atol(it->c_str());
		else // we have a package selected
			continue; // next one

		try{
			dclient->delete_file(id);
		}catch(client_exception &e){
			if(e.get_id() == 19) // file error
				wxMessageBox(tsl("Error occured at deleting File of Download %p1.", id), tsl("Error"));
		}
	}

	mx.unlock();
	deselect_lines();
 }


void myframe::on_activate(wxCommandEvent &event){
	if(!check_connection(true, "Please connect before activating Downloads."))
		return;

	vector<string>::iterator it;
	int id;
	string error_string;

	find_selected_lines(); // save selection into selected_lines
	if(selected_lines.empty()){
		wxMessageBox(tsl("At least one Row should be selected."), tsl("Error"));
		return;
	}

	mx.lock();
	for(it = selected_lines.begin(); it<selected_lines.end(); it++){

		if((*it)[0] != 'P') // we have a real download
			id = atol(it->c_str());
		else // we have a package selected
			continue; // next one

		try{
			dclient->activate_download(id);
		}catch(client_exception &e){}
	}

	mx.unlock();
	get_content();
 }


void myframe::on_deactivate(wxCommandEvent &event){
	if(!check_connection(true, "Please connect before deactivating Downloads."))
		return;

	vector<string>::iterator it;
	int id;
	string error_string;

	find_selected_lines(); // save selection into selected_lines
	if(selected_lines.empty()){
		wxMessageBox(tsl("At least one Row should be selected."), tsl("Error"));
		return;
	}

	mx.lock();
	for(it = selected_lines.begin(); it<selected_lines.end(); it++){

		if((*it)[0] != 'P') // we have a real download
			id = atol(it->c_str());
		else // we have a package selected
			continue; // next one

		try{
			dclient->deactivate_download(id);
		}catch(client_exception &e){}
	}

	mx.unlock();
	get_content();
}


 void myframe::on_priority_up(wxCommandEvent &event){
	if(!check_connection(true, "Please connect before increasing Priority."))
		return;

	vector<string>::iterator it;
	int id;
	string error_string;

	find_selected_lines(); // save selection into selected_lines
	if(selected_lines.empty()){
		wxMessageBox(tsl("At least one Row should be selected."), tsl("Error"));
		return;
	}

	mx.lock();
	reselect_lines.clear();
	for(it = selected_lines.begin(); it<selected_lines.end(); it++){
		reselect_lines.push_back(*it);

		if((*it)[0] != 'P') // we have a real download
			id = atol(it->c_str());
		else // we have a package selected
			continue; // next one


		try{
			dclient->priority_up(id);
		}catch(client_exception &e){
			if(e.get_id() == 14)
				reselect_lines.clear();
				break;
		}
	}

	deselect_lines();
	mx.unlock();
	get_content();
}


void myframe::on_priority_down(wxCommandEvent &event){
	if(!check_connection(true, "Please connect before decreasing Priority."))
		return;

	vector<string>::reverse_iterator rit;
	int id;
	string error_string;

	find_selected_lines(); // save selection into selected_lines
	if(selected_lines.empty()){
		wxMessageBox(tsl("At least one Row should be selected."), tsl("Error"));
		return;
	}

	mx.lock();
	reselect_lines.clear();
	for(rit = selected_lines.rbegin(); rit<selected_lines.rend(); rit++){
		reselect_lines.push_back(*rit);

		if((*rit)[0] != 'P') // we have a real download
			id = atol(rit->c_str());
		else // we have a package selected
			continue; // next one


		try{
			dclient->priority_down(id);
		}catch(client_exception &e){
			if(e.get_id() == 14)
				reselect_lines.clear();
				break;
		}
	}

	deselect_lines();
	mx.unlock();
	get_content();
}


void myframe::on_configure(wxCommandEvent &event){
	if(!check_connection(true, "Please connect before you configurating DownloadDaemon."))
		return;

	configure_dialog dialog(this);
	dialog.ShowModal();
}


void myframe::on_download_activate(wxCommandEvent &event){
	if(!check_connection(true, "Please connect before you activate Downloading."))
		return;

	mx.lock();
	try{
		dclient->set_var("downloading_active", "1");
	}catch(client_exception &e){}
	mx.unlock();

	// update toolbar
	toolbar->RemoveTool(id_toolbar_download_activate);
	toolbar->RemoveTool(id_toolbar_download_deactivate);
	toolbar->AddTool(download_deactivate);
	toolbar->Realize();

	file_menu->Enable(id_toolbar_download_deactivate, true);
	file_menu->Enable(id_toolbar_download_activate, false);

}


void myframe::on_download_deactivate(wxCommandEvent &event){
	if(!check_connection(true, "Please connect before deactivate Downloading."))
		return;

	mx.lock();
	try{
		dclient->set_var("downloading_active", "0");
	}catch(client_exception &e){}
	mx.unlock();

	// update toolbar
	toolbar->RemoveTool(id_toolbar_download_activate);
	toolbar->RemoveTool(id_toolbar_download_deactivate);
	toolbar->AddTool(download_activate);
	toolbar->Realize();

	file_menu->Enable(id_toolbar_download_activate, true);
	file_menu->Enable(id_toolbar_download_deactivate, false);
}


void myframe::on_copy_url(wxCommandEvent &event){
	if(!check_connection(true, "Please connect before copying URLs."))
		return;

	vector<string>::iterator it;
	string url;
	wxString clipboard_data = wxT("");

	find_selected_lines(); // save selection into selected_lines
	if(selected_lines.empty()){
		wxMessageBox(tsl("At least one Row should be selected."), tsl("Error"));
		return;
	}

	mx.lock();

	vector<package>::iterator content_it;
	vector<download>::iterator download_it;
	bool found = false;

	for(it = selected_lines.begin(); it<selected_lines.end(); it++){
		if((*it)[0] == 'P') // we have a package
			continue; // next one

		for(content_it = content.begin(); ((content_it < content.end()) && (!found)); content_it++){ // search url of saved id
			for(download_it = content_it->dls.begin(); ((download_it < content_it->dls.end()) && (!found)); download_it++){
				if(download_it->id == atol((*it).c_str())){
					url = download_it->url;
					found = true; // leave the two loops
				}
			}

		}

		url += "\n";
		found = false;
		clipboard_data += wxString(url.c_str(), wxConvUTF8);
	}

	if(wxTheClipboard->Open()){
		wxTheClipboard->SetData(new wxTextDataObject(clipboard_data));
		wxTheClipboard->Close();
	}else
		wxMessageBox(tsl("Couldn't write into clipboard."), tsl("Clipboard Error"));

	mx.unlock();

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

	if(!check_connection()){
		wxString status_text = tsl("Not connected");
		SetStatusText(status_text, 1);
		return;
	}

	mx.lock();
	compare_all_packages();
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
downloadc *myframe::get_connection(){
	return dclient;
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

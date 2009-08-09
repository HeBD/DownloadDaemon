/***************************************************************
 * Name:	  ddclient-wx_main.h
 * Purpose:   Header for Frame Class
 * Author:	ko ()
 * Created:   2009-08-04
 * Copyright: ko ()
 * License:
 **************************************************************/

#ifndef DDCLIENT_WX_MAIN_H
#define DDCLIENT_WX_MAIN_H


#include <vector>
#include <iomanip>
#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include <wx/msgdlg.h> // for wxmessagebox
#include <wx/frame.h>
#include <wx/panel.h>
#include <wx/notebook.h>
#include <wx/stattext.h>
#include <wx/menu.h>
#include <wx/toolbar.h>
#include <wx/artprov.h>
#include <wx/sizer.h>
#include <wx/listctrl.h>
#include <wx/string.h>
#include <wx/gdicmn.h> // color database

#include "ddclient-wx_connect_dialog.h"
#include "../lib/netpptk/netpptk.h"

using namespace std;

class myframe : public wxFrame{
	public:
		myframe(wxWindow *parent, const wxString &title, wxWindowID id=wxID_ANY,const wxPoint& pos=wxDefaultPosition,const wxSize& size=wxDefaultSize);
		~myframe();

		// getter and setter methods
		void set_connection_attributes(tkSock *mysock, string password);
		tkSock *get_connection_attributes();

	private:

		vector<vector<string> > content;

		// connection attributes
		tkSock *mysock;
		string password;

		// elements for bars
		wxMenuBar *menu;
		wxMenu *file_menu;
		wxMenu *help_menu;
		wxToolBar *toolbar;

		// elements for content
		wxNotebook *notebook;
		wxPanel *panel_all;
		wxPanel *panel_running;
		wxPanel *panel_finished;
		wxBoxSizer *sizer_all;
		wxBoxSizer *sizer_running;
		wxBoxSizer *sizer_finished;
		wxListCtrl *list[3];

		// element IDs
		static const long id_menu_quit;
		static const long id_menu_about;
		static const long id_toolbar_connect;
		static const long id_toolbar_add;
		static const long id_toolbar_delete;
		static const long id_toolbar_stop;
		static const long id_toolbar_start;

		void add_bars();
		void add_content();
		void fill_lists();
		string build_status(string &status_text, vector<string> &splitted_line);

		// methods for comparing and actualizing content if necessary
		void compare_vectorvector(vector<vector<string> >::iterator new_content_it, vector<vector<string> >::iterator new_content_end);
		void compare_vector(size_t line_nr, vector<string> &splitted_line_new, vector<string>::iterator it_old, vector<string>::iterator end_old);

		// event handle methods
		void on_quit(wxCommandEvent &event);
		void on_about(wxCommandEvent &event);
		void on_connect(wxCommandEvent &event);
		void on_add(wxCommandEvent &event);
		void on_delete(wxCommandEvent &event);
		void on_stop(wxCommandEvent &event);
		void on_start(wxCommandEvent &event);

		DECLARE_EVENT_TABLE()
};

#endif //DDCLIENT_WX_MAIN_H

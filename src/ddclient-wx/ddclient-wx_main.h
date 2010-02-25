/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef DDCLIENT_WX_MAIN_H
#define DDCLIENT_WX_MAIN_H


#include <vector>

#include <wx/frame.h>
#include <wx/panel.h>
#include <wx/notebook.h>
#include <wx/toolbar.h>
#include <wx/sizer.h>
#include <wx/listctrl.h>

//#include <netpptk/netpptk.h>
#include <downloadc/downloadc.h>
#ifdef __WXMSW__
    #include <wx/msw/winundef.h>
#endif

#include <language/language.h>


using namespace std;


/** Main Class: the Frame */
class myframe : public wxFrame{
	public:

		/** Constructor with standard wxFrame Parameters */
		myframe(wxChar *parameter, wxWindow *parent, const wxString &title, wxWindowID id=wxID_ANY);

		/** Destructor */
		~myframe();

		/** Updates Status of the Program, should be called after Connecting to a Server
		*	@param server Location of the Server, which the client is connected to
		*/
		void update_status(wxString server);

		// getter and setter methods
		/** Getter for Download-Client Object (used for Communication with Daemon)
		*	@returns Download-Client Object
		*/
		downloadc *get_connection();

		/** Getter for Mutex, that makes sure only one Thread uses Socket and Content List at a time
		*	@returns Mutex
		*/
		boost::mutex *get_mutex();

		/** Setter for Language
		*	@param lang_to_set Language
		*/
		void set_language(std::string lang_to_set);

		/** Translates a string and changes it into a wxString
		*	@param text string to translate
		*	@returns translated wxString
		*/
		wxString tsl(string text, ...);

		/** Checks connection
		*	@param tell_user show to user via message box if connection is lost
		*	@param individual_message part of a message to send to user
		*	@returns connection available or not
		*/
		bool check_connection(bool tell_user = false, string individual_message = "");

	private:

		std::vector<package> content;
		std::vector<package> new_content;
		vector<string> selected_lines;
		vector<string> reselect_lines;
		wxString working_dir;
		wxString config_dir;
		boost::mutex mx;
		language lang;

		// connection attributes
		downloadc *dclient;

		// elements for bars
		wxMenuBar *menu;
		wxMenu *file_menu;
		wxMenu *help_menu;
		wxToolBar *toolbar;
		wxToolBarToolBase *download_activate;
		wxToolBarToolBase *download_deactivate;
		wxStatusBar *status_bar;

		// elements for content
		wxPanel *panel_downloads;
		wxBoxSizer *sizer_downloads;
		wxListCtrl *list;

		// element IDs
		static const long id_menu_about;
		static const long id_menu_select_all_lines;
		static const long id_toolbar_connect;
		static const long id_toolbar_add;
		static const long id_toolbar_delete;
		static const long id_toolbar_delete_finished;
		static const long id_menu_delete_file;
		static const long id_toolbar_activate;
		static const long id_toolbar_deactivate;
		static const long id_toolbar_priority_up;
		static const long id_toolbar_priority_down;
		static const long id_toolbar_configure;
		static const long id_toolbar_download_activate;
		static const long id_toolbar_download_deactivate;
		static const long id_toolbar_copy_url;
		static const long id_right_click;

		void add_bars();
		void update_bars();
		void add_components();
		void update_components();
		void update_list();
		void get_content();
		void cut_time(string &time_left);
		string build_status(string &status_text, string &time_left, download &dl);
		void find_selected_lines();
		void select_lines();
		void select_line_by_id(string id);
		void deselect_lines();

		// methods for comparing and actualizing content if necessary
		void compare_all_packages();
		bool compare_package(int &line_nr, package &pkg_new, package &pkg_old);
		void compare_download(int &line_nr, download &new_dl, download &old_dl);

		// event handle methods
		void on_quit(wxCommandEvent &event);
		void on_about(wxCommandEvent &event);
		void on_select_all_lines(wxCommandEvent &event);
		void on_connect(wxCommandEvent &event);
		void on_add(wxCommandEvent &event);
		void on_delete(wxCommandEvent &event);
		void on_delete_finished(wxCommandEvent &event);
		void on_delete_file(wxCommandEvent &event);
		void on_activate(wxCommandEvent &event);
		void on_deactivate(wxCommandEvent &event);
		void on_priority_up(wxCommandEvent &event);
		void on_priority_down(wxCommandEvent &event);
		void on_configure(wxCommandEvent &event);
		void on_download_activate(wxCommandEvent &event);
		void on_download_deactivate(wxCommandEvent &event);
		void on_copy_url(wxCommandEvent &event);
		void on_resize(wxSizeEvent &event);
		void on_reload(wxEvent &event);
		void on_right_click(wxContextMenuEvent &event);

		DECLARE_EVENT_TABLE()
};

struct login_data{
	char host[256];
	int port;
	char pass[256];
	char lang[128];
};

#endif // DDCLIENT_WX_MAIN_H

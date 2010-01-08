/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef DDCLIENT_WX_CONFIGURE_DIALOG_H
#define DDCLIENT_WX_CONFIGURE_DIALOG_H

#include <vector>

#include <wx/msgdlg.h> // for wxmessagebox
#include <wx/dialog.h>
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/textctrl.h>
#include <wx/notebook.h>
#include <wx/choice.h>
#include <wx/checkbox.h>

#include "../lib/netpptk/netpptk.h"
#if defined(__WXMSW__)
	#include <wx/msw/winundef.h> // because of conflicting wxWidgets and windows.h
#endif
#include "ddclient-wx_main.h"


/** Configure Dialog Class. Shows a Dialog to configure to DownloadDaemon Server. */
class configure_dialog : public wxDialog{
	public:

		/** Constructor
		*	@param parent Parent wxWindow
		*/
		configure_dialog(wxWindow *parent);

	private:
		wxNotebook *notebook;
		wxPanel *general_panel;
		wxPanel *download_panel;
		wxPanel *pass_panel;
		wxPanel *log_panel;
		wxPanel *reconnect_panel;
		enum var_type {NORMAL_T, ROUTER_T, PREMIUM_T};

		// for general_panel
		std::vector<std::string> premium_host_list;
		wxStaticText *premium_host_text;
		wxStaticText *premium_user_text;
		wxStaticText *premium_pass_text;
		wxChoice *premium_host_choice;
		wxTextCtrl *premium_user_input;
		wxTextCtrl *premium_pass_input;
		wxCheckBox *overwrite_check;
		wxCheckBox *refuse_existing_check;
		wxButton *premium_button;
		wxButton *general_button;
		wxButton *general_cancel_button;

		wxBoxSizer *overall_general_sizer;
		wxStaticBoxSizer *outer_premium_sizer;
		wxStaticBoxSizer *outer_general_sizer;
		wxFlexGridSizer *premium_sizer;
		wxBoxSizer *general_button_sizer;

		// for download_panel
		wxStaticText *exp_time_text;
		wxStaticText *start_time_text;
		wxStaticText *end_time_text;
		wxStaticText *save_dir_text;
		wxStaticText *exp_count_text;
		wxStaticText *count_text;
		wxStaticText *speed_text;
		wxTextCtrl *start_time_input;
		wxTextCtrl *end_time_input;
		wxTextCtrl *save_dir_input;
		wxTextCtrl *count_input;
		wxTextCtrl *speed_input;
		wxButton *download_button;
		wxButton *download_cancel_button;

		wxBoxSizer *overall_download_sizer;
		wxStaticBoxSizer *outer_time_sizer;
		wxStaticBoxSizer *outer_save_dir_sizer;
		wxStaticBoxSizer *outer_count_sizer;
		wxFlexGridSizer *time_sizer;
		wxFlexGridSizer *count_sizer;
		wxBoxSizer *download_button_sizer;

		// for pass_panel
		wxStaticText *old_pass_text;
		wxStaticText *new_pass_text;
		wxTextCtrl *old_pass_input;
		wxTextCtrl *new_pass_input;
		wxButton *pass_button;
		wxButton *pass_cancel_button;

		wxBoxSizer *overall_pass_sizer;
		wxStaticBoxSizer *outer_pass_sizer;
		wxFlexGridSizer *pass_sizer;
		wxBoxSizer *pass_button_sizer;

		// for log_panel
		wxStaticText *log_activity_text;
		wxStaticText *log_output_text;
		wxChoice *log_activity_choice;
		wxChoice *log_output_choice;
		wxButton *log_button;
		wxButton *log_cancel_button;

		wxBoxSizer *overall_log_sizer;
		wxStaticBoxSizer *outer_log_activity_sizer;
		wxStaticBoxSizer *outer_log_output_sizer;
		wxBoxSizer *log_button_sizer;

		// for reconnect_panel
		std::vector<std::string> router_model_list;
		wxCheckBox *enable_reconnecting_check;
		wxStaticText *policy_text;
		wxStaticText *router_model_text;
		wxStaticText *router_ip_text;
		wxStaticText *router_user_text;
		wxStaticText *router_pass_text;
		wxChoice *policy_choice;
		wxChoice *router_model_choice;
		wxTextCtrl *router_ip_input;
		wxTextCtrl *router_user_input;
		wxTextCtrl *router_pass_input;
		wxButton *reconnect_button;
		wxButton *reconnect_cancel_button;

		wxBoxSizer *overall_reconnect_sizer;
		wxStaticBoxSizer *outer_policy_sizer;
		wxStaticBoxSizer *outer_router_sizer;
		wxFlexGridSizer *policy_sizer;
		wxFlexGridSizer *router_sizer;
		wxBoxSizer *reconnect_button_sizer;

		void create_general_panel();
		void create_download_panel();
		void create_pass_panel();
		void create_log_panel();
		void create_reconnect_panel();
		void enable_reconnect_panel();
		void disable_reconnect_panel();
		wxString get_var(const std::string &var, var_type typ = NORMAL_T);

		// element IDs
		static const long id_premium_host_choice;
		static const long id_enable_reconnect_check;

		// event handle methods
		void on_apply(wxCommandEvent &event);
		void on_pass_change(wxCommandEvent &event);
		void on_premium_change(wxCommandEvent &event);
		void on_cancel(wxCommandEvent &event);
		void on_select_premium_host(wxCommandEvent &event);
		void on_checkbox_change(wxCommandEvent &event);

		DECLARE_EVENT_TABLE()
};



#endif // DDCLIENT_WX_CONFIGURE_DIALOG_H
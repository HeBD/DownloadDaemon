/***************************************************************
 * Name:	  ddclient-wx_configure_dialog.h
 * Purpose:   Header for Configure Dialog Class
 * Author:	ko ()
 * Created:   2009-08-26
 * Copyright: ko ()
 * License:
 **************************************************************/

#ifndef DDCLIENT_WX_CONFIGURE_DIALOG_H
#define DDCLIENT_WX_CONFIGURE_DIALOG_H

#include <wx/msgdlg.h> // for wxmessagebox
#include <wx/dialog.h>
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/textctrl.h>
#include <wx/notebook.h>
#include <wx/choice.h>

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
		wxPanel *download_panel;
		wxPanel *pass_panel;
		wxPanel *log_panel;

		// for download_panel
		wxStaticText *exp_time_text;
		wxStaticText *start_time_text;
		wxStaticText *end_time_text;
		wxStaticText *exp_save_dir_text;
		wxStaticText *save_dir_text;
		wxStaticText *exp_count_text;
		wxStaticText *count_text;
		wxTextCtrl *start_time_input;
		wxTextCtrl *end_time_input;
		wxTextCtrl *save_dir_input;
		wxTextCtrl *count_input;
		wxButton *download_button;
		wxButton *download_cancel_button;

		wxBoxSizer *overall_download_sizer;
		wxStaticBoxSizer *outer_time_sizer;
		wxStaticBoxSizer *outer_save_dir_sizer;
		wxStaticBoxSizer *outer_count_sizer;
		wxFlexGridSizer *time_sizer;
		wxFlexGridSizer *save_dir_sizer;
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
		wxStaticText *exp_log_output_text;
		wxStaticText *log_output_text;
		wxChoice *log_activity_choice;
		wxTextCtrl *log_output_input;
		wxButton *log_button;
		wxButton *log_cancel_button;

		wxBoxSizer *overall_log_sizer;
		wxStaticBoxSizer *outer_log_activity_sizer;
		wxStaticBoxSizer *outer_log_output_sizer;
		wxFlexGridSizer *log_output_sizer;
		wxBoxSizer *log_button_sizer;

		// element IDs
		static const long id_download_change;
		static const long id_pass_change;
		static const long id_log_change;

		void create_download_panel();
		void create_pass_panel();
		void create_log_panel();
		wxString get_var(const std::string &var);

		// event handle methods
		void on_apply(wxCommandEvent &event);
		void on_pass_change(wxCommandEvent &event);
		void on_cancel(wxCommandEvent &event);

		DECLARE_EVENT_TABLE()
};



#endif // DDCLIENT_WX_CONFIGURE_DIALOG_H

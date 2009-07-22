#ifndef CONNECT_DLG_H
#define CONNECT_DLG_H

//(*Headers(connect_dlg)
#include <wx/spinctrl.h>
#include <wx/dialog.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
//*)

class connect_dlg: public wxDialog
{
	public:

		connect_dlg(wxWindow* parent,wxWindowID id=wxID_ANY,const wxPoint& pos=wxDefaultPosition,const wxSize& size=wxDefaultSize);
		virtual ~connect_dlg();

		//(*Declarations(connect_dlg)
		wxButton* conn_conn_btn;
		wxStaticText* StaticText1;
		wxTextCtrl* hostname_txt;
		wxButton* conn_cancel_btn;
		wxSpinCtrl* port_txt;
		wxStaticText* StaticText2;
		//*)

	protected:

		//(*Identifiers(connect_dlg)
		static const long ID_STATICTEXT1;
		static const long conndlg_host;
		static const long ID_STATICTEXT2;
		static const long conndlg_port;
		static const long ID_BUTTON1;
		static const long ID_BUTTON2;
		//*)

	private:

		//(*Handlers(connect_dlg)
		void OnButton1Click(wxCommandEvent& event);
		void OnButton1Click1(wxCommandEvent& event);
		void Onconn_cancel_btnClick(wxCommandEvent& event);
		void Onconn_conn_btnClick(wxCommandEvent& event);
		//*)

		DECLARE_EVENT_TABLE()
};

#endif

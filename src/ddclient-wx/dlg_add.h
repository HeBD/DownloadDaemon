#ifndef DLG_ADD_H
#define DLG_ADD_H

//(*Headers(dlg_add)
#include <wx/dialog.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
//*)

class dlg_add: public wxDialog
{
	public:

		dlg_add(wxWindow* parent,wxWindowID id=wxID_ANY,const wxPoint& pos=wxDefaultPosition,const wxSize& size=wxDefaultSize);
		virtual ~dlg_add();

		//(*Declarations(dlg_add)
		wxStaticText* StaticText2;
		wxButton* add_cancel_btn;
		wxTextCtrl* add_dl_links;
		wxButton* add_btn;
		//*)

	protected:

		//(*Identifiers(dlg_add)
		static const long ID_STATICTEXT2;
		static const long ID_TEXTCTRL2;
		static const long ID_BUTTON1;
		static const long ID_BUTTON2;
		//*)

	private:

		//(*Handlers(dlg_add)
		void OnTextCtrl2Text(wxCommandEvent& event);
		void OnButton1Click(wxCommandEvent& event);
		void OnButton2Click(wxCommandEvent& event);
		void Onadd_btnClick(wxCommandEvent& event);
		void OnClose(wxCloseEvent& event);
		void Onadd_cancel_btnClick(wxCommandEvent& event);
		//*)

		DECLARE_EVENT_TABLE()
};

#endif

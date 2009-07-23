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
		wxButton* Button1;
		wxButton* Button2;
		wxStaticText* StaticText2;
		wxTextCtrl* TextCtrl2;
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
		//*)

		DECLARE_EVENT_TABLE()
};

#endif

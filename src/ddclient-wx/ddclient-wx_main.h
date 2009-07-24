#ifndef DDCLIENT_WX_MAIN_H
#define DDCLIENT_WX_MAIN_H

//(*Headers(ddclient_main)
#include <wx/toolbar.h>
#include <wx/sizer.h>
#include <wx/listctrl.h>
#include <wx/notebook.h>
#include <wx/menu.h>
#include <wx/panel.h>
#include <wx/statusbr.h>
#include <wx/frame.h>
//*)

class ddclient_main: public wxFrame
{
	public:

		ddclient_main(wxWindow* parent,wxWindowID id=wxID_ANY,const wxPoint& pos=wxDefaultPosition,const wxSize& size=wxDefaultSize);
		virtual ~ddclient_main();

		//(*Declarations(ddclient_main)
		wxToolBarToolBase* ToolBarItem5;
		wxStatusBar* StatusBar1;
		wxPanel* Panel2;
		wxToolBarToolBase* ToolBarItem2;
		wxListView* DownloadList;
		wxListCtrl* ListCtrl1;
		wxToolBar* ToolBar1;
		wxPanel* Panel3;
		wxToolBarToolBase* ToolBarItem4;
		wxToolBarToolBase* ToolBarItem1;
		wxMenuBar* MenuBar1;
		wxNotebook* Notebook1;
		wxToolBarToolBase* ToolBarItem3;
		//*)

	protected:

		//(*Identifiers(ddclient_main)
		static const long ID_LISTCTRL2;
		static const long ID_LISTCTRL1;
		static const long ID_PANEL2;
		static const long ID_PANEL3;
		static const long ID_NOTEBOOK1;
		static const long connect_btn;
		static const long add_btn;
		static const long del_btn;
		static const long stop_btn;
		static const long start_btn;
		static const long ID_TOOLBAR1;
		static const long ID_STATUSBAR1;
		//*)

	private:

		//(*Handlers(ddclient_main)
		void OnConnectBtnClicked(wxCommandEvent& event);
		void OnAddBtnClicked(wxCommandEvent& event);
		void OnListCtrl2BeginDrag(wxListEvent& event);
		void OnDownloadListBeginDrag(wxListEvent& event);
		//*)

		DECLARE_EVENT_TABLE()
};

#endif

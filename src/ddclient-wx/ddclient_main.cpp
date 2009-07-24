#include "ddclient-wx_main.h"
#include "connect_dlg.h"
#include "dlg_add.h"
//(*InternalHeaders(ddclient_main)
#include <wx/string.h>
#include <wx/intl.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/artprov.h>
//*)

//(*IdInit(ddclient_main)
const long ddclient_main::ID_LISTCTRL2 = wxNewId();
const long ddclient_main::ID_LISTCTRL1 = wxNewId();
const long ddclient_main::ID_PANEL2 = wxNewId();
const long ddclient_main::ID_PANEL3 = wxNewId();
const long ddclient_main::ID_NOTEBOOK1 = wxNewId();
const long ddclient_main::connect_btn = wxNewId();
const long ddclient_main::add_btn = wxNewId();
const long ddclient_main::del_btn = wxNewId();
const long ddclient_main::stop_btn = wxNewId();
const long ddclient_main::start_btn = wxNewId();
const long ddclient_main::ID_TOOLBAR1 = wxNewId();
const long ddclient_main::ID_STATUSBAR1 = wxNewId();
//*)

BEGIN_EVENT_TABLE(ddclient_main,wxFrame)
	//(*EventTable(ddclient_main)
	//*)
END_EVENT_TABLE()

ddclient_main::ddclient_main(wxWindow* parent,wxWindowID id,const wxPoint& pos,const wxSize& size)
{
	//(*Initialize(ddclient_main)
	wxBoxSizer* BoxSizer3;
	wxBoxSizer* BoxSizer2;
	
	Create(parent, wxID_ANY, _("DDClient-wx - A DownloadDaemon Client"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_FRAME_STYLE, _T("wxID_ANY"));
	SetClientSize(wxSize(776,383));
	Notebook1 = new wxNotebook(this, ID_NOTEBOOK1, wxPoint(88,48), wxDefaultSize, 0, _T("ID_NOTEBOOK1"));
	DownloadList = new wxListView(Notebook1, ID_LISTCTRL2, wxPoint(120,-46), wxSize(0,0), wxLC_LIST|wxLC_REPORT, wxDefaultValidator, _T("ID_LISTCTRL2"));
	DownloadList->SetFocus();
	Panel2 = new wxPanel(Notebook1, ID_PANEL2, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL, _T("ID_PANEL2"));
	BoxSizer2 = new wxBoxSizer(wxHORIZONTAL);
	ListCtrl1 = new wxListCtrl(Panel2, ID_LISTCTRL1, wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, _T("ID_LISTCTRL1"));
	BoxSizer2->Add(ListCtrl1, 1, wxALL|wxEXPAND|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
	Panel2->SetSizer(BoxSizer2);
	BoxSizer2->Fit(Panel2);
	BoxSizer2->SetSizeHints(Panel2);
	Panel3 = new wxPanel(Notebook1, ID_PANEL3, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL, _T("ID_PANEL3"));
	BoxSizer3 = new wxBoxSizer(wxHORIZONTAL);
	Panel3->SetSizer(BoxSizer3);
	BoxSizer3->Fit(Panel3);
	BoxSizer3->SetSizeHints(Panel3);
	Notebook1->AddPage(DownloadList, _("All Downloads"), false);
	Notebook1->AddPage(Panel2, _("Running Downloads"), false);
	Notebook1->AddPage(Panel3, _("Finished Downloads"), false);
	MenuBar1 = new wxMenuBar();
	SetMenuBar(MenuBar1);
	ToolBar1 = new wxToolBar(this, ID_TOOLBAR1, wxDefaultPosition, wxDefaultSize, wxTB_HORIZONTAL|wxNO_BORDER, _T("ID_TOOLBAR1"));
	ToolBarItem1 = ToolBar1->AddTool(connect_btn, _("Connect"), wxArtProvider::GetBitmap(wxART_MAKE_ART_ID_FROM_STR(_T("wxART_EXECUTABLE_FILE")),wxART_TOOLBAR), wxNullBitmap, wxITEM_NORMAL, _("Connect to a DownloadDaemon"), wxEmptyString);
	ToolBar1->AddSeparator();
	ToolBarItem2 = ToolBar1->AddTool(add_btn, _("Add"), wxArtProvider::GetBitmap(wxART_MAKE_ART_ID_FROM_STR(_T("wxART_ADD_BOOKMARK")),wxART_TOOLBAR), wxNullBitmap, wxITEM_NORMAL, _("Adds a new Download"), wxEmptyString);
	ToolBarItem3 = ToolBar1->AddTool(del_btn, _("Delete"), wxArtProvider::GetBitmap(wxART_MAKE_ART_ID_FROM_STR(_T("wxART_DEL_BOOKMARK")),wxART_TOOLBAR), wxNullBitmap, wxITEM_NORMAL, _("Deletes the selected Download"), wxEmptyString);
	ToolBar1->AddSeparator();
	ToolBarItem4 = ToolBar1->AddTool(stop_btn, _("Stop"), wxArtProvider::GetBitmap(wxART_MAKE_ART_ID_FROM_STR(_T("wxART_CROSS_MARK")),wxART_TOOLBAR), wxNullBitmap, wxITEM_NORMAL, _("Stop downloading files"), wxEmptyString);
	ToolBarItem5 = ToolBar1->AddTool(start_btn, _("Start"), wxArtProvider::GetBitmap(wxART_MAKE_ART_ID_FROM_STR(_T("wxART_TICK_MARK")),wxART_TOOLBAR), wxNullBitmap, wxITEM_NORMAL, _("Start downloading files"), wxEmptyString);
	ToolBar1->Realize();
	SetToolBar(ToolBar1);
	StatusBar1 = new wxStatusBar(this, ID_STATUSBAR1, 0, _T("ID_STATUSBAR1"));
	int __wxStatusBarWidths_1[1] = { -10 };
	int __wxStatusBarStyles_1[1] = { wxSB_NORMAL };
	StatusBar1->SetFieldsCount(1,__wxStatusBarWidths_1);
	StatusBar1->SetStatusStyles(1,__wxStatusBarStyles_1);
	SetStatusBar(StatusBar1);
	
	Connect(connect_btn,wxEVT_COMMAND_TOOL_CLICKED,(wxObjectEventFunction)&ddclient_main::OnConnectBtnClicked);
	Connect(add_btn,wxEVT_COMMAND_TOOL_CLICKED,(wxObjectEventFunction)&ddclient_main::OnAddBtnClicked);
	//*)
}

ddclient_main::~ddclient_main()
{
	//(*Destroy(ddclient_main)
	//*)
}


void ddclient_main::OnConnectBtnClicked(wxCommandEvent& event)
{
	connect_dlg* connect = new connect_dlg(this, wxID_ANY, wxDefaultPosition, wxDefaultSize);
	connect->ShowModal();
}


void ddclient_main::OnAddBtnClicked(wxCommandEvent& event)
{
	dlg_add* add = new dlg_add(this, wxID_ANY, wxDefaultPosition, wxDefaultSize);
	add->ShowModal();
}

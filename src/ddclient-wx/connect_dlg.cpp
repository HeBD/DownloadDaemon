#include "connect_dlg.h"
#include "../lib/netpptk/netpptk.h"

extern tkSock srvConnection;
extern bool connected_successfully;

#include <sstream>
#include <wx/msgdlg.h>
//(*InternalHeaders(connect_dlg)
#include <wx/string.h>
#include <wx/intl.h>
//*)

//(*IdInit(connect_dlg)
const long connect_dlg::ID_STATICTEXT1 = wxNewId();
const long connect_dlg::conndlg_host = wxNewId();
const long connect_dlg::ID_STATICTEXT2 = wxNewId();
const long connect_dlg::conndlg_port = wxNewId();
const long connect_dlg::ID_BUTTON1 = wxNewId();
const long connect_dlg::ID_BUTTON2 = wxNewId();
//*)

BEGIN_EVENT_TABLE(connect_dlg,wxDialog)
	//(*EventTable(connect_dlg)
	//*)
END_EVENT_TABLE()

connect_dlg::connect_dlg(wxWindow* parent,wxWindowID id,const wxPoint& pos,const wxSize& size)
{
	//(*Initialize(connect_dlg)
	wxBoxSizer* BoxSizer3;
	wxBoxSizer* BoxSizer2;
	wxBoxSizer* BoxSizer1;

	Create(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE, _T("wxID_ANY"));
	SetClientSize(wxSize(416,243));
	BoxSizer1 = new wxBoxSizer(wxVERTICAL);
	BoxSizer3 = new wxBoxSizer(wxHORIZONTAL);
	StaticText1 = new wxStaticText(this, ID_STATICTEXT1, _("Host:"), wxDefaultPosition, wxSize(42,16), 0, _T("ID_STATICTEXT1"));
	BoxSizer3->Add(StaticText1, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
	hostname_txt = new wxTextCtrl(this, conndlg_host, _("127.0.0.1"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, _T("conndlg_host"));
	BoxSizer3->Add(hostname_txt, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
	StaticText2 = new wxStaticText(this, ID_STATICTEXT2, _("Port:"), wxDefaultPosition, wxDefaultSize, 0, _T("ID_STATICTEXT2"));
	BoxSizer3->Add(StaticText2, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
	port_txt = new wxSpinCtrl(this, conndlg_port, _T("4321"), wxDefaultPosition, wxDefaultSize, 0, 0, 65000, 4321, _T("conndlg_port"));
	port_txt->SetValue(_T("4321"));
	BoxSizer3->Add(port_txt, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
	BoxSizer1->Add(BoxSizer3, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
	BoxSizer2 = new wxBoxSizer(wxHORIZONTAL);
	conn_cancel_btn = new wxButton(this, ID_BUTTON1, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, _T("ID_BUTTON1"));
	BoxSizer2->Add(conn_cancel_btn, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
	conn_conn_btn = new wxButton(this, ID_BUTTON2, _("Connect"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, _T("ID_BUTTON2"));
	conn_conn_btn->SetDefault();
	BoxSizer2->Add(conn_conn_btn, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
	BoxSizer1->Add(BoxSizer2, 1, wxALL|wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL, 5);
	SetSizer(BoxSizer1);
	BoxSizer1->SetSizeHints(this);
	Center();

	Connect(ID_BUTTON1,wxEVT_COMMAND_BUTTON_CLICKED,(wxObjectEventFunction)&connect_dlg::Onconn_cancel_btnClick);
	Connect(ID_BUTTON2,wxEVT_COMMAND_BUTTON_CLICKED,(wxObjectEventFunction)&connect_dlg::Onconn_conn_btnClick);
	//*)
}

connect_dlg::~connect_dlg()
{
	//(*Destroy(connect_dlg)
	//*)
}


void connect_dlg::Onconn_cancel_btnClick(wxCommandEvent& event)
{
	this->Close();
}

void connect_dlg::Onconn_conn_btnClick(wxCommandEvent& event)
{
	std::stringstream ss;
	ss << hostname_txt->GetValue().ToAscii();
	if(srvConnection.connect(ss.str(), port_txt->GetValue())) {
		connected_successfully = true;
		std::string rec;
		//srvConnection >> rec;
		srvConnection.recv(rec);
		if(rec[0] == '0') {
			this->Close();
		} else if(rec[0] == '1') {
			wxMessageBox(_("Password required!"), _("Connect"));
		} else {
			wxMessageBox(_("An error occured..."), _("Connect"));
		}
	} else {
		wxMessageBox(_("Failed to connect."), _("Connect"));
	}
}


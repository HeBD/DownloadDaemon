#include "dlg_add.h"
#include "../lib/netpptk/netpptk.h"
#include <string>
extern tkSock srvConnection;
extern bool connected_successfully;
extern bool enable_poll;

#include <wx/msgdlg.h>
//(*InternalHeaders(dlg_add)
#include <wx/string.h>
#include <wx/intl.h>
//*)

//(*IdInit(dlg_add)
const long dlg_add::ID_STATICTEXT2 = wxNewId();
const long dlg_add::ID_TEXTCTRL2 = wxNewId();
const long dlg_add::ID_BUTTON1 = wxNewId();
const long dlg_add::ID_BUTTON2 = wxNewId();
//*)

BEGIN_EVENT_TABLE(dlg_add,wxDialog)
	//(*EventTable(dlg_add)
	//*)
END_EVENT_TABLE()

dlg_add::dlg_add(wxWindow* parent,wxWindowID id,const wxPoint& pos,const wxSize& size)
{
	//(*Initialize(dlg_add)
	wxBoxSizer* BoxSizer3;
	wxBoxSizer* BoxSizer4;
	wxBoxSizer* BoxSizer1;

	Create(parent, wxID_ANY, _("Add Download"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE, _T("wxID_ANY"));
	SetClientSize(wxSize(411,365));
	BoxSizer1 = new wxBoxSizer(wxVERTICAL);
	BoxSizer3 = new wxBoxSizer(wxHORIZONTAL);
	StaticText2 = new wxStaticText(this, ID_STATICTEXT2, _("Download Links:"), wxDefaultPosition, wxDefaultSize, 0, _T("ID_STATICTEXT2"));
	BoxSizer3->Add(StaticText2, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
	BoxSizer1->Add(BoxSizer3, 1, wxALL|wxEXPAND|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
	add_dl_links = new wxTextCtrl(this, ID_TEXTCTRL2, wxEmptyString, wxDefaultPosition, wxSize(-1,200), wxTE_MULTILINE|wxHSCROLL|wxVSCROLL, wxDefaultValidator, _T("ID_TEXTCTRL2"));
	add_dl_links->SetMinSize(wxSize(500,-1));
	BoxSizer1->Add(add_dl_links, 5, wxALL|wxEXPAND|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
	BoxSizer4 = new wxBoxSizer(wxHORIZONTAL);
	add_cancel_btn = new wxButton(this, ID_BUTTON1, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, _T("ID_BUTTON1"));
	BoxSizer4->Add(add_cancel_btn, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
	add_btn = new wxButton(this, ID_BUTTON2, _("Add"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, _T("ID_BUTTON2"));
	BoxSizer4->Add(add_btn, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
	BoxSizer1->Add(BoxSizer4, 1, wxALL|wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL, 5);
	SetSizer(BoxSizer1);
	BoxSizer1->SetSizeHints(this);

	Connect(ID_BUTTON1,wxEVT_COMMAND_BUTTON_CLICKED,(wxObjectEventFunction)&dlg_add::Onadd_cancel_btnClick);
	Connect(ID_BUTTON2,wxEVT_COMMAND_BUTTON_CLICKED,(wxObjectEventFunction)&dlg_add::Onadd_btnClick);
	Connect(wxID_ANY,wxEVT_CLOSE_WINDOW,(wxObjectEventFunction)&dlg_add::OnClose);
	//*)
	enable_poll = false;
}

dlg_add::~dlg_add()
{
	//(*Destroy(dlg_add)
	//*)
}


void dlg_add::Onadd_btnClick(wxCommandEvent& event)
{
	if(!connected_successfully || !srvConnection) {
		wxMessageBox(_("You have to Connect to a DownloadDaemon in order to add downloads."), _("Add Downloads"));
		return;
	}

	if(add_dl_links->GetNumberOfLines() == 0) {
		wxMessageBox(_("Please put the links to add in the Text-field above, one per line."), _("Add Downloads"));
		return;
	}

	wxString link;
	std::string response;
	for(int i = 0; i < add_dl_links->GetNumberOfLines(); ++i) {
		link = add_dl_links->GetLineText(i);
		std::stringstream ss;
		ss << "DDP ADD " << link.ToAscii();

		srvConnection << ss.str();
		srvConnection >> response;
		if(response.length() < 0 || response.find("-1") != std::string::npos) {
			wxMessageBox(_("Could not add download ") + link, _("Add Downloads"));
		}
	}
	wxMessageBox(_("Downloads added"), _("Add Downloads"));
	this->Close();
}

void dlg_add::OnClose(wxCloseEvent& event)
{
	enable_poll = true;
	Destroy();
}

void dlg_add::Onadd_cancel_btnClick(wxCommandEvent& event)
{
	this->Close();
}

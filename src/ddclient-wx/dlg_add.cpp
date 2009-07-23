#include "dlg_add.h"

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
	TextCtrl2 = new wxTextCtrl(this, ID_TEXTCTRL2, wxEmptyString, wxDefaultPosition, wxSize(-1,200), wxTE_MULTILINE|wxHSCROLL|wxVSCROLL, wxDefaultValidator, _T("ID_TEXTCTRL2"));
	TextCtrl2->SetMinSize(wxSize(500,-1));
	BoxSizer1->Add(TextCtrl2, 5, wxALL|wxEXPAND|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
	BoxSizer4 = new wxBoxSizer(wxHORIZONTAL);
	Button1 = new wxButton(this, ID_BUTTON1, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, _T("ID_BUTTON1"));
	BoxSizer4->Add(Button1, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
	Button2 = new wxButton(this, ID_BUTTON2, _("Add"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, _T("ID_BUTTON2"));
	BoxSizer4->Add(Button2, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
	BoxSizer1->Add(BoxSizer4, 1, wxALL|wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL, 5);
	SetSizer(BoxSizer1);
	BoxSizer1->SetSizeHints(this);

	Connect(ID_BUTTON1,wxEVT_COMMAND_BUTTON_CLICKED,(wxObjectEventFunction)&dlg_add::OnButton1Click);
	//*)
}

dlg_add::~dlg_add()
{
	//(*Destroy(dlg_add)
	//*)
}


void dlg_add::OnButton1Click(wxCommandEvent& event)
{
	this->Close();
}

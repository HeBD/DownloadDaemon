/***************************************************************
 * Name:      frame.cpp
 * Purpose:   Code for Frame Class
 * Author:    ko ()
 * Created:   2009-08-04
 * Copyright: ko ()
 * License:
 **************************************************************/

#include "frame.h"

// IDs
const long myframe::id_menu_quit = wxNewId();
const long myframe::id_menu_about = wxNewId();
const long myframe::id_toolbar = wxNewId();
const long myframe::id_toolbar_connect = wxNewId();
const long myframe::id_toolbar_add = wxNewId();
const long myframe::id_toolbar_del = wxNewId();
const long myframe::id_toolbar_stop = wxNewId();
const long myframe::id_toolbar_start = wxNewId();


myframe::myframe(wxWindow *parent, const wxString &title, wxWindowID id,const wxPoint &pos,const wxSize &size): wxFrame(parent, -1, title){

    SetClientSize(wxSize(700,500));
    SetMinSize(wxSize(700,500));
    CenterOnScreen();
    //SetIcon(); // later

    add_bars();
    add_content();

    Layout();
    Fit();
}


void myframe::add_bars(){

    // menubar
    wxMenuBar *menu = new wxMenuBar();
    SetMenuBar(menu);

    wxMenu* file_menu = new wxMenu(_T(""));
    file_menu->Append(id_menu_quit, _("&Quit\tAlt-F4"), _("Quit"));
    menu->Append(file_menu, _("&File"));

    wxMenu* help_menu = new wxMenu(_T(""));
    help_menu->Append(id_menu_about, _("&About\tF1"), _("About"));
    menu->Append(help_menu, _("&Help"));


    // toolbar with icons
    wxToolBar *toolbar = new wxToolBar(this, id_toolbar);

    // wxToolBarToolBase *connect_item = // not needed
    toolbar->AddTool(id_toolbar_connect, wxT("Connect"), wxArtProvider::GetBitmap(wxART_MAKE_ART_ID_FROM_STR(_T("wxART_EXECUTABLE_FILE")),wxART_TOOLBAR), wxNullBitmap, wxITEM_NORMAL, wxT("Connect to a DownloadDaemon"), wxEmptyString);
    toolbar->AddSeparator();

    toolbar->AddTool(id_toolbar_add, wxT("Add"), wxArtProvider::GetBitmap(wxART_MAKE_ART_ID_FROM_STR(_T("wxART_ADD_BOOKMARK")),wxART_TOOLBAR), wxNullBitmap, wxITEM_NORMAL, wxT("Adds a new Download"), wxEmptyString);
	toolbar->AddTool(id_toolbar_del, wxT("Delete"), wxArtProvider::GetBitmap(wxART_MAKE_ART_ID_FROM_STR(_T("wxART_DEL_BOOKMARK")),wxART_TOOLBAR), wxNullBitmap, wxITEM_NORMAL, wxT("Deletes the selected Download"), wxEmptyString);
	toolbar->AddSeparator();

	toolbar->AddTool(id_toolbar_stop, wxT("Stop"), wxArtProvider::GetBitmap(wxART_MAKE_ART_ID_FROM_STR(_T("wxART_CROSS_MARK")),wxART_TOOLBAR), wxNullBitmap, wxITEM_NORMAL, wxT("Stop downloading files"), wxEmptyString);
	toolbar->AddTool(id_toolbar_start, wxT("Start"), wxArtProvider::GetBitmap(wxART_MAKE_ART_ID_FROM_STR(_T("wxART_TICK_MARK")),wxART_TOOLBAR), wxNullBitmap, wxITEM_NORMAL, wxT("Start downloading files"), wxEmptyString);

	toolbar->Realize();
	SetToolBar(toolbar);


    // statusbar
    CreateStatusBar(2);
    SetStatusText(wxT("I'm not useful atm..."),0);
    SetStatusText(wxT(".."),1);

    return;
}


void myframe::add_content(){

    wxNotebook *notebook = new wxNotebook(this, -1, wxPoint(88,48));
    wxPanel* panel_all = new wxPanel(notebook, -1, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
    wxPanel* panel_running = new wxPanel(notebook, wxNewId(), wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
    wxPanel* panel_finished = new wxPanel(notebook, wxNewId(), wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
    panel_all->SetFocus();

    wxBoxSizer* sizer_all = new wxBoxSizer(wxHORIZONTAL); // lines/rows, colums, vgap, hgap
    wxBoxSizer* sizer_running = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* sizer_finished = new wxBoxSizer(wxHORIZONTAL);

    panel_all->SetSizer(sizer_all);
    panel_running->SetSizer(sizer_running);
    panel_finished->SetSizer(sizer_finished);

    notebook->AddPage(panel_all, wxT("All Downloads"), false);
    notebook->AddPage(panel_running, wxT("Running Downloads"), false);
    notebook->AddPage(panel_finished, wxT("Finished Downloads"), false);


    // all download lists
    wxListCtrl *list[3];
    list[0] = new wxListCtrl(panel_all, -1, wxPoint(120, -46), wxSize(0,0), wxLC_REPORT);
    sizer_all->Add(list[0] , 1, wxALL|wxEXPAND|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
    list[1] = new wxListCtrl(panel_running, -1, wxPoint(120, -46), wxSize(0,0), wxLC_REPORT);
    sizer_running->Add(list[1] , 1, wxALL|wxEXPAND|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
    list[2] = new wxListCtrl(panel_finished, -1, wxPoint(120, -46), wxSize(0,0), wxLC_REPORT);
    sizer_finished->Add(list[2] , 1, wxALL|wxEXPAND|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);


    int num = 0; // for dummy input

    for(int i=0; i<3; i++){
        // columns
        list[i]->InsertColumn(0, wxT("ID"), wxLIST_AUTOSIZE_USEHEADER, -1);
        list[i]->InsertColumn(1, wxT("Added"), wxLIST_AUTOSIZE_USEHEADER, -1);
        list[i]->InsertColumn(2, wxT("Title"), wxLIST_AUTOSIZE_USEHEADER, -1);
        list[i]->InsertColumn(3, wxT("\tURL\t"), wxLIST_AUTOSIZE_USEHEADER, -1);
        list[i]->InsertColumn(4, wxT("Status"), wxLIST_AUTOSIZE_USEHEADER, -1);
        list[i]->InsertColumn(5, wxT("Bytes Done"), wxLIST_AUTOSIZE_USEHEADER, 90);
        list[i]->InsertColumn(6, wxT("Bytes Total"), wxLIST_AUTOSIZE_USEHEADER, 90);
        list[i]->InsertColumn(7, wxT("Wait"), wxLIST_AUTOSIZE_USEHEADER, 50);
        list[i]->InsertColumn(8, wxT("Error"), wxLIST_AUTOSIZE_USEHEADER, -1);

        // dummy input
        wxString buffer;

        int index;

        for(int j=0; j<3; j++){
                buffer.Printf(wxT("Item %d"), num++);
                index = list[i]->InsertItem(0, buffer);

                for(int k=1; k<9; k++){

                        buffer.Printf(wxT("Col %d, item %d"), k, num);
                        list[i]->SetItem(index, k, buffer);
                }
        }
    }

    return;
}


myframe::~myframe(){
}


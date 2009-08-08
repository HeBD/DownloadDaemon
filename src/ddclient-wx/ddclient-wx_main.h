/***************************************************************
 * Name:      ddclient-wx_main.h
 * Purpose:   Header for Frame Class
 * Author:    ko ()
 * Created:   2009-08-04
 * Copyright: ko ()
 * License:
 **************************************************************/

#ifndef DDCLIENT_WX_MAIN_H
#define DDCLIENT_WX_MAIN_H


#include <vector>

#include <wx/msgdlg.h> // for wxmessagebox
#include <wx/frame.h>
#include <wx/panel.h>
#include <wx/notebook.h>
#include <wx/stattext.h>
#include <wx/menu.h>
#include <wx/toolbar.h>
#include <wx/artprov.h>
#include <wx/sizer.h>
#include <wx/listctrl.h>
#include <wx/string.h>

#include "ddclient-wx_connect_dialog.h"
#include "../lib/netpptk/netpptk.h"

class myframe : public wxFrame{
    public:
        myframe(wxWindow *parent, const wxString &title, wxWindowID id=wxID_ANY,const wxPoint& pos=wxDefaultPosition,const wxSize& size=wxDefaultSize);
        ~myframe();

        // getter and setter methods
        void set_connection_attributes(tkSock *mysock, std::string password);
        tkSock *get_connection_attributes();

        void fill_lists(); // set private later

    private:

        // connection attributes
        tkSock *mysock;
        std::string password;

        // elements for bars
        wxMenuBar *menu;
        wxMenu *file_menu;
        wxMenu *help_menu;
        wxToolBar *toolbar;

        // elements for content
        wxNotebook *notebook;
        wxPanel *panel_all;
        wxPanel *panel_running;
        wxPanel *panel_finished;
        wxBoxSizer *sizer_all;
        wxBoxSizer *sizer_running;
        wxBoxSizer *sizer_finished;
        wxListCtrl *list[3];

        // element IDs
        static const long id_menu_quit;
        static const long id_menu_about;
        static const long id_toolbar_connect;
        static const long id_toolbar_add;
        static const long id_toolbar_delete;
        static const long id_toolbar_stop;
        static const long id_toolbar_start;

        void add_bars();
        void add_content();

        // event handle methods
        void on_quit(wxCommandEvent &event);
        void on_about(wxCommandEvent &event);
        void on_connect(wxCommandEvent &event);
        void on_add(wxCommandEvent &event);
        void on_delete(wxCommandEvent &event);
        void on_stop(wxCommandEvent &event);
        void on_start(wxCommandEvent &event);

        DECLARE_EVENT_TABLE()
};

#endif //DDCLIENT_WX_MAIN_H

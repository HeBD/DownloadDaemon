/***************************************************************
 * Name:      frame.h
 * Purpose:   Header for Frame Class
 * Author:    ko ()
 * Created:   2009-08-04
 * Copyright: ko ()
 * License:
 **************************************************************/

#ifndef FRAME_H
#define FRAME_H

#include "picture.h"

#include <vector>
#include <wx/frame.h>
#include <wx/notebook.h>
#include <wx/stattext.h>
#include <wx/menu.h>
#include <wx/toolbar.h>
#include <wx/artprov.h>
#include <wx/sizer.h>
#include <wx/listctrl.h>
#include <wx/string.h>

class myframe : public wxFrame{
    public:
        myframe(wxWindow *parent, const wxString &title, wxWindowID id=wxID_ANY,const wxPoint& pos=wxDefaultPosition,const wxSize& size=wxDefaultSize);
        ~myframe();

    private:
        std::vector<picture> pics; // all pictures
        std::vector<wxStaticText> text; // all texts

        // element IDs
        static const long id_menu_quit;
        static const long id_menu_about;
        static const long id_toolbar;
        static const long id_toolbar_connect;
        static const long id_toolbar_add;
        static const long id_toolbar_del;
        static const long id_toolbar_stop;
        static const long id_toolbar_start;

        void add_bars();
        void add_content();


};


#endif //FRAME_H

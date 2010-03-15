#include "ddclient_gui_update_thread.h"

update_thread::update_thread(ddclient_gui *parent) : parent(parent){
    told = false;
}


void update_thread::run(){
    while(true){ // thread will live till program cancel

        if(!(parent->check_connection(false)) && (told == false)){ // connection failed for the first time => we're calling get_content to update the gui
            parent->get_content();
            told = true;
        }else if(parent->check_connection(false)){ // connection is valid
            parent->get_content();
            told = false;
        }

        parent->clear_last_error_message();
        sleep(2); // reload every two seconds
    }
}

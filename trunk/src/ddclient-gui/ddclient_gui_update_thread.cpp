#include "ddclient_gui_update_thread.h"

update_thread::update_thread(ddclient_gui *parent) : parent(parent){
}


void update_thread::run(){
    while(true){ // thread will live till program cancel

        if(parent->check_connection())
            parent->get_content();

        parent->clear_last_error_message();
        sleep(2); // reload every two seconds
    }
}

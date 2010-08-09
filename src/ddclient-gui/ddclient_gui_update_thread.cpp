/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "ddclient_gui_update_thread.h"

update_thread::update_thread(ddclient_gui *parent, int interval) : parent(parent), subscription_enabled(false), told(false),
    update(true), interval(interval), term(false){
}


void update_thread::run(){

    // first update
    if(!(parent->check_connection(false)) && (told == false)){ // connection failed for the first time => we're calling get_content to update the gui
        parent->get_content();
        told = true;
    }else if(parent->check_connection(false)){ // connection is valid
        parent->get_content();
        told = false;
    }

    parent->clear_last_error_message();


    // check if downloaddaemon supports subscription
    subscription_enabled = parent->check_subscritpion();



    // CAREFUL: what happens, when subscriptions is enabled and you reconnect?
    // do you see the whole list?


    // behaviour with subscriptions => less traffic and cpu usage
    /*if(subscription_enabled){
        while(true){

            if(term)
                return;

            if(!(parent->check_connection(false)) && (told == false)){ // connection failed for the first time => we're calling get_content to update the gui
                parent->get_content(); // called to clear list
                told = true;

                sleep(1); // otherwise we get 100% cpu if there is no connection

            }else if(parent->check_connection(false)){ // connection is valid
                parent->get_content(true); // get updates // lock hier
                told = false;
            }

            parent->clear_last_error_message();
        }

    }*/

    // normal behaviour, if subscription is not enabled
    while(true){

        if(term)
            return;

        if(update){

            if(!(parent->check_connection(false)) && (told == false)){ // connection failed for the first time => we're calling get_content to update the gui
                parent->get_content(); // called to clear list
                told = true;

            }else if(parent->check_connection(false)){ // connection is valid
                parent->get_content();
                told = false;
            }

            parent->clear_last_error_message();
        }

        for(int i = 0; i < interval; i++){
            if(term)
                return;

            sleep(1); // wait till update intervall is finished
        }
    }
}


void update_thread::set_update_interval(int interval){
    this->interval = interval;
}


void update_thread::toggle_updating(){
    update = !update;
}


void update_thread::terminate_yourself(){
    term = true;
}

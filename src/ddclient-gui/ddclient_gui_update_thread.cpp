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
	QMutexLocker lock(parent->get_mutex());
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

    // behaviour with subscriptions => less traffic and cpu usage
    if(subscription_enabled){
        while(true){

            if(term)
                return;

            if(!(parent->check_connection(false)) && (told == false)){ // connection failed for the first time => we're calling get_content to update the gui
                parent->get_content(); // called to clear list
                told = true;

                sleep(1); // otherwise we get 100% cpu if there is no connection

            }else if(parent->check_connection(false)){ // connection is valid
				lock.unlock();
                parent->get_content(true); // get updates // lock hier
				lock.relock();
                told = false;
            }

            parent->clear_last_error_message();
        }

    }

    // normal behaviour, if subscription is not enabled
    for(int i = 0; i < interval; i++){ // wait a little bit, we don't want two updates in a row
        if(term)
            return;
		lock.unlock();
        sleep(1); // wait till update intervall is finished
		lock.relock();
    }

    while(true){

        if(term)
            return;

        if(update){

            if(!(parent->check_connection(false)) && (told == false)){ // connection failed for the first time => we're calling get_content to update the gui
				lock.unlock();
				parent->get_content(); // called to clear list
				lock.relock();
                told = true;

            }else if(parent->check_connection(false)){ // connection is valid
				lock.unlock();
                parent->get_content();
				lock.relock();
                told = false;
            }

            parent->clear_last_error_message();
        }

        for(int i = 0; i < interval; i++){
            if(term)
                return;
			lock.unlock();
            sleep(1); // wait till update intervall is finished
			lock.relock();
        }
    }
}


void update_thread::set_update_interval(int interval){
    this->interval = interval;
}


int update_thread::get_update_interval(){
    return interval;
}


void update_thread::toggle_updating(){
    update = !update;
}


void update_thread::terminate_yourself(){
    term = true;
}

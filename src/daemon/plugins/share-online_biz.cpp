/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "plugin_helpers.h"
#include <curl/curl.h>
#include <cstdlib>
#include <crypt/base64.h>
#include <iostream>
using namespace std;

size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp) {
        std::string *blubb = (std::string*)userp;
        blubb->append((char*)buffer, nmemb);
        return nmemb;
}

int overload_counter = 0;

//Only Free-User support
plugin_status plugin_exec(plugin_input &inp, plugin_output &outp) {
        ddcurl* handle = get_handle();
        std::string resultstr;
        string url = get_url();
        resultstr.clear();

        handle->setopt(CURLOPT_COOKIE, "http://www.share-online.biz; page_language=english");
        handle->setopt(CURLOPT_URL, url.c_str());
        handle->setopt(CURLOPT_WRITEFUNCTION, write_data);
        handle->setopt(CURLOPT_WRITEDATA, &resultstr);

        int success = handle->perform();

        if(success != 0) {
                return PLUGIN_CONNECTION_ERROR;
        }

        //is file deleted?
        if (resultstr.find("The requested file is not available!") != std::string::npos)
            return PLUGIN_FILE_NOT_FOUND;

        //get free User Link
        url = search_between(resultstr, "var url=\"","\"");
        resultstr.clear();
        handle->setopt(CURLOPT_POST,1);
        handle->setopt(CURLOPT_COPYPOSTFIELDS, "dl_free=1&choice=free");
        handle->setopt(CURLOPT_FOLLOWLOCATION, 1);
        handle->setopt(CURLOPT_URL, url.c_str());
        handle->setopt(CURLOPT_WRITEFUNCTION, write_data);
        handle->setopt(CURLOPT_WRITEDATA, &resultstr);
        success = handle->perform();

        if(success != 0) {
                return PLUGIN_CONNECTION_ERROR;
        }

        //this can be true very often, somethimes you need to wait very long time to get a Downloadticket
        //if downloadlink can't get at 30 retrys set waittime to 10 min to prevent IP block from server
        if (resultstr.find("No free slots for free users!") != std::string::npos){
            if (overload_counter < 20){
            overload_counter++;
            set_wait_time(3);
            return PLUGIN_SERVER_OVERLOADED;
            }
            else{
                overload_counter = 0;
                set_wait_time(600);
            }

        }
            //set back counter to 0
        overload_counter = 0;

        if (resultstr.find("This file is too big for you remaining download volume!") != std::string::npos){
            set_wait_time(1800);
            return PLUGIN_SERVER_OVERLOADED;
        }
                //get real link
        string code = search_between(resultstr, "dl=\"","\"");
        string data = base64_decode(code);

                //wait-time until download can start
        string wait = search_between(resultstr, "var wait=",";");
        set_wait_time(atoi(wait.c_str()));

        outp.download_url = data;
        return PLUGIN_SUCCESS;

}

extern "C" void plugin_getinfo(plugin_input &inp, plugin_output &outp) {
        if(!inp.premium_user.empty() && !inp.premium_password.empty()) {
                outp.allows_resumption = true;
                outp.allows_multiple = true;
        } else {
                                outp.allows_resumption = false;
                                outp.allows_multiple = false;
        }
                outp.offers_premium = false;
}

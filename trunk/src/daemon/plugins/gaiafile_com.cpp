/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

//#define PLUGIN_CAN_PRECHECK
//#define PLUGIN_WANTS_POST_PROCESSING
#include "plugin_helpers.h"
#include <curl/curl.h>
#include <cstdlib>
#include <iostream>
using namespace std;

size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp) {
        std::string *blubb = (std::string*)userp;
        blubb->append((char*)buffer, nmemb);
        return nmemb;
}

//Only Free-User support
plugin_status plugin_exec(plugin_input &inp, plugin_output &outp) {
        ddcurl* handle = get_handle();
        std::string resultstr;
        string url = get_url();
        string wait_time;
        string minutes;
        string seconds;

        handle->setopt(CURLOPT_URL, url.c_str());
        handle->setopt(CURLOPT_WRITEFUNCTION, write_data);
        handle->setopt(CURLOPT_WRITEDATA, &resultstr);
        handle->setopt(CURLOPT_COOKIEFILE, "");
        int success = handle->perform();

        if(success != 0) {
                return PLUGIN_CONNECTION_ERROR;
        }

        if (resultstr.find("File Not Found") != std::string::npos)
           return PLUGIN_FILE_NOT_FOUND;

        if (resultstr.find("You have to wait") != std::string::npos)
        {
            wait_time = search_between(resultstr, "You have to wait ", "</b>");
            minutes = search_between(wait_time, "<b>", "minute");
            seconds = search_between(wait_time, ",", "second");
            set_wait_time(atoi(minutes.c_str())*60+atoi(seconds.c_str()));
            return PLUGIN_LIMIT_REACHED;
        }
        wait_time = search_between(resultstr,">Please wait <","/");
        wait_time = search_between(wait_time, ">","<");
        set_wait_time(atoi(wait_time.c_str()));

        string op             = search_between(resultstr, "name=\"op\" value=\"", "\"");
        string id             = search_between(resultstr, "name=\"id\" value=\"","\"");
        string rand           = search_between(resultstr, "name=\"rand\" value=\"","\"");
        string referer        = search_between(resultstr, "name=\"referer\" value=\"","\"");
        string method_free    = search_between(resultstr, "name=\"method_free\" value=\"","\"");
        string method_premium = search_between(resultstr, "name=\"method_premium\" value=\"","\"");
        replace_all(method_free, " ", "+");
        replace_all(referer, "/", "%2F");
        replace_all(referer, ":", "%3A");

        string postdata       = "op=" + op + "&id=" + id + "&rand=" + rand + "&referer=" + referer + "&method_free=" + method_free + "&method_premium=&down_script=1";

        // prepare handle for DD
        handle->setopt(CURLOPT_HTTPPOST,1);
        handle->setopt(CURLOPT_FOLLOWLOCATION, 1);
        handle->setopt(CURLOPT_URL, url.c_str());
        handle->setopt(CURLOPT_COPYPOSTFIELDS, postdata);

        outp.download_url = url;
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

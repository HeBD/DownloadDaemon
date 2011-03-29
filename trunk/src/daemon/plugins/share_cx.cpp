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
        struct curl_slist *headers=NULL;
        headers = curl_slist_append(headers, "Host: share.cx");

        // Form 1 load and save to resultstr
        handle->setopt(CURLOPT_URL, url);
        handle->setopt(CURLOPT_HTTPHEADER, headers);
        handle->setopt(CURLOPT_COOKIEFILE, "");
        handle->setopt(CURLOPT_WRITEFUNCTION, write_data);
        handle->setopt(CURLOPT_WRITEDATA, &resultstr);
        int success = handle->perform();

        if(success != 0) {
                return PLUGIN_CONNECTION_ERROR;
        }

        // Setup post data Form1
        string op          = search_between(resultstr, "name=\"op\" value=\"", "\"");
        string usr_login   = search_between(resultstr, "name=\"usr_login\" value=\"","\"");
        string id          = search_between(resultstr, "name=\"id\" value=\"","\"");
        string fname       = search_between(resultstr, "name=\"fname\" value=\"","\"");
        string referer     = search_between(resultstr, "name=\"referer\" value=\"","\"");
        string method_free = search_between(resultstr, "name=\"method_free\" value=\"","\"");
        replace_all(method_free, " ", "+");
        string postdata    = "op=" + op + "&usr_login=" + usr_login + "&id=" + id + "&fname=" + fname +
                             "&referer=" + referer + "&method_free=" + method_free;

        // post data to Form1, to get Form2
        handle->setopt(CURLOPT_HTTPPOST,1);
        handle->setopt(CURLOPT_FOLLOWLOCATION, 0);
        handle->setopt(CURLOPT_URL, url.c_str());
        handle->setopt(CURLOPT_COPYPOSTFIELDS, postdata);
        handle->setopt(CURLOPT_WRITEFUNCTION, write_data);
        handle->setopt(CURLOPT_WRITEDATA, &resultstr);
        success = handle->perform();
        string Timer = search_between(resultstr, "> startTimer(",")");
        if (Timer != ""){
            set_wait_time(atoi(Timer.c_str())+1);
            return PLUGIN_LIMIT_REACHED;
        }
        // Setup post data Form2
        string op2             = search_between(resultstr, "name=\"op\" value=\"", "\"");
        string id2             = search_between(resultstr, "name=\"id\" value=\"","\"");
        string rand2           = search_between(resultstr, "name=\"rand\" value=\"","\"");
        string referer2        = search_between(resultstr, "name=\"referer\" value=\"","\"");
        string method_free2    = search_between(resultstr, "name=\"method_free\" value=\"","\"");
        string method_premium2 = search_between(resultstr, "name=\"method_premium\" value=\"","\"");
        string down_script2    = search_between(resultstr, "name=\"down_script\" value=\"","\"");
        string wait = search_between(resultstr, "countdown\">","<");
        replace_all(method_free2, " ", "+");
        replace_all(referer2, "/", "%2F");
        replace_all(referer2, ":", "%3A");
        postdata = "op=" + op2 + "&id=" + id2 + "&rand=" + rand2 + "&referer=" + referer2 +
                   "&method_free=" + method_free2 + "&method_premium=" + method_premium2 +
                   "&down_script=" + down_script2;

        //Set Waittime to receive real Link
        set_wait_time(atoi(wait.c_str()));

        // prepare handle for DD
        handle->setopt(CURLOPT_HTTPPOST,1);
        handle->setopt(CURLOPT_FOLLOWLOCATION, 0);
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

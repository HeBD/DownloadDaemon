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

plugin_status plugin_exec(plugin_input &inp, plugin_output &outp) {
	CURL* handle = get_handle();

	string result;
	curl_easy_setopt(handle, CURLOPT_URL, get_url());
	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_data);
	curl_easy_setopt(handle, CURLOPT_WRITEDATA, &result);
	curl_easy_setopt(handle, CURLOPT_COOKIEFILE, "");
	int res = curl_easy_perform(handle);
	if(res != 0) {
		return PLUGIN_ERROR;
	}

	string url = get_url();

	size_t pos = url.find("v=") + 2;
	std::string video_id = url.substr(pos, url.find("&", pos) - pos);

	pos = result.find("\"t\": \"") + 6;
	std::string t = result.substr(pos, result.find("\"", pos) - pos);

	pos = result.find("<meta name=\"title\" content=\"") + 28;
	std::string title = result.substr(pos, result.find("\">", pos) - pos);

	outp.download_url = "http://youtube.com/get_video?video_id=" + video_id + "&t=" + t;
	outp.download_filename = title + ".flv";
	return PLUGIN_SUCCESS;

}


extern "C" void plugin_getinfo(plugin_input &inp, plugin_output &outp) {
	if(!inp.premium_user.empty() && !inp.premium_password.empty()) {
		outp.allows_resumption = false;
		outp.allows_multiple = true;
	} else {
		outp.allows_resumption = false;
		outp.allows_multiple = true;
	}
	outp.offers_premium = false; // no login support yet
}

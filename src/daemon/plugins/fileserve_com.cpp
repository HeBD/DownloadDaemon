/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#define PLUGIN_CAN_PRECHECK
#include "plugin_helpers.h"
#include <curl/curl.h>
#include <cstdlib>
#include <iostream>
#include <fstream>
using namespace std;

size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp) {
	std::string *blubb = (std::string*)userp;
	blubb->append((char*)buffer, nmemb);
	return nmemb;
}

plugin_status plugin_exec(plugin_input &inp, plugin_output &outp) {
	if(inp.premium_user.empty() || inp.premium_password.empty()) {
			return PLUGIN_AUTH_FAIL; // free download not supported yet
	}
	string result;
	ddcurl* handle = get_handle();
	string data = "loginUserName=" + handle->escape(inp.premium_user) + "&loginUserPassword=" + handle->escape(inp.premium_password) +
				  "&autoLogin=on&recaptcha_response_field=&recaptcha_challenge_field=&recaptcha_shortencode_field=&loginFormSubmit=Login";
	handle->setopt(CURLOPT_URL, "http://fileserve.com/login.php");
	handle->setopt(CURLOPT_WRITEFUNCTION, write_data);
	handle->setopt(CURLOPT_WRITEDATA, &result);
	handle->setopt(CURLOPT_COPYPOSTFIELDS, data.c_str());

	int ret = handle->perform();
	if(ret != CURLE_OK)
		return PLUGIN_CONNECTION_ERROR;
	if(ret == CURLE_OK) {
		if(result.find("Invalid login.") != string::npos) {
			return PLUGIN_AUTH_FAIL;
		}
	}

	outp.download_url = get_url();
	return PLUGIN_SUCCESS;
}

bool get_file_status(plugin_input &inp, plugin_output &outp) {
	ddcurl handle;
	string result;
	handle.setopt(CURLOPT_URL, inp.url);
	handle.setopt(CURLOPT_WRITEFUNCTION, write_data);
	handle.setopt(CURLOPT_WRITEDATA, &result);
	int ret = handle.perform();
	if(ret != CURLE_OK) return true;
	result = search_between(result, "<div class=\"panel file_download\">", "<a class=\"addthis_button\"");
	if(result.empty()) {
		outp.file_online = PLUGIN_FILE_NOT_FOUND;
		return true;
	}
	try {
		outp.download_filename = search_between(result, "<h1>", "<br/>");
		string size_total = search_between(result, "left;\"><strong>", "</strong>");
		vector<string> size_split = split_string(size_total, " ");
		filesize_t size = string_to_long(size_split[0]);

		if(size_split[1] == "MB")
			size *= 1024*1024;
		else if(size_split[1] == "KB")
			size *= 1024;
		else if(size_split[1] == "GB")
			size *= 1024*1024*1024;

		outp.file_size = size;
	} catch(...) {
		outp.file_online = PLUGIN_FILE_NOT_FOUND;
		return true;
	}
	return true;
}

extern "C" void plugin_getinfo(plugin_input &inp, plugin_output &outp) {
	if(!inp.premium_user.empty() && !inp.premium_password.empty()) {
		outp.allows_resumption = true;
		outp.allows_multiple = true;
	} else {
		outp.allows_resumption = false;
		outp.allows_multiple = false;
	}
	outp.offers_premium = true;
}

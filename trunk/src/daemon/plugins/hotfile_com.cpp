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
using namespace std;

size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp) {
	std::string *blubb = (std::string*)userp;
	blubb->append((char*)buffer, nmemb);
	return nmemb;
}

void form_opt(const string &result, size_t &pos, const string &var, string &val) {
	pos = result.find(var, pos);
	pos = result.find("value=", pos) + 6;
	val = result.substr(pos, result.find(">", pos) - pos);
	pos += val.size();

}

plugin_status plugin_exec(plugin_input &inp, plugin_output &outp) {
	CURL* handle = get_handle();
	outp.allows_multiple = false;
	outp.allows_resumption = false;

	std::string resultstr;
	string url = get_url();
	url += "&lang=en";
	curl_easy_setopt(handle, CURLOPT_URL, get_url());
	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_data);
	curl_easy_setopt(handle, CURLOPT_WRITEDATA, &resultstr);
	int success = curl_easy_perform(handle);

	if(success != 0) {
		return PLUGIN_CONNECTION_ERROR;
	}

	if(resultstr.find("This file is either removed") != std::string::npos || resultstr.find("404 - Not Found") != std::string::npos) {
		return PLUGIN_FILE_NOT_FOUND;
	}

	size_t pos;
	if((pos = resultstr.find("function starthtimer(){")) == std::string::npos) {
		return PLUGIN_ERROR;
	}
	pos = resultstr.find("d.getTime()+", pos);
	pos += 12;
	string wait_t = resultstr.substr(pos, resultstr.find(";", pos) - pos);
	int wait = atoi(wait_t.c_str());
	wait /= 1000;
	if(wait > 0) {
		set_wait_time(wait);
		return PLUGIN_LIMIT_REACHED;
	}

	set_wait_time(60);

	pos = resultstr.find("action=\"", pos);
	pos += 8;
	size_t end = resultstr.find('\"', pos);

	url = "http://hotfile.com" + resultstr.substr(pos, end - pos);
	url += "&lang=en";

	curl_easy_setopt(handle, CURLOPT_LOW_SPEED_LIMIT, 100);
	curl_easy_setopt(handle, CURLOPT_LOW_SPEED_TIME, 20);
	curl_easy_setopt(handle, CURLOPT_URL, url.c_str());
	curl_easy_setopt(handle, CURLOPT_POST, 1);
	//curl_easy_setopt(prepare_handle, CURLOPT_COPYPOSTFIELDS, "dl.start=\"Free\"");
	string action, tm, tmhash, wait_var, waithash;
	form_opt(resultstr, pos, "action", action);
	form_opt(resultstr, pos, "tm", tm);
	form_opt(resultstr, pos, "tmhash", tmhash);
	form_opt(resultstr, pos, "wait", wait_var);
	form_opt(resultstr, pos, "waithash", waithash);
	string post_data = "action=" + action + "&tm=" + tm + "&tmhash=" + tmhash + "&wait=" + wait_var + "&waithash=" + waithash;
	curl_easy_setopt(handle, CURLOPT_COPYPOSTFIELDS, post_data.c_str());

	resultstr.clear();

	int to_wait = 60;
	while(to_wait > 0) {
		set_wait_time(to_wait--);
		sleep(1);
	}

	curl_easy_perform(handle);
	curl_easy_setopt(handle, CURLOPT_POST, 0);
	curl_easy_setopt(handle, CURLOPT_COPYPOSTFIELDS, "");

	pos = resultstr.find("<td><a href=\"");
	pos += 13;
	end = resultstr.find("\"", pos);
	outp.download_url = resultstr.substr(pos, end - pos);

	return PLUGIN_SUCCESS;

}

bool get_file_status(plugin_input &inp, plugin_output &outp) {
	std::string url = get_url();
	url += "&lang=en";
	std::string result;
	CURL* handle = curl_easy_init();
	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_data);
	curl_easy_setopt(handle, CURLOPT_WRITEDATA, &result);
	curl_easy_setopt(handle, CURLOPT_COOKIEFILE, "");
	curl_easy_setopt(handle, CURLOPT_URL, url.c_str());
	int res = curl_easy_perform(handle);
	curl_easy_cleanup(handle);

	ofstream tmp("/home/ben/Desktop/log.html");
	tmp.write(result.c_str(), result.size());
	if(res != 0) {
		outp.file_online = PLUGIN_CONNECTION_LOST;
		return true;
	}
	try {
		size_t n = result.find("Downloading:");
		if(n == string::npos) {
			outp.file_online = PLUGIN_FILE_NOT_FOUND;
			return true;
		}
		n = result.find("<strong>", n);
		if(n == string::npos) {
			outp.file_online = PLUGIN_FILE_NOT_FOUND;
			return true;
		}
		n += 8;
		size_t end = result.find(" ", n);
		long size = atol(result.substr(n, end - n).c_str());
		size *= 1024 * 1024; // yes, rapidshare uses this factor.
		outp.file_online = PLUGIN_SUCCESS;
		outp.file_size = (long)size;
	} catch(...) {
		outp.file_online = PLUGIN_FILE_NOT_FOUND;
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
	outp.offers_premium = false;
}

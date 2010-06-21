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
#define PLGFILE depositfiles_com
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
	CURL* prepare_handle = get_handle();

	std::string resultstr;
	std::string url = get_url();
	replace_all(url, "/ru/", "/en/");
	replace_all(url, "/de/", "/en/");
	replace_all(url, "/es/", "/en/");
	replace_all(url, "/pt/", "/en/");

	curl_easy_setopt(prepare_handle, CURLOPT_URL, url.c_str());
	curl_easy_setopt(prepare_handle, CURLOPT_WRITEFUNCTION, write_data);
	curl_easy_setopt(prepare_handle, CURLOPT_WRITEDATA, &resultstr);
	int success = curl_easy_perform(prepare_handle);

	if(success != 0) {
		return PLUGIN_CONNECTION_ERROR;
	}

	if(resultstr.find("file does not exist") != string::npos) { // FILE NOT FOUND
		return PLUGIN_FILE_NOT_FOUND;
	}

	size_t pos = resultstr.find("<tr class=\"grayline\">");
	pos = resultstr.find("action=\"", pos) + 8;
	string next_url = "http://depositfiles.com" + resultstr.substr(pos, resultstr.find("\"", pos) - pos);
	curl_easy_setopt(prepare_handle, CURLOPT_URL, next_url.c_str());
	curl_easy_setopt(prepare_handle, CURLOPT_POST, 1);
	curl_easy_setopt(prepare_handle, CURLOPT_COPYPOSTFIELDS, "gateway_result=1");
	resultstr.clear();
	success = curl_easy_perform(prepare_handle);

	if(success != 0) {
		return PLUGIN_CONNECTION_ERROR;
	}

	pos = resultstr.find("<div id=\"download_url\"");
	pos = resultstr.find("action=\"", pos) + 8;
	outp.download_url = resultstr.substr(pos, resultstr.find("\"", pos) - pos);
	return PLUGIN_SUCCESS;
}

bool get_file_status(plugin_input &inp, plugin_output &outp) {
	std::string url = get_url();
	replace_all(url, "/ru/", "/en/");
	replace_all(url, "/de/", "/en/");
	replace_all(url, "/es/", "/en/");
	replace_all(url, "/pt/", "/en/");
	std::string result;
	CURL* handle = curl_easy_init();
	curl_easy_setopt(handle, CURLOPT_LOW_SPEED_LIMIT, (long)10);
	curl_easy_setopt(handle, CURLOPT_LOW_SPEED_TIME, (long)20);
	curl_easy_setopt(handle, CURLOPT_CONNECTTIMEOUT, (long)30);
	curl_easy_setopt(handle, CURLOPT_NOSIGNAL, 1);
	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_data);
	curl_easy_setopt(handle, CURLOPT_WRITEDATA, &result);
	curl_easy_setopt(handle, CURLOPT_COOKIEFILE, "");
	curl_easy_setopt(handle, CURLOPT_URL, url.c_str());
	int res = curl_easy_perform(handle);
	curl_easy_cleanup(handle);
	if(res != 0) {
		outp.file_online = PLUGIN_CONNECTION_LOST;
		return true;
	}
	try {
		size_t n = result.find("File size: <b>");
		if(n == string::npos) {
			outp.file_online = PLUGIN_FILE_NOT_FOUND;
			return true;
		}
		n += 14;
		size_t end = result.find("</b>", n) ;
		vector<string> value = split_string(result.substr(n, end - n), "&nbsp;");
		const char * oldlocale = setlocale(LC_NUMERIC, "C");
		double size = strtod(value[0].c_str(), NULL);
		setlocale(LC_NUMERIC, oldlocale);
		if(value[1] == "MB")
			size *= 1024*1024;
		else if(value[1] == "KB")
			size *= 1024;

		outp.file_online = PLUGIN_SUCCESS;
		outp.file_size = size;
	} catch(...) {
		outp.file_online = PLUGIN_FILE_NOT_FOUND;
	}
	return true;
}

extern "C" void plugin_getinfo(plugin_input &inp, plugin_output &outp) {
	//if(!inp.premium_user.empty() && !inp.premium_password.empty()) {
	//	outp.allows_resumption = true;
	//	outp.allows_multiple = true;
	//} else {
		outp.allows_resumption = false;
		outp.allows_multiple = false;
	//}
	outp.offers_premium = false;
}
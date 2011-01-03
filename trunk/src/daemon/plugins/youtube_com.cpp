/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#define PLGFILE youtube_com
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
	ddcurl* handle = get_handle();

	string result;
	handle->setopt(CURLOPT_URL, get_url());
	handle->setopt(CURLOPT_WRITEFUNCTION, write_data);
	handle->setopt(CURLOPT_WRITEDATA, &result);
	handle->setopt(CURLOPT_COOKIEFILE, "");
	int res = handle->perform();
	if(res != 0) {
		return PLUGIN_ERROR;
	}

	string title = search_between(result, "document.title = '", "';");
	title.erase(0, 9); // remote the "YouTube -" in the beginning
	trim_string(title);
	make_valid_filename(title);
	outp.download_filename = title + ".flv";

	string url = get_url();
	size_t pos = result.find("var swfHTML");
	if(pos == string::npos) {
		return PLUGIN_FILE_NOT_FOUND;
	}
	string fmt_url_map = search_between(result, "fmt_url_map=", "&");
	fmt_url_map = ddcurl::unescape(fmt_url_map);
	vector<string> fumv = split_string(fmt_url_map, ",");
	map<int, string> fum;
	int best_fmt = 0;
	for(vector<string>::iterator it = fumv.begin(); it != fumv.end(); ++it) {
		vector<string> tmp = split_string(*it, "|");
		if(tmp.size() != 2) continue;
		int fmt = atoi(tmp[0].c_str());
		fum.insert(make_pair(fmt, tmp[1]));
		if(fmt > best_fmt) best_fmt = fmt;
	}
	outp.download_url = fum[best_fmt];

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

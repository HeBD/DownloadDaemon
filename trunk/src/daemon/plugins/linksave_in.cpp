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
#define PLGFILE linksave_in
#include "plugin_helpers.h"
#include <curl/curl.h>
#include <crypt/base64.h>
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
	string dlclink = search_between(result, "document.getElementById('dlc_link').href=unescape('", "');");
	if(!dlclink.empty()) {
		string oldresult = result;
		dlclink = "http://linksave.in/" + ddcurl::unescape(dlclink);
		handle->setopt(CURLOPT_URL, dlclink);
		result.clear();
		res = handle->perform();
		if(res == CURLE_OK) {
			// we have a dlc file.. use it
			download_container d;
			decode_dlc(result, &d);
			replace_this_download(d);
			return PLUGIN_SUCCESS;
		}
		result = oldresult;
	}
	
	vector<string> links = search_all_between(result, "<a href=\"", "\" onclick=\"javascript:document.getElementById('img", 0, true);
	
	download_container d;
	for(vector<string>::iterator it = links.begin(); it != links.end(); ++it) {
		if(it->empty()) continue;
		try {
			result.clear();
			handle->setopt(CURLOPT_URL, *it);
			handle->perform();
			string url = search_between(result, "scrolling=\"auto\" noresize src=\"", "\">");
			handle->setopt(CURLOPT_URL, url);
			result.clear();
			handle->perform();
			result = ddcurl::unescape(ddcurl::unescape(result)); // double-urldecode
			string b64 = search_between(result, "a('", "')");
			if(b64.empty()) { // happens sometimes, we are already at the link
				size_t urlpos = result.find("&#104;&#116;&#116;&#112;");
				if(urlpos == string::npos) continue;
				result = result.substr(urlpos, result.find_first_of("\"'", urlpos) - urlpos);
			} else {
				b64 = base64_decode(b64);
				size_t urlpos = b64.find("http://linksave.in/dl-");
				if(urlpos == string::npos) continue;
				result = b64.substr(urlpos, b64.find_first_of("\"'", urlpos) - urlpos);
				handle->setopt(CURLOPT_URL, result);
				result.clear();
				handle->perform();
				// we now have the html-encoded link and just have to decode it...
				result = search_between(result, "<iframe src=\"", "\"");
			}
			vector<string> chars = search_all_between(result, "&#", ";");
			result.clear();
			for(vector<string>::iterator it = chars.begin(); it != chars.end(); ++it) {
				result += (char)atoi(it->c_str());
			}
			d.add_download(result, "");
		} catch (...) {} // ignore errors
	}
	replace_this_download(d);
	return PLUGIN_SUCCESS;
}

bool get_file_status(plugin_input &inp, plugin_output &outp) {
	return false;
}

extern "C" void plugin_getinfo(plugin_input &inp, plugin_output &outp) {
	outp.allows_resumption = false;
	outp.allows_multiple = false;

	outp.offers_premium = false;
}

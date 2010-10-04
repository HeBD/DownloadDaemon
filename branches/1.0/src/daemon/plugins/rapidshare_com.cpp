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
#define PLUGIN_WANTS_POST_PROCESSING
#define PLGFILE rapidshare_com
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

bool use_premium = true; // if the premium limit is exceeded, this is set to false and we restart the download from the beginning without premium

plugin_status plugin_exec(plugin_input &inp, plugin_output &outp) {
	if(!inp.premium_user.empty() && !inp.premium_password.empty() && use_premium) {
		ddcurl* handle = get_handle();
		outp.allows_multiple = true;
		outp.allows_resumption = true;
		string post_data("sub=getaccountdetails_v1&withcookie=1&login=" + handle->escape(inp.premium_user) + "&password=" + handle->escape(inp.premium_password));
		string result;
		handle->setopt(CURLOPT_URL, string("http://api.rapidshare.com/cgi-bin/rsapi.cgi?" + post_data).c_str());
		handle->setopt(CURLOPT_WRITEFUNCTION, write_data);
		handle->setopt(CURLOPT_WRITEDATA, &result);
		handle->setopt(CURLOPT_COOKIEFILE, "");
		int res = handle->perform();
		if(res == 0) {
			if(result.find("Login failed") != string::npos) {
				return PLUGIN_AUTH_FAIL;
			}

			outp.download_url = get_url();
			// get the handle ready (get cookies)
			handle->setopt(CURLOPT_POST, 0);
			size_t pos = result.find("cookie=");
			if(pos == string::npos)
				return PLUGIN_AUTH_FAIL;

			string tmp = result.substr(pos + 7);
			tmp = tmp.substr(0, tmp.find_first_of(" \r\n"));
			handle->setopt(CURLOPT_COOKIE, string("enc=" + tmp).c_str());
			return PLUGIN_SUCCESS;
		}
		return PLUGIN_ERROR;
	}
	use_premium = true;

	ddcurl* handle = get_handle();

	std::string resultstr;
	string url = get_url();
	vector<string> splitted_url = split_string(url, "/");
	string filename = splitted_url.back();
	string fileid = *(splitted_url.end() - 2);
	string dispatch_url = "http://api.rapidshare.com/cgi-bin/rsapi.cgi?sub=download_v1&fileid=" + fileid + "&filename=" + filename + "&try=1&cbf=RSAPIDispatcher&cbid=1";
	handle->setopt(CURLOPT_URL, dispatch_url);
	handle->setopt(CURLOPT_WRITEFUNCTION, write_data);
	handle->setopt(CURLOPT_WRITEDATA, &resultstr);
	int success = handle->perform();

	if(success != 0) {
		return PLUGIN_CONNECTION_ERROR;
	}

	vector<string> dispatch_data = split_string(resultstr, ",");
	if(dispatch_data.size() >= 2) {
		// everything seems okay
		if(dispatch_data[1].find("DL:") == string::npos) return PLUGIN_FILE_NOT_FOUND;
		url = "http://" + dispatch_data[1].substr(dispatch_data[1].find(":") + 1) + "/cgi-bin/rsapi.cgi?sub=download_v1&editparentlocation=0&bin=1&fileid=" + fileid;
		url += "&filename=" + filename + "&dlauth=" + dispatch_data[2];
		handle->setopt(CURLOPT_URL, url.c_str());
		set_wait_time(atoi(dispatch_data[3].c_str()) + 1);
		outp.download_filename = handle->unescape(filename);
		outp.download_url = url;
		return PLUGIN_SUCCESS;
	}
	return PLUGIN_ERROR;
}

bool get_file_status(plugin_input &inp, plugin_output &outp) {
	vector<string> splitted_url = split_string(get_url(), "/");
	if(splitted_url.size() <= 2) return true;
	string filename = splitted_url.back();
	string fileid = *(splitted_url.end() - 2);
	string url = "http://api.rapidshare.com/cgi-bin/rsapi.cgi?sub=checkfiles_v1&files=" + fileid + "&filenames=" + filename;
	std::string result;
	ddcurl handle;
	handle.setopt(CURLOPT_LOW_SPEED_LIMIT, (long)10);
	handle.setopt(CURLOPT_LOW_SPEED_TIME, (long)20);
	handle.setopt(CURLOPT_CONNECTTIMEOUT, (long)30);
	handle.setopt(CURLOPT_NOSIGNAL, 1);
	handle.setopt(CURLOPT_WRITEFUNCTION, write_data);
	handle.setopt(CURLOPT_WRITEDATA, &result);
	handle.setopt(CURLOPT_COOKIEFILE, "");
	handle.setopt(CURLOPT_URL, url.c_str());
	int res = handle.perform();
	handle.cleanup();
	if(res != 0) {
		outp.file_online = PLUGIN_CONNECTION_LOST;
		return true;
	}
	try {
		vector<string> answer = split_string(result, ",");
		long size = atoi(answer[2].c_str());
		if (size == 0) {
			outp.file_online = PLUGIN_FILE_NOT_FOUND;
			return true;
		}
		outp.file_online = PLUGIN_SUCCESS;
		outp.file_size = size;
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
	outp.offers_premium = true;
}

void post_process_download(plugin_input &inp) {
	if(inp.premium_user.empty() || inp.premium_password.empty()) return;
	string filename = dl_list->get_output_file(dlid);
	struct pstat st;
	if(pstat(filename.c_str(), &st) != 0) {
		return;
	}
	if(st.st_size > 1024 * 100 /* 100kb */ ) {
		return;
	}
	ifstream ifs(filename.c_str());
	string tmp;
	char buf[1024];
	while(ifs) {
		ifs.read(buf, 1024);
		tmp.append(buf, ifs.gcount());
	}

	if(tmp.find("You have exceeded the download limit.") != string::npos) {
		use_premium = false;
		dl_list->set_status(dlid, DOWNLOAD_PENDING);
		dl_list->set_error(dlid, PLUGIN_SUCCESS);
		remove(filename.c_str());
		dl_list->set_output_file(dlid, "");
	}

}
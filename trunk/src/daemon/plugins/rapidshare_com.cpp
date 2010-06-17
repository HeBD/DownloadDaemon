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
		CURL* handle = get_handle();
		outp.allows_multiple = true;
		outp.allows_resumption = true;

		string post_data("uselandingpage=1&login=" + inp.premium_user + "&password=" + inp.premium_password);
		string result;
		curl_easy_setopt(handle, CURLOPT_URL, "https://ssl.rapidshare.com/cgi-bin/premiumzone.cgi");
		curl_easy_setopt(handle, CURLOPT_POST, 1);
		curl_easy_setopt(handle, CURLOPT_POSTFIELDS, post_data.c_str());
		curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_data);
		curl_easy_setopt(handle, CURLOPT_WRITEDATA, &result);
		curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, false);
		curl_easy_setopt(handle, CURLOPT_COOKIEFILE, "");
		int res = curl_easy_perform(handle);
		if(res == 0) {
			if(result.find("Forgotten your password?") != string::npos) {
				return PLUGIN_AUTH_FAIL;
			}

			outp.download_url = get_url();
			// get the handle ready (get cookies)
			curl_easy_setopt(handle, CURLOPT_POST, 0);

			string orig_url = get_url();
			// as of DownloadDaemon 0.9, this should not be needed any more, since rapidshare sends the Content-Disposition header that includes the filename
			//if(orig_url.find(".html") != string::npos) {
			//	size_t n = orig_url.find_last_of("/\\");
			//	outp.download_filename = orig_url.substr(n + 1, orig_url.find(".html") - n - 1);
			//}
			outp.download_url = orig_url;
			return PLUGIN_SUCCESS;
		}
		return PLUGIN_ERROR;
	}
	use_premium = true;

	CURL* handle = get_handle();

	std::string resultstr;
	curl_easy_setopt(handle, CURLOPT_URL, get_url());
	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_data);
	curl_easy_setopt(handle, CURLOPT_WRITEDATA, &resultstr);
	int success = curl_easy_perform(handle);

	if(success != 0) {
		return PLUGIN_CONNECTION_ERROR;
	}

	if(resultstr.find("The file could not be found") != std::string::npos || resultstr.find("404 Not Found") != std::string::npos || resultstr.find("file has been removed") != std::string::npos
		|| resultstr.find("suspected to contain illegal content and has been blocked") != std::string::npos || resultstr.find("The uploader has removed this file from the server") != std::string::npos) {
		return PLUGIN_FILE_NOT_FOUND;
	}

	size_t pos;
	if((pos = resultstr.find("<form id=\"ff\"")) == std::string::npos) {
		return PLUGIN_ERROR;
	}

	std::string url;
	pos = resultstr.find("http://", pos);
	size_t end = resultstr.find('\"', pos);

	url = resultstr.substr(pos, end - pos);

	resultstr.clear();
	curl_easy_setopt(handle, CURLOPT_URL, url.c_str());
	curl_easy_setopt(handle, CURLOPT_POST, 1);
	curl_easy_setopt(handle, CURLOPT_COPYPOSTFIELDS, "dl.start=\"Free\"");
	curl_easy_perform(handle);
	curl_easy_setopt(handle, CURLOPT_POST, 0);
	curl_easy_setopt(handle, CURLOPT_COPYPOSTFIELDS, "");

	if(resultstr.find("Or try again in about") != std::string::npos) {
		pos = resultstr.find("Or try again in about");
		pos = resultstr.find("about", pos);
		pos = resultstr.find(' ', pos);
		++pos;
		end = resultstr.find(' ', pos);
		std::string have_to_wait(resultstr.substr(pos, end - pos));
		set_wait_time(atoi(have_to_wait.c_str()) * 60);
		return PLUGIN_LIMIT_REACHED;
	} else if(resultstr.find("Please try again in 2 minutes") != std::string::npos
			  || resultstr.find("wait 2 minutes") != std::string::npos
			  || resultstr.find("our servers are overloaded") != std::string::npos) {
		set_wait_time(120);
		return PLUGIN_SERVER_OVERLOADED;
	} else if(resultstr.find("is already downloading a file.  Please wait until the download is completed.") != std::string::npos) {
		set_wait_time(120);
		return PLUGIN_LIMIT_REACHED;
	} else {
		if((pos = resultstr.find("var c")) == std::string::npos) {
			// Unable to get the wait time, will wait 135 seconds
			set_wait_time(135);
		} else {
			pos = resultstr.find("=", pos) + 1;
			end = resultstr.find(";", pos) - 1;
			set_wait_time(atoi(resultstr.substr(pos, end).c_str()));
		}

		if((pos = resultstr.find("<form name=\"dlf\"")) == std::string::npos) {
			return PLUGIN_ERROR;
		}

		pos = resultstr.find("http://", pos);
		end = resultstr.find('\"', pos);
		if(pos == string::npos || end == string::npos || pos == end) {
			return PLUGIN_ERROR;
		}

		outp.download_url = resultstr.substr(pos, end - pos);
		curl_easy_setopt(handle, CURLOPT_POST, 1);
		curl_easy_setopt(handle, CURLOPT_COPYPOSTFIELDS, "mirror=on&x=66&y=61");
		return PLUGIN_SUCCESS;
	}
}

bool get_file_status(plugin_input &inp, plugin_output &outp) {
	std::string url = get_url();
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
		size_t n = result.find("<p class=\"downloadlink\">");
		if(n == string::npos) {
			outp.file_online = PLUGIN_FILE_NOT_FOUND;
			return true;
		}
		n = result.find("| ", n);
		if(n == string::npos) {
			outp.file_online = PLUGIN_FILE_NOT_FOUND;
			return true;
		}
		n += 2;
		size_t end = result.find(" ", n);
		int size = atoi(result.substr(n, end - n).c_str()); // int is enough. rapidshare files are max. 200 MB
		size *= 1000; // yes, rapidshare uses this factor.
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

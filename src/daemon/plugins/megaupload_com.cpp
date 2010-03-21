/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#define PLUGIN_WANTS_POST_PROCESSING
#include "plugin_helpers.h"
#include <curl/curl.h>
#include <cstdlib>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
using namespace std;

size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp) {
	std::string *blubb = (std::string*)userp;
	blubb->append((char*)buffer, nmemb);
	return nmemb;
}

plugin_status plugin_exec(plugin_input &inp, plugin_output &outp) {
	CURL* handle = get_handle();
	string result;
	bool done = false;
	// so we can never get in an infinite loop..
	int while_tries = 0;
	while(!done && while_tries < 100) {
		++while_tries;
		curl_easy_setopt(handle, CURLOPT_URL, get_url());
		curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_data);
		curl_easy_setopt(handle, CURLOPT_WRITEDATA, &result);
		curl_easy_setopt(handle, CURLOPT_COOKIEFILE, "");
		curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1);
		result.clear();
		int res = curl_easy_perform(handle);
		if(res != 0) {
			return PLUGIN_CONNECTION_ERROR;
		}

		if(result.find("Unfortunately, the link you have clicked is not available") != string::npos) {
			return PLUGIN_FILE_NOT_FOUND;
		}

		if(result.find("The file that you're trying to download is larger than 1 GB") != string::npos
			|| result.find("The file you're trying to download is password protected") != string::npos) {
			return PLUGIN_AUTH_FAIL;
		}

		if(result.find("We have detected an elevated number of requests from your IP address. You have to wait 2 minutes") != string::npos) {
			set_wait_time(120);
			return PLUGIN_SERVER_OVERLOADED;
		}

		if(result.find("gencap.php") == string::npos) {
			return PLUGIN_FILE_NOT_FOUND;
		}

		string captcha_url;
		try {
			size_t n = result.find("<TD width=\"100\" align=\"center\" height=\"40\"><img src=\"");
			n = result.find("http://", n);
			captcha_url = result.substr(n, result.find("\"", n) - n);

			n = result.find("name=\"captchacode\" value=\"") + 26;
			std::string captchacode = result.substr(n, result.find("\"", n) - n);

			n = result.find("name=\"megavar\"");
			n = result.find("value=\"", n) + 7;
			std::string megavar = result.substr(n, result.find("\"", n) - n);


			curl_easy_setopt(handle, CURLOPT_URL, captcha_url.c_str());
			result.clear();
			res = curl_easy_perform(handle);
			if(res != 0) {
				return PLUGIN_ERROR;
			}
			// megaupload uses .gif captchas that don't work with gocr. We use giftopnm to convert it (which is installed with netpbm, which
			// is a gocr dependency anyway.
			ofstream captcha_fs("/tmp/tmp_captcha_megaupload.gif");
			captcha_fs << result;
			captcha_fs.close();
			FILE* captcha_as_pnm = popen("giftopnm /tmp/tmp_captcha_megaupload.gif", "r");

			if(captcha_as_pnm == NULL) {
				captcha_exception e;
				throw e;
			}
			// if the captcha is larger than 10kb.. it's not.
			result.clear();

			int c;
			do{
				c = fgetc(captcha_as_pnm);
				if(c != EOF) result.push_back(c);
			} while(c != EOF);


			pclose(captcha_as_pnm);
			remove("/tmp/tmp_captcha_megaupload.gif");
			captcha cap(result);
			std::string captcha_text = cap.process_image("-C 1-9A-NP-Z", "pnm", 4, true);
			if(captcha_text.empty()) continue;

			std::string post_data = "captchacode=" + captchacode + "&megavar=" + megavar + "&captcha=" + captcha_text;

			curl_easy_setopt(handle, CURLOPT_URL, get_url());
			curl_easy_setopt(handle, CURLOPT_POST, 1);
			curl_easy_setopt(handle, CURLOPT_COPYPOSTFIELDS, post_data.c_str());
			result.clear();
			res = curl_easy_perform(handle);
			curl_easy_setopt(handle, CURLOPT_POST, 0);
			if(res != 0) {
				return PLUGIN_ERROR;
			}
			if(result.find("<input type=\"text\" name=\"captcha\" id=\"captchafield\"") != string::npos) {
				// captcha wrong.. try again
				continue;
			} else {
				set_wait_time(45);
				n = result.find("id=\"downloadlink\"><a href=\"") + 27;
				outp.download_url = result.substr(n, result.find("\"", n) - n);
				return PLUGIN_SUCCESS;

			}

		} catch(std::exception &e) {
			return PLUGIN_ERROR;
		}

	}

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
	outp.offers_premium = false; // no login support yet
}


void post_process_download(plugin_input &inp) {
	if(!inp.premium_user.empty() && !inp.premium_password.empty()) {
		return;
	} else {
		string filename = dl_list->get_output_file(dlid);
		struct stat64 st;
		if(stat64(filename.c_str(), &st) != 0) {
			return;
		}
		if(st.st_size > 1024 * 1024 /* 1 mb */ ) {
			return;
		}
		ifstream ifs(filename.c_str());
		string tmp;
		getline(ifs, tmp);
		if(tmp.find("</HEAD><BODY>Download limit exceeded</BODY></HTML>") != string::npos) {
			set_wait_time(120);
			dl_list->set_status(dlid, DOWNLOAD_WAITING);
			dl_list->set_error(dlid, PLUGIN_LIMIT_REACHED);
		}


	}
}


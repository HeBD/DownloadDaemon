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

	if(url.find("/watch?v=") != string::npos) {
		size_t pos = url.find("v=") + 2;
		std::string video_id = url.substr(pos, url.find("&", pos) - pos);

		pos = result.find("&t=");
		if(pos == string::npos) {
			return PLUGIN_FILE_NOT_FOUND;
		}
		pos += 3;
		std::string t = result.substr(pos, result.find("&", pos) - pos);

		pos = result.find("<meta name=\"title\" content=\"");
		if(pos == string::npos) {
			return PLUGIN_ERROR;
		}
		pos += 28;
		std::string title = result.substr(pos, result.find("\">", pos) - pos);

		outp.download_url = "http://youtube.com/get_video?video_id=" + video_id + "&t=" + t;
		outp.download_filename = title + ".flv";
		replace_html_special_chars(outp.download_filename);
		return PLUGIN_SUCCESS;
	} else if(url.find("/view_play_list?p=") != string::npos) {
		download_container urls;
		for(int i = 1; i < 50; ++i) {
			result.clear();
			string page_url = url.substr(0, url.find("&"));
			page_url += "&page=" + convert_to_string(i);

			curl_easy_setopt(handle, CURLOPT_URL, page_url.c_str());
			res = curl_easy_perform(handle);
			if(res != 0) {
				return PLUGIN_ERROR;
			}

			int added_downloads_this_round = 0;

			size_t pos = 0;
			while((pos = result.find("video-main-content", pos + 1)) != string::npos) {
				pos = result.find("href=\"", pos) + 6;
				string curr_url = "http://www.youtube.com";
				curr_url += result.substr(pos, result.find("\"", pos) - pos);

				if(!urls.url_is_in_list(curr_url)) {
					string title;
					size_t title_pos = result.find("title=\"", pos);
					if(title_pos != string::npos) {
						title_pos += 7;
						size_t end_pos = result.find("\"", title_pos);
						while(result[end_pos - 1] == '\\') {
							++end_pos;
							end_pos = result.find("\"", end_pos);
						}
						title = result.substr(title_pos, end_pos - title_pos);
						replace_html_special_chars(title);
					}

					urls.add_download(curr_url, title);
					++added_downloads_this_round;
				}


			}
			if(added_downloads_this_round == 0) {
				break;
			}


		}

		replace_this_download(urls);
		return PLUGIN_SUCCESS;


	}

	return PLUGIN_ERROR;
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

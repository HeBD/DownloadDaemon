/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <fstream>
#include <vector>

#include "../dl/download.h"
#include "../dl/download_container.h"
#include "helperfunctions.h"
#include "../global.h"

#include <sys/types.h>
#include <sys/stat.h>
using namespace std;

size_t write_file(void *buffer, size_t size, size_t nmemb, void *userp) {
	std::pair<std::pair<fstream*, std::string*>, CURL*>* callback_opt = (std::pair<std::pair<fstream*, std::string*>, CURL*>*)userp;
	fstream* output_file = callback_opt->first.first;
	std::string* cache = callback_opt->first.second;
	cache->append((char*)buffer, nmemb);
	double speed;
	curl_easy_getinfo(callback_opt->second, CURLINFO_SPEED_DOWNLOAD, &speed);
	if(speed <= 0 || cache->size() >= speed / 2 || cache->size() >= 1048576) {
		// wite twice per sec, if speed is 0, and if the cache is >= 1MB
		output_file->write(cache->c_str(), cache->size());
		cache->clear();
		global_download_list.dump_to_file();
	}
	return nmemb;
}

int report_progress(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow) {
	dlindex id = *(dlindex*)clientp;
	CURL* curr_handle = global_download_list.get_handle(id);

	global_download_list.set_size(id, dltotal + 0.5);
	std::string output_file;
	output_file = global_download_list.get_output_file(id);

	double curr_speed;
	curl_easy_getinfo(curr_handle, CURLINFO_SPEED_DOWNLOAD, &curr_speed);
	if(curr_speed == 0) {
		global_download_list.set_speed(id, -1);
	} else {
		global_download_list.set_speed(id, curr_speed + 0.5);
	}

	struct stat st;
	if(stat(output_file.c_str(), &st) == 0) {
		global_download_list.set_downloaded_bytes(id, st.st_size);
	}

	if(global_download_list.get_need_stop(id)) {
		// break up the download
		return -1;
	}
	return 0;
}

size_t parse_header( void *ptr, size_t size, size_t nmemb, void *clientp) {
	string* filename = (string*)clientp;
	string header((char*)ptr, size * nmemb);
	size_t n;
	if((n = header.find("Content-Disposition:")) != string::npos) {
		n += 20;
		header = header.substr(n, header.find("\n") - n);
		if((n = header.find("filename=")) == string::npos) return nmemb;
		header = header.substr(n + 9);
		if(header.size() == 0) return nmemb;
		if(header[0] == '"') {
			n = header.find('"', 1);
			if(n == string::npos) return nmemb;
			*filename = header.substr(1, n - 1);
		} else {
			*filename = header.substr(0, header.find_first_of("\r\n "));
		}
	}
	return nmemb;
}

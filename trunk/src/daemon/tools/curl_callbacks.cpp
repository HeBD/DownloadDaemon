/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <config.h>

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
	if(speed < 1 || cache->size() >= speed / 2 || cache->size() >= 1048576) {
		// wite twice per sec, if speed is 0, and if the cache is >= 1MB
		output_file->write(cache->c_str(), cache->size());
		cache->clear();
		global_download_list.dump_to_file();
	}
	return nmemb;
}

int report_progress(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow) {
	#ifdef HAVE_UINT64_T
	std::pair<dlindex, uint64_t> *prog_data = (std::pair<dlindex, uint64_t>*)clientp;
	#else
	std::pair<dlindex, double> *prog_data = (std::pair<dlindex, double>*)clientp;
	#endif
	dlindex id = prog_data->first;
	CURL* curr_handle = global_download_list.get_handle(id);

	double curr_speed_param;
	curl_easy_getinfo(curr_handle, CURLINFO_SPEED_DOWNLOAD, &curr_speed_param);
	#ifdef HAVE_UINT64_T
	uint64_t size_conv = (uint64_t)(dltotal + 0.5) + prog_data->second;
	uint64_t speed_conv = (uint64_t)(curr_speed_param);
	#else
	double size_conf = dltotal + prog_data->second;
	double speed_conv = curr_speed_param;
	#endif
	global_download_list.set_size(id, size_conv);
	std::string output_file;
	output_file = global_download_list.get_output_file(id);


	global_download_list.set_speed(id, speed_conv);

	struct stat64 st;
	if(stat64(output_file.c_str(), &st) == 0) {
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

int get_size_progress_callback(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow) {
	if(dltotal < 1 && dlnow < 100) { // the first time this function is called, dltotal is 0. We have to try again. but not too often
		return 0;					 // because dltotal might be 0 through the whole download process, if curl fails to get the size.
	}								 // so if we downloaded 100 bytes and still don't know the size, we give up

	#ifdef HAVE_UINT64_T
		uint64_t *result = (uint64_t*)clientp;
		*result = (uint64_t)dltotal + 0.5;
	#else
		double *result = (double*)clientp;
		*result = dltotal;
	#endif
	return -1;
}

size_t pretend_write_file(void *buffer, size_t size, size_t nmemb, void *userp) {
	return nmemb;
}

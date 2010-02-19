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
	fstream* output_file = (fstream*)userp;
	output_file->write((char*)buffer, nmemb);
	return nmemb;
}

int report_progress(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow) {
	dlindex id = *(dlindex*)clientp;
	CURL* curr_handle = global_download_list.get_handle(id);

	global_download_list.set_size(id, dltotal);
	std::string output_file;
	output_file = global_download_list.get_output_file(id);

	double curr_speed;
	curl_easy_getinfo(curr_handle, CURLINFO_SPEED_DOWNLOAD, &curr_speed);
	if(curr_speed == 0) {
		global_download_list.set_speed(id, -1);
	} else {
		global_download_list.set_speed(id, curr_speed);
	}

	struct stat st;
	if(stat(output_file.c_str(), &st) == 0) {
		global_download_list.set_downloaded_bytes(id, st.st_size);
	}
	global_download_list.dump_to_file();
	if(global_download_list.get_need_stop(id)) {
		// break up the download
		return -1;
	}
	return 0;
}

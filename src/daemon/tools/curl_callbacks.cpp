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
	// careful! this is a POINTER to an iterator!
	int id = *(int*)clientp;
	if(global_download_list.get_int_property(id, DL_NEED_STOP)) {
		curl_easy_setopt(global_download_list.get_pointer_property(id, DL_HANDLE), CURLOPT_TIMEOUT, 1);
		global_download_list.set_int_property(id, DL_NEED_STOP, false);
	}
	global_download_list.set_int_property(id, DL_SIZE, dltotal);
	std::string output_file;
	try {
		output_file = global_download_list.get_string_property(id, DL_OUTPUT_FILE);
	} catch(download_exception &e) {
		log_string("Download ID " + int_to_string(id) + " has been deleted internally while actively downloading. Please report this bug!", LOG_SEVERE);
		global_download_list.deactivate(id);
		return 0;
	}

	CURL* dl_handle = global_download_list.get_pointer_property(id, DL_HANDLE);
	double curr_speed;
	curl_easy_getinfo(dl_handle, CURLINFO_SPEED_DOWNLOAD, &curr_speed);
	if(curr_speed == 0) {
		global_download_list.set_int_property(id, DL_SPEED, -1);
	} else {
		global_download_list.set_int_property(id, DL_SPEED, curr_speed);
	}

	struct stat st;
	if(stat(output_file.c_str(), &st) == 0) {
		global_download_list.set_int_property(id, DL_DOWNLOADED_BYTES, st.st_size);
	} else {
		global_download_list.set_int_property(id, DL_DOWNLOADED_BYTES, dlnow);
	}
	return 0;
}

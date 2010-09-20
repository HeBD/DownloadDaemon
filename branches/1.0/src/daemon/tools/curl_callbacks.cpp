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
	dl_cb_info *info = (dl_cb_info*)userp;

	fstream* output_file = info->out_stream;
	std::string* cache = &(info->cache);

	cache->append((char*)buffer, nmemb);
	double speed;
        info->curl_handle->getinfo(CURLINFO_SPEED_DOWNLOAD, &speed);
	if(speed < 1 || cache->size() >= speed / 2 || cache->size() >= 1048576) {
		// wite twice per sec, if speed is 0, and if the cache is >= 1MB
		if (!output_file->is_open()) {
			if(info->filename.empty()) {
				char* fn_cstr = 0;
                                info->curl_handle->getinfo(CURLINFO_EFFECTIVE_URL, &fn_cstr);
				info->filename = filename_from_url(fn_cstr);
				make_valid_filename(info->filename);
				info->filename_from_effective_url = true;
			}
			info->filename = ddcurl::unescape(info->filename);
			make_valid_filename(info->filename);

			if(info->download_dir.empty()) {
				log_string("Failed to get download folder", LOG_ERR);
				info->break_reason = PLUGIN_WRITE_FILE_ERROR;
				return 0;
			}
			string dl_path = info->download_dir + '/' + info->filename + ".part";
			correct_path(dl_path);
			if(info->resume_from > 0) {
				output_file->open(dl_path.c_str(), ios::out | ios::binary | ios::app);
			} else {
				output_file->open(dl_path.c_str(), ios::out | ios::binary | ios::trunc);
			}
			global_download_list.set_output_file(info->id, dl_path);
			if(!output_file->is_open() || !output_file) {
				return 0;
			}
		}

		output_file->write(cache->c_str(), cache->size());
		if(output_file->bad()) {
			info->break_reason = PLUGIN_WRITE_FILE_ERROR;
			log_string("Successfully opened the download-file, but failed to write to it. Is your harddisk full?", LOG_ERR);
			return 0;
		}
		global_download_list.set_downloaded_bytes(info->id, global_download_list.get_downloaded_bytes(info->id) + cache->size());
		cache->clear();
		global_download_list.dump_to_file();
	}
	return nmemb;
}

int report_progress(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow) {

	//std::pair<dlindex, filesize_t> *prog_data = (std::pair<dlindex, filesize_t>*)clientp;
	dl_cb_info *info = (dl_cb_info*)clientp;

	dlindex id = info->id;
        ddcurl* curr_handle = info->curl_handle;

	double curr_speed_param;
        curr_handle->getinfo(CURLINFO_SPEED_DOWNLOAD, &curr_speed_param);
	filesize_t curr_speed = (filesize_t)(curr_speed_param + 0.5);

	filesize_t  dl_size = info->resume_from + (filesize_t)(dltotal + 0.5);

	global_download_list.set_size(id, dl_size);
	std::string output_file = info->download_dir + '/' + info->filename;
	if(output_file.empty()) { // a bit too early.. let's wait another round
		return 0;
	}


	global_download_list.set_speed(id, curr_speed);

	//filesize_t downloaded = info->resume_from + (filesize_t)(dlnow + 0.5);
	//struct pstat st;
	//if(pstat(output_file.c_str(), &st) == 0) {
	//	global_download_list.set_downloaded_bytes(id, downloaded);
	//}

	if(global_download_list.get_need_stop(id)) {
		// break up the download
		return -1;
	}
	return 0;
}

size_t parse_header( void *ptr, size_t size, size_t nmemb, void *clientp) {
	dl_cb_info *info = (dl_cb_info*)clientp;
	string* filename = & (info->filename);
        string header((char*)ptr, size * nmemb);
        if(!filename->empty()) return nmemb; // the filename has been set by the plugin -- priority
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
	if(dltotal < 1. && dlnow < 100.) { // the first time this function is called, dltotal is 0. We have to try again. but not too often
		return 0;					 // because dltotal might be 0 through the whole download process, if curl fails to get the size.
	}								 // so if we downloaded 100 bytes and still don't know the size, we give up

	filesize_t *result = (filesize_t*)clientp;
	#ifdef HAVE_UINT64_T
		*result = (filesize_t)(dltotal + 0.5);
	#else
		*result = dltotal;
	#endif
	return -1;
}

size_t pretend_write_file(void *buffer, size_t size, size_t nmemb, void *userp) {
	return nmemb;
}

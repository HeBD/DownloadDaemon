#include <string>
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>
#include <vector>
#include <fstream>
#include <cstdlib>
#include <ctime>

#include <curl/curl.h>

#include "../../lib/cfgfile.h"
#include "download.h"
#include "download_thread.h"
#include "../tools/helperfunctions.h"
#include "../tools/curl_callbacks.h"


using namespace std;

extern cfgfile global_config;
extern std::vector<download> global_download_list;
extern std::string program_root;

void download_thread_main() {
	vector<download>::iterator downloadable;
	while(1) {
		downloadable = global_download_list.end();
		downloadable = get_next_downloadable();
		if(downloadable == global_download_list.end()) {
			sleep(1);
			continue;
		} else {
			downloadable->status = DOWNLOAD_RUNNING;
			boost::thread t(download_thread, downloadable);
		}
	}
}

void download_thread(vector<download>::iterator download) {
	parsed_download parsed_dl;
	int success = download->get_download(parsed_dl);
	switch(success) {
		case INVALID_HOST:
			download->status = DOWNLOAD_INACTIVE;
			download->error = INVALID_HOST;
			log_string(string("Invalid host for download ID: ") + int_to_string(download->id), LOG_WARNING);
			return;
		break;
		case INVALID_PLUGIN_PATH:
			download->error = INVALID_PLUGIN_PATH;
			download->status = DOWNLOAD_PENDING;
			log_string("Could not locate plugin folder!", LOG_SEVERE);
			exit(-1);
		break;
		case MISSING_PLUGIN:
			download->error = MISSING_PLUGIN;
			log_string(string("Plugin missing for download ID: ") + int_to_string(download->id), LOG_WARNING);
			download->status = DOWNLOAD_INACTIVE;
			return;
		break;
	}

	if(parsed_dl.plugin_return_val == -1) {
		log_string(string("Plugin reported unknown error for download ID: ") + int_to_string(download->id), LOG_WARNING);
		download->status = DOWNLOAD_INACTIVE;
		download->error = PLUGIN_ERROR;
		return;
	}

	if(parsed_dl.download_parse_success) {
		download->status = DOWNLOAD_RUNNING;
		log_string(string("Successfully parsed download ID: ") + int_to_string(download->id), LOG_DEBUG);
		if(parsed_dl.wait_before_download > 0) {
			log_string(string("Download ID: ") + int_to_string(download->id) + " has to wait " +
					   int_to_string(parsed_dl.wait_before_download) + " seconds before downloading can start", LOG_DEBUG);
			download->wait_seconds = parsed_dl.wait_before_download;
			sleep(parsed_dl.wait_before_download);
		}

		string output_filename(global_config.get_cfg_value("download_folder"));
		if(parsed_dl.download_url != "") {
			output_filename += '/' + parsed_dl.download_url.substr(parsed_dl.download_url.find_last_of("/\\"));
		} else {
			// weird unknown error by wrong plugin implementation
		}
		fstream output_file(output_filename.c_str(), ios::out | ios::binary);
		CURL* handle = curl_easy_init();
		// set url
		curl_easy_setopt(handle, CURLOPT_URL, parsed_dl.download_url.c_str());
		// set file-writing function as callback
		curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_file);
		curl_easy_setopt(handle, CURLOPT_WRITEDATA, &output_file);
		// show progress
		curl_easy_setopt(handle, CURLOPT_NOPROGRESS, 0);
		curl_easy_setopt(handle, CURLOPT_PROGRESSFUNCTION, report_progress);
		curl_easy_setopt(handle, CURLOPT_PROGRESSDATA, &download);

		log_string(string("Starting download ID: ") + int_to_string(download->id), LOG_DEBUG);
		int success = curl_easy_perform(handle);
		if(success == 0) {
			log_string(string("Finished download ID: ") + int_to_string(download->id), LOG_DEBUG);
			download->status = DOWNLOAD_FINISHED;
			return;
		} else {
			log_string(string("Connection lost for download ID: ") + int_to_string(download->id), LOG_WARNING);
			download->error = CONNECTION_LOST;
			download->status = DOWNLOAD_PENDING;
			return;
		}
	} else {
		if(parsed_dl.download_parse_errmsg == "SERVER_OVERLOAD") {
			log_string(string("Server overloaded for download ID: ") + int_to_string(download->id), LOG_WARNING);
			download->status = DOWNLOAD_WAITING;
			download->wait_seconds = parsed_dl.download_parse_wait;
			return;
		} else if(parsed_dl.download_parse_errmsg == "LIMIT_REACHED") {
			log_string(string("Download limit reached for download ID: ") + int_to_string(download->id) + " (" + download->get_host() + ")", LOG_WARNING);
			download->status = DOWNLOAD_WAITING;
			download->wait_seconds = parsed_dl.download_parse_wait;
			return;
		} else if(parsed_dl.download_parse_errmsg == "CONNECTION_FAILED") {
			log_string(string("Plugin failed to connect for ID:") + int_to_string(download->id), LOG_WARNING);
			download->error = CONNECTION_LOST;
			download->status = DOWNLOAD_INACTIVE;
			return;
		} else if(parsed_dl.download_parse_errmsg == "FILE_NOT_FOUND") {
			log_string(string("File could not be found on the server: ") + parsed_dl.download_url, LOG_WARNING);
			download->error = FILE_NOT_FOUND;
			download->status = DOWNLOAD_INACTIVE;
			return;
		}
	}
}

vector<download>::iterator get_next_downloadable() {
	vector<download>::iterator downloadable = global_download_list.end();
	if(global_download_list.empty()) {
		return downloadable;
	}

	if(get_running_count() >= atoi(global_config.get_cfg_value("simultaneous_downloads").c_str())) {
		return downloadable;
	}

	// Checking if we are in download-time...
	if(global_config.get_cfg_value("download_timing_start").find(':') != string::npos && !global_config.get_cfg_value("download_timing_end").find(':') != string::npos ) {
		time_t rawtime;
		struct tm * current_time;
		time ( &rawtime );
		current_time = localtime ( &rawtime );
		int starthours, startminutes, endhours, endminutes;
		starthours = atoi(global_config.get_cfg_value("download_timing_start").substr(0, global_config.get_cfg_value("download_timing_start").find(':')).c_str());
		endhours = atoi(global_config.get_cfg_value("download_timing_end").substr(0, global_config.get_cfg_value("download_timing_end").find(':')).c_str());
		startminutes = atoi(global_config.get_cfg_value("download_timing_start").substr(global_config.get_cfg_value("download_timing_start").find(':') + 1).c_str());
		endminutes = atoi(global_config.get_cfg_value("download_timing_end").substr(global_config.get_cfg_value("download_timing_end").find(':') + 1).c_str());
		// we have a 0:00 shift
		if(starthours > endhours) {
			if(current_time->tm_hour < starthours && current_time->tm_hour > endhours) {
				return downloadable;
			}
			if(current_time->tm_hour == starthours) {
				if(current_time->tm_min < startminutes) {
					return downloadable;
				}
			} else if(current_time->tm_hour == endhours) {
				if(current_time->tm_min > endminutes) {
					return downloadable;
				}
			}
		// download window are just a few minutes
		} else if(starthours == endhours) {
			if(current_time->tm_min < startminutes || current_time->tm_min > endminutes) {
				return downloadable;
			}
		// no 0:00 shift
		} else if(starthours < endhours) {
			if(current_time->tm_hour < starthours || current_time->tm_hour > endhours) {
				return downloadable;
			}
			if(current_time->tm_hour == starthours) {
				if(current_time->tm_min < startminutes) {
					return downloadable;
				}
			} else if(current_time->tm_hour == endhours) {
				if(current_time->tm_min > endminutes) {
					return downloadable;
				}
			}
		}
	}


	for(vector<download>::iterator it = global_download_list.begin(); it != global_download_list.end(); ++it) {
		if(it->status != DOWNLOAD_INACTIVE && it->status != DOWNLOAD_FINISHED && it->status != DOWNLOAD_RUNNING && it->status != DOWNLOAD_WAITING) {
			string current_host(it->get_host());
			bool can_attach = true;
			hostinfo hinfo = it->get_hostinfo();
			for(vector<download>::iterator it2 = global_download_list.begin(); it2 != global_download_list.end(); ++it2) {
				if(it2->get_host() == current_host && (it2->status == DOWNLOAD_RUNNING || it2->status == DOWNLOAD_WAITING) && !hinfo.allows_multiple_downloads_free) {
					can_attach = false;
				}
			}
			if(can_attach) {
				downloadable = it;
				break;
			}
		}
	}

	return downloadable;
}

int get_running_count() {
	int running_downloads = 0;
	for(vector<download>::iterator it = global_download_list.begin(); it != global_download_list.end(); ++it) {
		if(it->status == DOWNLOAD_RUNNING) {
			++running_downloads;
		}
	}
	return running_downloads;
}



vector<download>::iterator get_download_by_id(int id) {
	for(vector<download>::iterator it = global_download_list.begin(); it != global_download_list.end(); ++it) {
		if(it->id == id) {
			return it;
		}
	}
	return global_download_list.end();
}

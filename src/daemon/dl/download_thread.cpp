#include <string>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <vector>
#include <fstream>
#include <cstdlib>
#include <ctime>

#include <curl/curl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "../../lib/cfgfile/cfgfile.h"
#include "download.h"
#include "download_thread.h"
#include "download_container.h"
#include "../tools/helperfunctions.h"
#include "../tools/curl_callbacks.h"


using namespace std;

extern cfgfile global_config;
extern download_container global_download_list;
extern std::string program_root;

/** Main thread for managing downloads */
void download_thread_main() {
	download_container::iterator downloadable;
	while(1) {
		downloadable = global_download_list.end();
		downloadable = global_download_list.get_next_downloadable();
		if(downloadable == global_download_list.end()) {
			sleep(1);
			continue;
		} else {
			downloadable->set_status(DOWNLOAD_RUNNING);
			boost::thread t(boost::bind(download_thread, downloadable));
		}
	}
}

/** This function does the magic of downloading a file, calling the right plugin, etc.
 *	@param download iterator to a download in the global download list, that we should load
 */
void download_thread(download_container::iterator download) {
	parsed_download parsed_dl;
	int success = download->get_download(parsed_dl);

	switch(success) {
		case INVALID_HOST:
			download->set_status(DOWNLOAD_INACTIVE, true);
			download->error = INVALID_HOST;
			log_string(string("Invalid host for download ID: ") + int_to_string(download->id), LOG_WARNING);
			return;
		break;
		case INVALID_PLUGIN_PATH:
			download->error = INVALID_PLUGIN_PATH;
			download->set_status(DOWNLOAD_PENDING, true);
			log_string("Could not locate plugin folder!", LOG_SEVERE);
			exit(-1);
		break;
		case MISSING_PLUGIN:
			download->error = MISSING_PLUGIN;
			log_string(string("Plugin missing for download ID: ") + int_to_string(download->id), LOG_WARNING);
			download->set_status(DOWNLOAD_INACTIVE, true);
			return;
		break;
	}

	if(parsed_dl.plugin_return_val == -1) {
		log_string(string("Plugin reported unknown error for download ID: ") + int_to_string(download->id), LOG_WARNING);
		download->set_status(DOWNLOAD_INACTIVE, true);
		download->error = PLUGIN_ERROR;
		return;
	}

	if(parsed_dl.download_parse_success) {
		download->error = NO_ERROR;
		log_string(string("Successfully parsed download ID: ") + int_to_string(download->id), LOG_DEBUG);
		if(parsed_dl.wait_before_download > 0) {

			if(download->get_status() != DOWNLOAD_DELETED) {
				log_string(string("Download ID: ") + int_to_string(download->id) + " has to wait " +
					       int_to_string(parsed_dl.wait_before_download) + " seconds before downloading can start", LOG_DEBUG);
			}
			download->wait_seconds = parsed_dl.wait_before_download + 1;

			while(global_download_list.get_download_by_id(download->id) != global_download_list.end() && download->wait_seconds > 0) {
				--download->wait_seconds;
				sleep(1);
				if(download->get_status() == DOWNLOAD_INACTIVE) {
					download->wait_seconds = 0;
					return;
				} else if(download->get_status() == DOWNLOAD_DELETED) {
					curl_easy_cleanup(download->handle);
					global_download_list.erase(download);
					return;
				}
			}
		}

		string output_filename;
		string download_folder = global_config.get_cfg_value("download_folder");
		correct_path(download_folder);
		if(parsed_dl.download_filename == "") {
			if(parsed_dl.download_url != "" && parsed_dl.download_url.find('/') != string::npos) {
				output_filename += download_folder;
				output_filename += '/' + parsed_dl.download_url.substr(parsed_dl.download_url.find_last_of("/\\"));
			}
		} else {
			output_filename += download_folder;
			output_filename += '/' + parsed_dl.download_filename;
		}

		// Check if we can do a download resume or if we have to start from the beginning
        hostinfo hinfo = download->get_hostinfo();
        struct stat st;
        fstream output_file;
        if(hinfo.allows_download_resumption_free && global_config.get_cfg_value("enable_resume") != "0" &&
           stat(output_filename.c_str(), &st) == 0 && st.st_size == download->downloaded_bytes) {
            curl_easy_setopt(download->handle, CURLOPT_RESUME_FROM, st.st_size);
            output_file.open(output_filename.c_str(), ios::out | ios::binary | ios::app);
            log_string(string("Download already started. Will continue to download ID: ") + int_to_string(download->id), LOG_DEBUG);
        } else {
            output_file.open(output_filename.c_str(), ios::out | ios::binary);
        }

		if(!output_file.good()) {
			log_string(string("Could not write to file: ") + output_filename, LOG_SEVERE);
			download->error = WRITE_FILE_ERROR;
			download->set_status(DOWNLOAD_PENDING, true);
			return;
		}

        download->output_file = output_filename;

		if(parsed_dl.download_url.empty()) {
			log_string(string("Empty URL for download ID: ") + int_to_string(download->id), LOG_SEVERE);
			download->error = PLUGIN_ERROR;
			download->set_status(DOWNLOAD_PENDING, true);
			return;
		}

		// set url
		curl_easy_setopt(download->handle, CURLOPT_FOLLOWLOCATION, 1);
		curl_easy_setopt(download->handle, CURLOPT_URL, parsed_dl.download_url.c_str());
		// set file-writing function as callback
		curl_easy_setopt(download->handle, CURLOPT_WRITEFUNCTION, write_file);
		curl_easy_setopt(download->handle, CURLOPT_WRITEDATA, &output_file);
		// show progress
		curl_easy_setopt(download->handle, CURLOPT_NOPROGRESS, 0);
		curl_easy_setopt(download->handle, CURLOPT_PROGRESSFUNCTION, report_progress);
		curl_easy_setopt(download->handle, CURLOPT_PROGRESSDATA, &download);
		// set timeouts
		curl_easy_setopt(download->handle, CURLOPT_LOW_SPEED_LIMIT, 100);
		curl_easy_setopt(download->handle, CURLOPT_LOW_SPEED_TIME, 20);

		log_string(string("Starting download ID: ") + int_to_string(download->id), LOG_DEBUG);

		success = curl_easy_perform(download->handle);
		switch(success) {
			case 0:
				log_string(string("Finished download ID: ") + int_to_string(download->id), LOG_DEBUG);
				download->set_status(DOWNLOAD_FINISHED, true);
				curl_easy_reset(download->handle);
				return;
			case 28:
				if(download->get_status() == DOWNLOAD_INACTIVE) {
					log_string(string("Stopped download ID: ") + int_to_string(download->id), LOG_WARNING);
					curl_easy_reset(download->handle);
				} else if(download->get_status() == DOWNLOAD_DELETED) {
					curl_easy_cleanup(download->handle);
					global_download_list.erase(download);
				} else {
					log_string(string("Connection lost for download ID: ") + int_to_string(download->id), LOG_WARNING);
					download->set_status(DOWNLOAD_PENDING, true);
					download->error = CONNECTION_LOST;
					curl_easy_reset(download->handle);
				}
				return;
			default:
				log_string(string("Download error for download ID: ") + int_to_string(download->id), LOG_WARNING);
				download->set_status(DOWNLOAD_PENDING, true);
				download->error = CONNECTION_LOST;
				curl_easy_reset(download->handle);
				return;
		}
	} else {
		if(parsed_dl.download_parse_errmsg == "SERVER_OVERLOAD") {
			log_string(string("Server overloaded for download ID: ") + int_to_string(download->id), LOG_WARNING);
			download->set_status(DOWNLOAD_WAITING, true);
			download->wait_seconds = parsed_dl.download_parse_wait;
			return;
		} else if(parsed_dl.download_parse_errmsg == "LIMIT_REACHED") {
			log_string(string("Download limit reached for download ID: ") + int_to_string(download->id) + " (" + download->get_host() + ")", LOG_WARNING);
			download->set_status(DOWNLOAD_WAITING, true);
			download->wait_seconds = parsed_dl.download_parse_wait;
			return;
		} else if(parsed_dl.download_parse_errmsg == "CONNECTION_FAILED") {
			log_string(string("Plugin failed to connect for ID:") + int_to_string(download->id), LOG_WARNING);
			download->error = CONNECTION_LOST;
			download->set_status(DOWNLOAD_INACTIVE, true);
			return;
		} else if(parsed_dl.download_parse_errmsg == "FILE_NOT_FOUND") {
			log_string(string("File could not be found on the server for ID: ") + int_to_string(download->id), LOG_WARNING);
			download->error = FILE_NOT_FOUND;
			download->set_status(DOWNLOAD_INACTIVE, true);
			return;
		}
	}
	// something weird happened...
	download->set_status(DOWNLOAD_PENDING, true);
	return;
}


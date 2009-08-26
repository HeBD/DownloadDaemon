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
extern mt_string program_root;

/** Main thread for managing downloads */
void download_thread_main() {
	download_container::iterator downloadable(global_download_list.end());
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
	plugin_output plug_outp;
	int success = download->get_download(plug_outp);

	switch(success) {
		case PLUGIN_INVALID_HOST:
			download->set_plugin_status(PLUGIN_INVALID_HOST);
			download->set_status(DOWNLOAD_INACTIVE, true);
			log_string(mt_string("Invalid host for download ID: ") + int_to_string(download->get_id()), LOG_WARNING);
			return;
		break;
		case PLUGIN_INVALID_PATH:
			download->set_plugin_status(PLUGIN_INVALID_PATH);
			download->set_status(DOWNLOAD_PENDING, true);
			log_string("Could not locate plugin folder!", LOG_SEVERE);
			exit(-1);
		break;
		case PLUGIN_MISSING:
			download->set_plugin_status(PLUGIN_MISSING);
			log_string(mt_string("Plugin missing for download ID: ") + int_to_string(download->get_id()), LOG_WARNING);
			download->set_status(DOWNLOAD_INACTIVE, true);
			return;
		break;
	}

	if(success == PLUGIN_SUCCESS) {
		download->set_plugin_status(PLUGIN_SUCCESS);
		log_string(mt_string("Successfully parsed download ID: ") + int_to_string(download->get_id()), LOG_DEBUG);
		if(download->get_wait_seconds() > 0) {

			if(download->get_status() != DOWNLOAD_DELETED) {
				log_string(mt_string("Download ID: ") + int_to_string(download->get_id()) + " has to wait " +
					       int_to_string(download->get_wait_seconds()) + " seconds before downloading can start", LOG_DEBUG);
			}

			while(global_download_list.get_download_by_id(download->get_id()) != global_download_list.end() && download->get_wait_seconds() > 0) {
				download->set_wait_seconds(download->get_wait_seconds() - 1);
				sleep(1);
				if(download->get_status() == DOWNLOAD_INACTIVE) {
					download->set_wait_seconds(0);
					return;
				} else if(download->get_status() == DOWNLOAD_DELETED) {
					curl_easy_reset(download->get_handle());
					global_download_list.erase(download);
					return;
				}
			}
		}

		mt_string output_filename;
		mt_string download_folder = global_config.get_cfg_value("download_folder");
		correct_path(download_folder);
		if(plug_outp.download_filename == "") {
			if(plug_outp.download_url != "" && plug_outp.download_url.find('/') != mt_string::npos) {
				output_filename += download_folder;
				output_filename += '/' + plug_outp.download_url.substr(plug_outp.download_url.find_last_of("/\\"));
			}
		} else {
			output_filename += download_folder;
			output_filename += '/' + plug_outp.download_filename;
		}

		// Check if we can do a download resume or if we have to start from the beginning
        struct stat st;
        fstream output_file;
        if(download->get_hostinfo().allows_multiple && global_config.get_cfg_value("enable_resume") != "0" &&
           stat(output_filename.c_str(), &st) == 0 && st.st_size == download->get_downloaded_bytes()) {
            curl_easy_setopt(download->get_handle(), CURLOPT_RESUME_FROM, st.st_size);
            output_file.open(output_filename.c_str(), ios::out | ios::binary | ios::app);
            log_string(mt_string("Download already started. Will continue to download ID: ") + int_to_string(download->get_id()), LOG_DEBUG);
        } else {
            output_file.open(output_filename.c_str(), ios::out | ios::binary);
        }

		if(!output_file.good()) {
			log_string(mt_string("Could not write to file: ") + output_filename, LOG_SEVERE);
			download->set_plugin_status(PLUGIN_WRITE_FILE_ERROR);
			download->set_status(DOWNLOAD_PENDING, true);
			return;
		}

        download->set_output_file(output_filename);

		if(plug_outp.download_url.empty()) {
			log_string(mt_string("Empty URL for download ID: ") + int_to_string(download->get_id()), LOG_SEVERE);
			download->set_plugin_status(PLUGIN_ERROR);
			download->set_status(DOWNLOAD_PENDING, true);
			return;
		}

		// set url
		curl_easy_setopt(download->get_handle(), CURLOPT_FOLLOWLOCATION, 1);
		curl_easy_setopt(download->get_handle(), CURLOPT_URL, plug_outp.download_url.c_str());
		// set file-writing function as callback
		curl_easy_setopt(download->get_handle(), CURLOPT_WRITEFUNCTION, write_file);
		curl_easy_setopt(download->get_handle(), CURLOPT_WRITEDATA, &output_file);
		// show progress
		curl_easy_setopt(download->get_handle(), CURLOPT_NOPROGRESS, 0);
		curl_easy_setopt(download->get_handle(), CURLOPT_PROGRESSFUNCTION, report_progress);
		curl_easy_setopt(download->get_handle(), CURLOPT_PROGRESSDATA, &download);
		// set timeouts
		curl_easy_setopt(download->get_handle(), CURLOPT_LOW_SPEED_LIMIT, 100);
		curl_easy_setopt(download->get_handle(), CURLOPT_LOW_SPEED_TIME, 20);

		log_string(mt_string("Starting download ID: ") + int_to_string(download->get_id()), LOG_DEBUG);

		success = curl_easy_perform(download->get_handle());
		switch(success) {
			case 0:
				log_string(mt_string("Finished download ID: ") + int_to_string(download->get_id()), LOG_DEBUG);
				download->set_status(DOWNLOAD_FINISHED, true);
				curl_easy_reset(download->get_handle());
				return;
			case 28:
				if(download->get_status() == DOWNLOAD_INACTIVE) {
					log_string(mt_string("Stopped download ID: ") + int_to_string(download->get_id()), LOG_WARNING);
					curl_easy_reset(download->get_handle());
				} else if(download->get_status() == DOWNLOAD_DELETED) {
					curl_easy_reset(download->get_handle());
					global_download_list.erase(download);
				} else {
					log_string(mt_string("Connection lost for download ID: ") + int_to_string(download->get_id()), LOG_WARNING);
					download->set_status(DOWNLOAD_PENDING, true);
					download->set_plugin_status(PLUGIN_CONNECTION_LOST);
					curl_easy_reset(download->get_handle());
				}
				return;
			default:
				log_string(mt_string("Download error for download ID: ") + int_to_string(download->get_id()), LOG_WARNING);
				download->set_status(DOWNLOAD_PENDING, true);
				download->set_plugin_status(PLUGIN_CONNECTION_LOST);
				curl_easy_reset(download->get_handle());
				return;
		}
	} else {
		if(success == PLUGIN_SERVER_OVERLOADED) {
			log_string(mt_string("Server overloaded for download ID: ") + int_to_string(download->get_id()), LOG_WARNING);
			download->set_status(DOWNLOAD_WAITING, true);
			return;
		} else if(success == PLUGIN_LIMIT_REACHED) {
			log_string(mt_string("Download limit reached for download ID: ") + int_to_string(download->get_id()) + " (" + download->get_host() + ")", LOG_WARNING);
			download->set_status(DOWNLOAD_WAITING, true);
			return;
		} else if(success == PLUGIN_CONNECTION_ERROR) {
			log_string(mt_string("Plugin failed to connect for ID:") + int_to_string(download->get_id()), LOG_WARNING);
			download->set_plugin_status(PLUGIN_CONNECTION_ERROR);
			download->set_status(DOWNLOAD_INACTIVE, true);
			return;
		} else if(success == PLUGIN_FILE_NOT_FOUND) {
			log_string(mt_string("File could not be found on the server for ID: ") + int_to_string(download->get_id()), LOG_WARNING);
			download->set_plugin_status(PLUGIN_FILE_NOT_FOUND);
			download->set_status(DOWNLOAD_INACTIVE, true);
			return;
		}
	}
	// something weird happened...
	download->set_status(DOWNLOAD_PENDING, true);
	return;
}


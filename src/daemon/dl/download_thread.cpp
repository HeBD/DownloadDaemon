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
	int downloadable = 0;
	while(1) {
		downloadable = global_download_list.get_next_downloadable();
		if(downloadable == LIST_ID) {
			sleep(1);
			continue;
		} else {
			global_download_list.set_int_property(downloadable, DL_IS_RUNNING, true);
			global_download_list.set_int_property(downloadable, DL_STATUS, DOWNLOAD_RUNNING);
			boost::thread t(boost::bind(download_thread, downloadable));
		}
	}
}

/** This function does the magic of downloading a file, calling the right plugin, etc.
 *	@param download iterator to a download in the global download list, that we should load
 */
void download_thread(int download) {
	plugin_output plug_outp;
	int success = global_download_list.prepare_download(download, plug_outp);
	if(global_download_list.get_int_property(download, DL_STATUS) == DOWNLOAD_DELETED) {
		return;
	}

	switch(success) {
		case PLUGIN_INVALID_HOST:
			global_download_list.set_int_property(download, DL_PLUGIN_STATUS, PLUGIN_INVALID_HOST);
			global_download_list.set_int_property(download, DL_STATUS, DOWNLOAD_INACTIVE);
			log_string(std::string("Invalid host for download ID: ") + int_to_string(download), LOG_WARNING);
			global_download_list.set_int_property(download, DL_IS_RUNNING, false);
			return;
		break;
		case PLUGIN_INVALID_PATH:
			global_download_list.set_int_property(download, DL_PLUGIN_STATUS, PLUGIN_INVALID_PATH);
			global_download_list.set_int_property(download, DL_STATUS, DOWNLOAD_PENDING);
			global_download_list.set_int_property(download, DL_IS_RUNNING, false);
			log_string("Could not locate plugin folder!", LOG_SEVERE);
			exit(-1);
		break;
		case PLUGIN_MISSING:
			global_download_list.set_int_property(download, DL_PLUGIN_STATUS, PLUGIN_MISSING);
			global_download_list.set_int_property(download, DL_STATUS, DOWNLOAD_INACTIVE);
			log_string(std::string("Plugin missing for download ID: ") + int_to_string(download), LOG_WARNING);
			global_download_list.set_int_property(download, DL_IS_RUNNING, false);
			return;
		break;
	}

	if(success == PLUGIN_SUCCESS) {
		global_download_list.set_int_property(download, DL_PLUGIN_STATUS, PLUGIN_SUCCESS);
		log_string(std::string("Successfully parsed download ID: ") + int_to_string(download), LOG_DEBUG);
		if(global_download_list.get_int_property(download, DL_WAIT_SECONDS) > 0) {

			if(global_download_list.get_int_property(download, DL_STATUS) != DOWNLOAD_DELETED) {
				log_string(std::string("Download ID: ") + int_to_string(download) + " has to wait " +
					       int_to_string(global_download_list.get_int_property(download, DL_WAIT_SECONDS)) + " seconds before downloading can start", LOG_DEBUG);
					       std::string blah(global_download_list.get_string_property(download, DL_URL));
			} else {
				return;
			}

			while(global_download_list.get_int_property(download, DL_WAIT_SECONDS) > 0) {
				if(global_download_list.get_int_property(download, DL_STATUS) == DOWNLOAD_INACTIVE || global_download_list.get_int_property(download, DL_STATUS) == DOWNLOAD_DELETED) {
					break;
				}
				sleep(1);
				global_download_list.set_int_property(download, DL_WAIT_SECONDS, global_download_list.get_int_property(download, DL_WAIT_SECONDS) - 1);
			}
		}
		if(global_download_list.get_int_property(download, DL_STATUS) == DOWNLOAD_DELETED || global_download_list.get_int_property(download, DL_STATUS) == DOWNLOAD_INACTIVE) {
			global_download_list.set_int_property(download, DL_WAIT_SECONDS, 0);
			global_download_list.set_int_property(download, DL_IS_RUNNING, false);
			return;
		}

		std::string output_filename;
		std::string download_folder = global_config.get_cfg_value("download_folder");
		correct_path(download_folder);
		if(plug_outp.download_filename == "") {
			if(plug_outp.download_url != "" && plug_outp.download_url.find('/') != std::string::npos) {
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
        if(global_download_list.get_hostinfo(download).allows_multiple && global_config.get_cfg_value("enable_resume") != "0" &&
           stat(output_filename.c_str(), &st) == 0 && st.st_size == global_download_list.get_int_property(download, DL_DOWNLOADED_BYTES)) {
            curl_easy_setopt(global_download_list.get_pointer_property(download, DL_HANDLE), CURLOPT_RESUME_FROM, st.st_size);
            output_file.open(output_filename.c_str(), ios::out | ios::binary | ios::app);
            log_string(std::string("Download already started. Will continue to download ID: ") + int_to_string(download), LOG_DEBUG);
        } else {
            output_file.open(output_filename.c_str(), ios::out | ios::binary);
        }

		if(!output_file.good()) {
			log_string(std::string("Could not write to file: ") + output_filename, LOG_SEVERE);
			global_download_list.set_int_property(download, DL_PLUGIN_STATUS, PLUGIN_WRITE_FILE_ERROR);
			global_download_list.set_int_property(download, DL_STATUS, DOWNLOAD_PENDING);
			global_download_list.set_int_property(download, DL_IS_RUNNING, false);
			return;
		}

		global_download_list.set_string_property(download, DL_OUTPUT_FILE, output_filename);

		if(plug_outp.download_url.empty()) {
			log_string(std::string("Empty URL for download ID: ") + int_to_string(download), LOG_SEVERE);
			global_download_list.set_int_property(download, DL_PLUGIN_STATUS, PLUGIN_ERROR);
			global_download_list.set_int_property(download, DL_STATUS, DOWNLOAD_PENDING);
			global_download_list.set_int_property(download, DL_IS_RUNNING, false);
			return;
		}

		// set url
		curl_easy_setopt(global_download_list.get_pointer_property(download, DL_HANDLE), CURLOPT_FOLLOWLOCATION, 1);
		curl_easy_setopt(global_download_list.get_pointer_property(download, DL_HANDLE), CURLOPT_URL, plug_outp.download_url.c_str());
		// set file-writing function as callback
		curl_easy_setopt(global_download_list.get_pointer_property(download, DL_HANDLE), CURLOPT_WRITEFUNCTION, write_file);
		curl_easy_setopt(global_download_list.get_pointer_property(download, DL_HANDLE), CURLOPT_WRITEDATA, &output_file);
		// show progress
		curl_easy_setopt(global_download_list.get_pointer_property(download, DL_HANDLE), CURLOPT_NOPROGRESS, 0);
		curl_easy_setopt(global_download_list.get_pointer_property(download, DL_HANDLE), CURLOPT_PROGRESSFUNCTION, report_progress);
		curl_easy_setopt(global_download_list.get_pointer_property(download, DL_HANDLE), CURLOPT_PROGRESSDATA, &download);
		// set timeouts
		curl_easy_setopt(global_download_list.get_pointer_property(download, DL_HANDLE), CURLOPT_LOW_SPEED_LIMIT, 100);
		curl_easy_setopt(global_download_list.get_pointer_property(download, DL_HANDLE), CURLOPT_LOW_SPEED_TIME, 20);

		log_string(std::string("Starting download ID: ") + int_to_string(download), LOG_DEBUG);
		success = curl_easy_perform(global_download_list.get_pointer_property(download, DL_HANDLE));
		curl_easy_reset(global_download_list.get_pointer_property(download, DL_HANDLE));
		output_file.close();
		switch(success) {
			case 0:
				log_string(std::string("Finished download ID: ") + int_to_string(download), LOG_DEBUG);
				global_download_list.set_int_property(download, DL_STATUS, DOWNLOAD_FINISHED);
				curl_easy_reset(global_download_list.get_pointer_property(download, DL_HANDLE));
				global_download_list.set_int_property(download, DL_IS_RUNNING, false);
				return;
			case 28:
				if(global_download_list.get_int_property(download, DL_STATUS) == DOWNLOAD_INACTIVE) {
					log_string(std::string("Stopped download ID: ") + int_to_string(download), LOG_WARNING);
					curl_easy_reset(global_download_list.get_pointer_property(download, DL_HANDLE));
				} else if(global_download_list.get_int_property(download, DL_STATUS) == DOWNLOAD_DELETED) {
					// nothing to do...
				} else {
					log_string(std::string("Connection lost for download ID: ") + int_to_string(download), LOG_WARNING);
					global_download_list.set_int_property(download, DL_PLUGIN_STATUS, PLUGIN_CONNECTION_LOST);
					global_download_list.set_int_property(download, DL_STATUS, DOWNLOAD_PENDING);
					curl_easy_reset(global_download_list.get_pointer_property(download, DL_HANDLE));
				}
				global_download_list.set_int_property(download, DL_IS_RUNNING, false);
				return;
			default:
				log_string(std::string("Download error for download ID: ") + int_to_string(download), LOG_WARNING);
				global_download_list.set_int_property(download, DL_PLUGIN_STATUS, PLUGIN_CONNECTION_LOST);
				global_download_list.set_int_property(download, DL_STATUS, DOWNLOAD_PENDING);
				curl_easy_reset(global_download_list.get_pointer_property(download, DL_HANDLE));
				global_download_list.set_int_property(download, DL_IS_RUNNING, false);
				return;
		}
	} else {
		if(success == PLUGIN_SERVER_OVERLOADED) {
			log_string(std::string("Server overloaded for download ID: ") + int_to_string(download), LOG_WARNING);
			global_download_list.set_int_property(download, DL_STATUS, DOWNLOAD_WAITING);
			global_download_list.set_int_property(download, DL_IS_RUNNING, false);
			return;
		} else if(success == PLUGIN_LIMIT_REACHED) {
			log_string(std::string("Download limit reached for download ID: ") + int_to_string(download) + " (" + global_download_list.get_host(download) + ")", LOG_WARNING);
			global_download_list.set_int_property(download, DL_STATUS, DOWNLOAD_WAITING);
			global_download_list.set_int_property(download, DL_IS_RUNNING, false);
			return;
		} else if(success == PLUGIN_CONNECTION_ERROR) {
			log_string(std::string("Plugin failed to connect for ID:") + int_to_string(download), LOG_WARNING);
			global_download_list.set_int_property(download, DL_PLUGIN_STATUS, PLUGIN_CONNECTION_ERROR);
			global_download_list.set_int_property(download, DL_STATUS, DOWNLOAD_INACTIVE);
			global_download_list.set_int_property(download, DL_IS_RUNNING, false);
			return;
		} else if(success == PLUGIN_FILE_NOT_FOUND) {
			log_string(std::string("File could not be found on the server for ID: ") + int_to_string(download), LOG_WARNING);
			global_download_list.set_int_property(download, DL_PLUGIN_STATUS, PLUGIN_FILE_NOT_FOUND);
			global_download_list.set_int_property(download, DL_STATUS, DOWNLOAD_INACTIVE);
			global_download_list.set_int_property(download, DL_IS_RUNNING, false);
			return;
		}
	}
	// something weird happened...
	global_download_list.set_int_property(download, DL_STATUS, DOWNLOAD_PENDING);
	global_download_list.set_int_property(download, DL_IS_RUNNING, false);
	return;
}


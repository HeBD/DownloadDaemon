/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

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
#include "../global.h"

using namespace std;

/** start the next possible download in a new thread */
void start_next_download() {
	int downloadable = global_download_list.get_next_downloadable();
	if(downloadable == LIST_ID) {
		return;
	} else {
		global_download_list.set_int_property(downloadable, DL_IS_RUNNING, true);
		global_download_list.set_int_property(downloadable, DL_STATUS, DOWNLOAD_RUNNING);
		boost::thread t(boost::bind(download_thread, downloadable));
		start_next_download();
	}
}

/** This function does the magic of downloading a file, calling the right plugin, etc.
 *	@param download ID of a download in the global download list, that we should load
 */
void download_thread(int download) {
	plugin_output plug_outp;
	int success = global_download_list.prepare_download(download, plug_outp);
	if(global_download_list.get_int_property(download, DL_STATUS) == DOWNLOAD_DELETED) {
		return;
	}
	// Used later as temporary variable for all the <error>_wait config variables to save I/O
	long wait;
	global_download_list.set_int_property(download, DL_PLUGIN_STATUS, success);

	switch(success) {
		case PLUGIN_INVALID_HOST:
			global_download_list.set_int_property(download, DL_STATUS, DOWNLOAD_INACTIVE);
			log_string(std::string("Invalid host for download ID: ") + int_to_string(download), LOG_WARNING);
			global_download_list.set_int_property(download, DL_IS_RUNNING, false);
			return;
		break;
		case PLUGIN_INVALID_PATH:
			global_download_list.set_int_property(download, DL_STATUS, DOWNLOAD_PENDING);
			log_string("Could not locate plugin folder!", LOG_SEVERE);
			global_download_list.set_int_property(download, DL_IS_RUNNING, false);
			exit(-1);
		break;
		case PLUGIN_AUTH_FAIL:
			global_download_list.set_int_property(download, DL_STATUS, DOWNLOAD_PENDING);
			wait = atol(global_config.get_cfg_value("auth_fail_wait").c_str());
			if(wait == 0) {
				global_download_list.set_int_property(download, DL_STATUS, DOWNLOAD_INACTIVE);
			} else {
				global_download_list.set_int_property(download, DL_WAIT_SECONDS, wait);
				global_download_list.set_int_property(download, DL_STATUS, DOWNLOAD_PENDING);
			}
			log_string(std::string("Premium authentication failed for download ") + int_to_string(download), LOG_WARNING);
			global_download_list.set_int_property(download, DL_IS_RUNNING, false);
			return;
	}

	if(success == PLUGIN_SUCCESS) {
		log_string(std::string("Successfully parsed download ID: ") + int_to_string(download), LOG_DEBUG);
		if(global_download_list.get_int_property(download, DL_WAIT_SECONDS) > 0) {

			if(global_download_list.get_int_property(download, DL_STATUS) != DOWNLOAD_DELETED) {
				log_string(std::string("Download ID: ") + int_to_string(download) + " has to wait " +
						   int_to_string(global_download_list.get_int_property(download, DL_WAIT_SECONDS)) + " seconds before downloading can start", LOG_DEBUG);
						   std::string blah(global_download_list.get_string_property(download, DL_URL));
			} else {
				global_download_list.set_int_property(download, DL_IS_RUNNING, false);
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
				if(output_filename.find('?') != string::npos) {
					output_filename = output_filename.substr(0, output_filename.find('?'));
				}
			}
		} else {
			output_filename += download_folder;
			output_filename += '/' + plug_outp.download_filename;
		}

		std::string final_filename(output_filename);
		output_filename += ".part";

		struct stat st;

		// Check if we can do a download resume or if we have to start from the beginning
		fstream output_file;
		if(global_download_list.get_hostinfo(download).allows_resumption && global_config.get_cfg_value("enable_resume") != "0" &&
		   stat(output_filename.c_str(), &st) == 0 && st.st_size == global_download_list.get_int_property(download, DL_DOWNLOADED_BYTES)) {
			curl_easy_setopt(global_download_list.get_pointer_property(download, DL_HANDLE), CURLOPT_RESUME_FROM, st.st_size);
			output_file.open(output_filename.c_str(), ios::out | ios::binary | ios::app);
			log_string(std::string("Download already started. Will continue to download ID: ") + int_to_string(download), LOG_DEBUG);
		} else {
			// Check if the file should be overwritten if it exists
			string overwrite(global_config.get_cfg_value("overwrite_files"));
			if(overwrite == "0" || overwrite == "false" || overwrite == "no") {
				if(stat(final_filename.c_str(), &st) == 0) {
					global_download_list.set_int_property(download, DL_STATUS, DOWNLOAD_INACTIVE);
					global_download_list.set_int_property(download, DL_PLUGIN_STATUS, PLUGIN_WRITE_FILE_ERROR);
					global_download_list.set_int_property(download, DL_IS_RUNNING, false);
					return;

				}
			}
			output_file.open(output_filename.c_str(), ios::out | ios::binary | ios::trunc);
		}

		if(!output_file.good()) {
			log_string(std::string("Could not write to file: ") + output_filename, LOG_SEVERE);
			global_download_list.set_int_property(download, DL_PLUGIN_STATUS, PLUGIN_WRITE_FILE_ERROR);
			wait = atol(global_config.get_cfg_value("write_error_wait").c_str());
			if(wait == 0) {
				global_download_list.set_int_property(download, DL_STATUS, DOWNLOAD_INACTIVE);
			} else {
				global_download_list.set_int_property(download, DL_STATUS, DOWNLOAD_PENDING);
				global_download_list.set_int_property(download, DL_WAIT_SECONDS, wait);
			}
			global_download_list.set_int_property(download, DL_IS_RUNNING, false);
			return;
		}

		global_download_list.set_string_property(download, DL_OUTPUT_FILE, output_filename);

		if(plug_outp.download_url.empty()) {
			log_string(std::string("Empty URL for download ID: ") + int_to_string(download), LOG_SEVERE);
			global_download_list.set_int_property(download, DL_PLUGIN_STATUS, PLUGIN_ERROR);
			wait = atol(global_config.get_cfg_value("plugin_fail_wait").c_str());
			if(wait == 0) {
				global_download_list.set_int_property(download, DL_STATUS, DOWNLOAD_INACTIVE);
			} else {
				global_download_list.set_int_property(download, DL_STATUS, DOWNLOAD_PENDING);
				global_download_list.set_int_property(download, DL_WAIT_SECONDS, wait);
			}
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
		long http_code;
		curl_easy_getinfo(global_download_list.get_pointer_property(download, DL_HANDLE), CURLINFO_RESPONSE_CODE, &http_code);
		curl_easy_reset(global_download_list.get_pointer_property(download, DL_HANDLE));
		output_file.close();

		switch(http_code) {
			case 401:
				log_string(std::string("Invalid premium account credentials, ID: ") + int_to_string(download), LOG_WARNING);
				wait = atol(global_config.get_cfg_value("auth_fail_wait").c_str());
				if(wait == 0) {
					global_download_list.set_int_property(download, DL_STATUS, DOWNLOAD_INACTIVE);
				} else {
					global_download_list.set_int_property(download, DL_STATUS, DOWNLOAD_PENDING);
					global_download_list.set_int_property(download, DL_WAIT_SECONDS, wait);
				}
				global_download_list.set_int_property(download, DL_IS_RUNNING, false);
				return;
			case 404:
				log_string(std::string("File not found, ID: ") + int_to_string(download), LOG_WARNING);
				global_download_list.set_int_property(download, DL_STATUS, DOWNLOAD_INACTIVE);
				global_download_list.set_int_property(download, DL_PLUGIN_STATUS, PLUGIN_FILE_NOT_FOUND);
				string file(global_download_list.get_string_property(download, DL_OUTPUT_FILE));
				if(!file.empty()) {
					remove(file.c_str());
					global_download_list.set_string_property(download, DL_OUTPUT_FILE, "");
				}
				global_download_list.set_int_property(download, DL_IS_RUNNING, false);
				return;
		}

		switch(success) {
			case 0:
				log_string(std::string("Finished download ID: ") + int_to_string(download), LOG_DEBUG);
				global_download_list.set_int_property(download, DL_STATUS, DOWNLOAD_FINISHED);
				// RENAME .part FILE HERE!! and change DL_OUTPUT_FILE
				if(rename(output_filename.c_str(), final_filename.c_str()) != 0) {
					log_string(std::string("Unable to rename .part file. You can do so manually."), LOG_SEVERE);
				}
				global_download_list.set_int_property(download, DL_IS_RUNNING, false);
				return;
			case 28:
				if(global_download_list.get_int_property(download, DL_STATUS) == DOWNLOAD_INACTIVE) {
					log_string(std::string("Stopped download ID: ") + int_to_string(download), LOG_WARNING);
				} else if(global_download_list.get_int_property(download, DL_STATUS) != DOWNLOAD_DELETED) {
					log_string(std::string("Connection lost for download ID: ") + int_to_string(download), LOG_WARNING);
					global_download_list.set_int_property(download, DL_PLUGIN_STATUS, PLUGIN_CONNECTION_LOST);
					wait = atol(global_config.get_cfg_value("connection_lost_wait").c_str());
					if(wait == 0) {
						global_download_list.set_int_property(download, DL_STATUS, DOWNLOAD_INACTIVE);
					} else {
						global_download_list.set_int_property(download, DL_STATUS, DOWNLOAD_PENDING);
						global_download_list.set_int_property(download, DL_WAIT_SECONDS, wait);
					}
				}
				global_download_list.set_int_property(download, DL_IS_RUNNING, false);
				return;
			default:
				log_string(std::string("Download error for download ID: ") + int_to_string(download), LOG_WARNING);
				global_download_list.set_int_property(download, DL_PLUGIN_STATUS, PLUGIN_CONNECTION_LOST);
				wait = atol(global_config.get_cfg_value("connection_lost_wait").c_str());
				if(wait == 0) {
					global_download_list.set_int_property(download, DL_STATUS, DOWNLOAD_INACTIVE);
				} else {
					global_download_list.set_int_property(download, DL_STATUS, DOWNLOAD_PENDING);
					global_download_list.set_int_property(download, DL_WAIT_SECONDS, wait);
				}
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
			global_download_list.set_int_property(download, DL_STATUS, DOWNLOAD_PENDING);
			global_download_list.set_int_property(download, DL_WAIT_SECONDS, 10);
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
	global_download_list.set_int_property(download, DL_WAIT_SECONDS, 30);
	global_download_list.set_int_property(download, DL_IS_RUNNING, false);
	return;
}

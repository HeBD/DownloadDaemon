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
#include "recursive_parser.h"

using namespace std;

/** start the next possible download in a new thread */
void start_next_download() {
	int downloadable = global_download_list.get_next_downloadable();
	if(downloadable == LIST_ID) {
		return;
	} else {
		global_download_list.set_int_property(downloadable, DL_IS_RUNNING, true);
		global_download_list.set_int_property(downloadable, DL_STATUS, DOWNLOAD_RUNNING);
		boost::thread t(boost::bind(download_thread_wrapper, downloadable));
		start_next_download();
	}
}

/** Does basic work before download_thread is called and after it has finished */
void download_thread_wrapper(int download) {
	global_download_list.init_handle(download);
	download_thread(download);
	global_download_list.cleanup_handle(download);
	global_download_list.set_int_property(download, DL_IS_RUNNING, false);

}

/** This function does the magic of downloading a file, calling the right plugin, etc.
 *	@param download ID of a download in the global download list, that we should load
 */
void download_thread(int download) {
	plugin_output plug_outp;
	int success = global_download_list.prepare_download(download, plug_outp);
	std::string url = plug_outp.download_url;
	if(url[url.size() - 1] == '/' && url.find("ftp://") == 0 && global_config.get_cfg_value("recursive_ftp_download") != "0") {
		recursive_parser parser(url);
		parser.add_to_list();
		if(global_download_list.get_int_property(download, DL_STATUS) == DOWNLOAD_DELETED) {
			return;
		}
		global_download_list.set_int_property(download, DL_STATUS, DOWNLOAD_INACTIVE);
		log_string("Downloading directory: " + url, LOG_DEBUG);
		return;
	}

	if(global_download_list.get_int_property(download, DL_STATUS) != DOWNLOAD_RUNNING) {
		return;
	}
	// Used later as temporary variable for all the <error>_wait config variables to save I/O
	long wait;
	global_download_list.set_int_property(download, DL_PLUGIN_STATUS, success);

	switch(success) {
		case PLUGIN_INVALID_HOST:
			global_download_list.set_int_property(download, DL_STATUS, DOWNLOAD_INACTIVE);
			log_string(std::string("Invalid host for download ID: ") + int_to_string(download), LOG_WARNING);
			return;
		break;
		case PLUGIN_INVALID_PATH:
			global_download_list.set_int_property(download, DL_STATUS, DOWNLOAD_PENDING);
			log_string("Could not locate plugin folder!", LOG_ERR);
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
			return;
		case PLUGIN_SERVER_OVERLOADED:
			log_string(std::string("Server overloaded for download ID: ") + int_to_string(download), LOG_WARNING);
			global_download_list.set_int_property(download, DL_STATUS, DOWNLOAD_WAITING);
			return;
		case PLUGIN_LIMIT_REACHED:
			log_string(std::string("Download limit reached for download ID: ") + int_to_string(download) + " (" + global_download_list.get_host(download) + ")", LOG_WARNING);
			global_download_list.set_int_property(download, DL_STATUS, DOWNLOAD_WAITING);
			return;
		// do the same for both
		case PLUGIN_CONNECTION_ERROR:
		case PLUGIN_CONNECTION_LOST:
			log_string(std::string("Plugin failed to connect for ID:") + int_to_string(download), LOG_WARNING);
			global_download_list.set_int_property(download, DL_STATUS, DOWNLOAD_PENDING);
			global_download_list.set_int_property(download, DL_WAIT_SECONDS, 10);
			return;
		case PLUGIN_FILE_NOT_FOUND:
			log_string(std::string("File could not be found on the server for ID: ") + int_to_string(download), LOG_WARNING);
			global_download_list.set_int_property(download, DL_STATUS, DOWNLOAD_INACTIVE);
			return;
		case PLUGIN_ERROR:
			log_string("An error occured on download ID: " + int_to_string(download), LOG_WARNING);
			global_download_list.set_int_property(download, DL_STATUS, DOWNLOAD_PENDING);
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
			return;
		}

		std::string output_filename;
		std::string download_folder = global_config.get_cfg_value("download_folder");
		correct_path(download_folder);
		mkdir_recursive(download_folder);
		if(plug_outp.download_filename == "") {
			if(plug_outp.download_url != "") {
				std::string fn = filename_from_url(plug_outp.download_url);
				output_filename = download_folder + '/';
				if(fn.empty()) {
					output_filename += "file";
				} else {
					output_filename += fn;
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
		   stat(output_filename.c_str(), &st) == 0 && st.st_size == global_download_list.get_int_property(download, DL_DOWNLOADED_BYTES) &&
		   global_download_list.get_int_property(download, DL_CAN_RESUME)) {

			curl_easy_setopt(global_download_list.get_pointer_property(download, DL_HANDLE), CURLOPT_RESUME_FROM, global_download_list.get_int_property(download, DL_DOWNLOADED_BYTES));
			output_file.open(output_filename.c_str(), ios::out | ios::binary | ios::app);
			log_string(std::string("Download already started. Will try to continue to download ID: ") + int_to_string(download), LOG_DEBUG);
		} else {
			// Check if the file should be overwritten if it exists
			string overwrite(global_config.get_cfg_value("overwrite_files"));
			if(overwrite == "0" || overwrite == "false" || overwrite == "no") {
				if(stat(final_filename.c_str(), &st) == 0) {
					global_download_list.set_int_property(download, DL_STATUS, DOWNLOAD_INACTIVE);
					global_download_list.set_int_property(download, DL_PLUGIN_STATUS, PLUGIN_WRITE_FILE_ERROR);
					return;

				}
			}
			output_file.open(output_filename.c_str(), ios::out | ios::binary | ios::trunc);
		}

		if(!output_file.good()) {
			log_string(std::string("Could not write to file: ") + output_filename, LOG_ERR);
			global_download_list.set_int_property(download, DL_PLUGIN_STATUS, PLUGIN_WRITE_FILE_ERROR);
			wait = atol(global_config.get_cfg_value("write_error_wait").c_str());
			if(wait == 0) {
				global_download_list.set_int_property(download, DL_STATUS, DOWNLOAD_INACTIVE);
			} else {
				global_download_list.set_int_property(download, DL_STATUS, DOWNLOAD_PENDING);
				global_download_list.set_int_property(download, DL_WAIT_SECONDS, wait);
			}
			return;
		}

		global_download_list.set_string_property(download, DL_OUTPUT_FILE, output_filename);

		if(plug_outp.download_url.empty()) {
			log_string(std::string("Empty URL for download ID: ") + int_to_string(download), LOG_ERR);
			global_download_list.set_int_property(download, DL_PLUGIN_STATUS, PLUGIN_ERROR);
			wait = atol(global_config.get_cfg_value("plugin_fail_wait").c_str());
			if(wait == 0) {
				global_download_list.set_int_property(download, DL_STATUS, DOWNLOAD_INACTIVE);
			} else {
				global_download_list.set_int_property(download, DL_STATUS, DOWNLOAD_PENDING);
				global_download_list.set_int_property(download, DL_WAIT_SECONDS, wait);
			}
			return;
		}

		CURL* handle_copy = global_download_list.get_pointer_property(download, DL_HANDLE);
		// set url
		curl_easy_setopt(handle_copy, CURLOPT_FOLLOWLOCATION, 1);
		curl_easy_setopt(handle_copy, CURLOPT_URL, plug_outp.download_url.c_str());
		// set file-writing function as callback
		curl_easy_setopt(handle_copy, CURLOPT_WRITEFUNCTION, write_file);
		curl_easy_setopt(handle_copy, CURLOPT_WRITEDATA, &output_file);
		// show progress
		curl_easy_setopt(handle_copy, CURLOPT_NOPROGRESS, 0);
		curl_easy_setopt(handle_copy, CURLOPT_PROGRESSFUNCTION, report_progress);
		curl_easy_setopt(handle_copy, CURLOPT_PROGRESSDATA, &download);
		// set timeouts
		curl_easy_setopt(handle_copy, CURLOPT_LOW_SPEED_LIMIT, 100);
		curl_easy_setopt(handle_copy, CURLOPT_LOW_SPEED_TIME, 20);
		long dl_speed = atol(global_config.get_cfg_value("max_dl_speed").c_str()) * 1024;
		if(dl_speed > 0) {
			curl_easy_setopt(handle_copy, CURLOPT_MAX_RECV_SPEED_LARGE, dl_speed);
		}

		log_string(std::string("Starting download ID: ") + int_to_string(download), LOG_DEBUG);
		success = curl_easy_perform(handle_copy);



		long http_code;
		curl_easy_getinfo(handle_copy, CURLINFO_RESPONSE_CODE, &http_code);
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
				return;
			case 404:
				log_string(std::string("File not found, ID: ") + int_to_string(download), LOG_WARNING);
				global_download_list.set_int_property(download, DL_STATUS, DOWNLOAD_INACTIVE);
				global_download_list.set_int_property(download, DL_PLUGIN_STATUS, PLUGIN_FILE_NOT_FOUND);
				if(!output_filename.empty()) {
					remove(output_filename.c_str());
					global_download_list.set_string_property(download, DL_OUTPUT_FILE, "");
				}
				return;
			case 530:
				log_string("Invalid http authentication on download ID: " + int_to_string(download), LOG_WARNING);
				global_download_list.set_int_property(download, DL_STATUS, DOWNLOAD_INACTIVE);
				global_download_list.set_int_property(download, DL_PLUGIN_STATUS, PLUGIN_AUTH_FAIL);
				remove(output_filename.c_str());
				global_download_list.set_string_property(download, DL_OUTPUT_FILE, "");
				return;
		}

		switch(success) {
			case 0:
				log_string(std::string("Finished download ID: ") + int_to_string(download), LOG_DEBUG);
				global_download_list.set_int_property(download, DL_STATUS, DOWNLOAD_FINISHED);
				if(rename(output_filename.c_str(), final_filename.c_str()) != 0) {
					log_string(std::string("Unable to rename .part file. You can do so manually."), LOG_ERR);
				} else {
					global_download_list.set_string_property(download, DL_OUTPUT_FILE, final_filename);
				}
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
				return;
			case 36:
				// if download resumption fails, retry without resumption
				global_download_list.set_int_property(download, DL_CAN_RESUME, false);
				global_download_list.set_int_property(download, DL_STATUS, DOWNLOAD_PENDING);
				log_string("Resuming download failed. retrying without resumption", LOG_WARNING);
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
				return;
		}
	} else {
		log_string("Plugin returned an invalid/unhandled status. Please report! (status returned: " + int_to_string(success) + ")", LOG_ERR);
		global_download_list.set_int_property(download, DL_STATUS, DOWNLOAD_PENDING);
		global_download_list.set_int_property(download, DL_WAIT_SECONDS, 30);
		return;
	}
}

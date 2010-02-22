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
#ifndef USE_STD_THREAD
#include <boost/thread.hpp>
namespace std {
	using namespace boost;
}
#else
#include <thread>
#endif

#include <vector>
#include <fstream>
#include <cstdlib>
#include <ctime>

#include <string>
#include <curl/curl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <cfgfile/cfgfile.h>
#include "download_thread.h"
#include "download_container.h"
#include "../tools/helperfunctions.h"
#include "../tools/curl_callbacks.h"
#include "../global.h"
#include "recursive_parser.h"

using namespace std;

/** start the next possible download in a new thread */
void start_next_download() {
	dlindex downloadable = global_download_list.get_next_downloadable();
	if(downloadable.first == LIST_ID) {
		return;
	} else {
		global_download_list.set_running(downloadable, true);
		global_download_list.set_status(downloadable, DOWNLOAD_RUNNING);
		thread t(bind(download_thread_wrapper, downloadable));
		t.detach();
		start_next_download();
	}
}

/** Does basic work before download_thread is called and after it has finished */
void download_thread_wrapper(dlindex download) {
	global_download_list.set_need_stop(download, false);
	global_download_list.init_handle(download);
	download_thread(download);

	global_download_list.set_need_stop(download, false);
	global_download_list.set_running(download, false);

	download_status status = global_download_list.get_status(download);
	plugin_status error = global_download_list.get_error(download);

	if(status == DOWNLOAD_RUNNING) {
		global_download_list.set_status(download, DOWNLOAD_PENDING);
	}

	if(status == DOWNLOAD_WAITING && error == PLUGIN_LIMIT_REACHED && global_download_list.set_next_proxy(download) == 1) {
		global_download_list.set_status(download, DOWNLOAD_PENDING);
		global_download_list.set_error(download, PLUGIN_SUCCESS);
		global_download_list.set_wait(download, 0);
	} else if((error == PLUGIN_ERROR || error == PLUGIN_CONNECTION_ERROR || error == PLUGIN_CONNECTION_LOST || error == PLUGIN_INVALID_HOST)
			  && !atoi(global_config.get_cfg_value("assume_proxys_online").c_str()) && global_download_list.set_next_proxy(download) == 1) {
		global_download_list.set_status(download, DOWNLOAD_PENDING);
		global_download_list.set_error(download, PLUGIN_SUCCESS);
		global_download_list.set_wait(download, 0);
	} else {
		global_download_list.set_proxy(download, "");
	}

	global_download_list.cleanup_handle(download);
}

/** This function does the magic of downloading a file, calling the right plugin, etc.
 *	@param download ID of a download in the global download list, that we should load
 */
void download_thread(dlindex download) {
	plugin_output plug_outp;
	int success = global_download_list.prepare_download(download, plug_outp);
	std::string dlid_log = int_to_string(download.first) + "->" + int_to_string(download.second);
	if(global_download_list.get_need_stop(download)) {
		return;
	}

	std::string url = plug_outp.download_url;
	if(url[url.size() - 1] == '/' && url.find("ftp://") == 0 && global_config.get_cfg_value("recursive_ftp_download") != "0") {
		recursive_parser parser(url);
		parser.add_to_list(download.first);
		if(global_download_list.get_status(download) == DOWNLOAD_DELETED) {
			return;
		}
		global_download_list.set_status(download, DOWNLOAD_INACTIVE);
		log_string("Downloading directory: " + url, LOG_DEBUG);
		return;
	}

	if(global_download_list.get_status(download) != DOWNLOAD_RUNNING) {
		return;
	}
	// Used later as temporary variable for all the <error>_wait config variables to save I/O
	long wait;
	global_download_list.set_error(download, (plugin_status)success);

	switch(success) {
		case PLUGIN_INVALID_HOST:
			global_download_list.set_status(download, DOWNLOAD_INACTIVE);
			log_string(std::string("Invalid host for download ID: ") + dlid_log, LOG_WARNING);
			return;
		break;
		case PLUGIN_AUTH_FAIL:
			global_download_list.set_status(download, DOWNLOAD_PENDING);
			wait = atol(global_config.get_cfg_value("auth_fail_wait").c_str());
			if(wait == 0) {
				global_download_list.set_status(download, DOWNLOAD_INACTIVE);
			} else {
				global_download_list.set_wait(download, wait);
				global_download_list.set_status(download, DOWNLOAD_PENDING);
			}
			log_string(std::string("Premium authentication failed for download ") + dlid_log, LOG_WARNING);
			return;
		case PLUGIN_SERVER_OVERLOADED:
			log_string(std::string("Server overloaded for download ID: ") + dlid_log, LOG_WARNING);
			global_download_list.set_status(download, DOWNLOAD_WAITING);
			return;
		case PLUGIN_LIMIT_REACHED:
			log_string(std::string("Download limit reached for download ID: ") + dlid_log + " (" + global_download_list.get_host(download) + ")", LOG_WARNING);
			global_download_list.set_status(download, DOWNLOAD_WAITING);
			return;
		// do the same for both
		case PLUGIN_CONNECTION_ERROR:
		case PLUGIN_CONNECTION_LOST:
			log_string(std::string("Plugin failed to connect for ID:") + dlid_log, LOG_WARNING);
			wait = atol(global_config.get_cfg_value("connection_lost_wait").c_str());
			if(wait == 0) {
				global_download_list.set_status(download, DOWNLOAD_INACTIVE);
			} else {
				global_download_list.set_status(download, DOWNLOAD_PENDING);
				global_download_list.set_wait(download, wait);
			}
			return;
		case PLUGIN_FILE_NOT_FOUND:
			log_string(std::string("File could not be found on the server for ID: ") + dlid_log, LOG_WARNING);
			global_download_list.set_status(download, DOWNLOAD_INACTIVE);
			return;
		case PLUGIN_ERROR:
			log_string("An error occured on download ID: " + dlid_log, LOG_WARNING);
			wait = atol(global_config.get_cfg_value("plugin_fail_wait").c_str());
			if(wait == 0) {
				global_download_list.set_status(download, DOWNLOAD_INACTIVE);
			} else {
				global_download_list.set_status(download, DOWNLOAD_PENDING);
				global_download_list.set_wait(download, wait);
			}
			return;
	}

	if(success == PLUGIN_SUCCESS) {
		log_string(std::string("Successfully parsed download ID: ") + dlid_log, LOG_DEBUG);
		if(global_download_list.get_wait(download) > 0) {

			if(global_download_list.get_status(download) != DOWNLOAD_DELETED) {
				log_string(std::string("Download ID: ") + dlid_log + " has to wait " +
						   int_to_string(global_download_list.get_wait(download)) + " seconds before downloading can start", LOG_DEBUG);
			} else {
				return;
			}

			global_download_list.wait(download);
		}

		if(global_download_list.get_need_stop(download)) return;

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
		   stat(output_filename.c_str(), &st) == 0 && (unsigned)st.st_size == global_download_list.get_downloaded_bytes(download) &&
		   global_download_list.get_can_resume(download)) {

			curl_easy_setopt(global_download_list.get_handle(download), CURLOPT_RESUME_FROM, global_download_list.get_downloaded_bytes(download));
			output_file.open(output_filename.c_str(), ios::out | ios::binary | ios::app);
			log_string(std::string("Download already started. Will try to continue to download ID: ") + dlid_log, LOG_DEBUG);
		} else {
			// Check if the file should be overwritten if it exists
			string overwrite(global_config.get_cfg_value("overwrite_files"));
			if(overwrite == "0" || overwrite == "false" || overwrite == "no") {
				if(stat(final_filename.c_str(), &st) == 0) {
					global_download_list.set_status(download, DOWNLOAD_INACTIVE);
					global_download_list.set_error(download, PLUGIN_WRITE_FILE_ERROR);
					return;

				}
			}
			output_file.open(output_filename.c_str(), ios::out | ios::binary | ios::trunc);
		}

		if(!output_file.good()) {
			log_string(std::string("Could not write to file: ") + output_filename, LOG_ERR);
			global_download_list.set_error(download, PLUGIN_WRITE_FILE_ERROR);
			wait = atol(global_config.get_cfg_value("write_error_wait").c_str());
			if(wait == 0) {
				global_download_list.set_status(download, DOWNLOAD_INACTIVE);
			} else {
				global_download_list.set_status(download, DOWNLOAD_PENDING);
				global_download_list.set_wait(download, wait);
			}
			return;
		}

		global_download_list.set_output_file(download, output_filename);

		if(plug_outp.download_url.empty()) {
			log_string(std::string("Empty URL for download ID: ") + dlid_log, LOG_ERR);
			global_download_list.set_error(download, PLUGIN_ERROR);
			wait = atol(global_config.get_cfg_value("plugin_fail_wait").c_str());
			if(wait == 0) {
				global_download_list.set_status(download, DOWNLOAD_INACTIVE);
			} else {
				global_download_list.set_status(download, DOWNLOAD_PENDING);
				global_download_list.set_wait(download, wait);
			}
			return;
		}

		CURL* handle_copy = global_download_list.get_handle(download);
		// set url
		curl_easy_setopt(handle_copy, CURLOPT_FOLLOWLOCATION, 1);
		curl_easy_setopt(handle_copy, CURLOPT_URL, plug_outp.download_url.c_str());
		// set file-writing function as callback
		curl_easy_setopt(handle_copy, CURLOPT_WRITEFUNCTION, write_file);
		std::string cache;
		std::pair<fstream*, std::string*> callback_opt_left(&output_file, &cache);
		std::pair<std::pair<fstream*, std::string*>, CURL*> callback_opt(callback_opt_left, handle_copy);
		curl_easy_setopt(handle_copy, CURLOPT_WRITEDATA, &callback_opt);
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

		log_string(std::string("Starting download ID: ") + dlid_log, LOG_DEBUG);
		success = curl_easy_perform(handle_copy);
		// because the callback only safes every half second, there is still an unsafed rest-data:
		output_file.write(cache.c_str(), cache.size());



		long http_code;
		curl_easy_getinfo(handle_copy, CURLINFO_RESPONSE_CODE, &http_code);
		output_file.close();

		switch(http_code) {
			case 401:
				log_string(std::string("Invalid premium account credentials, ID: ") + dlid_log, LOG_WARNING);
				wait = atol(global_config.get_cfg_value("auth_fail_wait").c_str());
				if(wait == 0) {
					global_download_list.set_status(download, DOWNLOAD_INACTIVE);
				} else {
					global_download_list.set_status(download, DOWNLOAD_PENDING);
					global_download_list.set_wait(download, wait);
				}
				return;
			case 404:
				log_string(std::string("File not found, ID: ") + dlid_log, LOG_WARNING);
				global_download_list.set_status(download, DOWNLOAD_INACTIVE);
				global_download_list.set_error(download, PLUGIN_FILE_NOT_FOUND);
				if(!output_filename.empty()) {
					remove(output_filename.c_str());
					global_download_list.set_output_file(download, "");
				}
				return;
			case 530:
				log_string("Invalid http authentication on download ID: " + dlid_log, LOG_WARNING);
				global_download_list.set_status(download, DOWNLOAD_INACTIVE);
				global_download_list.set_error(download, PLUGIN_AUTH_FAIL);
				remove(output_filename.c_str());
				global_download_list.set_output_file(download, "");
				return;
		}

		switch(success) {
			case CURLE_OK:
				log_string(std::string("Finished download ID: ") + dlid_log, LOG_DEBUG);
				global_download_list.set_status(download, DOWNLOAD_FINISHED);
				if(rename(output_filename.c_str(), final_filename.c_str()) != 0) {
					log_string(std::string("Unable to rename .part file. You can do so manually."), LOG_ERR);
				} else {
					global_download_list.set_output_file(download, final_filename);
				}
				global_download_list.post_process_download(download);
				return;
			case CURLE_OPERATION_TIMEDOUT:
			case CURLE_COULDNT_RESOLVE_HOST:
				log_string(std::string("Connection lost for download ID: ") + dlid_log, LOG_WARNING);
				global_download_list.set_error(download, PLUGIN_CONNECTION_LOST);
				wait = atol(global_config.get_cfg_value("connection_lost_wait").c_str());
				if(wait == 0) {
					global_download_list.set_status(download, DOWNLOAD_INACTIVE);
				} else {
					global_download_list.set_status(download, DOWNLOAD_PENDING);
					global_download_list.set_wait(download, wait);
				}
				return;
			case CURLE_ABORTED_BY_CALLBACK:
				log_string(std::string("Stopped download ID: ") + dlid_log, LOG_WARNING);
				return;
			case CURLE_BAD_DOWNLOAD_RESUME:
				// if download resumption fails, retry without resumption
				global_download_list.set_can_resume(download, false);
				global_download_list.set_status(download, DOWNLOAD_PENDING);
				log_string("Resuming download failed. retrying without resumption", LOG_WARNING);
				return;
			default:
				log_string(std::string("Download error for download ID: ") + dlid_log, LOG_WARNING);
				global_download_list.set_error(download, PLUGIN_CONNECTION_LOST);
				wait = atol(global_config.get_cfg_value("connection_lost_wait").c_str());
				if(wait == 0) {
					global_download_list.set_status(download, DOWNLOAD_INACTIVE);
				} else {
					global_download_list.set_status(download, DOWNLOAD_PENDING);
					global_download_list.set_wait(download, wait);
				}
				return;
		}
	} else {
		log_string("Plugin returned an invalid/unhandled status. Please report! (status returned: " + int_to_string(success) + ")", LOG_ERR);
		global_download_list.set_status(download, DOWNLOAD_PENDING);
		global_download_list.set_wait(download, 30);
		return;
	}
}

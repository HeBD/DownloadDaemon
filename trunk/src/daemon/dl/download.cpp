/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <dlfcn.h>
#include <dirent.h>

#include <config.h>

#include "download.h"
#include "download_container.h"
#ifndef IS_PLUGIN
#include <cfgfile/cfgfile.h>
#include "../tools/curl_callbacks.h"
#include "../tools/helperfunctions.h"
#include "../global.h"
#include "recursive_parser.h"
#include "../plugins/captcha.h"
#include "plugin_container.h"

#endif

#include <string>
#include <ctime>
#include <cstdlib>
#include <fstream>
#include <sstream>

using namespace std;

download::download(const std::string& dl_url)
	: url(dl_url), id(0), downloaded_bytes(0), size(0), wait_seconds(0), error(PLUGIN_SUCCESS),
	is_running(false), need_stop(false), status(DOWNLOAD_PENDING), speed(0), can_resume(true), handle(NULL), already_prechecked(false) {
	lock_guard<recursive_mutex> lock(mx);
	time_t rawtime;
	struct tm* timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	char timestr[20];
	strftime(timestr, 20, "%Y-%m-%d %X", timeinfo);
	add_date = timestr;
	//add_date.erase(add_date.length() - 1);
}

//download::download(std::string& serializedDL) {
//	from_serialized(serializedDL);
//}

#ifndef IS_PLUGIN
void download::from_serialized(std::string& serializedDL) {
	lock_guard<recursive_mutex> lock(mx);
	std::string current_entry;
	size_t curr_pos = 0;
	size_t entry_num = 0;
	while(serializedDL.find('|', curr_pos) != std::string::npos) {
		current_entry = serializedDL.substr(curr_pos, serializedDL.find('|', curr_pos) - curr_pos);
		curr_pos = serializedDL.find('|', curr_pos);
		if(serializedDL[curr_pos - 1] == '\\') {
			++curr_pos;
			continue;
		}

		switch(entry_num) {
			case 0:
				id = atoi(current_entry.c_str());
				break;
			case 1:
				add_date = current_entry;
				break;
			case 2:
				comment = current_entry;
				break;
			case 3:
				url = current_entry;
				break;
			case 4:
				if(current_entry == "1") {
					status = DOWNLOAD_PENDING;
				} else if(current_entry == "2") {
					status = DOWNLOAD_INACTIVE;
				} else if(current_entry == "3") {
					status = DOWNLOAD_FINISHED;
				} else {
					status = DOWNLOAD_PENDING;
				}
				break;
			case 5:
				downloaded_bytes = string_to_long(current_entry.c_str());
				break;
			case 6:
				size = string_to_long(current_entry.c_str());
				break;
			case 7:
				output_file = current_entry;
				break;
		}
		++curr_pos;
		++entry_num;
	}
	error = PLUGIN_SUCCESS;
	wait_seconds = 0;
	is_running = false;
	need_stop = false;
	speed = 0;
	can_resume = true;
	handle = NULL;
	already_prechecked = false;
}
#endif

download::download(const download& dl) {
	operator=(dl);
}

void download::operator=(const download& dl) {
	lock_guard<recursive_mutex> lock(mx);
	url = dl.url;
	comment = dl.comment;
	add_date = dl.add_date;
	id = dl.id;
	downloaded_bytes = dl.downloaded_bytes;
	size = dl.size;
	wait_seconds = dl.wait_seconds;
	error = dl.error;
	output_file = dl.output_file;
	status = dl.status;
	is_running = dl.is_running;
	speed = 0;
	can_resume = dl.can_resume;
	need_stop = dl.need_stop;
	handle = dl.handle;
	already_prechecked = dl.already_prechecked;
}

download::~download() {
	lock_guard<recursive_mutex> lock(mx);
	while(is_running) {
		need_stop = true;
		status = DOWNLOAD_DELETED;
		usleep(10);
	}
}

std::string download::serialize() {
	lock_guard<recursive_mutex> lock(mx);
	if(status == DOWNLOAD_DELETED) {
		return "";
	}

	string dl_bytes;
	string dl_size;
	stringstream conv1, conv2;
	#ifndef HAVE_UINT64_T
		conv1 << fixed << downloaded_bytes;
		string tmp = conv1.str();
		dl_bytes = tmp.substr(0, tmp.find("."));
		conv2 << fixed << size;
		tmp = conv2.str();
		dl_size = tmp.substr(0, tmp.find("."));
	#else
		conv1 << fixed << downloaded_bytes;
		dl_bytes = conv1.str();
		conv2 << fixed << size;
		dl_size = conv2.str();
	#endif
	while(dl_bytes.size() > 1 && dl_bytes[0] == '0') dl_bytes.erase(0, 1);
	while(dl_size.size() > 1 && dl_size[0] == '0') dl_size.erase(0, 1);

	stringstream ss;
	ss << id << '|' << add_date << '|' << comment << '|' << url << '|' << status << '|'
	<< dl_bytes << '|' << dl_size << '|' << output_file << "|\n";
	return ss.str();
}

std::string download::get_host(bool do_lock) {
	unique_lock<recursive_mutex> lock(mx, defer_lock);
	if(do_lock) {
		lock.lock();
	}
	std::string result = url;
	try {
		if(result.find("http://") == 0 || result.find("ftp://") == 0 || result.find("https://") == 0) {
			result = result.substr(result.find('/') + 2);
		}
		if(result.find("www.") == 0) {
			result = result.substr(4);
		}
		result = result.substr(0, result.find_first_of("/\\"));
	} catch(...) {
		return "";
	}
	return result;
}

const char* download::get_error_str() {
	lock_guard<recursive_mutex> lock(mx);
	switch(error) {
		case PLUGIN_SUCCESS:
			return "PLUGIN_SUCCESS";
		case PLUGIN_INVALID_HOST:
			return "Invalid hostname";
		case PLUGIN_CONNECTION_LOST:
			return "Connection lost";
		case PLUGIN_FILE_NOT_FOUND:
			return "File not found on server";
		case PLUGIN_WRITE_FILE_ERROR:
			return "Unable to write file";
		case PLUGIN_ERROR:
			return "Plugin error";
		case PLUGIN_LIMIT_REACHED:
			return "Download limit reached";
		case PLUGIN_CONNECTION_ERROR:
			return "Connection failed";
		case PLUGIN_SERVER_OVERLOADED:
			return "Server overloaded";
		case PLUGIN_AUTH_FAIL:
			return "Authentication failed";
	}
	return "Unknown plugin error - please report";
}

const char* download::get_status_str() {
	lock_guard<recursive_mutex> lock(mx);
	switch(status) {
		case DOWNLOAD_PENDING:
			return "DOWNLOAD_PENDING";
		case DOWNLOAD_INACTIVE:
			return "DOWNLOAD_INACTIVE";
		case DOWNLOAD_FINISHED:
			return "DOWNLOAD_FINISHED";
		case DOWNLOAD_RUNNING:
			return "DOWNLOAD_RUNNING";
		case DOWNLOAD_WAITING:
			return "DOWNLOAD_WAITING";
		case DOWNLOAD_RECONNECTING:
			return "DOWNLOAD_RECONNECTING";
		default:
			return "DOWNLOAD_DELETING";
	}
}


#ifndef IS_PLUGIN
plugin_output download::get_hostinfo() {
	lock_guard<recursive_mutex> lock(mx);
	return plugin_cache.get_info(get_host(), plugin_container::P_HOST);
}


void download::download_me() {
	unique_lock<recursive_mutex> lock(mx);
	need_stop = false;
	handle = curl_easy_init();
	dl_cb_info cb_info;
	curl_easy_setopt(handle, CURLOPT_LOW_SPEED_LIMIT, (long)10);
	curl_easy_setopt(handle, CURLOPT_LOW_SPEED_TIME, (long)20);
	curl_easy_setopt(handle, CURLOPT_CONNECTTIMEOUT, (long)30);
	curl_easy_setopt(handle, CURLOPT_NOSIGNAL, 1);

	curl_off_t dl_speed = global_config.get_int_value("max_dl_speed") * 1024;
	if(dl_speed > 0) {
		curl_easy_setopt(handle, CURLOPT_MAX_RECV_SPEED_LARGE, dl_speed);
	}
	lock.unlock();
	download_me_worker(cb_info);
	lock.lock();

	struct pstat st;
	if(pstat(output_file.c_str(), &st) == 0 && st.st_size == 0) {
		remove(output_file.c_str());
		output_file = "";
	}

	if(status == DOWNLOAD_RUNNING) {
		status = DOWNLOAD_PENDING;
                wait_seconds = 0;
	}

	if(status == DOWNLOAD_WAITING && error == PLUGIN_LIMIT_REACHED) {
		int ret = set_next_proxy();
		if(ret == 1) {
			status = DOWNLOAD_PENDING;
			error =  PLUGIN_SUCCESS;
			wait_seconds = 0;
		} else if(ret == 2 || ret == 3) {
			lock.unlock();
			if(global_download_list.reconnect_needed()) {
				try {
					thread t(bind(&package_container::do_reconnect, &global_download_list));
					t.detach();
				} catch(...) {
					log_string("Failed to start the reconnect-thread. There are probably too many running threads.", LOG_ERR);
				}
			}
			lock.lock();
		}
	} else if((error == PLUGIN_ERROR || error == PLUGIN_CONNECTION_ERROR || error == PLUGIN_CONNECTION_LOST || error == PLUGIN_INVALID_HOST)
			  && !global_config.get_bool_value("assume_proxys_online") && !global_config.get_cfg_value("proxy_list").empty()
			  && !proxy.empty() && set_next_proxy() == 1) {
		status = DOWNLOAD_PENDING;
		error = PLUGIN_SUCCESS;
		wait_seconds = 0;
	} else {
		proxy = "";
	}
	curl_easy_cleanup(handle);


	if(status == DOWNLOAD_FINISHED) {
		lock.unlock();
		if(global_download_list.package_finished(parent)) {
			// we don't have to do that in a seperate thread, we are in a non-blocking thread already, so unlock is enough
			global_download_list.extract_package(parent);
			//try {
			//	thread t(bind(&package_container::extract_package, &global_download_list, parent));
			//	t.detach();
			//} catch(...) {
			//	log_string("Failed to start extractor-thread. There are probably too many running threads.", LOG_ERR);
			//}
		}
		lock.lock();
	}

	need_stop = false;
	is_running = false;
	lock.unlock();
	try {
		thread t(bind(&package_container::start_next_downloadable, &global_download_list));
		t.detach();
	} catch(...) {
		log_string("Failed to spawn start_next_downloadable() thread. DownloadDaemon might hang now until"
				   " you trigger start_next_downloadable manually (change a config val). DownloadDaemon will try to recover automatically, but you never know.", LOG_ERR);
		global_download_list.start_next_downloadable();
	}
	global_download_list.dump_to_file();
}

void download::download_me_worker(dl_cb_info &cb_info) {
	plugin_output plug_outp;
	plugin_status success = prepare_download(plug_outp);

	unique_lock<recursive_mutex> lock(mx);

	std::string dlid_log = int_to_string(id);
	if(need_stop) {
		return;
	}

	std::string dl_url = plug_outp.download_url;
	if(dl_url[dl_url.size() - 1] == '/' && dl_url.find("ftp://") == 0 && global_config.get_bool_value("recursive_ftp_download")) {
		lock.unlock();
		recursive_parser parser(dl_url);
		parser.add_to_list(parent);
		lock.lock();
		if(status == DOWNLOAD_DELETED) {
			return;
		}
		status = DOWNLOAD_INACTIVE;
		log_string("Downloading directory: " + dl_url, LOG_DEBUG);
		return;
	}

	if(status != DOWNLOAD_RUNNING) {
		return;
	}
	// Used later as temporary variable for all the <error>_wait config variables to save I/O
	long wait_n;
	error = success;

	dlindex dl(make_pair<int, int>(parent, id));

	switch(success) {
		case PLUGIN_INVALID_HOST:
			status = DOWNLOAD_INACTIVE;
			log_string(std::string("Invalid host for download ID: ") + dlid_log, LOG_WARNING);
			return;
		break;
		case PLUGIN_AUTH_FAIL:
			status = DOWNLOAD_PENDING;
			wait_n = global_config.get_int_value("auth_fail_wait");
			if(wait_n == 0) {
				status = DOWNLOAD_INACTIVE;
			} else {
				wait_seconds = wait_n;
				status = DOWNLOAD_PENDING;
			}
			log_string(std::string("Premium authentication failed for download ") + dlid_log, LOG_WARNING);
			return;
		case PLUGIN_SERVER_OVERLOADED:
			log_string(std::string("Server overloaded for download ID: ") + dlid_log, LOG_WARNING);
			status = DOWNLOAD_WAITING;
			return;
		case PLUGIN_LIMIT_REACHED:
			log_string(std::string("Download limit reached for download ID: ") + dlid_log + " (" + get_host(false) + ")", LOG_WARNING);
			status = DOWNLOAD_WAITING;
			return;
		// do the same for both
		case PLUGIN_CONNECTION_ERROR:
		case PLUGIN_CONNECTION_LOST:
			log_string(std::string("Plugin failed to connect for ID:") + dlid_log, LOG_WARNING);
			wait_n = global_config.get_int_value("connection_lost_wait");
			if(wait_n == 0) {
				status = DOWNLOAD_INACTIVE;
			} else {
				status = DOWNLOAD_PENDING;
				wait_seconds = wait_n;
			}
			return;
		case PLUGIN_FILE_NOT_FOUND:
			log_string(std::string("File could not be found on the server for ID: ") + dlid_log, LOG_WARNING);
			status = DOWNLOAD_INACTIVE;
			return;
		case PLUGIN_ERROR:
			log_string("An error occured on download ID: " + dlid_log, LOG_WARNING);
			wait_n = global_config.get_int_value("plugin_fail_wait");
			if(wait_n == 0) {
				status = DOWNLOAD_INACTIVE;
			} else {
				status = DOWNLOAD_PENDING;
				wait_seconds = wait_n;
			}
			return;
		default: break;
	}

	if(success == PLUGIN_SUCCESS) {
		log_string(std::string("Successfully parsed download ID: ") + dlid_log, LOG_DEBUG);
		if(wait_seconds > 0) {
			lock.unlock();
			wait();
			lock.lock();
		}

		if(need_stop) return;
		cb_info.download_dir = global_config.get_cfg_value("download_folder");
		correct_path(cb_info.download_dir);
		if(global_config.get_bool_value("download_to_subdirs")) {
			lock.unlock();
			std::string dl_subfolder = global_download_list.get_pkg_name(parent);
			lock.lock();
			make_valid_filename(dl_subfolder);
			if(dl_subfolder.empty()) {
				lock.unlock();
				std::vector<int> dls = global_download_list.get_download_list(parent);

				if(dls.empty()) return;
				int first_id = dls[0];
				dl_subfolder = filename_from_url(global_download_list.get_url(make_pair<int, int>(parent, first_id)));
				lock.lock();
				if(dl_subfolder.find(".") != string::npos) {
					dl_subfolder = dl_subfolder.substr(0, dl_subfolder.rfind("."));
				}// else {
				//	dl_subfolder = "";
				//}
			}
			cb_info.download_dir += "/" + dl_subfolder;
		}

		if(!mkdir_recursive(cb_info.download_dir)) {
			log_string("Failed to create the download target directory for Download " + int_to_string(id) + ": " + cb_info.download_dir, LOG_ERR);
			status = DOWNLOAD_INACTIVE;
			error = PLUGIN_WRITE_FILE_ERROR;
			return;
		}
		if(plug_outp.download_filename != "") {
			cb_info.filename = plug_outp.download_filename;
			make_valid_filename(cb_info.filename);
		}
//		if(plug_outp.download_filename == "") {
//			if(plug_outp.download_url != "") {
//				std::string fn = filename_from_url(plug_outp.download_url);
//				output_filename = download_folder + '/';
//				if(fn.empty()) {
//					output_filename += "file";
//				} else {
//					output_filename += fn;
//				}
//			}
//		} else {
//			output_filename += download_folder;
//			make_valid_filename(plug_outp.download_filename);
//			output_filename += '/' + plug_outp.download_filename;
//		}

//		output_filename += ".part";

		struct pstat st;


		// Check if we can do a download resume or if we have to start from the beginning
		fstream output_file_s;
		cb_info.out_stream = &output_file_s;
		if(get_hostinfo().allows_resumption && global_config.get_bool_value("enable_resume") && !output_file.empty() &&
		   pstat(output_file.c_str(), &st) == 0 && fequal((double)st.st_size, (double)downloaded_bytes) && can_resume && error == PLUGIN_SUCCESS
		   && !fequal((double)downloaded_bytes, (double)size)) {

			cb_info.resume_from = downloaded_bytes;
			curl_easy_setopt(handle, CURLOPT_RESUME_FROM, (long)downloaded_bytes);

			//output_file_s.open(output_filename.c_str(), ios::out | ios::binary | ios::app);
			log_string(std::string("Download already started. Will try to continue to download ID: ") + dlid_log, LOG_DEBUG);
		} else {
			downloaded_bytes = 0;
		}




		if(plug_outp.download_url.empty()) {
			log_string(std::string("Empty URL for download ID: ") + dlid_log, LOG_ERR);
			error = PLUGIN_ERROR;
			wait_n = global_config.get_int_value("plugin_fail_wait");
			if(wait_n == 0) {
				status = DOWNLOAD_INACTIVE;
			} else {
				status = DOWNLOAD_PENDING;
				wait_seconds = wait_n;
			}
			return;
		}

		// set url
		curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1);
		curl_easy_setopt(handle, CURLOPT_URL, plug_outp.download_url.c_str());
		// set file-writing function as callback
		curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_file);
		// reserve a cache of 512 kb
		cb_info.cache.reserve(512000);
		//std::pair<fstream*, std::string*> callback_opt_left(&output_file_s, &cache);
		//std::pair<std::pair<fstream*, std::string*>, CURL*> callback_opt(callback_opt_left, handle);
		curl_easy_setopt(handle, CURLOPT_WRITEDATA, &cb_info);
		// show progress
		curl_easy_setopt(handle, CURLOPT_NOPROGRESS, 0);
		curl_easy_setopt(handle, CURLOPT_PROGRESSFUNCTION, report_progress);

		//std::pair<dlindex, filesize_t> progressdata(dl, resume_size);

		curl_easy_setopt(handle, CURLOPT_PROGRESSDATA, &cb_info);
		// set timeouts
		curl_easy_setopt(handle, CURLOPT_LOW_SPEED_LIMIT, (long)10);
		curl_easy_setopt(handle, CURLOPT_LOW_SPEED_TIME, (long)20);
		curl_off_t dl_speed = global_config.get_int_value("max_dl_speed") * 1024;
		if(dl_speed > 0) {
			curl_easy_setopt(handle, CURLOPT_MAX_RECV_SPEED_LARGE, dl_speed);
		}

		// set headers to parse content-disposition
		curl_easy_setopt(handle, CURLOPT_HEADERFUNCTION, parse_header);
		curl_easy_setopt(handle, CURLOPT_WRITEHEADER, &cb_info);

		cb_info.curl_handle = handle;
		cb_info.id = make_pair<int, int>(parent, id);

		log_string(std::string("Starting download ID: ") + dlid_log, LOG_DEBUG);
		lock.unlock();
		int curlsucces = curl_easy_perform(handle);
		lock.lock();

		//if(plug_outp.download_filename.empty() && !fn_from_header.empty()) {
		//	final_filename = final_filename.substr(0, final_filename.find_last_of("/\\"));
		//	final_filename += "/" + fn_from_header;
		//}

		// because the callback only safes every half second, there is still an unsafed rest-data:
		output_file_s.write(cb_info.cache.c_str(), cb_info.cache.size());
		downloaded_bytes += cb_info.cache.size();
		output_file_s.close();

		long http_code;
		curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &http_code);

		switch(http_code) {
			case 401:
				log_string(std::string("Invalid premium account credentials, ID: ") + dlid_log, LOG_WARNING);
				wait_n = global_config.get_int_value("auth_fail_wait");
				if(wait_n == 0) {
					status = DOWNLOAD_INACTIVE;
				} else {
					status = DOWNLOAD_PENDING;
					wait_seconds = wait_n;
				}
				return;
			case 404:
				log_string(std::string("File not found, ID: ") + dlid_log, LOG_WARNING);
				status = DOWNLOAD_INACTIVE;
				error = PLUGIN_FILE_NOT_FOUND;
				if(!output_file.empty()) {
					remove(output_file.c_str());
					output_file = "";
				}
				return;
			case 530:
				log_string("Invalid http authentication on download ID: " + dlid_log, LOG_WARNING);
				status = DOWNLOAD_INACTIVE;
				error = PLUGIN_AUTH_FAIL;
				remove(output_file.c_str());
				output_file = "";
				return;
		}
		string new_fn;

		//if(!global_config.get_bool_value("overwrite_files") && pstat.c_str(), &st) == 0) {
		//			status = DOWNLOAD_INACTIVE;
		//			error = PLUGIN_WRITE_FILE_ERROR;
		//			return;
		//	}
			//output_file_s.open(output_filename.c_str(), ios::out | ios::binary | ios::trunc);
		//}

		//cb_info.filename = output_filename;

		//if(!output_file_s.good()) {
		//	log_string(std::string("Could not write to file: ") + output_filename, LOG_ERR);
		//	error = PLUGIN_WRITE_FILE_ERROR;
		//	wait_n = global_config.get_int_value("write_error_wait");
		//	if(wait_n == 0) {
		//		status = DOWNLOAD_INACTIVE;
		//	} else {
		//		status = DOWNLOAD_PENDING;
		//		wait_seconds = wait_n;
		//	}
		//	return;
		//}

		switch(curlsucces) {
			case CURLE_OK:
				log_string(std::string("Finished download ID: ") + dlid_log, LOG_DEBUG);
				status = DOWNLOAD_FINISHED;
				new_fn = output_file.substr(0, output_file.rfind(".part"));
				if(pstat(new_fn.c_str(), &st) == 0 && !global_config.get_bool_value("overwrite_files")) {
					log_string("Could not rename " + output_file + ": It already exists and overwrite_files is set to false. You can rename the file manually.", LOG_ERR);
					status = DOWNLOAD_FINISHED;
					error = PLUGIN_SUCCESS;
					lock.unlock();
					post_process_download();
					return;
				} else if(pstat(new_fn.c_str(), &st) == 0) {
					remove(new_fn.c_str());
				}
				if(rename(output_file.c_str(), new_fn.c_str()) != 0) {
					log_string(std::string("Unable to rename .part file. You can do so manually."), LOG_ERR);
				} else {
					output_file = new_fn;
				}
				lock.unlock();
				post_process_download();
				return;

			case CURLE_WRITE_ERROR:
				error = cb_info.break_reason;
				wait_n = global_config.get_int_value("write_error_wait");
				if(wait_n == 0) {
					status = DOWNLOAD_INACTIVE;
				} else {
					status = DOWNLOAD_PENDING;
					wait_seconds = wait_n;
				}
				return;
			case CURLE_OPERATION_TIMEDOUT:
			case CURLE_COULDNT_RESOLVE_HOST:
				log_string(std::string("Connection lost for download ID: ") + dlid_log, LOG_WARNING);
				error = PLUGIN_CONNECTION_LOST;
				wait_n = global_config.get_int_value("connection_lost_wait");
				if(wait_n == 0) {
					status = DOWNLOAD_INACTIVE;
				} else {
					status = DOWNLOAD_PENDING;
					wait_seconds = wait_n;
				}
				return;
			case CURLE_ABORTED_BY_CALLBACK:
				log_string(std::string("Stopped download ID: ") + dlid_log, LOG_WARNING);
				return;
			case CURLE_BAD_DOWNLOAD_RESUME:
				// if download resumption fails, retry without resumption
				can_resume = false;
				status = DOWNLOAD_PENDING;
				log_string("Resuming download failed. retrying without resumption", LOG_WARNING);
				return;
			case CURLE_PARTIAL_FILE:
				log_string("The filesize reported by the server was unexpected", LOG_WARNING);
				status = DOWNLOAD_INACTIVE;
				error = PLUGIN_CONNECTION_ERROR;
				return;
			default:
				log_string(std::string("Download error for download ID: ") + dlid_log, LOG_WARNING);
				log_string(string("Unhandled curl error: ") + curl_easy_strerror((CURLcode)curlsucces) + string(". Please report this error."), LOG_ERR);
				error = PLUGIN_CONNECTION_LOST;
				wait_n = atol(global_config.get_cfg_value("connection_lost_wait").c_str());
				if(wait_n == 0) {
					status = DOWNLOAD_INACTIVE;
				} else {
					status = DOWNLOAD_PENDING;
					wait_seconds = wait_n;
				}
				return;
		}
	} else {
		log_string("Plugin returned an invalid/unhandled status. Please report! (status returned: " + int_to_string(success) + ")", LOG_ERR);
		status = DOWNLOAD_PENDING;
		wait_seconds = 30;
		return;
	}
}

void download::wait() {
	while(get_wait() > 0) {
		sleep(1);
	}
}

plugin_status download::prepare_download(plugin_output &poutp) {
	unique_lock<recursive_mutex> lock(mx);

	plugin_input pinp;
	string pluginfile(get_plugin_file());

	// If the generic plugin is used (no real host-plugin is found), we do "parsing" right here
	if(pluginfile.empty()) {
		log_string("No plugin found, using generic download", LOG_DEBUG);
		poutp.download_url = url;
		return PLUGIN_SUCCESS;
	}



	plugin_status (*plugin_exec_func)(download_container*, download*, int, plugin_input&, plugin_output&, int, std::string, std::string);
	bool ret = plugin_cache.load_function(get_host(), "plugin_exec_wrapper", plugin_exec_func);

	if(!ret) {
		return PLUGIN_ERROR;
	}

	pinp.premium_user = global_premium_config.get_cfg_value(get_host(false) + "_user");
	pinp.premium_password = global_premium_config.get_cfg_value(get_host(false) + "_password");
	pinp.url = url;
	trim_string(pinp.premium_user);
	trim_string(pinp.premium_password);

	// enable a proxy if neccessary
	string proxy_str = proxy;
	if(!proxy_str.empty()) {
		size_t n;
		std::string proxy_ipport;
		if((n = proxy_str.find("@")) != string::npos &&  proxy_str.size() > n + 1) {
			curl_easy_setopt(handle, CURLOPT_PROXYUSERPWD, proxy_str.substr(0, n).c_str());
			curl_easy_setopt(handle, CURLOPT_PROXY, proxy_str.substr(n + 1).c_str());
			log_string("Setting proxy: " + proxy_str.substr(n + 1) + " for " + url, LOG_DEBUG);
		} else {
			curl_easy_setopt(handle, CURLOPT_PROXY, proxy_str.c_str());
			log_string("Setting proxy: " + proxy_str + " for " + url, LOG_DEBUG);
		}
	}

	lock.unlock();

	plugin_status retval;
	try {
		retval = plugin_exec_func(global_download_list.get_listptr(parent), this, id, pinp, poutp, global_config.get_int_value("captcha_retrys"),
								  global_config.get_cfg_value("gocr_binary"), program_root);
	} catch(captcha_exception &e) {
		log_string("Failed to decrypt captcha. Giving up (" + pluginfile + ")", LOG_ERR);
		set_status(DOWNLOAD_INACTIVE);
		retval = PLUGIN_ERROR;
	} catch(...) {
		retval = PLUGIN_ERROR;
	}

	try {
		thread t(bind(&package_container::correct_invalid_ids, &global_download_list));
		t.detach();
	} catch (...) {
		log_string("Thread creation failed: package_container::correct_invalid_ids(). This may cause inconsitency! please report this!", LOG_ERR);
	}
	return retval;
}

void download::post_process_download() {
	unique_lock<recursive_mutex> lock(mx);
	plugin_input pinp;

	std::string host(get_host());

	// Load the plugin function needed
        void (*post_process_func)(download_container& dlc, download*, int id, plugin_input& pinp);
	bool ret = plugin_cache.load_function(host, "post_process_dl_init", post_process_func);

	if(!ret) {
		return;
	}

	pinp.premium_user = global_premium_config.get_cfg_value(get_host() + "_user");
	pinp.premium_password = global_premium_config.get_cfg_value(get_host() + "_password");
	pinp.url = url;
	trim_string(pinp.premium_user);
	trim_string(pinp.premium_password);
	is_running = true;
	lock.unlock();

	try {
                post_process_func(*global_download_list.get_listptr(parent), this, id, pinp);
	} catch(...) {}
        global_download_list.dump_to_file();
	lock.lock();
	is_running = false;

}

void download::preset_file_status() {
	unique_lock<recursive_mutex> lock(mx);
	already_prechecked = true;
	is_running = true;
	plugin_output outp;

	// If the generic plugin is used (no real host-plugin is found), we do "parsing" right here
	if(plugin_cache[get_host()] == 0) {
		log_string("No plugin found, using generic downloadtp preset the file-status", LOG_DEBUG);
		CURL* precheck_handle = curl_easy_init();
		curl_easy_setopt(handle, CURLOPT_LOW_SPEED_LIMIT, (long)10);
		curl_easy_setopt(handle, CURLOPT_LOW_SPEED_TIME, (long)20);
		curl_easy_setopt(handle, CURLOPT_CONNECTTIMEOUT, (long)30);
		curl_easy_setopt(handle, CURLOPT_NOSIGNAL, 1);
		curl_easy_setopt(precheck_handle, CURLOPT_URL, url.c_str());
		// set url
		curl_easy_setopt(precheck_handle, CURLOPT_FOLLOWLOCATION, 1);
		// set file-writing function as callback
		curl_easy_setopt(precheck_handle, CURLOPT_WRITEFUNCTION, pretend_write_file);
		// show progress
		curl_easy_setopt(precheck_handle, CURLOPT_NOPROGRESS, 0);

		filesize_t new_size;

		curl_easy_setopt(precheck_handle, CURLOPT_PROGRESSFUNCTION, get_size_progress_callback);
		curl_easy_setopt(precheck_handle, CURLOPT_PROGRESSDATA, &new_size);

		lock.unlock();
		int curlsucces = curl_easy_perform(precheck_handle);
		lock.lock();

		curl_easy_cleanup(precheck_handle);

		long http_code;
		curl_easy_getinfo(precheck_handle, CURLINFO_RESPONSE_CODE, &http_code);

		switch(http_code) {
			case 401:
			case 530:
				status = DOWNLOAD_INACTIVE;
				error = PLUGIN_AUTH_FAIL;
			break;
			case 404:
				status = DOWNLOAD_INACTIVE;
				error = PLUGIN_FILE_NOT_FOUND;
			break;
		}

		if(curlsucces == CURLE_ABORTED_BY_CALLBACK) {
			size = new_size;
		} else if(curlsucces != 0) {
			error = PLUGIN_FILE_NOT_FOUND;
			status = DOWNLOAD_INACTIVE;
		}
		is_running = false;
		return;
	}

	// Load the plugin function needed

	plugin_input pinp;

        bool (*file_status_func)(download_container& dlc, download*, int id, plugin_input& pinp, plugin_output &outp);
	bool ret = plugin_cache.load_function(get_host(), "get_file_status_init", file_status_func);

	if (!ret)  {
		is_running = false;
		return;
	}

	pinp.premium_user = global_premium_config.get_cfg_value(get_host() + "_user");
	pinp.premium_password = global_premium_config.get_cfg_value(get_host() + "_password");
	pinp.url = url;
	trim_string(pinp.premium_user);
	trim_string(pinp.premium_password);
	lock.unlock();

	bool is_host = true;
	try {
                is_host = file_status_func(*global_download_list.get_listptr(parent), this, id, pinp, outp);
	} catch(...) {}

	if(!is_host) {
		try {
			thread t(&download::download_me, this);
			t.detach();
		} catch(...) {
			log_string("Failed to start decrypter-thread. There are probably too many running threads.", LOG_ERR);
		}
	} else {
		lock.lock();
		size = outp.file_size;
		error = outp.file_online;
		if(error == PLUGIN_FILE_NOT_FOUND) {
			set_status(DOWNLOAD_INACTIVE);
		}
	}
	is_running = false;
}

std::string download::get_plugin_file() {
	string host = get_host(false);
	if(host == "") {
		return "";
	}
	for(size_t i = 0; i < host.size(); ++i) host[i] = tolower(host[i]);

	std::string plugindir = program_root + "/plugins/";
	correct_path(plugindir);
	plugindir += '/';

	DIR *dp;
	struct dirent *ep;
	dp = opendir(plugindir.c_str());
	if (dp == NULL) {
		log_string("Could not open plugin directory!", LOG_ERR);
		return "";
	}

	std::string result;
	std::string current;
	while ((ep = readdir(dp))) {
		if(ep->d_name[0] == '.') {
			continue;
		}
		current = ep->d_name;
		for(size_t i = 0; i < current.size(); ++i) current[i] = tolower(current[i]);
		if(current.find("lib") != 0) continue;
		current = current.substr(3);
		if(current.find(".so") == string::npos) continue;
		current = current.substr(0, current.find(".so"));
		if(host.find(current) != string::npos) {
			result = plugindir + ep->d_name;
			break;
		}
	}

	closedir(dp);

	return result;
}

int download::set_next_proxy() {
	//unique_lock<recursive_mutex> lock(mx);
	std::string last_proxy = proxy;
	std::string proxy_list = global_config.get_cfg_value("proxy_list");
	size_t n = 0;
	if(proxy_list.empty()) return 3;

	if(!last_proxy.empty() && (n = proxy_list.find(last_proxy)) == string::npos) {
		proxy = "";
		return 2;
	}

	n = proxy_list.find(";", n);
	if(!last_proxy.empty() && n == string::npos) {
		proxy = "";
		return 2;
	}

	if(last_proxy.empty()) {
		string tmp = proxy_list.substr(0, proxy_list.find(";"));
		trim_string(tmp);
		proxy = tmp;
		return 1;
	} else {
		if(proxy_list.size() > n + 2) {
			string tmp = proxy_list.substr(n + 1, proxy_list.find(";", n + 1) - (n + 1));
			trim_string(tmp);
			proxy = tmp;
			return 1;
		}
	}

	proxy = "";
	log_string("Invalid proxy-list syntax!", LOG_ERR);
	return 2;


}

#endif

bool operator<(const download& x, const download& y) {
	return x.get_id() < y.get_id();
}

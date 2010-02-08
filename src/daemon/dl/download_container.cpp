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

#include <sstream>
#include <string>
#include <cstdlib>

#include "download_container.h"
#include "download.h"
#include "../plugins/captcha.h"

#ifndef IS_PLUGIN
	#include "../../lib/cfgfile/cfgfile.h"
	#include "../tools/helperfunctions.h"
	#include "../reconnect/reconnect_parser.h"
	#include "../global.h"
#endif
using namespace std;

#ifndef IS_PLUGIN
download_container::download_container(const char* filename) {
	from_file(filename);
}

int download_container::from_file(const char* filename) {
	boost::mutex::scoped_lock lock(download_mutex);
	list_file = filename;
	ifstream dlist(filename);
	std::string line;
	while(getline(dlist, line)) {
		download dl;
		dl.from_serialized(line);
		download_list.push_back(dl);
	}

	if(!dlist.good()) {
		dlist.close();
		return LIST_PERMISSION;
	}
	dlist.close();
	return LIST_SUCCESS;
}
#endif

int download_container::total_downloads() {
	boost::mutex::scoped_lock lock(download_mutex);
	return download_list.size();
}

int download_container::add_download(download &dl) {
	boost::mutex::scoped_lock lock(download_mutex);
	dl.id = get_next_id();
	download_list.push_back(dl);

	dump_to_file();
	return LIST_SUCCESS;
}

int download_container::add_download(const std::string& url, const std::string& title) {
	download dl(url);
	dl.comment = title;
	return add_download(dl);
}

int download_container::move_up(int id) {
	boost::mutex::scoped_lock lock(download_mutex);
	download_container::iterator it;
	int location1 = 0, location2 = 0;
	for(it = download_list.begin(); it != download_list.end(); ++it) {
		if(it->id == id) {
			break;
		}
		++location1;
	}
	if(it == download_list.end() || it == download_list.begin() || location1 == 0) {
		return LIST_ID;
	}
	location2 = location1;

	download_container::iterator it2 = it;
	--it2;
	--location2;
	while(it2->get_status() == DOWNLOAD_DELETED) {
		--it2;
		--location2;
		if(location2 == -1) {
			return LIST_ID;
		}
	}
	download_container::iterator first, second;
	first = download_list.begin();
	second = download_list.begin();
	for(int i = 0; i < location1; ++i)
		++first;
	for(int i = 0; i < location2; ++i)
		++second;

	iter_swap(first, second);
	dump_to_file();
	return LIST_SUCCESS;
}

int download_container::move_down(int id) {
	boost::mutex::scoped_lock lock(download_mutex);
	download_container::iterator it;
	int location1 = 0, location2 = 0;
	for(it = download_list.begin(); it != download_list.end(); ++it) {
		if(it->id == id) {
			break;
		}
		++location1;
	}
	if(it == download_list.end() || it == --download_list.end()) {
		return LIST_ID;
	}
	location2 = location1;

	download_container::iterator it2 = it;
	++it2;
	++location2;
	while(it2->get_status() == DOWNLOAD_DELETED) {
		++it2;
		++location2;
		if(it2 == download_list.end()) {
			return LIST_ID;
		}
	}
	download_container::iterator first, second;
	first = download_list.begin();
	second = download_list.begin();
	for(int i = 0; i < location1; ++i)
		++first;
	for(int i = 0; i < location2; ++i)
		++second;

	iter_swap(first, second);
	dump_to_file();
	return LIST_SUCCESS;
}

int download_container::activate(int id) {
	boost::mutex::scoped_lock lock(download_mutex);
	download_container::iterator it = get_download_by_id(id);
	if(it == download_list.end()) {
		return LIST_ID;
	} else if(it->get_status() != DOWNLOAD_INACTIVE) {
		return LIST_PROPERTY;
	} else {
		set_dl_status(it, DOWNLOAD_PENDING);
		dump_to_file();
	}
	return LIST_SUCCESS;
}

int download_container::deactivate(int id) {
	boost::mutex::scoped_lock lock(download_mutex);
	download_container::iterator it = get_download_by_id(id);
	if(it == download_list.end()) {
		return LIST_ID;
	} else {
		if(it->get_status() == DOWNLOAD_INACTIVE) {
			return LIST_PROPERTY;
		}

		set_dl_status(it, DOWNLOAD_INACTIVE);

		it->wait_seconds = 0;
		dump_to_file();
	}
	return LIST_SUCCESS;
}

#ifndef IS_PLUGIN

int download_container::get_next_downloadable(bool do_lock) {
	boost::mutex::scoped_lock lock(download_mutex, boost::defer_lock);
	if(do_lock) {
		lock.lock();
	}

	if(is_reconnecting || download_list.empty()) {
		return LIST_ID;
	}
	int pending_downloads = 0;
	for(download_container::iterator it = download_list.begin(); it != download_list.end(); ++it) {
		if(it->get_status() == DOWNLOAD_PENDING) {
			++pending_downloads;
		}
	}
	if(pending_downloads == 0) {
		return LIST_ID;
	}

	download_container::iterator downloadable = download_list.end();
	if(running_downloads() >= atoi(global_config.get_cfg_value("simultaneous_downloads").c_str()) || global_config.get_cfg_value("downloading_active") == "0") {
		return LIST_ID;
	}

	// Checking if we are in download-time...
	std::string dl_start(global_config.get_cfg_value("download_timing_start"));
	if(global_config.get_cfg_value("download_timing_start").find(':') != std::string::npos && global_config.get_cfg_value("download_timing_end").find(':') != std::string::npos ) {
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
				return LIST_ID;
			}
			if(current_time->tm_hour == starthours) {
				if(current_time->tm_min < startminutes) {
					return LIST_ID;
				}
			} else if(current_time->tm_hour == endhours) {
				if(current_time->tm_min > endminutes) {
					return LIST_ID;
				}
			}
		// download window are just a few minutes
		} else if(starthours == endhours) {
			if(current_time->tm_min < startminutes || current_time->tm_min > endminutes) {
				return LIST_ID;
			}
		// no 0:00 shift
		} else if(starthours < endhours) {
			if(current_time->tm_hour < starthours || current_time->tm_hour > endhours) {
				return LIST_ID;
			}
			if(current_time->tm_hour == starthours) {
				if(current_time->tm_min < startminutes) {
					return LIST_ID;
				}
			} else if(current_time->tm_hour == endhours) {
				if(current_time->tm_min > endminutes) {
					return LIST_ID;
				}
			}
		}
	}

	for(iterator it = download_list.begin(); it != download_list.end(); ++it) {
		if(it->get_status() == DOWNLOAD_PENDING && it->wait_seconds == 0 && !it->is_running) {
			std::string current_host(it->get_host());
			bool can_attach = true;
			for(download_container::iterator it2 = download_list.begin(); it2 != download_list.end(); ++it2) {
				if(it->wait_seconds > 0 || it == it2) {
					continue;
				}
				if(it2->get_host() == current_host && (it2->get_status() == DOWNLOAD_RUNNING || it2->get_status() == DOWNLOAD_WAITING || it2->is_running)
				   && !it2->get_hostinfo().allows_multiple) {
					can_attach = false;
					break;
				}
			}
			if(can_attach) {
				return it->id;
			}
		}
	}
	return LIST_ID;
}

#endif // IS_PLUGIN

int download_container::set_string_property(int id, string_property prop, std::string value) {
	boost::mutex::scoped_lock lock(download_mutex);
	download_container::iterator dl = get_download_by_id(id);
	if(dl == download_list.end()) {
		return LIST_ID;
	}
	std::string old_val = value;
	switch(prop) {
		case DL_URL:
			dl->url = value;
			dump_to_file();
		break;
		case DL_COMMENT:
			dl->comment = value;
			dump_to_file();
		break;
		case DL_ADD_DATE:
			dl->add_date = value;
			dump_to_file();
		break;
		case DL_OUTPUT_FILE:
			dl->output_file = value;
			dump_to_file();
		break;
	}
	return LIST_SUCCESS;
}

int download_container::set_int_property(int id, property prop, double value) {
	boost::mutex::scoped_lock lock(download_mutex);
	download_container::iterator dl = get_download_by_id(id);
	if(dl == download_list.end()) {
		return LIST_ID;
	}

	switch(prop) {
		case  DL_ID:
			dl->id = value;
			dump_to_file();
		case DL_DOWNLOADED_BYTES:
			dl->downloaded_bytes = value;
			dump_to_file();
		break;
		case DL_SIZE:
			dl->size = value;
			dump_to_file();
		break;
		case DL_WAIT_SECONDS:
			dl->wait_seconds = value;
			dump_to_file();
		break;
		case DL_PLUGIN_STATUS:
			dl->error = plugin_status(value);
			dump_to_file();
		break;
		case DL_STATUS:
			set_dl_status(dl, download_status(value));
			dump_to_file();
		break;
		case DL_IS_RUNNING:
			dl->is_running = value;
			dump_to_file();
		break;
		case DL_SPEED:
			dl->speed = value;
			// No need to dump, nothing will change because it's not saved to the dlist file
		break;
		case DL_CAN_RESUME:
			dl->can_resume = value;
		break;
		default:
			return LIST_PROPERTY;
		break;
	}

	return LIST_SUCCESS;
}

int download_container::set_pointer_property(int id, pointer_property prop, void* value) {
	boost::mutex::scoped_lock lock(download_mutex);
	download_container::iterator dl = get_download_by_id(id);
	if(dl == download_list.end()) {
		return LIST_ID;
	}
	//switch(prop) {
	//	case DL_HANDLE:
	//		dl->cleanup_handle();
	//		dl->handle = value;
	//	break;
	//}
	return LIST_SUCCESS;
}

double download_container::get_int_property(int id, property prop) {
	boost::mutex::scoped_lock lock(download_mutex);
	download_container::iterator dl = get_download_by_id(id);
	if(dl == download_list.end()) {
		return LIST_ID;
	}
	switch(prop) {
		case  DL_ID:
			return dl->id;
		break;
		case DL_DOWNLOADED_BYTES:
			return dl->downloaded_bytes;
		break;
		case DL_SIZE:
			return dl->size;
		break;
		case DL_WAIT_SECONDS:
			return dl->wait_seconds;
		break;
		case DL_PLUGIN_STATUS:
			return dl->error;
		break;
		case DL_STATUS:
			return dl->get_status();
		break;
		case DL_IS_RUNNING:
			return dl->is_running;
		break;
		case DL_NEED_STOP:
			return dl->need_stop;
		break;
		case DL_SPEED:
			return dl->speed;
		break;
		case DL_CAN_RESUME:
			return dl->can_resume;
		break;
	}
	return -1;
}

std::string download_container::get_string_property(int id, string_property prop) {
	boost::mutex::scoped_lock lock(download_mutex);
	download_container::iterator dl = get_download_by_id(id);
	if(dl == download_list.end()) {
		stringstream ss;
		ss << "Invalid download ID: " << id;
		throw download_exception(ss.str().c_str());
	}
	switch(prop) {
		case DL_URL:
			return dl->url;
		break;
		case DL_COMMENT:
			return dl->comment;
		case DL_ADD_DATE:
			return dl->add_date;
		case DL_OUTPUT_FILE:
			return dl->output_file;
	}
	return "";
}

void* download_container::get_pointer_property(int id, pointer_property prop) {
	boost::mutex::scoped_lock lock(download_mutex);
	download_container::iterator dl = get_download_by_id(id);
	if(dl == download_list.end()) {
		return 0;
	}
	switch(prop) {
		case DL_HANDLE:
			return dl->handle;
	}

	return 0;
}

#ifndef IS_PLUGIN
bool download_container::reconnect_needed() {
	// only called in download::set_status which is only called by set_*_property, which locks.
	// therefore no lock needed
	std::string reconnect_policy;

	if(global_config.get_cfg_value("enable_reconnect") == "0") {
		return false;
	}

	reconnect_policy = global_router_config.get_cfg_value("reconnect_policy");
	if(reconnect_policy.empty()) {
		log_string("Reconnecting activated, but no reconnect policy specified", LOG_WARNING);
		return false;
	}



	bool need_reconnect = false;

	for(download_container::iterator it = download_list.begin(); it != download_list.end(); ++it) {
		if(it->get_status() == DOWNLOAD_WAITING && it->error == PLUGIN_LIMIT_REACHED) {
			need_reconnect = true;
		}
	}
	if(!need_reconnect) {
		return false;
	}

	// Always reconnect if needed
	if(reconnect_policy == "HARD") {
		return true;

	// Only reconnect if no non-continuable download is running
	} else if(reconnect_policy == "CONTINUE") {
		for(download_container::iterator it = download_list.begin(); it != download_list.end(); ++it) {
			if(it->get_status() == DOWNLOAD_RUNNING) {
				if(!it->get_hostinfo().allows_resumption) {
					return false;
				}
			}
		}
		return true;

	// Only reconnect if no download is running
	} else if(reconnect_policy == "SOFT") {
		for(download_container::iterator it = download_list.begin(); it != download_list.end(); ++it) {
			if(it->get_status() == DOWNLOAD_RUNNING) {
				return false;
			}
		}
		return true;

	// Only reconnect if no download is running and no download can be started
	} else if(reconnect_policy == "PUSSY") {
		for(download_container::iterator it = download_list.begin(); it != download_list.end(); ++it) {
			if(it->get_status() == DOWNLOAD_RUNNING) {
				return false;
			}
		}

		if(get_next_downloadable(false) != LIST_ID) {
			return false;
		}
		return true;

	} else {
		log_string("Invalid reconnect policy", LOG_ERR);
		return false;
	}
}

void download_container::do_reconnect() {
	if(is_reconnecting) {
		return;
	}
	is_reconnecting = true;
	boost::mutex::scoped_lock lock(download_mutex);
	std::string router_ip, router_username, router_password, reconnect_plugin;

	router_ip = global_router_config.get_cfg_value("router_ip");
	if(router_ip.empty()) {
		log_string("Reconnecting activated, but no router ip specified", LOG_WARNING);
		is_reconnecting = false;
		return;
	}

	router_username = global_router_config.get_cfg_value("router_username");
	router_password = global_router_config.get_cfg_value("router_password");
	reconnect_plugin = global_router_config.get_cfg_value("router_model");
	if(reconnect_plugin.empty()) {
		log_string("Reconnecting activated, but no router model specified", LOG_WARNING);
		is_reconnecting = false;
		return;
	}

	std::string reconnect_script = program_root + "/reconnect/" + reconnect_plugin;
	correct_path(reconnect_script);
	struct stat st;
	if(stat(reconnect_script.c_str(), &st) != 0) {
		log_string("Reconnect plugin for selected router model not found!", LOG_ERR);
		is_reconnecting = false;
		return;
	}

	reconnect rc(reconnect_script, router_ip, router_username, router_password);
	log_string("Reconnecting now!", LOG_WARNING);
	for(download_container::iterator it = download_list.begin(); it != download_list.end(); ++it) {
		if(it->get_status() == DOWNLOAD_WAITING) {
			set_dl_status(it, DOWNLOAD_RECONNECTING);
			it->wait_seconds = 0;
		}
	}
	lock.unlock();
	if(!rc.do_reconnect()) {
		log_string("Reconnect failed, change your settings.", LOG_ERR);
	}
	lock.lock();
	for(download_container::iterator it = download_list.begin(); it != download_list.end(); ++it) {
		if(it->get_status() == DOWNLOAD_WAITING || it->get_status() == DOWNLOAD_RECONNECTING) {
			set_dl_status(it, DOWNLOAD_PENDING);
			it->wait_seconds = 0;
		}
	}
	is_reconnecting = false;
	return;
}

int download_container::prepare_download(int dl, plugin_output &poutp) {
	boost::mutex::scoped_lock lock(download_mutex);
	plugin_input pinp;
	download_container::iterator dlit = get_download_by_id(dl);

	std::string host(dlit->get_host());
	std::string plugindir = program_root + "/plugins/";
	correct_path(plugindir);
	plugindir += '/';
	if(host == "") {
		return PLUGIN_INVALID_HOST;
	}

	std::string pluginfile(plugindir + "lib" + host + ".so");
	bool use_generic = false;

	struct stat st;
	if(stat(pluginfile.c_str(), &st) != 0) {
		use_generic = true;
	}

	// If the generic plugin is used (no real host-plugin is found), we do "parsing" right here
	if(use_generic) {
		log_string("No plugin found, using generic download", LOG_WARNING);
		poutp.download_url = dlit->url.c_str();

		return PLUGIN_SUCCESS;
	}

	// Load the plugin function needed
	void* handle = dlopen(pluginfile.c_str(), RTLD_LAZY);
	if (!handle) {
		log_string(std::string("Unable to open library file: ") + dlerror(), LOG_ERR);
		return PLUGIN_ERROR;
	}

	dlerror();	// Clear any existing error

	plugin_status (*plugin_exec_func)(download_container&, int, plugin_input&, plugin_output&, int, std::string);
	plugin_exec_func = (plugin_status (*)(download_container&, int, plugin_input&, plugin_output&, int, std::string))dlsym(handle, "plugin_exec_wrapper");

	char *error;
	if ((error = dlerror()) != NULL)  {
		log_string(std::string("Unable to execute plugin: ") + error, LOG_ERR);
		dlclose(handle);
		return PLUGIN_ERROR;
	}

	pinp.premium_user = global_premium_config.get_cfg_value(dlit->get_host() + "_user");
	pinp.premium_password = global_premium_config.get_cfg_value(dlit->get_host() + "_password");
	trim_string(pinp.premium_user);
	trim_string(pinp.premium_password);
	lock.unlock();
	plugin_mutex.lock();
	plugin_status retval;
	try {
		retval = plugin_exec_func(*this, dl, pinp, poutp, atoi(global_config.get_cfg_value("captcha_retrys").c_str()), global_config.get_cfg_value("gocr_binary"));
	} catch(captcha_exception &e) {
		log_string("Failed to decrypt captcha after " + global_config.get_cfg_value("captcha_retrys") + " retrys. Giving up (" + host + ")", LOG_ERR);
		set_dl_status(dlit, DOWNLOAD_INACTIVE);
		retval = PLUGIN_ERROR;
	} catch(...) {
		retval = PLUGIN_ERROR;
	}

	dlclose(handle);
	plugin_mutex.unlock();
	// lock, followed by scoped unlock. otherwise lock will be called 2 times
	lock.lock();
	dump_to_file();
	return retval;
}

plugin_output download_container::get_hostinfo(int dl) {
	boost::mutex::scoped_lock lock(download_mutex);
	download_container::iterator it = get_download_by_id(dl);
	return it->get_hostinfo();
}

#endif
std::string download_container::get_host(int dl) {
	boost::mutex::scoped_lock lock(download_mutex);
	download_container::iterator it = get_download_by_id(dl);
	return it->get_host();
}
#ifndef IS_PLUGIN

void download_container::decrease_waits() {
	boost::mutex::scoped_lock lock(download_mutex);
	for(download_container::iterator it = download_list.begin(); it != download_list.end(); ++it) {
		if(it->wait_seconds > 0) {
			--(it->wait_seconds);
			if(it->get_status() == DOWNLOAD_INACTIVE) it->wait_seconds = 0;
		} else if(it->wait_seconds == 0 && it->get_status() == DOWNLOAD_WAITING) {
			set_dl_status(it, DOWNLOAD_PENDING);
		}
	}
}

void download_container::purge_deleted() {
	boost::mutex::scoped_lock lock(download_mutex);
	for(download_container::iterator it = download_list.begin(); it != download_list.end(); ++it) {
		if(it->get_status() == DOWNLOAD_DELETED && it->is_running == false) {
			remove_download(it->id);
			it = download_list.begin();
		}
	}
}

std::string download_container::create_client_list() {
	boost::mutex::scoped_lock lock(download_mutex);

	std::stringstream ss;

	for(download_container::iterator it = download_list.begin(); it != download_list.end(); ++it) {
		if(it->get_status() == DOWNLOAD_DELETED) {
			continue;
		}
		ss << it->id << '|' << it->add_date << '|';
		std::string comment = it->comment;
		replace_all(comment, "|", "\\|");
		ss << comment << '|' << it->url << '|' << it->get_status_str() << '|' << it->downloaded_bytes << '|' << it->size
		   << '|' << it->wait_seconds << '|' << it->get_error_str() << '|' << it->speed << '\n';
	}
	return ss.str();
}

#endif

int download_container::get_next_id() {
	int max_id = -1;
	for(download_container::iterator it = download_list.begin(); it != download_list.end(); ++it) {
		if(it->id > max_id) {
			max_id = it->id;
		}
	}
	return ++max_id;
}

int download_container::stop_download(int id) {
    boost::mutex::scoped_lock lock(download_mutex);
	download_container::iterator it = get_download_by_id(id);
	if(it == download_list.end() || it->get_status() == DOWNLOAD_DELETED) {
		return LIST_ID;
	} else {
		if(it->get_status() != DOWNLOAD_INACTIVE) set_dl_status(it, DOWNLOAD_PENDING);
		it->need_stop = true;
		dump_to_file();
		return LIST_SUCCESS;
	}
}

download_container::iterator download_container::get_download_by_id(int id) {
	for(download_container::iterator it = download_list.begin(); it != download_list.end(); ++it) {
		if(it->id == id) {
			return it;
		}
	}
	return download_list.end();
}

bool download_container::dump_to_file() {
	// in-memory download container
	if(list_file == "") {
		return true;
	}

	fstream dlfile(list_file.c_str(), ios::trunc | ios::out);
	if(!dlfile.good()) {
		return false;
	}
	for(download_container::iterator it = download_list.begin(); it != download_list.end(); ++it) {
		dlfile << it->serialize();
	}
	dlfile.close();

	return true;
}


int download_container::running_downloads() {
	int running_dls = 0;
	for(download_container::iterator it = download_list.begin(); it != download_list.end(); ++it) {
		if(it->is_running) {
			++running_dls;
		}
	}
	return running_dls;
}

int download_container::remove_download(int id) {
	download_container::iterator it = get_download_by_id(id);
	if(it == download_list.end()) {
		return LIST_ID;
	}
	download tmpdl = *it;
	download_list.erase(it);
	dump_to_file();
	return LIST_SUCCESS;
}

bool download_container::url_is_in_list(std::string url) {
	boost::mutex::scoped_lock lock(download_mutex);
	for(download_container::iterator it = download_list.begin(); it != download_list.end(); ++it) {
		if(url == it->url) {
			return true;
		}
	}
	return false;
}

#ifndef IS_PLUGIN
void download_container::init_handle(int id) {
	boost::mutex::scoped_lock lock(download_mutex);
	download_container::iterator it = get_download_by_id(id);
	it->handle = curl_easy_init();
	it->is_init = true;
}

void download_container::cleanup_handle(int id) {
	boost::mutex::scoped_lock lock(download_mutex);
	download_container::iterator it = get_download_by_id(id);
	if(it->is_init)
		curl_easy_cleanup(it->handle);
    it->handle = NULL;
	it->is_init = false;
}
#endif

int download_container::get_list_position(int id) {
	boost::mutex::scoped_lock lock(download_mutex);
	int curr_pos = 0;
	for(download_container::iterator it = download_list.begin(); it != download_list.end(); ++it) {
		if(id == it->id) {
			return curr_pos;
		}
		++curr_pos;
	}
	return LIST_ID;
}

void download_container::insert_downloads(int pos, download_container &dl) {
	boost::mutex::scoped_lock lock(download_mutex);
	download_container::iterator insert_it = download_list.begin();
	// set input iterator to correct position
	for(int i = 0; i < pos; ++i) {
		++insert_it;
	}

	for(download_container::iterator it = dl.download_list.begin(); it != dl.download_list.end(); ++it) {
		insert_it = download_list.insert(insert_it, *it);
		insert_it->id = get_next_id();
		++insert_it;
	}

	dump_to_file();
}

#ifndef IS_PLUGIN
void download_container::post_process_download(int id) {
	boost::mutex::scoped_lock lock(download_mutex);
	download_container::iterator it = get_download_by_id(id);
	plugin_input pinp;

	std::string host(it->get_host());
	std::string plugindir = program_root + "/plugins/";
	correct_path(plugindir);
	plugindir += '/';
	if(host == "") {
		return;
	}

	struct stat st;
	if(stat(plugindir.c_str(), &st) != 0) {
		return;
	}

	std::string pluginfile(plugindir + "lib" + host + ".so");
	if(stat(pluginfile.c_str(), &st) != 0) {
		return;
	}

	// Load the plugin function needed
	void* handle = dlopen(pluginfile.c_str(), RTLD_LAZY);
	if (!handle) {
		log_string(std::string("Unable to open library file: ") + dlerror(), LOG_ERR);
		return;
	}

	dlerror();	// Clear any existing error

	void (*post_process_func)(download_container& dlc, int id, plugin_input& pinp);
	post_process_func = (void (*)(download_container& dlc, int id, plugin_input& pinp))dlsym(handle, "post_process_dl_init");

	char *error;
	if ((error = dlerror()) != NULL)  {
	    dlclose(handle);
		return;
	}

	pinp.premium_user = global_premium_config.get_cfg_value(it->get_host() + "_user");
	pinp.premium_password = global_premium_config.get_cfg_value(it->get_host() + "_password");
	trim_string(pinp.premium_user);
	trim_string(pinp.premium_password);
	lock.unlock();
	plugin_mutex.lock();
	try {
		post_process_func(*this, id, pinp);
	} catch(...) {}

	dlclose(handle);
	plugin_mutex.unlock();
	// lock, followed by scoped unlock. otherwise lock will be called 2 times
	lock.lock();
	dump_to_file();
}
#endif

void download_container::set_dl_status(download_container::iterator it, download_status st) {
	if(it->status == DOWNLOAD_DELETED) {
		return;
	} else {
		if(st == DOWNLOAD_INACTIVE || st == DOWNLOAD_DELETED) {
			it->need_stop = true;
		}
		it->status = st;
	}

	if(st == DOWNLOAD_INACTIVE || st == DOWNLOAD_PENDING || st == DOWNLOAD_DELETED) {
		it->wait_seconds = 0;
	}
	#ifndef IS_PLUGIN
	if(global_download_list.reconnect_needed()) {
		boost::thread t(boost::bind(&download_container::do_reconnect, this));
	}
	#endif

}

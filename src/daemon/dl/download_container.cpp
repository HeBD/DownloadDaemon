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
	#include <cfgfile/cfgfile.h>
	#include "../tools/helperfunctions.h"
	#include "../reconnect/reconnect_parser.h"
	#include "../global.h"
#endif

#ifndef HAVE_UINT64_T
	#define uint64_t double
#endif
#include <iostream>
using namespace std;

download_container::download_container(int id, std::string container_name) : container_id(id), name(container_name) {
}

download_container::download_container(const download_container &cnt) : download_list(cnt.download_list), container_id(cnt.container_id), name(cnt.name) {}

download_container::~download_container() {
	lock_guard<mutex> lock(download_mutex);
	#ifndef IS_PLUGIN
	if(!download_list.empty()) {
		log_string("Memory leak detected: container deleted, but there are still Downloads in the list. Please report!", LOG_ERR);
		for(download_container::iterator it = download_list.begin(); it != download_list.end(); ++it) {
			// this is just the last way out to prevent memory-leaking.. usually this should never happen
			delete *it;
		}
	}
	#else
		cerr << "Memory leak detected: container deleted, but there are still Downloads in the list. Please report! (happend in a plugin-call)" << endl;
	#endif
	//lock_guard<mutex> lock(download_mutex);
	// download-containers may only be destroyed after the ownage of the pointers is moved to another object
}


int download_container::total_downloads() {
	lock_guard<mutex> lock(download_mutex);
	return download_list.size();
}

int download_container::add_download(download *dl, int dl_id) {
	lock_guard<mutex> lock(download_mutex);
	dl->set_id(dl_id);
	dl->set_parent(container_id);
	download_list.push_back(dl);
	return LIST_SUCCESS;
}

#ifdef IS_PLUGIN
int download_container::add_download(const std::string& url, const std::string& title) {
	download* dl = new download(url);
	dl->set_title(title);
	dl->set_parent(container_id);
	int ret = add_download(dl, -1);
	if(ret != LIST_SUCCESS) delete dl;
	return ret;
}
#endif

int download_container::move_up(int id) {
	lock_guard<mutex> lock(download_mutex);
	download_container::iterator it;
	int location1 = 0, location2 = 0;
	for(it = download_list.begin(); it != download_list.end(); ++it) {
		if((*it)->get_id() == id) {
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
	while((*it2)->get_status() == DOWNLOAD_DELETED) {
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
	return LIST_SUCCESS;
}

int download_container::move_down(int id) {
	lock_guard<mutex> lock(download_mutex);
	download_container::iterator it;
	int location1 = 0, location2 = 0;
	for(it = download_list.begin(); it != download_list.end(); ++it) {
		if((*it)->get_id() == id) {
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
	while((*it2)->get_status() == DOWNLOAD_DELETED) {
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
	return LIST_SUCCESS;
}

int download_container::activate(int id) {
	lock_guard<mutex> lock(download_mutex);
	download_container::iterator it = get_download_by_id(id);
	if(it == download_list.end()) {
		return LIST_ID;
	} else if((*it)->get_status() != DOWNLOAD_INACTIVE) {
		return LIST_PROPERTY;
	} else {
		(*it)->set_status(DOWNLOAD_PENDING);
	}
	return LIST_SUCCESS;
}

int download_container::deactivate(int id) {
	lock_guard<mutex> lock(download_mutex);
	download_container::iterator it = get_download_by_id(id);
	if(it == download_list.end()) {
		return LIST_ID;
	} else {
		if((*it)->get_status() == DOWNLOAD_INACTIVE) {
			return LIST_PROPERTY;
		}

		(*it)->set_status(DOWNLOAD_INACTIVE);

		(*it)->set_wait(0);
	}
	return LIST_SUCCESS;
}

#ifndef IS_PLUGIN

int download_container::get_next_downloadable(bool do_lock) {
	unique_lock<mutex> lock(download_mutex, defer_lock);
	if(do_lock) {
		lock.lock();
	}

	if(download_list.empty()) {
		return LIST_ID;
	}
	int pending_downloads = 0;
	for(download_container::iterator it = download_list.begin(); it != download_list.end(); ++it) {
		if((*it)->get_status() == DOWNLOAD_PENDING) {
			++pending_downloads;
		}
	}
	if(pending_downloads == 0) {
		return LIST_ID;
	}

	download_container::iterator downloadable = download_list.end();
	if(running_downloads() >= global_config.get_int_value("simultaneous_downloads") || !global_config.get_bool_value("downloading_active")) {
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
		if((*it)->get_status() == DOWNLOAD_PENDING && (*it)->get_wait() == 0 && !(*it)->get_running() && (*it)->get_id() >= 0) {
			std::string current_host((*it)->get_host());
			bool can_attach = true;
			for(download_container::iterator it2 = download_list.begin(); it2 != download_list.end(); ++it2) {
				if((*it)->get_wait() > 0 || it == it2) {
					continue;
				}
				if((*it2)->get_host() == current_host && ((*it2)->get_status() == DOWNLOAD_RUNNING || (*it2)->get_status() == DOWNLOAD_WAITING || (*it2)->get_running())
				   && !(*it2)->get_hostinfo().allows_multiple) {
					can_attach = false;
					break;
				}
			}
			if(can_attach) {
				return (*it)->get_id();
			}
		}
	}
	return LIST_ID;
}

#endif // IS_PLUGIN


void download_container::set_url(int id, std::string url) {
	lock_guard<mutex> lock(download_mutex);
	download_container::iterator dl = get_download_by_id(id);
	if(dl == download_list.end()) return;
	(*dl)->set_url(url);
}

void download_container::set_title(int id, std::string title) {
	lock_guard<mutex> lock(download_mutex);
	download_container::iterator dl = get_download_by_id(id);
	if(dl == download_list.end()) return;
	(*dl)->set_title(title);
}

void download_container::set_add_date(int id, std::string add_date) {
	lock_guard<mutex> lock(download_mutex);
	download_container::iterator dl = get_download_by_id(id);
	if(dl == download_list.end()) return;
	(*dl)->set_add_date(add_date);
}

void download_container::set_downloaded_bytes(int id, uint64_t bytes) {
	lock_guard<mutex> lock(download_mutex);
	download_container::iterator dl = get_download_by_id(id);
	if(dl == download_list.end()) return;
	(*dl)->set_downloaded_bytes(bytes);
}

void download_container::set_size(int id, uint64_t size) {
	lock_guard<mutex> lock(download_mutex);
	download_container::iterator dl = get_download_by_id(id);
	if(dl == download_list.end()) return;
	(*dl)->set_size(size);
}

void download_container::set_wait(int id, int seconds) {
	lock_guard<mutex> lock(download_mutex);
	download_container::iterator dl = get_download_by_id(id);
	if(dl == download_list.end()) return;
	(*dl)->set_wait(seconds);
}

void download_container::set_error(int id, plugin_status error) {
	lock_guard<mutex> lock(download_mutex);
	download_container::iterator dl = get_download_by_id(id);
	if(dl == download_list.end()) return;
	(*dl)->set_error(error);
}

void download_container::set_output_file(int id, std::string output_file) {
	lock_guard<mutex> lock(download_mutex);
	download_container::iterator dl = get_download_by_id(id);
	if(dl == download_list.end()) return;
	(*dl)->set_filename(output_file);
}

void download_container::set_running(int id, bool running) {
	lock_guard<mutex> lock(download_mutex);
	download_container::iterator dl = get_download_by_id(id);
	if(dl == download_list.end()) return;
	(*dl)->set_running(running);
}

void download_container::set_need_stop(int id, bool need_stop) {
	lock_guard<mutex> lock(download_mutex);
	download_container::iterator dl = get_download_by_id(id);
	if(dl == download_list.end()) return;
	(*dl)->set_need_stop(need_stop);
}

void download_container::set_status(int id, download_status status) {
	lock_guard<mutex> lock(download_mutex);
	download_container::iterator dl = get_download_by_id(id);
	if(dl == download_list.end()) return;
	(*dl)->set_status(status);
}

void download_container::set_speed(int id, int speed) {
	lock_guard<mutex> lock(download_mutex);
	download_container::iterator dl = get_download_by_id(id);
	if(dl == download_list.end()) return;
	(*dl)->set_speed(speed);
}

void download_container::set_can_resume(int id, bool can_resume) {
	lock_guard<mutex> lock(download_mutex);
	download_container::iterator dl = get_download_by_id(id);
	if(dl == download_list.end()) return;
	(*dl)->set_resumable(can_resume);
}


void download_container::set_proxy(int id, std::string proxy) {
	lock_guard<mutex> lock(download_mutex);
	download_container::iterator dl = get_download_by_id(id);
	if(dl == download_list.end()) return;
	(*dl)->set_proxy(proxy);
}

std::string download_container::get_proxy(int id) {
	lock_guard<mutex> lock(download_mutex);
	download_container::iterator dl = get_download_by_id(id);
	if(dl == download_list.end()) return false;
	return (*dl)->get_proxy();
}

bool download_container::get_can_resume(int id) {
	lock_guard<mutex> lock(download_mutex);
	download_container::iterator dl = get_download_by_id(id);
	if(dl == download_list.end()) return false;
	return (*dl)->get_resumable();
}

download_status download_container::get_status(int id) {
	lock_guard<mutex> lock(download_mutex);
	download_container::iterator dl = get_download_by_id(id);
	if(dl == download_list.end()) return DOWNLOAD_DELETED;
	return (*dl)->get_status();
}

bool download_container::get_need_stop(int id) {
	lock_guard<mutex> lock(download_mutex);
	download_container::iterator dl = get_download_by_id(id);
	if(dl == download_list.end()) return true;
	return (*dl)->get_need_stop();
}

bool download_container::get_running(int id) {
	lock_guard<mutex> lock(download_mutex);
	download_container::iterator dl = get_download_by_id(id);
	if(dl == download_list.end()) return false;
	return (*dl)->get_running();
}

std::string download_container::get_output_file(int id) {
	lock_guard<mutex> lock(download_mutex);
	download_container::iterator dl = get_download_by_id(id);
	if(dl == download_list.end()) return "";
	return (*dl)->get_filename();
}

plugin_status download_container::get_error(int id) {
	lock_guard<mutex> lock(download_mutex);
	download_container::iterator dl = get_download_by_id(id);
	if(dl == download_list.end()) return PLUGIN_ERROR;
	return (*dl)->get_error();
}

int download_container::get_wait(int id) {
	lock_guard<mutex> lock(download_mutex);
	download_container::iterator dl = get_download_by_id(id);
	if(dl == download_list.end()) return 0;
	return (*dl)->get_wait();
}

std::string download_container::get_add_date(int id) {
	lock_guard<mutex> lock(download_mutex);
	download_container::iterator dl = get_download_by_id(id);
	if(dl == download_list.end()) return "";
	return (*dl)->get_add_date();
}

std::string download_container::get_title(int id) {
	lock_guard<mutex> lock(download_mutex);
	download_container::iterator dl = get_download_by_id(id);
	if(dl == download_list.end()) return "";
	return (*dl)->get_title();
}

std::string download_container::get_url(int id) {
	lock_guard<mutex> lock(download_mutex);
	download_container::iterator dl = get_download_by_id(id);
	if(dl == download_list.end()) return "";
	return (*dl)->get_url();
}

uint64_t download_container::get_downloaded_bytes(int id) {
	lock_guard<mutex> lock(download_mutex);
	download_container::iterator dl = get_download_by_id(id);
	if(dl == download_list.end()) return 0;
	return (*dl)->get_downloaded_bytes();
}

uint64_t download_container::get_size(int id) {
	lock_guard<mutex> lock(download_mutex);
	download_container::iterator dl = get_download_by_id(id);
	if(dl == download_list.end()) return 0;
	return (*dl)->get_size();
}

int download_container::get_speed(int id) {
	lock_guard<mutex> lock(download_mutex);
	download_container::iterator dl = get_download_by_id(id);
	if(dl == download_list.end()) return 0;
	return (*dl)->get_speed();
}

CURL* download_container::get_handle(int id) {
	lock_guard<mutex> lock(download_mutex);
	download_container::iterator dl = get_download_by_id(id);
	if(dl == download_list.end()) return NULL;
	return (*dl)->get_handle();
}

std::string download_container::get_host(int dl) {
	lock_guard<mutex> lock(download_mutex);
	download_container::iterator it = get_download_by_id(dl);
	return (*it)->get_host();
}

void download_container::set_password(const std::string& passwd) {
	lock_guard<mutex> lock(download_mutex);
	password = passwd;
}

std::string download_container::get_password() {
	lock_guard<mutex> lock(download_mutex);
	return password;
}

void download_container::set_pkg_name(const std::string& pkg_name) {
	lock_guard<mutex> lock(download_mutex);
	name = pkg_name;
}

std::string download_container::get_pkg_name() {
	lock_guard<mutex> lock(download_mutex);
	return name;
}

void download_container::decrease_waits() {
	lock_guard<mutex> lock(download_mutex);
	for(download_container::iterator it = download_list.begin(); it != download_list.end(); ++it) {
		size_t wait = (*it)->get_wait();
		if(wait > 0) {
			(*it)->set_wait(wait - 1);
			if((*it)->get_status() == DOWNLOAD_INACTIVE) (*it)->set_wait(0);
		} else if((*it)->get_status() == DOWNLOAD_WAITING) {
			(*it)->set_status(DOWNLOAD_PENDING);
		}
	}
}

void download_container::purge_deleted() {
	lock_guard<mutex> lock(download_mutex);
	for(download_container::iterator it = download_list.begin(); it != download_list.end(); ++it) {
		if((*it)->get_status() == DOWNLOAD_DELETED && (*it)->get_running() == false) {
			remove_download((*it)->get_id());
			it = download_list.begin();
		}
	}
}

std::string download_container::create_client_list() {
	lock_guard<mutex> lock(download_mutex);

	std::stringstream ss;

	for(download_container::iterator it = download_list.begin(); it != download_list.end(); ++it) {
		if((*it)->get_status() == DOWNLOAD_DELETED || (*it)->get_id() < 0) {
			continue;
		}
		ss << (*it)->get_id() << '|' << (*it)->get_add_date() << '|';
		std::string comment = (*it)->get_title();
		ss << comment << '|' << (*it)->get_url() << '|' << (*it)->get_status_str() << '|' << (*it)->get_downloaded_bytes() << '|' << (*it)->get_size()
		   << '|' << (*it)->get_wait() << '|' << (*it)->get_error_str() << '|' << (*it)->get_speed() << '\n';
	}
	return ss.str();
}

int download_container::stop_download(int id) {
	lock_guard<mutex> lock(download_mutex);
	download_container::iterator it = get_download_by_id(id);
	if(it == download_list.end() || (*it)->get_status() == DOWNLOAD_DELETED) {
		return LIST_ID;
	} else {
		if((*it)->get_status() != DOWNLOAD_INACTIVE) (*it)->set_status(DOWNLOAD_PENDING);
		(*it)->set_need_stop(true);
		return LIST_SUCCESS;
	}
}

download_container::iterator download_container::get_download_by_id(int id) {
	for(download_container::iterator it = download_list.begin(); it != download_list.end(); ++it) {
		if((*it)->get_id() == id) {
			return it;
		}
	}
	return download_list.end();
}

int download_container::running_downloads() {
	int running_dls = 0;
	for(download_container::iterator it = download_list.begin(); it != download_list.end(); ++it) {
		if((*it)->get_running()) {
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
	delete *it;
	download_list.erase(it);
	return LIST_SUCCESS;
}

bool download_container::url_is_in_list(std::string url) {
	lock_guard<mutex> lock(download_mutex);
	for(download_container::iterator it = download_list.begin(); it != download_list.end(); ++it) {
		if(url == (*it)->get_url()) {
			return true;
		}
	}
	return false;
}

int download_container::get_list_position(int id) {
	lock_guard<mutex> lock(download_mutex);
	int curr_pos = 0;
	for(download_container::iterator it = download_list.begin(); it != download_list.end(); ++it) {
		if(id == (*it)->get_id()) {
			return curr_pos;
		}
		++curr_pos;
	}
	return LIST_ID;
}

void download_container::insert_downloads(int pos, download_container &dl) {
	lock_guard<mutex> lock(download_mutex);
	download_container::iterator insert_it = download_list.begin();
	// set input iterator to correct position
	for(int i = 0; i < pos; ++i) {
		++insert_it;
	}
	dl.download_mutex.lock();
	for(download_container::iterator it = dl.download_list.begin(); it != dl.download_list.end(); ++it) {
		(*it)->set_id(-1); // -1 means that they have to be assigned later
		(*it)->set_parent(container_id);
		insert_it = download_list.insert(insert_it, *it);
		++insert_it;
	}
	dl.download_mutex.unlock();

}

#ifndef IS_PLUGIN
void download_container::do_download(int id) {
	lock_guard<mutex> lock(download_mutex);
	download_container::iterator it = get_download_by_id(id);
	if(it == download_list.end()) return;
	(*it)->set_running(true);
	(*it)->set_status(DOWNLOAD_RUNNING);
	thread t(bind(&download::download_me, *it));
	t.detach();
}

#endif

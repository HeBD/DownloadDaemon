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
#include "package_extractor.h"


#ifndef IS_PLUGIN
	#include <cfgfile/cfgfile.h>
	#include "../tools/helperfunctions.h"
	#include "../reconnect/reconnect_parser.h"
	#include "../global.h"
	#include "../mgmt/global_management.h"
#endif

#include <iostream>
using namespace std;

download_container::download_container(int id, std::string container_name) : container_id(id), name(container_name) {
}

download_container::download_container(const download_container &cnt) : download_list(cnt.download_list), container_id(cnt.container_id), name(cnt.name) {}

download_container::~download_container() {
	lock_guard<recursive_mutex> lock(download_mutex);
	#ifndef IS_PLUGIN
	if(!download_list.empty()) {
		log_string("Memory leak detected: container deleted, but there are still Downloads in the list. Please report!", LOG_ERR);
	}
	#endif

	for(download_container::iterator it = download_list.begin(); it != download_list.end(); ++it) {
		// this is just the last way out to prevent memory-leaking.. usually this should never happen
		delete *it;
	}
	//lock_guard<recursive_mutex> lock(download_mutex);
	// download-containers may only be destroyed after the ownage of the pointers is moved to another object
}


int download_container::total_downloads() {
	lock_guard<recursive_mutex> lock(download_mutex);
	return download_list.size();
}

int download_container::add_download(download *dl, int dl_id) {
	lock_guard<recursive_mutex> lock(download_mutex);
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
	lock_guard<recursive_mutex> lock(download_mutex);
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
	lock_guard<recursive_mutex> lock(download_mutex);
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
	lock_guard<recursive_mutex> lock(download_mutex);
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
	lock_guard<recursive_mutex> lock(download_mutex);
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
	unique_lock<recursive_mutex> lock(download_mutex, defer_lock);
	if(do_lock) {
		lock.lock();
	}

	if(download_list.empty()) {
		return LIST_ID;
	}

	download_container::iterator downloadable = download_list.end();

	for(iterator it = download_list.begin(); it != download_list.end(); ++it) {
		if((*it)->get_status() == DOWNLOAD_PENDING && (*it)->get_wait() == 0 && !(*it)->get_running() && (*it)->get_id() >= 0) {
			std::string current_host((*it)->get_host());

			plugin_output hinfo = (*it)->get_hostinfo();
			if(do_lock) lock.unlock();
			if(!hinfo.allows_multiple && global_download_list.count_running_waiting_dls_of_host((*it)->get_host()) > 0) {
				if(do_lock) lock.lock();
				continue;
			}
			if(do_lock) lock.lock();
			return (*it)->get_id();
		}
	}
	return LIST_ID;
}

#endif // IS_PLUGIN


void download_container::set_url(int id, std::string url) {
	lock_guard<recursive_mutex> lock(download_mutex);
	download_container::iterator dl = get_download_by_id(id);
	if(dl == download_list.end()) return;
	(*dl)->set_url(url);
}

void download_container::set_title(int id, std::string title) {
	lock_guard<recursive_mutex> lock(download_mutex);
	download_container::iterator dl = get_download_by_id(id);
	if(dl == download_list.end()) return;
	(*dl)->set_title(title);
}

void download_container::set_add_date(int id, std::string add_date) {
	lock_guard<recursive_mutex> lock(download_mutex);
	download_container::iterator dl = get_download_by_id(id);
	if(dl == download_list.end()) return;
	(*dl)->set_add_date(add_date);
}

void download_container::set_downloaded_bytes(int id, filesize_t bytes) {
	lock_guard<recursive_mutex> lock(download_mutex);
	download_container::iterator dl = get_download_by_id(id);
	if(dl == download_list.end()) return;
	(*dl)->set_downloaded_bytes(bytes);
}

void download_container::set_size(int id, filesize_t size) {
	lock_guard<recursive_mutex> lock(download_mutex);
	download_container::iterator dl = get_download_by_id(id);
	if(dl == download_list.end()) return;
	(*dl)->set_size(size);
}

void download_container::set_wait(int id, int seconds) {
	lock_guard<recursive_mutex> lock(download_mutex);
	download_container::iterator dl = get_download_by_id(id);
	if(dl == download_list.end()) return;
	(*dl)->set_wait(seconds);
}

void download_container::set_error(int id, plugin_status error) {
	lock_guard<recursive_mutex> lock(download_mutex);
	download_container::iterator dl = get_download_by_id(id);
	if(dl == download_list.end()) return;
	(*dl)->set_error(error);
}

void download_container::set_output_file(int id, std::string output_file) {
	lock_guard<recursive_mutex> lock(download_mutex);
	download_container::iterator dl = get_download_by_id(id);
	if(dl == download_list.end()) return;
	(*dl)->set_filename(output_file);
}

void download_container::set_running(int id, bool running) {
	lock_guard<recursive_mutex> lock(download_mutex);
	download_container::iterator dl = get_download_by_id(id);
	if(dl == download_list.end()) return;
	(*dl)->set_running(running);
}

void download_container::set_need_stop(int id, bool need_stop) {
	lock_guard<recursive_mutex> lock(download_mutex);
	download_container::iterator dl = get_download_by_id(id);
	if(dl == download_list.end()) return;
	(*dl)->set_need_stop(need_stop);
}

void download_container::set_status(int id, download_status status) {
	lock_guard<recursive_mutex> lock(download_mutex);
	download_container::iterator dl = get_download_by_id(id);
	if(dl == download_list.end()) return;
	(*dl)->set_status(status);
}

void download_container::set_speed(int id, int speed) {
	lock_guard<recursive_mutex> lock(download_mutex);
	download_container::iterator dl = get_download_by_id(id);
	if(dl == download_list.end()) return;
	(*dl)->set_speed(speed);
}

void download_container::set_can_resume(int id, bool can_resume) {
	lock_guard<recursive_mutex> lock(download_mutex);
	download_container::iterator dl = get_download_by_id(id);
	if(dl == download_list.end()) return;
	(*dl)->set_resumable(can_resume);
}


void download_container::set_proxy(int id, std::string proxy) {
	lock_guard<recursive_mutex> lock(download_mutex);
	download_container::iterator dl = get_download_by_id(id);
	if(dl == download_list.end()) return;
	(*dl)->set_proxy(proxy);
}

std::string download_container::get_proxy(int id) {
	lock_guard<recursive_mutex> lock(download_mutex);
	download_container::iterator dl = get_download_by_id(id);
	if(dl == download_list.end()) return false;
	return (*dl)->get_proxy();
}

bool download_container::get_can_resume(int id) {
	lock_guard<recursive_mutex> lock(download_mutex);
	download_container::iterator dl = get_download_by_id(id);
	if(dl == download_list.end()) return false;
	return (*dl)->get_resumable();
}

download_status download_container::get_status(int id) {
	lock_guard<recursive_mutex> lock(download_mutex);
	download_container::iterator dl = get_download_by_id(id);
	if(dl == download_list.end()) return DOWNLOAD_DELETED;
	return (*dl)->get_status();
}

bool download_container::get_need_stop(int id) {
	lock_guard<recursive_mutex> lock(download_mutex);
	download_container::iterator dl = get_download_by_id(id);
	if(dl == download_list.end()) return true;
	return (*dl)->get_need_stop();
}

bool download_container::get_running(int id) {
	lock_guard<recursive_mutex> lock(download_mutex);
	download_container::iterator dl = get_download_by_id(id);
	if(dl == download_list.end()) return false;
	return (*dl)->get_running();
}

std::string download_container::get_output_file(int id) {
	lock_guard<recursive_mutex> lock(download_mutex);
	download_container::iterator dl = get_download_by_id(id);
	if(dl == download_list.end()) return "";
	return (*dl)->get_filename();
}

plugin_status download_container::get_error(int id) {
	lock_guard<recursive_mutex> lock(download_mutex);
	download_container::iterator dl = get_download_by_id(id);
	if(dl == download_list.end()) return PLUGIN_ERROR;
	return (*dl)->get_error();
}

int download_container::get_wait(int id) {
	lock_guard<recursive_mutex> lock(download_mutex);
	download_container::iterator dl = get_download_by_id(id);
	if(dl == download_list.end()) return 0;
	return (*dl)->get_wait();
}

std::string download_container::get_add_date(int id) {
	lock_guard<recursive_mutex> lock(download_mutex);
	download_container::iterator dl = get_download_by_id(id);
	if(dl == download_list.end()) return "";
	return (*dl)->get_add_date();
}

std::string download_container::get_title(int id) {
	lock_guard<recursive_mutex> lock(download_mutex);
	download_container::iterator dl = get_download_by_id(id);
	if(dl == download_list.end()) return "";
	return (*dl)->get_title();
}

std::string download_container::get_url(int id) {
	lock_guard<recursive_mutex> lock(download_mutex);
	download_container::iterator dl = get_download_by_id(id);
	if(dl == download_list.end()) return "";
	return (*dl)->get_url();
}

filesize_t download_container::get_downloaded_bytes(int id) {
	lock_guard<recursive_mutex> lock(download_mutex);
	download_container::iterator dl = get_download_by_id(id);
	if(dl == download_list.end()) return 0;
	return (*dl)->get_downloaded_bytes();
}

filesize_t download_container::get_size(int id) {
	lock_guard<recursive_mutex> lock(download_mutex);
	download_container::iterator dl = get_download_by_id(id);
	if(dl == download_list.end()) return 0;
	return (*dl)->get_size();
}

int download_container::get_speed(int id) {
	lock_guard<recursive_mutex> lock(download_mutex);
	download_container::iterator dl = get_download_by_id(id);
	if(dl == download_list.end()) return 0;
	return (*dl)->get_speed();
}

CURL* download_container::get_handle(int id) {
	lock_guard<recursive_mutex> lock(download_mutex);
	download_container::iterator dl = get_download_by_id(id);
	if(dl == download_list.end()) return NULL;
	return (*dl)->get_handle();
}

std::string download_container::get_host(int dl) {
	lock_guard<recursive_mutex> lock(download_mutex);
	download_container::iterator it = get_download_by_id(dl);
	return (*it)->get_host();
}

void download_container::set_password(const std::string& passwd) {
	lock_guard<recursive_mutex> lock(download_mutex);
	password = passwd;
}

std::string download_container::get_password() {
	lock_guard<recursive_mutex> lock(download_mutex);
	return password;
}

void download_container::set_pkg_name(const std::string& pkg_name) {
	lock_guard<recursive_mutex> lock(download_mutex);
	name = pkg_name;
}

std::string download_container::get_pkg_name() {
	lock_guard<recursive_mutex> lock(download_mutex);
	return name;
}

void download_container::decrease_waits() {
	unique_lock<recursive_mutex> lock(download_mutex);
	for(download_container::iterator it = download_list.begin(); it != download_list.end(); ++it) {
		size_t wait = (*it)->get_wait();
		if(wait > 0) {
			(*it)->set_wait(wait - 1);
			if((*it)->get_status() == DOWNLOAD_INACTIVE) (*it)->set_wait(0);
		} else if((*it)->get_status() == DOWNLOAD_WAITING) {
			(*it)->set_status(DOWNLOAD_PENDING);
			#ifndef IS_PLUGIN
			lock.unlock();
			global_download_list.start_next_downloadable();
			lock.lock();
			#endif
		}
	}
}

void download_container::purge_deleted() {
	lock_guard<recursive_mutex> lock(download_mutex);
	for(size_t i = 0; i < download_list.size(); ++i) {
		if(download_list[i]->get_status() == DOWNLOAD_DELETED && download_list[i]->get_running() == false) {
			remove_download(download_list[i]->get_id());
			--i;
		}
	}
}

std::string download_container::create_client_list() {
	lock_guard<recursive_mutex> lock(download_mutex);

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
	lock_guard<recursive_mutex> lock(download_mutex);
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
	lock_guard<recursive_mutex> lock(download_mutex);
	for(download_container::iterator it = download_list.begin(); it != download_list.end(); ++it) {
		if(url == (*it)->get_url()) {
			return true;
		}
	}
	return false;
}

int download_container::get_list_position(int id) {
	lock_guard<recursive_mutex> lock(download_mutex);
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
	lock_guard<recursive_mutex> lock(download_mutex);
	download_container::iterator insert_it = download_list.begin();
	// set input iterator to correct position
	advance(insert_it, pos);

	dl.download_mutex.lock();
	while(dl.download_list.size() > 0) {
		(*dl.download_list.begin())->set_id(-1);
		(*dl.download_list.begin())->set_parent(container_id);
		insert_it = download_list.insert(insert_it, *(dl.download_list.begin()));
		++insert_it;
		dl.download_list.erase(dl.download_list.begin());
	}
	dl.download_mutex.unlock();

}

#ifndef IS_PLUGIN
void download_container::do_download(int id) {
	lock_guard<recursive_mutex> lock(download_mutex);
	download_container::iterator it = get_download_by_id(id);
	if(it == download_list.end()) return;
	(*it)->set_running(true);
	(*it)->set_status(DOWNLOAD_RUNNING);
	try {
		thread t(bind(&download::download_me, *it));
		t.detach();
	} catch(...) {
		log_string("Failed to start download-thread. There are probably too many running threads.", LOG_ERR);
	}
}

void download_container::preset_file_status() {
	lock_guard<recursive_mutex> lock(download_mutex);
	try {
		size_t calls = 0;
		for(download_container::iterator it = download_list.begin(); it != download_list.end(); ++it) {
			if(!(*it)->get_prechecked() && (*it)->get_size() < 2 && (*it)->get_status() == DOWNLOAD_PENDING && global_config.get_bool_value("precheck_links")) {
				thread t(bind(&download::preset_file_status, *it));
				t.detach();
				++calls;
			}
			// we set prechecked here to make HDD spin-down possible. If we don't do this, it can happen that the config-value is checked every round.
			(*it)->set_prechecked(true);
			if(calls >=3) {
				// we start 3 threads per second as a maximum, so we don't get problems with boost::thread_resource_error
				return;
			}
		}
	} catch(...) {
		// boost might throw a thread_resource_error if too many threads are created at the same time. We just ignore it
		// and retry in a second, when this function is called again. Maybe some threads have closed then.
	}
}

void download_container::extract_package() {
    std::string fixed_passwd = get_password();
	string password_list = global_config.get_cfg_value("pkg_extractor_passwords");
	string curr_password = fixed_passwd;
	bool all_success = true;
	unique_lock<recursive_mutex> lock(download_mutex);
    for (size_t i = 0; i < download_list.size(); ++i) {
        string output_file = download_list[i]->get_filename();
        size_t n = output_file.rfind(".part");
        if (n != string::npos && output_file.find(".rar") != string::npos) {
            int num_part = 1;
            n += 5;
            num_part = atoi(output_file.substr(n, output_file.find(".", n) - n).c_str());
            if (num_part > 1) continue;
        }

        while(true) {
            trim_string(curr_password);
            log_string("Trying to extract... " + output_file, LOG_DEBUG);
            lock.unlock();
            pkg_extractor::extract_status ret = pkg_extractor::extract_package(output_file, curr_password);
            lock.lock();
            if(ret == pkg_extractor::PKG_ERROR || ret == pkg_extractor::PKG_INVALID) {
                all_success = false;
                break;
            }
            if(ret == pkg_extractor::PKG_PASSWORD) {
                // next password
                if(!fixed_passwd.empty() || (fixed_passwd.empty() && password_list.empty())) {
                        all_success = false;
                        break;
                }
                if(curr_password.empty()) {
                    curr_password = password_list.substr(0, password_list.find(";"));
                    continue;
                }
                try {
                    if(password_list.find(";") != string::npos) {
                        password_list = password_list.substr(password_list.find(";") + 1);
                    }
                    curr_password = password_list.substr(0, password_list.find(";"));
                } catch(...) {
                    curr_password = password_list.substr(0, password_list.find(";"));
                    password_list = "";
                } // ignore errors
                if(curr_password.empty()) {
                    all_success = false;
                    break;
                }
                continue;
            }

            if(ret == pkg_extractor::PKG_SUCCESS) {
                log_string("Successfully extracted file: " + output_file, LOG_DEBUG);
                break;
            }
        }
    }
    if(all_success) {
        log_string("All files extracted successfully.", LOG_DEBUG);
        if(global_config.get_bool_value("delete_extracted_archives")) {
            log_string("Removing downloaded archive files...", LOG_DEBUG);
            for(size_t i = 0; i < download_list.size(); ++i) {
                remove(download_list[i]->get_filename().c_str());
                download_list[i]->set_filename("");
            }
        }
    }
}

#endif

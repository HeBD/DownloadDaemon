#include <config.h>
#include "package_container.h"
#include "download.h"
#include "download_container.h"
#include "../global.h"
#include "../tools/helperfunctions.h"
#include "../plugins/captcha.h"
#include "../reconnect/reconnect_parser.h"

#include <string>

#include <sys/types.h>
#include <sys/stat.h>
#include <dlfcn.h>

#ifndef HAVE_UINT64_T
	#define uint64_t double
#endif

using namespace std;

package_container::package_container() : is_reconnecting(false) {
	//int pkg_id = add_package("");
	// set the global package to ID -1. It will not be shown. It's just a global container
	// where containers with an unsecure status are stored (eg. if they get a DELETE command, but the object
	// still exists)
	//lock_guard<recursive_mutex> lock(mx);
	//(*package_by_id(pkg_id))->container_id = -1;
}

package_container::~package_container() {
	lock_guard<recursive_mutex> lock(mx);
	dump_to_file(false);
	// only on program termination.. data will have to be cleaned up by the OS.. sadly

	//for(package_container::iterator it = packages.begin(); it != packages.end(); ++it) {
		// the download_container safely removes the downloads one by another.
	//	delete *it;
	//}
	//for(package:container::iterator it = packages_to_delete.begin(); it != packages_to_delete.end(); ++it) {
	//	delete *it;
	//}
}

int package_container::from_file(const char* filename) {
	list_file = filename;
	if(list_file.empty()) {
		log_string("No Download-list file specified! Using an in-memory list that will be lost when quitting DownloadDaemon", LOG_WARNING);
		return LIST_SUCCESS;
	}
	ifstream dlist(filename);
	std::string line;
	int curr_pkg_id = -1;
	while(getline(dlist, line)) {
		if(line.find("PKG|") == 0) {
			line = line.substr(4);
			if(line.find("|") == string::npos || line.find("|") == 0) return LIST_ID;
			curr_pkg_id = atoi(line.substr(0, line.find("|")).c_str());
			line = line.substr(line.find("|") + 1);
			int id = add_package(line.substr(0, line.find("|")));
			line = line.substr(line.find("|") + 1);
			package_container::iterator package = package_by_id(id);
			(*package)->container_id = curr_pkg_id;
			(*package)->password = line.substr(0, line.find("|"));
			continue;
		}
		if(curr_pkg_id == -1) {
			curr_pkg_id = add_package("imported");
		}

		download* dl = new download;
		dl->from_serialized(line);
		add_dl_to_pkg(dl, curr_pkg_id);
	}

	if(!dlist.good()) {
		dlist.close();
		return LIST_PERMISSION;
	}
	dlist.close();
	return LIST_SUCCESS;
}

int package_container::add_package(std::string pkg_name) {
	lock_guard<recursive_mutex> lock(mx);
	int cnt_id = get_next_id();
	download_container* cnt = new download_container(cnt_id, pkg_name);
	packages.push_back(cnt);
	return cnt_id;
}

int package_container::add_package(std::string pkg_name, download_container* downloads) {
	lock_guard<recursive_mutex> lock(mx);
	downloads->container_id = get_next_id();
	downloads->name = pkg_name;
	packages.push_back(downloads);
	return downloads->container_id;
}

int package_container::add_dl_to_pkg(download* dl, int pkg_id) {
	lock_guard<recursive_mutex> lock(mx);
	package_container::iterator it = package_by_id(pkg_id);
	if(it == packages.end()) {
		delete dl;
		dl = NULL;
		return LIST_ID;
	}
	return (*it)->add_download(dl, get_next_download_id());
}

void package_container::del_package(int pkg_id) {
	lock_guard<recursive_mutex> lock(mx);
	package_container::iterator package = package_by_id(pkg_id);
	if(package == packages.end()) return;

	// we are friend of download_container. so we lock the it's mutex and set all statuses to deleted.
	// then we move the package from the package list to the pending-deletion list
	(*package)->download_mutex.lock();
	for(download_container::iterator it = (*package)->download_list.begin(); it != (*package)->download_list.end(); ++it) {
		(*it)->set_status(DOWNLOAD_DELETED);
	}

	packages_to_delete.push_back(*package);
	(*package)->download_mutex.unlock();
	packages.erase(package);
}

bool package_container::empty() {
	lock_guard<recursive_mutex> lock(mx);
	// do not count the general-package which is always there
 	if(packages.size() <= 1) return true;
 	return false;
}


dlindex package_container::get_next_downloadable() {
	lock_guard<recursive_mutex> lock(mx);
	int running_downloads = 0;
	std::pair<int, int> result(LIST_ID, LIST_ID);
	for(package_container::iterator it = packages.begin(); it != packages.end(); ++it) {
		running_downloads += (*it)->running_downloads();
		if(result.first == LIST_ID && result.second == LIST_ID) {
			int ret = (*it)->get_next_downloadable();
			if(ret != LIST_ID) {
				result.first = (*it)->container_id;
				result.second = ret;
			}
		}
	}
	if(running_downloads < global_config.get_int_value("simultaneous_downloads")) {
		return result;
	} else {
		return make_pair<int, int>(LIST_ID, LIST_ID);
	}
}

void package_container::set_url(dlindex dl, std::string url) {
	lock_guard<recursive_mutex> lock(mx);
	package_container::iterator it = package_by_id(dl.first);
	if(it == packages.end()) return;
	(*it)->set_url(dl.second, url);
}

void package_container::set_title(dlindex dl, std::string title) {
	lock_guard<recursive_mutex> lock(mx);
	package_container::iterator it = package_by_id(dl.first);
	if(it == packages.end()) return;
	(*it)->set_title(dl.second, title);
}

void package_container::set_add_date(dlindex dl, std::string add_date) {
	lock_guard<recursive_mutex> lock(mx);
	package_container::iterator it = package_by_id(dl.first);
	(*it)->set_add_date(dl.second, add_date);
}

void package_container::set_downloaded_bytes(dlindex dl, uint64_t bytes) {
	lock_guard<recursive_mutex> lock(mx);
	package_container::iterator it = package_by_id(dl.first);
	if(it == packages.end()) return;
	(*it)->set_downloaded_bytes(dl.second, bytes);
}

void package_container::set_size(dlindex dl, uint64_t size) {
	lock_guard<recursive_mutex> lock(mx);
	package_container::iterator it = package_by_id(dl.first);
	if(it == packages.end()) return;
	(*it)->set_size(dl.second, size);
}

void package_container::set_wait(dlindex dl, int seconds) {
	lock_guard<recursive_mutex> lock(mx);
	package_container::iterator it = package_by_id(dl.first);
	if(it == packages.end()) return;
	(*it)->set_wait(dl.second, seconds);
}

void package_container::set_error(dlindex dl, plugin_status error) {
	lock_guard<recursive_mutex> lock(mx);
	package_container::iterator it = package_by_id(dl.first);
	if(it == packages.end()) return;
	(*it)->set_error(dl.second, error);
}

void package_container::set_output_file(dlindex dl, std::string output_file) {
	lock_guard<recursive_mutex> lock(mx);
	package_container::iterator it = package_by_id(dl.first);
	if(it == packages.end()) return;
	(*it)->set_output_file(dl.second, output_file);
}

void package_container::set_running(dlindex dl, bool running) {
	lock_guard<recursive_mutex> lock(mx);
	package_container::iterator it = package_by_id(dl.first);
	if(it == packages.end()) return;
	(*it)->set_running(dl.second, running);
}

void package_container::set_need_stop(dlindex dl, bool need_stop) {
	lock_guard<recursive_mutex> lock(mx);
	package_container::iterator it = package_by_id(dl.first);
	if(it == packages.end()) return;
	(*it)->set_need_stop(dl.second, need_stop);
}

void package_container::set_status(dlindex dl, download_status status) {
	lock_guard<recursive_mutex> lock(mx);
	package_container::iterator it = package_by_id(dl.first);
	if(it == packages.end()) return;
	(*it)->set_status(dl.second, status);
}

void package_container::set_speed(dlindex dl, int speed) {
	lock_guard<recursive_mutex> lock(mx);
	package_container::iterator it = package_by_id(dl.first);
	if(it == packages.end()) return;
	(*it)->set_speed(dl.second, speed);
}

void package_container::set_can_resume(dlindex dl, bool can_resume) {
	lock_guard<recursive_mutex> lock(mx);
	package_container::iterator it = package_by_id(dl.first);
	if(it == packages.end()) return;
	(*it)->set_can_resume(dl.second, can_resume);
}

void package_container::set_proxy(dlindex dl, std::string proxy) {
	lock_guard<recursive_mutex> lock(mx);
	package_container::iterator it = package_by_id(dl.first);
	if(it == packages.end()) return;
	(*it)->set_proxy(dl.second, proxy);
}

std::string package_container::get_proxy(dlindex dl) {
	lock_guard<recursive_mutex> lock(mx);
	package_container::iterator it = package_by_id(dl.first);
	if(it == packages.end()) return "";
	return (*it)->get_proxy(dl.second);
}

CURL* package_container::get_handle(dlindex dl) {
	lock_guard<recursive_mutex> lock(mx);
	package_container::iterator it = package_by_id(dl.first);
	if(it == packages.end()) return NULL;
	return (*it)->get_handle(dl.second);
}

bool package_container::get_can_resume(dlindex dl) {
	lock_guard<recursive_mutex> lock(mx);
	package_container::iterator it = package_by_id(dl.first);
	if(it == packages.end()) return false;
	return (*it)->get_can_resume(dl.second);
}

int package_container::get_speed(dlindex dl) {
	lock_guard<recursive_mutex> lock(mx);
	package_container::iterator it = package_by_id(dl.first);
	if(it == packages.end()) return 0;
	return (*it)->get_speed(dl.second);
}

download_status package_container::get_status(dlindex dl) {
	lock_guard<recursive_mutex> lock(mx);
	package_container::iterator it = package_by_id(dl.first);
	if(it == packages.end()) return DOWNLOAD_DELETED;
	return (*it)->get_status(dl.second);
}

bool package_container::get_need_stop(dlindex dl) {
	lock_guard<recursive_mutex> lock(mx);
	package_container::iterator it = package_by_id(dl.first);
	if(it == packages.end()) return true;
	return (*it)->get_need_stop(dl.second);
}

bool package_container::get_running(dlindex dl) {
	lock_guard<recursive_mutex> lock(mx);
	package_container::iterator it = package_by_id(dl.first);
	if(it == packages.end()) return false;
	return (*it)->get_running(dl.second);
}

std::string package_container::get_output_file(dlindex dl) {
	lock_guard<recursive_mutex> lock(mx);
	package_container::iterator it = package_by_id(dl.first);
	if(it == packages.end()) return "";
	return (*it)->get_output_file(dl.second);
}

plugin_status package_container::get_error(dlindex dl) {
	lock_guard<recursive_mutex> lock(mx);
	package_container::iterator it = package_by_id(dl.first);
	if(it == packages.end()) return PLUGIN_ERROR;
	return (*it)->get_error(dl.second);
}

int package_container::get_wait(dlindex dl) {
	lock_guard<recursive_mutex> lock(mx);
	package_container::iterator it = package_by_id(dl.first);
	if(it == packages.end()) return 0;
	return (*it)->get_wait(dl.second);
}

uint64_t package_container::get_size(dlindex dl) {
	lock_guard<recursive_mutex> lock(mx);
	package_container::iterator it = package_by_id(dl.first);
	if(it == packages.end()) return 0;
	return (*it)->get_size(dl.second);
}

uint64_t package_container::get_downloaded_bytes(dlindex dl) {
	lock_guard<recursive_mutex> lock(mx);
	package_container::iterator it = package_by_id(dl.first);
	if(it == packages.end()) return 0;
	return (*it)->get_downloaded_bytes(dl.second);
}

std::string package_container::get_add_date(dlindex dl) {
	lock_guard<recursive_mutex> lock(mx);
	package_container::iterator it = package_by_id(dl.first);
	if(it == packages.end()) return "";
	return (*it)->get_add_date(dl.second);
}

std::string package_container::get_title(dlindex dl) {
	lock_guard<recursive_mutex> lock(mx);
	package_container::iterator it = package_by_id(dl.first);
	if(it == packages.end()) return "";
	return (*it)->get_title(dl.second);
}

std::string package_container::get_url(dlindex dl) {
	lock_guard<recursive_mutex> lock(mx);
	package_container::iterator it = package_by_id(dl.first);
	if(it == packages.end()) return "";
	return (*it)->get_url(dl.second);
}

void package_container::set_password(int id, const std::string& passwd) {
	lock_guard<recursive_mutex> lock(mx);
	package_container::iterator it = package_by_id(id);
	if(it == packages.end()) return;
	(*it)->set_password(passwd);
}

std::string package_container::get_password(int id) {
	lock_guard<recursive_mutex> lock(mx);
	package_container::iterator it = package_by_id(id);
	if(it == packages.end()) return "";
	return (*it)->get_password();
}

void package_container::set_pkg_name(int id, const std::string& name) {
	lock_guard<recursive_mutex> lock(mx);
	package_container::iterator it = package_by_id(id);
	if(it == packages.end()) return;
	(*it)->set_pkg_name(name);
}

std::string package_container::get_pkg_name(int id) {
	lock_guard<recursive_mutex> lock(mx);
	package_container::iterator it = package_by_id(id);
	if(it == packages.end()) return "";
	return (*it)->get_pkg_name();
}

std::vector<int> package_container::get_download_list(int id) {
	lock_guard<recursive_mutex> lock(mx);
	std::vector<int> vec;
	package_container::iterator it = package_by_id(id);
	if(it == packages.end()) return vec;
	(*it)->download_mutex.lock();

	for(download_container::iterator dlit = (*it)->download_list.begin(); dlit != (*it)->download_list.end(); ++dlit) {
		vec.push_back((*dlit)->get_id());
	}
	(*it)->download_mutex.unlock();
	return vec;
}

std::string package_container::get_host(dlindex dl) {
	lock_guard<recursive_mutex> lock(mx);
	package_container::iterator it = package_by_id(dl.first);
	return (*it)->get_host(dl.second);
}


plugin_output package_container::get_hostinfo(dlindex dl) {
	lock_guard<recursive_mutex> lock(mx);
	package_container::iterator it = package_by_id(dl.first);
	lock_guard<recursive_mutex> lockdl((*it)->download_mutex);
	download_container::iterator it2 = (*it)->get_download_by_id(dl.second);
	return (*it2)->get_hostinfo();
}

int package_container::total_downloads() {
	lock_guard<recursive_mutex> lock(mx);
	int total = 0;
	for(package_container::iterator it = packages.begin(); it != packages.end(); ++it) {
		total += (*it)->total_downloads();
	}
	return total;
}

void package_container::decrease_waits() {
	lock_guard<recursive_mutex> lock(mx);
	for(package_container::iterator it = packages.begin(); it != packages.end(); ++it) {
		(*it)->decrease_waits();
	}
}

void package_container::purge_deleted() {
	lock_guard<recursive_mutex> lock(mx);
	for(package_container::iterator it = packages.begin(); it != packages.end(); ++it) {
		(*it)->purge_deleted();
	}
	for(package_container::iterator it = packages_to_delete.begin(); it != packages_to_delete.end(); ++it) {
		(*it)->purge_deleted();
		if((*it)->total_downloads() == 0) {
			delete *it;
			packages_to_delete.erase(it);
			it = packages_to_delete.begin();
			break;
		}

		for(download_container::iterator it2 = (*it)->download_list.begin(); it2 != (*it)->download_list.end(); ++it2) {
			(*it2)->set_status(DOWNLOAD_DELETED);
		}
	}
}

std::string package_container::create_client_list() {
	lock_guard<recursive_mutex> lock(mx);
	std::string list;
	for(package_container::iterator it = packages.begin(); it != packages.end(); ++it) {
		list += "PACKAGE|" + int_to_string((*it)->container_id)  + "|" + (*it)->name + "\n";
		list += (*it)->create_client_list();
	}
	trim_string(list);
	return list;
}

bool package_container::url_is_in_list(std::string url) {
	lock_guard<recursive_mutex> lock(mx);
	for(package_container::iterator it = packages.begin(); it != packages.end(); ++it) {
		if((*it)->url_is_in_list(url))
			return true;
	}
	return false;
}


void  package_container::move_dl(dlindex dl, package_container::direction d) {
	lock_guard<recursive_mutex> lock(mx);
	package_container::iterator it = package_by_id(dl.first);
	if(it == packages.end()) return;
	if(d == DIRECTION_UP) (*it)->move_up(dl.second);
	else (*it)->move_down(dl.second);
}

void  package_container::move_pkg(int dl, package_container::direction d) {
	lock_guard<recursive_mutex> lock(mx);
	if(dl == -1) return;
	package_container::iterator it = package_by_id(dl);
	if(it == packages.end()) return;
	package_container::iterator it2 = it;
	if(d == DIRECTION_UP && it != packages.begin()) {
		--it2;
		download_container* tmp = *it;
		*it = *it2;
		*it2 = tmp;
	}
	if(d == DIRECTION_DOWN) {
		++it;
		if(it == packages.end()) return;
		download_container* tmp = *it;
		*it = *it2;
		*it2 = tmp;
	}
}

bool package_container::reconnect_needed() {
	unique_lock<recursive_mutex> lock(mx);
	std::string reconnect_policy;

	if(!global_config.get_bool_value("enable_reconnect")) {
		return false;
	}

	reconnect_policy = global_router_config.get_cfg_value("reconnect_policy");
	if(reconnect_policy.empty()) {
		log_string("Reconnecting activated, but no reconnect policy specified", LOG_WARNING);
		return false;
	}

	bool need_reconnect = false;

	for(package_container::iterator pkg = packages.begin(); pkg != packages.end(); ++pkg) {
		(*pkg)->download_mutex.lock();
		for(download_container::iterator it = (*pkg)->download_list.begin(); it != (*pkg)->download_list.end(); ++it) {
			if((*it)->get_status() == DOWNLOAD_WAITING && (*it)->get_error() == PLUGIN_LIMIT_REACHED) {
				need_reconnect = true;
			}
		}
		(*pkg)->download_mutex.unlock();
	}

	if(!need_reconnect) {
		return false;
	}

	// Always reconnect if needed
	if(reconnect_policy == "HARD") {
		return true;

	// Only reconnect if no non-continuable download is running
	} else if(reconnect_policy == "CONTINUE") {
		for(package_container::iterator pkg = packages.begin(); pkg != packages.end(); ++pkg) {
			(*pkg)->download_mutex.lock();
			for(download_container::iterator it = (*pkg)->download_list.begin(); it != (*pkg)->download_list.end(); ++it) {
				if((*it)->get_status() == DOWNLOAD_RUNNING && !(*it)->get_hostinfo().allows_resumption) {
					return false;
				}
			}
			(*pkg)->download_mutex.unlock();
		}
		return true;

	// Only reconnect if no download is running
	} else if(reconnect_policy == "SOFT") {
		for(package_container::iterator pkg = packages.begin(); pkg != packages.end(); ++pkg) {
			(*pkg)->download_mutex.lock();
			for(download_container::iterator it = (*pkg)->download_list.begin(); it != (*pkg)->download_list.end(); ++it) {
				if((*it)->get_status() == DOWNLOAD_RUNNING) {
					return false;
				}
			}
			(*pkg)->download_mutex.unlock();
		}
		return true;

	// Only reconnect if no download is running and no download can be started
	} else if(reconnect_policy == "PUSSY") {
		for(package_container::iterator pkg = packages.begin(); pkg != packages.end(); ++pkg) {
			(*pkg)->download_mutex.lock();
			for(download_container::iterator it = (*pkg)->download_list.begin(); it != (*pkg)->download_list.end(); ++it) {
				if((*it)->get_status() == DOWNLOAD_RUNNING) {
					return false;
				}
			}
			(*pkg)->download_mutex.unlock();
		}
		lock.unlock();
		if(get_next_downloadable().first != LIST_ID) {
			return false;
		}
		return true;

	} else {
		log_string("Invalid reconnect policy", LOG_ERR);
		return false;
	}
}

void package_container::do_reconnect() {
	// to be executed in a seperate thread!
	unique_lock<recursive_mutex> lock(mx);
	if(is_reconnecting) {
		return;
	}

	is_reconnecting = true;
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
	for(package_container::iterator pkg = packages.begin(); pkg != packages.end(); ++pkg) {
		(*pkg)->download_mutex.lock();
		for(download_container::iterator it = (*pkg)->download_list.begin(); it != (*pkg)->download_list.end(); ++it) {
			if((*it)->get_status() == DOWNLOAD_WAITING) {
				(*it)->set_status(DOWNLOAD_RECONNECTING);
				(*it)->set_wait(0);
			}
		}
		(*pkg)->download_mutex.unlock();
	}

	lock.unlock();
	if(!rc.do_reconnect()) {
		log_string("Reconnect failed, change your settings.", LOG_ERR);
	}
	lock.lock();
	for(package_container::iterator pkg = packages.begin(); pkg != packages.end(); ++pkg) {
		(*pkg)->download_mutex.lock();
		for(download_container::iterator it = (*pkg)->download_list.begin(); it != (*pkg)->download_list.end(); ++it) {
			if((*it)->get_status() == DOWNLOAD_WAITING || (*it)->get_status() == DOWNLOAD_RECONNECTING) {
				(*it)->set_status(DOWNLOAD_PENDING);
				(*it)->set_wait(0);
			}
		}
		(*pkg)->download_mutex.unlock();
	}
	is_reconnecting = false;
	return;
}

void package_container::dump_to_file(bool do_lock) {
	lock_guard<recursive_mutex> lock(mx);

	ofstream ofs(list_file.c_str(), ios::trunc);
	for(package_container::iterator pkg = packages.begin(); pkg != packages.end(); ++pkg) {
		(*pkg)->download_mutex.lock();
		ofs << "PKG|" << (*pkg)->container_id << "|" << (*pkg)->name << "|" << (*pkg)->password << endl;
		for(download_container::iterator it = (*pkg)->download_list.begin(); it != (*pkg)->download_list.end(); ++it) {
			ofs << (*it)->serialize();
		}
		(*pkg)->download_mutex.unlock();
	}

}

int package_container::pkg_that_contains_download(int download_id) {
	lock_guard<recursive_mutex> lock(mx);
	for(package_container::iterator it = packages.begin(); it != packages.end(); ++it) {
		lock_guard<recursive_mutex> dllock((*it)->download_mutex);
		for(download_container::iterator dlit = (*it)->download_list.begin(); dlit != (*it)->download_list.end(); ++dlit) {
			if((*dlit)->get_id() == download_id) return (*it)->container_id;
		}
	}
	return LIST_ID;
}

bool package_container::pkg_exists(int id) {
	if(id < 0) return false;
	lock_guard<recursive_mutex> lock(mx);
	for(package_container::iterator it = packages.begin(); it != packages.end(); ++it) {
		if((*it)->container_id == id) return true;
	}
	return false;
}

bool package_container::package_finished(int id) {
	lock_guard<recursive_mutex> lock(mx);
	package_container::iterator pkg_it = package_by_id(id);
	lock_guard<recursive_mutex> listlock((*pkg_it)->download_mutex);
	for(download_container::iterator it = (*pkg_it)->download_list.begin(); it != (*pkg_it)->download_list.end(); ++it) {
		if((*it)->get_status() != DOWNLOAD_FINISHED)
			return false;
	}
	return true;
}

download_container* package_container::get_listptr(int id) {
	lock_guard<recursive_mutex> lock(mx);
	package_container::iterator pkg_it = package_by_id(id);
	if(pkg_it == packages.end()) return NULL;
	return *pkg_it;
}

void package_container::do_download(dlindex dl) {
	lock_guard<recursive_mutex> lock(mx);
	package_container::iterator pkg_it = package_by_id(dl.first);
	if(pkg_it == packages.end()) return;
	(*pkg_it)->do_download(dl.second);
}

void package_container::extract_package(int id) {
	if(!global_config.get_bool_value("enable_pkg_extractor")) return;
	unique_lock<recursive_mutex> lock(mx);
	package_container::iterator pkg_it = package_by_id(id);
	std::string fixed_passwd = (*pkg_it)->get_password();
	FILE* extractor;
	string to_exec;
	unique_lock<recursive_mutex> pkg_lock((*pkg_it)->download_mutex);
	download_container::iterator it = (*pkg_it)->download_list.begin();
	if((*pkg_it)->download_list.size() == 0) return;
	string output_file = (*it)->get_filename();
	string extension;
	string output_dir;
	size_t n = output_file.find_last_of(".");
	if(n != string::npos) {
		extension = output_file.substr(n);
		output_dir = output_file.substr(0, n);
	} else {
		return;
	}
	if(!(*pkg_it)->name.empty()) {
		string tmp = (*pkg_it)->name;
		make_valid_filename(tmp);
		if(!tmp.empty()) {
			output_dir = output_dir.substr(0, output_dir.find_last_of("/\\"));
			output_dir += "/" + tmp;
		}
	}

	pkg_lock.unlock();
	lock.unlock();
	string password_list = global_config.get_cfg_value("pkg_extractor_passwords");
	string password = fixed_passwd;
	std::string unrar_path = global_config.get_cfg_value("unrar_path");
	std::string tar_path = global_config.get_cfg_value("tar_path");
	while(true) {
		trim_string(password);
		if(extension == ".rar") {
			if(unrar_path.empty()) return;
			to_exec = unrar_path + " x";
			if(!password.empty())
				to_exec += " -p" + password;
			else
				to_exec += " -p-";
			to_exec += " -o+ -y '" + output_file + "' '" + output_dir + "'";
		//} else if(extension == ".zip") {
		//	to_exec = "unzip -o -qq";
		//	if(!password.empty())
		//		to_exec += " -P " + password;
		//	to_exec += "'" + output_file + "' -d '" + output_dir + "'";
		} else if(extension == ".gz" && output_file.find(".tar.gz") == output_file.size() - 7 && !tar_path.empty()) {
			to_exec = "tar xzf '" + output_file + "' -C '" + output_dir + "'";
		} else if(extension == ".bz2" && output_file.find(".tar.bz2") == output_file.size() - 8 && !tar_path.empty()) {
			to_exec = "tar xjf '" + output_file + "' -C '" + output_dir + "'";
		}
		if(to_exec.empty()) return;
		to_exec += " 2>&1";

		mkdir_recursive(output_dir);
		log_string("Trying to extract... " + to_exec, LOG_DEBUG);
		extractor = popen(to_exec.c_str(), "r");
		if(extractor == NULL) {
			log_string("Unable to open pipe to extractor of file: " + output_file, LOG_WARNING);
			return;
		}
		string result;
		int c;
		do{
			c = fgetc(extractor);
			if(c != EOF) result.push_back(c);
		} while(c != EOF);
		int retval = pclose(extractor);
		if(result.find("password incorrect") != string::npos) {
			// next password
			log_string("extraction failed. Password incorrect?", LOG_DEBUG);
			if(!fixed_passwd.empty()) return;
			if(password_list.empty()) return;
			if(password.empty()) {
				password = password_list.substr(0, password_list.find(";"));
				continue;
			}
			if(password_list.find(";") != string::npos)
				password_list = password_list.substr(password_list.find(";") + 1);
			password = password_list.substr(0, password_list.find(";"));
			if(password.empty()) return;
			continue;
		}

		if(retval == 0 && global_config.get_bool_value("delete_extracted_archives")) {
			// success, let's delete the files.
			log_string("Successfully extracted package " + int_to_string(id) + ". Removing archives...", LOG_DEBUG);
			pkg_lock.lock();
			for(it = (*pkg_it)->download_list.begin(); it != (*pkg_it)->download_list.end(); ++it) {
				remove((*it)->get_filename().c_str());
				(*it)->set_filename("");
			}
			pkg_lock.unlock();
		} else {
			log_string("Successfully extracted package " + int_to_string(id), LOG_DEBUG);
		}
		break;
	}
}

int package_container::get_next_download_id(bool lock_download_mutex) {
	int max_id = -1;
	for(package_container::iterator it = packages.begin(); it != packages.end(); ++it) {
		if(lock_download_mutex) (*it)->download_mutex.lock();
		int max_here = -1;
		for(download_container::iterator cnt_it = (*it)->download_list.begin(); cnt_it != (*it)->download_list.end(); ++cnt_it) {
			if((*cnt_it)->get_id() > max_here) {
				max_here = (*cnt_it)->get_id();
			}
		}
		if(max_here > max_id) max_id = max_here;
		if(lock_download_mutex) (*it)->download_mutex.unlock();
	}
	return ++max_id;
}

package_container::iterator package_container::package_by_id(int pkg_id) {
	lock_guard<recursive_mutex> lock(mx);
	package_container::iterator it = packages.begin();
	for(; it != packages.end(); ++it) {
		if((*it)->container_id == pkg_id) {
			break;
		}
	}
	return it;
}

int package_container::get_next_id() {
	lock_guard<recursive_mutex> lock(mx);
	int max_id = -1;
	for(package_container::iterator it = packages.begin(); it != packages.end(); ++it) {
		if((*it)->container_id > max_id) {
			max_id = (*it)->container_id;
		}
	}
	return max_id + 1;
}

void package_container::correct_invalid_ids() {
	lock_guard<recursive_mutex> lock(mx);
	for(package_container::iterator it = packages.begin(); it != packages.end(); ++it) {
		(*it)->download_mutex.lock();
		for(download_container::iterator dlit = (*it)->download_list.begin(); dlit != (*it)->download_list.end(); ++dlit) {
			if((*dlit)->get_id() == -1) {
				(*dlit)->set_id(get_next_download_id());
			}
		}
		(*it)->download_mutex.unlock();
	}
}

int package_container::count_running_waiting_dls_of_host(const std::string& host) {
	lock_guard<recursive_mutex> lock(mx);
	int count = 0;
	for(package_container::iterator it = packages.begin(); it != packages.end(); ++it) {
		(*it)->download_mutex.lock();
		for(download_container::iterator dlit = (*it)->download_list.begin(); dlit != (*it)->download_list.end(); ++dlit) {
			if(((*dlit)->get_running() || (*dlit)->get_status() == DOWNLOAD_WAITING) && (*dlit)->get_host() == host) {
				++count;
			}
		}
		(*it)->download_mutex.unlock();
	}
	return count;
}




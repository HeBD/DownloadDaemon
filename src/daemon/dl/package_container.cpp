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

using namespace std;

package_container::package_container() : is_reconnecting(false) {
	int pkg_id = add_package("");
	// set the global package to ID -1. It will not be shown. It's just a global container
	// where containers with an unsecure status are stored (eg. if they get a DELETE command, but the object
	// still exists)
	lock_guard<mutex> lock(mx);
	(*package_by_id(pkg_id))->container_id = -1;
}

package_container::~package_container() {
	for(package_container::iterator it = packages.begin(); it != packages.end(); ++it) {
		// the download_container safely removes the downloads one by another.
		delete *it;
	}
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
		if(line.find("PKG:") == 0) {
			line = line.substr(4);
			if(line.find(":") == string::npos || line.find(":") == 0) return LIST_ID;
			curr_pkg_id = atoi(line.substr(0, line.find(":")).c_str());
			line = line.substr(line.find(":") + 1);
			int id = add_package(line);
			package_container::iterator package = package_by_id(id);
			(*package)->container_id = curr_pkg_id;
			continue;
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
	lock_guard<mutex> lock(mx);
	int cnt_id = get_next_id();
	download_container* cnt = new download_container(cnt_id, pkg_name);
	packages.push_back(cnt);
	return cnt_id;
}

int package_container::add_package(std::string pkg_name, download_container* downloads) {
	lock_guard<mutex> lock(mx);
	downloads->container_id = get_next_id();
	downloads->name = pkg_name;
	packages.push_back(downloads);
	return downloads->container_id;
}

int package_container::add_dl_to_pkg(download* dl, int pkg_id) {
	lock_guard<mutex> lock(mx);
	package_container::iterator it = package_by_id(pkg_id);
	if(it == packages.end()) return LIST_ID;
	return (*it)->add_download(dl, get_next_download_id());
}

void package_container::del_package(int pkg_id) {
	lock_guard<mutex> lock(mx);
	package_container::iterator package = package_by_id(pkg_id);
	package_container::iterator global = package_by_id(-1);
	if(package == packages.end() || global == packages.end()) return;
	// we are friend of download_container. so we lock the it's mutex and set all statuses to deleted.
	// then we move all the downloads to the general list, so we can safely delete the list itsself
	(*package)->download_mutex.lock();
	for(download_container::iterator it = (*package)->download_list.begin(); it != (*package)->download_list.end(); ++it) {
		(*package)->set_dl_status(it, DOWNLOAD_DELETED);
	}
	(*global)->download_list = (*package)->download_list;
	(*package)->download_list.clear();
	(*package)->download_mutex.unlock();
	// now all the download-pointers are in the global list and wen can safely remove the package
	delete *package;
	packages.erase(package);
}

bool package_container::empty() {
	lock_guard<mutex> lock(mx);
	// do not count the general-package which is always there
 	if(packages.size() <= 1) return true;
 	return false;
}


dlindex package_container::get_next_downloadable() {
	lock_guard<mutex> lock(mx);
	for(package_container::iterator it = packages.begin() + 1; it != packages.end(); ++it) {
		int ret = (*it)->get_next_downloadable();
		if(ret != LIST_ID) {
			return make_pair<int, int>((*it)->container_id, ret);
		}
	}
	return make_pair<int, int>(LIST_ID, LIST_ID);
}




void package_container::set_url(dlindex dl, std::string url) {
	lock_guard<mutex> lock(mx);
	package_container::iterator it = package_by_id(dl.first);
	if(it == packages.end()) return;
	(*it)->set_url(dl.second, url);
}

void package_container::set_title(dlindex dl, std::string title) {
	lock_guard<mutex> lock(mx);
	package_container::iterator it = package_by_id(dl.first);
	if(it == packages.end()) return;
	(*it)->set_title(dl.second, title);
}

void package_container::set_add_date(dlindex dl, std::string add_date) {
	lock_guard<mutex> lock(mx);
	package_container::iterator it = package_by_id(dl.first);
	(*it)->set_add_date(dl.second, add_date);
}

void package_container::set_downloaded_bytes(dlindex dl, uint64_t bytes) {
	lock_guard<mutex> lock(mx);
	package_container::iterator it = package_by_id(dl.first);
	if(it == packages.end()) return;
	(*it)->set_downloaded_bytes(dl.second, bytes);
}

void package_container::set_size(dlindex dl, uint64_t size) {
	lock_guard<mutex> lock(mx);
	package_container::iterator it = package_by_id(dl.first);
	if(it == packages.end()) return;
	(*it)->set_size(dl.second, size);
}

void package_container::set_wait(dlindex dl, int seconds) {
	lock_guard<mutex> lock(mx);
	package_container::iterator it = package_by_id(dl.first);
	if(it == packages.end()) return;
	(*it)->set_wait(dl.second, seconds);
}

void package_container::set_error(dlindex dl, plugin_status error) {
	lock_guard<mutex> lock(mx);
	package_container::iterator it = package_by_id(dl.first);
	if(it == packages.end()) return;
	(*it)->set_error(dl.second, error);
}

void package_container::set_output_file(dlindex dl, std::string output_file) {
	lock_guard<mutex> lock(mx);
	package_container::iterator it = package_by_id(dl.first);
	if(it == packages.end()) return;
	(*it)->set_output_file(dl.second, output_file);
}

void package_container::set_running(dlindex dl, bool running) {
	lock_guard<mutex> lock(mx);
	package_container::iterator it = package_by_id(dl.first);
	if(it == packages.end()) return;
	(*it)->set_running(dl.second, running);
}

void package_container::set_need_stop(dlindex dl, bool need_stop) {
	lock_guard<mutex> lock(mx);
	package_container::iterator it = package_by_id(dl.first);
	if(it == packages.end()) return;
	(*it)->set_need_stop(dl.second, need_stop);
}

void package_container::set_status(dlindex dl, download_status status) {
	lock_guard<mutex> lock(mx);
	package_container::iterator it = package_by_id(dl.first);
	if(it == packages.end()) return;
	(*it)->set_status(dl.second, status);
}

void package_container::set_speed(dlindex dl, int speed) {
	lock_guard<mutex> lock(mx);
	package_container::iterator it = package_by_id(dl.first);
	if(it == packages.end()) return;
	(*it)->set_speed(dl.second, speed);
}

void package_container::set_can_resume(dlindex dl, bool can_resume) {
	lock_guard<mutex> lock(mx);
	package_container::iterator it = package_by_id(dl.first);
	if(it == packages.end()) return;
	(*it)->set_can_resume(dl.second, can_resume);
}

void package_container::set_proxy(dlindex dl, std::string proxy) {
	lock_guard<mutex> lock(mx);
	package_container::iterator it = package_by_id(dl.first);
	if(it == packages.end()) return;
	(*it)->set_proxy(dl.second, proxy);
}

std::string package_container::get_proxy(dlindex dl) {
	lock_guard<mutex> lock(mx);
	package_container::iterator it = package_by_id(dl.first);
	if(it == packages.end()) return false;
	return (*it)->get_proxy(dl.second);
}

CURL* package_container::get_handle(dlindex dl) {
	lock_guard<mutex> lock(mx);
	package_container::iterator it = package_by_id(dl.first);
	if(it == packages.end()) return NULL;
	return (*it)->get_handle(dl.second);
}

bool package_container::get_can_resume(dlindex dl) {
	lock_guard<mutex> lock(mx);
	package_container::iterator it = package_by_id(dl.first);
	if(it == packages.end()) return false;
	return (*it)->get_can_resume(dl.second);
}

int package_container::get_speed(dlindex dl) {
	lock_guard<mutex> lock(mx);
	package_container::iterator it = package_by_id(dl.first);
	if(it == packages.end()) return 0;
	return (*it)->get_speed(dl.second);
}

download_status package_container::get_status(dlindex dl) {
	lock_guard<mutex> lock(mx);
	package_container::iterator it = package_by_id(dl.first);
	if(it == packages.end()) return DOWNLOAD_DELETED;
	return (*it)->get_status(dl.second);
}

bool package_container::get_need_stop(dlindex dl) {
	lock_guard<mutex> lock(mx);
	package_container::iterator it = package_by_id(dl.first);
	if(it == packages.end()) return true;
	return (*it)->get_need_stop(dl.second);
}

bool package_container::get_running(dlindex dl) {
	lock_guard<mutex> lock(mx);
	package_container::iterator it = package_by_id(dl.first);
	if(it == packages.end()) return false;
	return (*it)->get_running(dl.second);
}

std::string package_container::get_output_file(dlindex dl) {
	lock_guard<mutex> lock(mx);
	package_container::iterator it = package_by_id(dl.first);
	if(it == packages.end()) return "";
	return (*it)->get_output_file(dl.second);
}

plugin_status package_container::get_error(dlindex dl) {
	lock_guard<mutex> lock(mx);
	package_container::iterator it = package_by_id(dl.first);
	if(it == packages.end()) return PLUGIN_ERROR;
	return (*it)->get_error(dl.second);
}

int package_container::get_wait(dlindex dl) {
	lock_guard<mutex> lock(mx);
	package_container::iterator it = package_by_id(dl.first);
	if(it == packages.end()) return 0;
	return (*it)->get_wait(dl.second);
}

uint64_t package_container::get_size(dlindex dl) {
	lock_guard<mutex> lock(mx);
	package_container::iterator it = package_by_id(dl.first);
	if(it == packages.end()) return 0;
	return (*it)->get_size(dl.second);
}

uint64_t package_container::get_downloaded_bytes(dlindex dl) {
	lock_guard<mutex> lock(mx);
	package_container::iterator it = package_by_id(dl.first);
	if(it == packages.end()) return 0;
	return (*it)->get_downloaded_bytes(dl.second);
}

std::string package_container::get_add_date(dlindex dl) {
	lock_guard<mutex> lock(mx);
	package_container::iterator it = package_by_id(dl.first);
	if(it == packages.end()) return "";
	return (*it)->get_add_date(dl.second);
}

std::string package_container::get_title(dlindex dl) {
	lock_guard<mutex> lock(mx);
	package_container::iterator it = package_by_id(dl.first);
	if(it == packages.end()) return "";
	return (*it)->get_title(dl.second);
}

std::string package_container::get_url(dlindex dl) {
	lock_guard<mutex> lock(mx);
	package_container::iterator it = package_by_id(dl.first);
	if(it == packages.end()) return "";
	return (*it)->get_url(dl.second);
}

void package_container::init_handle(dlindex dl) {
	lock_guard<mutex> lock(mx);
	package_container::iterator it = package_by_id(dl.first);
	if(it == packages.end()) return;
	(*it)->download_mutex.lock();
	download_container::iterator dlit = (*it)->get_download_by_id(dl.second);
	if((*dlit)->handle == NULL)
		(*dlit)->handle = curl_easy_init();
	(*it)->download_mutex.unlock();
	dump_to_file(false);
}

void package_container::cleanup_handle(dlindex dl) {
	lock_guard<mutex> lock(mx);
	package_container::iterator it = package_by_id(dl.first);
	if(it == packages.end()) return;
	(*it)->download_mutex.lock();
	download_container::iterator dlit = (*it)->get_download_by_id(dl.second);
	if((*dlit)->handle != NULL)
		curl_easy_cleanup((*dlit)->handle);
	(*dlit)->handle = NULL;
	(*it)->download_mutex.unlock();
	if(reconnect_needed()) {
		thread t(&package_container::do_reconnect, this);
		t.detach();
	}
	dump_to_file(false);
}

int package_container::prepare_download(dlindex dl, plugin_output &poutp) {
	unique_lock<mutex> plock(plugin_mutex);
	unique_lock<mutex> lock(mx);
	package_container::iterator it = package_by_id(dl.first);
	unique_lock<mutex> container_lock((*it)->download_mutex);
	plugin_input pinp;
	download_container::iterator dlit = (*it)->get_download_by_id(dl.second);

	string pluginfile(get_plugin_file(dlit));

	// If the generic plugin is used (no real host-plugin is found), we do "parsing" right here
	if(pluginfile.empty()) {
		log_string("No plugin found, using generic download", LOG_WARNING);
		poutp.download_url = (*dlit)->url.c_str();
		return PLUGIN_SUCCESS;
	}

	// Load the plugin function needed
	void* handle = dlopen(pluginfile.c_str(), RTLD_LAZY);
	if (!handle) {
		log_string(std::string("Unable to open library file: ") + dlerror(), LOG_ERR);
		return PLUGIN_ERROR;
	}

	dlerror();	// Clear any existing error

	plugin_status (*plugin_exec_func)(download_container&, int, plugin_input&, plugin_output&, int, std::string, std::string);
	plugin_exec_func = (plugin_status (*)(download_container&, int, plugin_input&, plugin_output&, int, std::string, std::string))dlsym(handle, "plugin_exec_wrapper");

	char *error;
	if ((error = dlerror()) != NULL)  {
		log_string(std::string("Unable to execute plugin: ") + error, LOG_ERR);
		dlclose(handle);
		return PLUGIN_ERROR;
	}

	pinp.premium_user = global_premium_config.get_cfg_value((*dlit)->get_host() + "_user");
	pinp.premium_password = global_premium_config.get_cfg_value((*dlit)->get_host() + "_password");
	trim_string(pinp.premium_user);
	trim_string(pinp.premium_password);

	// enable a proxy if neccessary
	string proxy_str = (*dlit)->proxy;
	if(!proxy_str.empty()) {
		size_t n;
		std::string proxy_ipport;
		if((n = proxy_str.find("@")) != string::npos &&  proxy_str.size() > n + 1) {
			curl_easy_setopt((*dlit)->handle, CURLOPT_PROXYUSERPWD, proxy_str.substr(0, n).c_str());
			curl_easy_setopt((*dlit)->handle, CURLOPT_PROXY, proxy_str.substr(n + 1).c_str());
			log_string("Setting proxy: " + proxy_str.substr(n + 1) + " for " + (*dlit)->url, LOG_DEBUG);
		} else {
			curl_easy_setopt((*dlit)->handle, CURLOPT_PROXY, proxy_str.c_str());
			log_string("Setting proxy: " + proxy_str + " for " + (*dlit)->url, LOG_DEBUG);
		}
	}

	container_lock.unlock();
	lock.unlock();

	plugin_status retval;
	try {
		retval = plugin_exec_func(**it, dl.second, pinp, poutp, global_config.get_int_value("captcha_retrys"),
                                  global_config.get_cfg_value("gocr_binary"), program_root);
	} catch(captcha_exception &e) {
		log_string("Failed to decrypt captcha. Giving up (" + pluginfile + ")", LOG_ERR);
		set_status(dl, DOWNLOAD_INACTIVE);
		retval = PLUGIN_ERROR;
	} catch(...) {
		retval = PLUGIN_ERROR;
	}

	dlclose(handle);
	lock.lock();
	correct_invalid_ids();
	return retval;
}

void package_container::post_process_download(dlindex dl) {
	unique_lock<mutex> plock(plugin_mutex);
	unique_lock<mutex> lock(mx);
	package_container::iterator it = package_by_id(dl.first);
	unique_lock<mutex> container_lock((*it)->download_mutex);
	plugin_input pinp;
	download_container::iterator dlit = (*it)->get_download_by_id(dl.second);

	string pluginfile(get_plugin_file(dlit));
	if(pluginfile.empty()) return;

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

	pinp.premium_user = global_premium_config.get_cfg_value((*dlit)->get_host() + "_user");
	pinp.premium_password = global_premium_config.get_cfg_value((*dlit)->get_host() + "_password");
	trim_string(pinp.premium_user);
	trim_string(pinp.premium_password);
	lock.unlock();
	plugin_mutex.lock();
	try {
		post_process_func(**it, dl.second, pinp);
	} catch(...) {}

	dlclose(handle);
	plugin_mutex.unlock();
	// lock, followed by scoped unlock. otherwise lock will be called 2 times
	lock.lock();
	correct_invalid_ids();
}


std::string package_container::get_host(dlindex dl) {
	lock_guard<mutex> lock(mx);
	package_container::iterator it = package_by_id(dl.first);
	return (*it)->get_host(dl.second);
}


plugin_output package_container::get_hostinfo(dlindex dl) {
	lock_guard<mutex> lock(mx);
	package_container::iterator it = package_by_id(dl.first);
	lock_guard<mutex> lockdl((*it)->download_mutex);
	download_container::iterator it2 = (*it)->get_download_by_id(dl.second);
	return (*it2)->get_hostinfo();
}

int package_container::total_downloads() {
	lock_guard<mutex> lock(mx);
	int total = 0;
	for(package_container::iterator it = packages.begin() + 1; it != packages.end(); ++it) {
		total += (*it)->total_downloads();
	}
	return total;
}

void package_container::decrease_waits() {
	lock_guard<mutex> lock(mx);
	for(package_container::iterator it = packages.begin() + 1; it != packages.end(); ++it) {
		(*it)->decrease_waits();
	}
}

void package_container::purge_deleted() {
	lock_guard<mutex> lock(mx);
	for(package_container::iterator it = packages.begin() + 1; it != packages.end(); ++it) {
		(*it)->purge_deleted();
	}
}

std::string package_container::create_client_list() {
	lock_guard<mutex> lock(mx);
	std::string list;
	for(package_container::iterator it = packages.begin() + 1; it != packages.end(); ++it) {
		list += "PACKAGE|" + int_to_string((*it)->container_id)  + "|" + (*it)->name + "\n";
		list += (*it)->create_client_list();
	}
	trim_string(list);
	return list;
}

bool package_container::url_is_in_list(std::string url) {
	lock_guard<mutex> lock(mx);
	for(package_container::iterator it = packages.begin() + 1; it != packages.end(); ++it) {
		if((*it)->url_is_in_list(url))
			return true;
	}
	return false;
}


void  package_container::move_dl(dlindex dl, package_container::direction d) {
	lock_guard<mutex> lock(mx);
	package_container::iterator it = package_by_id(dl.first);
	if(it == packages.end()) return;
	if(d == DIRECTION_UP) (*it)->move_up(dl.second);
	else (*it)->move_down(dl.second);
}

void  package_container::move_pkg(int dl, package_container::direction d) {
	lock_guard<mutex> lock(mx);
	if(dl == -1) return;
	package_container::iterator it = package_by_id(dl);
	if(it == packages.end()) return;
	package_container::iterator it2 = it;
	if(d == DIRECTION_UP && it != packages.begin() + 1) {
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
			if((*it)->get_status() == DOWNLOAD_WAITING && (*it)->error == PLUGIN_LIMIT_REACHED) {
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
	unique_lock<mutex> lock(mx);
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
				(*pkg)->set_dl_status(it, DOWNLOAD_RECONNECTING);
				(*it)->wait_seconds = 0;
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
				(*pkg)->set_dl_status(it, DOWNLOAD_PENDING);
				(*it)->wait_seconds = 0;
			}
		}
		(*pkg)->download_mutex.unlock();
	}
	is_reconnecting = false;
	return;
}

void package_container::dump_to_file(bool do_lock) {
	unique_lock<mutex> lock(mx, defer_lock);
	if(do_lock) {
		lock.lock();
	}
	ofstream ofs(list_file.c_str(), ios::trunc);
	for(package_container::iterator pkg = packages.begin() + 1; pkg != packages.end(); ++pkg) {
		(*pkg)->download_mutex.lock();
		ofs << "PKG:" << (*pkg)->container_id << ":" << (*pkg)->name << endl;
		for(download_container::iterator it = (*pkg)->download_list.begin(); it != (*pkg)->download_list.end(); ++it) {
			ofs << (*it)->serialize();
		}
		(*pkg)->download_mutex.unlock();
	}

}

void package_container::wait(dlindex dl) {
	mx.lock();
	package_container::iterator it = package_by_id(dl.first);
	mx.unlock();
	(*it)->wait(dl.second);
}

int package_container::pkg_that_contains_download(int download_id) {
	lock_guard<mutex> lock(mx);
	for(package_container::iterator it = packages.begin(); it != packages.end(); ++it) {
		lock_guard<mutex> dllock((*it)->download_mutex);
		for(download_container::iterator dlit = (*it)->download_list.begin(); dlit != (*it)->download_list.end(); ++dlit) {
			if((*dlit)->id == download_id) return (*it)->container_id;
		}
	}
	return LIST_ID;
}

bool package_container::pkg_exists(int id) {
	if(id < 0) return false;
	lock_guard<mutex> lock(mx);
	for(package_container::iterator it = packages.begin(); it != packages.end(); ++it) {
		if((*it)->container_id == id) return true;
	}
	return false;
}

int package_container::set_next_proxy(dlindex id) {
	lock_guard<mutex> lock(mx);
	package_container::iterator it = package_by_id(id.first);
	return (*it)->set_next_proxy(id.second);
}

bool package_container::package_finished(int id) {
	lock_guard<mutex> lock(mx);
	package_container::iterator pkg_it = package_by_id(id);
	lock_guard<mutex> listlock((*pkg_it)->download_mutex);
	for(download_container::iterator it = (*pkg_it)->download_list.begin(); it != (*pkg_it)->download_list.end(); ++it) {
		if((*it)->status != DOWNLOAD_FINISHED)
			return false;
	}
	return true;
}

void package_container::extract_package(int id) {
	if(!global_config.get_bool_value("enable_pkg_extractor")) return;
	unique_lock<mutex> lock(mx);
	package_container::iterator pkg_it = package_by_id(id);
	FILE* extractor;
	string to_exec;
	unique_lock<mutex> pkg_lock((*pkg_it)->download_mutex);
	download_container::iterator it = (*pkg_it)->download_list.begin();
	if((*pkg_it)->download_list.size() == 0) return;
	string output_file = (*it)->output_file;
	string extension(output_file.substr(output_file.find_last_of(".")));
	string output_dir(output_file.substr(0, output_file.find_last_of(".")));
	pkg_lock.unlock();
	lock.unlock();
	string password_list = global_config.get_cfg_value("pkg_extractor_passwords");
	string password;
	while(true) {
		trim_string(password);
		if(extension == ".rar") {
			to_exec = "unrar x";
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
		} else if(extension == ".gz" && output_file.find(".tar.gz") == output_file.size() - 7) {
			to_exec = "tar xzf '" + output_file + "' -C '" + output_dir + "'";
		} else if(extension == ".bz2" && output_file.find(".tar.bz2") == output_file.size() - 8) {
			to_exec = "tar xjf '" + output_file + "' -C '" + output_dir + "'";
		}
		if(to_exec.empty()) return;
		to_exec += " 2>&1";

		mkdir_recursive(output_dir);
		log_string("extracting... " + to_exec, LOG_DEBUG);
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
				remove((*it)->output_file.c_str());
				(*it)->output_file = "";
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
			if((*cnt_it)->id > max_here) {
				max_here = (*cnt_it)->id;
			}
		}
		if(max_here > max_id) max_id = max_here;
		if(lock_download_mutex) (*it)->download_mutex.unlock();
	}
	return ++max_id;
}

package_container::iterator package_container::package_by_id(int pkg_id) {
	package_container::iterator it = packages.begin();
	for(; it != packages.end(); ++it) {
		if((*it)->container_id == pkg_id) {
			break;
		}
	}
	return it;
}

int package_container::get_next_id() {
	int max_id = -1;
	for(package_container::iterator it = packages.begin(); it != packages.end(); ++it) {
		if((*it)->container_id > max_id) {
			max_id = (*it)->container_id;
		}
	}
	return max_id + 1;
}

std::string package_container::get_plugin_file(download_container::iterator dlit) {
	std::string host((*dlit)->get_host());
	if(host == "") {
		return "";
	}

	std::string plugindir = program_root + "/plugins/";
	correct_path(plugindir);
	plugindir += '/';


	std::string pluginfile(plugindir + "lib" + host + ".so");

	struct stat st;

	if(stat(pluginfile.c_str(), &st) != 0) {
		return "";
	}
	return pluginfile;
}

void package_container::correct_invalid_ids() {
	// sadly there is no other threadsafe way than looping 3 times - first lock, then do, then unlock
	// the problem is that get_next_download_id accesses ALL packages. so all of them need to be locked.
	// it can also lock automatically, but then I'd need to unlock before calling it, which might make
	// the iterators invalid -> no option
	for(package_container::iterator it = packages.begin(); it != packages.end(); ++it) {
		(*it)->download_mutex.lock();
	}
	for(package_container::iterator it = packages.begin(); it != packages.end(); ++it) {
		for(download_container::iterator dlit = (*it)->download_list.begin(); dlit != (*it)->download_list.end(); ++dlit) {
			if((*dlit)->id == -1) {
				(*dlit)->id = get_next_download_id(false);
			}
		}
		(*it)->download_mutex.unlock();
	}
	for(package_container::iterator it = packages.begin(); it != packages.end(); ++it) {
		(*it)->download_mutex.unlock();
	}
}

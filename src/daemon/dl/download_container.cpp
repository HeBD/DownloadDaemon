#include <sys/types.h>
#include <sys/stat.h>
#include <dlfcn.h>

#include <sstream>

#include "download_container.h"
#include "download.h"
#include "../../lib/cfgfile/cfgfile.h"
#include "../../lib/mt_string/mt_string.h"
#include "../tools/helperfunctions.h"

using namespace std;

#ifndef IS_PLUGIN
extern mt_string program_root;
extern cfgfile global_config;
#endif // IS_PLUGIN

#ifndef IS_PLUGIN
download_container::download_container(const char* filename) {
	from_file(filename);
}

int download_container::from_file(const char* filename) {
	boost::mutex::scoped_lock lock(download_mutex);
	list_file = filename;
	ifstream dlist(filename);
	mt_string line;
	while(getline(dlist, line)) {
		download_list.push_back(download(line));
	}
	if(!dlist.good()) {
		dlist.close();
		return LIST_PERMISSION;
	}
	dlist.close();
	return LIST_SUCCESS;
}

int download_container::total_downloads() {
	boost::mutex::scoped_lock lock(download_mutex);
	return download_list.size();
}

int download_container::add_download(const download &dl) {
	boost::mutex::scoped_lock lock(download_mutex);
	download_list.push_back(dl);
	if(!dump_to_file()) {
		download_list.pop_back();
		return LIST_PERMISSION;
	}
	return LIST_SUCCESS;
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

	swap(download_list[location1], download_list[location2]);
	if(!dump_to_file()) {
		swap(download_list[location1], download_list[location2]);
		return LIST_PERMISSION;
	}
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
	if(it == download_list.end() || it == download_list.end() - 1) {
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

	swap(download_list[location1], download_list[location2]);
	if(!dump_to_file()) {
		swap(download_list[location1], download_list[location2]);
		return LIST_PERMISSION;
	}
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
		it->set_status(DOWNLOAD_PENDING);
		if(!dump_to_file()) {
			it->set_status(DOWNLOAD_INACTIVE);
			return LIST_PERMISSION;
		} else {
			return LIST_SUCCESS;
		}
	}
}

int download_container::deactivate(int id) {
	boost::mutex::scoped_lock lock(download_mutex);
	download_container::iterator it = get_download_by_id(id);
	if(it == download_list.end()) {
		return LIST_ID;
	} else {
		download_status old_status = it->get_status();
		if(it->get_status() == DOWNLOAD_INACTIVE) {
			return LIST_PROPERTY;
		}
		if(it->get_status() == DOWNLOAD_RUNNING) {
			it->set_status(DOWNLOAD_INACTIVE);
			curl_easy_setopt(it->handle, CURLOPT_TIMEOUT, 1);
		} else {
			it->set_status(DOWNLOAD_INACTIVE);
		}
		it->wait_seconds = 0;
		if(!dump_to_file()) {
			it->set_status(old_status);
			return LIST_PERMISSION;
		} else {
			return LIST_SUCCESS;
		}
	}
}

int download_container::get_next_downloadable() {
	boost::mutex::scoped_lock lock(download_mutex);
	download_container::iterator downloadable = download_list.end();
	if(download_list.empty() || running_downloads() >= atoi(global_config.get_cfg_value("simultaneous_downloads").c_str())
	   || global_config.get_cfg_value("downloading_active") == "0") {
		return LIST_ID;
	}

	// Checking if we are in download-time...
	mt_string dl_start(global_config.get_cfg_value("download_timing_start"));
	if(global_config.get_cfg_value("download_timing_start").find(':') != mt_string::npos && !global_config.get_cfg_value("download_timing_end").find(':') != mt_string::npos ) {
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
		if(it->get_status() == DOWNLOAD_PENDING) {
			mt_string current_host(it->get_host());
			bool can_attach = true;
			for(download_container::iterator it2 = download_list.begin(); it2 != download_list.end(); ++it2) {
				if(it2->get_host() == current_host && (it2->get_status() == DOWNLOAD_RUNNING || it2->get_status() == DOWNLOAD_WAITING) && !it2->get_hostinfo().allows_multiple) {
					can_attach = false;
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
int download_container::set_string_property(int id, string_property prop, mt_string value) {
	boost::mutex::scoped_lock lock(download_mutex);
	download_container::iterator dl = get_download_by_id(id);
	if(dl == download_list.end()) {
		return LIST_ID;
	}
	mt_string old_val = value;
	switch(prop) {
		case DL_URL:
			dl->url = value;
			#ifndef IS_PLUGIN
			if(!dump_to_file()) {
				dl->url = old_val;
				return LIST_PERMISSION;
			}
			#endif
		break;
		case DL_COMMENT:
			dl->comment = value;
			#ifndef IS_PLUGIN
			if(!dump_to_file()) {
				dl->comment = old_val;
				return LIST_PERMISSION;
			}
			#endif
		break;
		case DL_ADD_DATE:
			dl->add_date = value;
			#ifndef IS_PLUGIN
			if(!dump_to_file()) {
				dl->add_date = old_val;
				return LIST_PERMISSION;
			}
			#endif
		break;
		case DL_OUTPUT_FILE:
			dl->output_file = value;
			#ifndef IS_PLUGIN
			if(!dump_to_file()) {
				dl->output_file = old_val;
				return LIST_PERMISSION;
			}
			#endif
		break;
	}
	return LIST_SUCCESS;
}

int download_container::set_int_property(int id, property prop, int value) {
	boost::mutex::scoped_lock lock(download_mutex);
	download_container::iterator dl = get_download_by_id(id);
	if(dl == download_list.end()) {
		return LIST_ID;
	}
	#ifndef IS_PLUGIN
	int old_val = value;
	#endif
	switch(prop) {
		case  DL_ID:
			dl->id = value;
			#ifndef IS_PLUGIN
			if(!dump_to_file()) {
				dl->id = old_val;
				return LIST_PERMISSION;
			}
			#endif
		break;
		case DL_DOWNLOADED_BYTES:
			dl->downloaded_bytes = value;
			#ifndef IS_PLUGIN
			if(!dump_to_file()) {
				dl->downloaded_bytes = old_val;
				return LIST_PERMISSION;
			}
			#endif
		break;
		case DL_SIZE:
			dl->size = value;
			#ifndef IS_PLUGIN
			if(!dump_to_file()) {
				dl->size = old_val;
				return LIST_PERMISSION;
			}
			#endif
		break;
		case DL_WAIT_SECONDS:
			dl->wait_seconds = value;
			#ifndef IS_PLUGIN
			if(!dump_to_file()) {
				dl->wait_seconds = old_val;
				return LIST_PERMISSION;
			}
			#endif
		break;
		case DL_PLUGIN_STATUS:
			dl->error = plugin_status(value);
			#ifndef IS_PLUGIN
			if(!dump_to_file()) {
				dl->error = plugin_status(old_val);
				return LIST_PERMISSION;
			}
			#endif
		break;
		case DL_STATUS:
			dl->set_status(download_status(value));
			#ifndef IS_PLUGIN
			if(!dump_to_file()) {
				dl->set_status(download_status(old_val));
				return LIST_PERMISSION;
			}
			#endif
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
	switch(prop) {
		case DL_HANDLE:
			curl_easy_cleanup(dl->handle);
			dl->handle = value;
		break;
	}
	return LIST_SUCCESS;
}

int download_container::get_int_property(int id, property prop) {
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
	}
	return -1;
}

mt_string download_container::get_string_property(int id, string_property prop) {
	boost::mutex::scoped_lock lock(download_mutex);
	download_container::iterator dl = get_download_by_id(id);
	if(dl == download_list.end()) {
		throw download_exception((string("Invalid download ID: ") + id). c_str());
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
int download_container::prepare_download(int dl, plugin_output &poutp) {
	boost::mutex::scoped_lock lock(download_mutex);
	plugin_input pinp;
	download_container::iterator dlit = get_download_by_id(dl);
	curl_easy_reset(dlit->handle);

	mt_string host(dlit->get_host());
	mt_string plugindir = global_config.get_cfg_value("plugin_dir");
	correct_path(plugindir);
	if(host == "") {
		return PLUGIN_INVALID_HOST;
	}

	struct stat st;
	if(stat(plugindir.c_str(), &st) != 0) {
		return PLUGIN_INVALID_PATH;
	}

	mt_string pluginfile(plugindir + "lib" + host + ".so");
	bool use_generic = false;
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
		log_string(mt_string("Unable to open library file: ") + dlerror() + '/' + pluginfile, LOG_SEVERE);
        return PLUGIN_ERROR;
    }

	dlerror();    // Clear any existing error

	plugin_status (*plugin_exec_func)(download_container&, int, plugin_input&, plugin_output&);
    plugin_exec_func = (plugin_status (*)(download_container&, int, plugin_input&, plugin_output&))dlsym(handle, "plugin_exec_wrapper");

    char *error;
    if ((error = dlerror()) != NULL)  {
    	log_string(mt_string("Unable to execute plugin: ") + error, LOG_SEVERE);
    	return PLUGIN_ERROR;
    }

	download_mutex.unlock();
	plugin_status retval = plugin_exec_func(*this, dl, pinp, poutp);
    dlclose(handle);
    return retval;
}

plugin_output download_container::get_hostinfo(int dl) {
	boost::mutex::scoped_lock lock(download_mutex);
	download_container::iterator it = get_download_by_id(dl);
	return it->get_hostinfo();
}

mt_string download_container::get_host(int dl) {
	boost::mutex::scoped_lock lock(download_mutex);
	download_container::iterator it = get_download_by_id(dl);
	return it->get_host();
}

void download_container::decrease_waits() {
	boost::mutex::scoped_lock lock(download_mutex);
	for(download_container::iterator it = download_list.begin(); it != download_list.end(); ++it) {
		if(it->wait_seconds > 0 && it->get_status() == DOWNLOAD_WAITING) {
			--(it->wait_seconds);
		} else if(it->wait_seconds == 0 && it->get_status() == DOWNLOAD_WAITING) {
			it->set_status(DOWNLOAD_PENDING);
		} else if(it->wait_seconds > 0 && it->get_status() == DOWNLOAD_INACTIVE) {
			it->wait_seconds = 0;
		}
	}
}

void download_container::purge_deleted() {
	boost::mutex::scoped_lock lock(download_mutex);
	if(download_list.empty()) {
		return;
	}
	for(download_container::iterator it = download_list.begin(); it != download_list.end(); ++it) {
		if(it->get_status() == DOWNLOAD_DELETED && it->is_running == false) {
			remove_download(it->id);
			it = download_list.begin();
		}
	}
}

mt_string download_container::create_client_list() {
	boost::mutex::scoped_lock lock(download_mutex);

	std::stringstream ss;

	for(download_container::iterator it = download_list.begin(); it != download_list.end(); ++it) {
		if(it->get_status() == DOWNLOAD_DELETED) {
			continue;
		}
		ss << it->id << '|' << it->add_date << '|';
		mt_string comment = it->comment;
		replace_all(comment, "|", "\\|");
		ss << comment << '|' << it->url << '|' << it->get_status_str() << '|' << it->downloaded_bytes << '|' << it->size
		   << '|' << it->wait_seconds << '|' << it->get_error_str() << '\n';
	}
	//log_string("Dumping download list to client", LOG_DEBUG);
	return ss.str();
}

int download_container::get_next_id() {
	boost::mutex::scoped_lock lock(download_mutex);
	int max_id = -1;
	for(download_container::iterator it = download_list.begin(); it != download_list.end(); ++it) {
		if(it->id > max_id) {
			max_id = it->id;
		}
	}
	return ++max_id;
}

int download_container::stop_download(int id) {
	download_container::iterator it = get_download_by_id(id);
	if(it == download_list.end() || it->get_status() == DOWNLOAD_DELETED) {
		return LIST_ID;
	} else {
		curl_easy_setopt(it->handle, CURLOPT_TIMEOUT, 1);
		if(it->get_status() != DOWNLOAD_INACTIVE) it->set_status(DOWNLOAD_PENDING);
		dump_to_file();
		return LIST_SUCCESS;
	}
}
#endif
download_container::iterator download_container::get_download_by_id(int id) {
	for(download_container::iterator it = download_list.begin(); it != download_list.end(); ++it) {
		if(it->id == id) {
			return it;
		}
	}
	return download_list.end();
}
#ifndef IS_PLUGIN
bool download_container::dump_to_file() {
	if(list_file == "") {
		return false;
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
	int running_downloads = 0;
	for(download_container::iterator it = download_list.begin(); it != download_list.end(); ++it) {
		if(it->get_status() == DOWNLOAD_RUNNING) {
			++running_downloads;
		}
	}
	return running_downloads;
}

int download_container::remove_download(int id) {
	download_container::iterator it = get_download_by_id(id);
	if(it == download_list.end()) {
		return LIST_ID;
	}

	download tmpdl = *it;
	download_list.erase(it);
	if(!dump_to_file()) {
		download_list.push_back(tmpdl);
		return LIST_PERMISSION;
	}
	return LIST_SUCCESS;
}
#endif

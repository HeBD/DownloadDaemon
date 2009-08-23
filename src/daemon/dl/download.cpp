#include <sys/types.h>
#include <sys/stat.h>
#include <dlfcn.h>

#include "download.h"
#include "download_container.h"
#include "../../lib/cfgfile/cfgfile.h"
#include "../tools/curl_callbacks.h"
#include "../tools/helperfunctions.h"

#include <string>
#include <ctime>
#include <cstdlib>
#include <fstream>
#include <sstream>

using namespace std;

extern cfgfile global_config;
extern std::string program_root;
extern download_container global_download_list;

download::download(std::string &url, int next_id)
	: url(url), id(next_id), downloaded_bytes(0), size(1), wait_seconds(0), error(PLUGIN_SUCCESS), status(DOWNLOAD_PENDING) {
	handle = curl_easy_init();
	time_t rawtime;
	time(&rawtime);
	add_date = ctime(&rawtime);
	add_date.erase(add_date.length() - 1);
}

download::download(std::string &serializedDL) {
	from_serialized(serializedDL);
}

void download::from_serialized(std::string &serializedDL) {
	std::string current_entry;
	size_t curr_pos = 0;
	size_t entry_num = 0;
	while(serializedDL.find('|', curr_pos) != string::npos) {
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
	handle = curl_easy_init();
}

std::string download::serialize() {
	if(status == DOWNLOAD_DELETED) {
		return "";
	}
	stringstream ss;
	ss << id << '|' << add_date << '|' << comment << '|' << url << '|' << status << '|' << downloaded_bytes
	   << '|' << size << '|' << output_file << "|\n";
	return ss.str();
}

plugin_status download::get_download(plugin_output &poutp) {
	plugin_input pinp;
	curl_easy_reset(handle);

	string host(get_host());
	string plugindir = global_config.get_cfg_value("plugin_dir");
	correct_path(plugindir);
	if(host == "") {
		return PLUGIN_INVALID_HOST;
	}

	struct stat st;
	if(stat(plugindir.c_str(), &st) != 0) {
		return PLUGIN_INVALID_PATH;
	}

	string pluginfile(plugindir + host + ".so");
	bool use_generic = false;
	if(stat(pluginfile.c_str(), &st) != 0) {
		use_generic = true;
	}

	// If the generic plugin is used (no real host-plugin is found), we do "parsing" right here
	if(use_generic) {
	    log_string("No plugin found, using generic download", LOG_WARNING);
	    poutp.download_url = url.c_str();
		return PLUGIN_SUCCESS;
	}

	// Load the plugin function needed
	void* handle = dlopen(pluginfile.c_str(), RTLD_LAZY);
    if (!handle) {
		log_string(string("Unable to open library file: ") + dlerror() + '/' + pluginfile, LOG_SEVERE);
        return PLUGIN_ERROR;
    }

	dlerror();    // Clear any existing error

	plugin_status (*plugin_exec_func)(download&, CURL*, plugin_input&, plugin_output&);
    plugin_exec_func = (plugin_status (*)(download&, CURL*, plugin_input&, plugin_output&))dlsym(handle, "plugin_exec");

    char *error;
    if ((error = dlerror()) != NULL)  {
    	log_string(string("Unable to execute plugin: ") + error, LOG_SEVERE);
    	return PLUGIN_ERROR;
    }

	plugin_status retval = plugin_exec_func(*this, handle, pinp, poutp);
    dlclose(handle);
    return retval;
}


std::string download::get_host() {
	size_t startpos = 0;

	if(url.find("www.") != std::string::npos) {
		startpos = url.find('.') + 1;
	} else if(url.find("http://") != std::string::npos || url.find("ftp://") != std::string::npos) {
		startpos = url.find('/') + 2;
	} else {
		return "";
	}
	return url.substr(startpos, url.find('/', startpos) - startpos);

}

const char* download::get_error_str() {
	switch(error) {
		case PLUGIN_SUCCESS:
			return "PLUGIN_SUCCESS";
		case PLUGIN_MISSING:
			return "PLUGIN_MISSING";
		case PLUGIN_INVALID_HOST:
			return "PLUGIN_INVALID_HOST";
		case PLUGIN_INVALID_PATH:
			return "PLUGIN_INVALID_PATH";
		case PLUGIN_CONNECTION_LOST:
			return "PLUGIN_CONNECTION_LOST";
		case PLUGIN_FILE_NOT_FOUND:
			return "PLUGIN_FILE_NOT_FOUND";
		case PLUGIN_WRITE_FILE_ERROR:
			return "PLUGIN_WRITE_FILE_ERROR";
		case PLUGIN_ERROR:
			return "PLUGIN_ERROR";
		case PLUGIN_LIMIT_REACHED:
			return "PLUGIN_LIMIT_REACHED";
		case PLUGIN_CONNECTION_ERROR:
			return "PLUGIN_CONNECTION_ERROR";
		case PLUGIN_SERVER_OVERLOADED:
			return "PLUGIN_SERVER_OVERLOADED";
	}
	return "PLUGIN_UNKNOWN_ERROR";
}

const char* download::get_status_str() {
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
	return "UNKNOWN_STATUS";
}

void download::set_status(download_status st, bool force) {
	if(force) {
		status = st;
		return;
	}

	if(status == DOWNLOAD_DELETED) {
		return;
	} else if(st == DOWNLOAD_DELETED && status != DOWNLOAD_RUNNING) {
		status = st;
		curl_easy_cleanup(handle);
		global_download_list.erase(global_download_list.get_download_by_id(id));
	} else {
		status = st;
		global_download_list.dump_to_file();
	}
}

download_status download::get_status() {
	return status;
}

plugin_output download::get_hostinfo() {
	plugin_input inp;
	plugin_output outp;
	outp.allows_resumption = false;
	outp.allows_multiple = false;

	string host(get_host());
	string plugindir = global_config.get_cfg_value("plugin_dir");
	correct_path(plugindir);
	if(host == "") {
		return outp;
	}

	struct stat st;
	if(stat(plugindir.c_str(), &st) != 0) {
		return outp;
	}

	string pluginfile(plugindir + host + ".so");
	bool use_generic = false;
	if(stat(pluginfile.c_str(), &st) != 0) {
		use_generic = true;
	}

	// If the generic plugin is used (no real host-plugin is found), we do "parsing" right here
	if(use_generic) {
	    log_string("No plugin found, using generic download", LOG_WARNING);
	    outp.allows_multiple = true;
	    outp.allows_resumption = true;
		return outp;
	}

	// Load the plugin function needed
	void* handle = dlopen(pluginfile.c_str(), RTLD_LAZY);
    if (!handle) {
		log_string(string("Unable to open library file: ") + dlerror() + '/' + pluginfile, LOG_SEVERE);
        return outp;
    }

	dlerror();    // Clear any existing error

	void (*plugin_getinfo)(plugin_input&, plugin_output&);
    plugin_getinfo = (void (*)(plugin_input&, plugin_output&))dlsym(handle, "plugin_getinfo");

    char *error;
    if ((error = dlerror()) != NULL)  {
    	log_string(string("Unable to get plugin information: ") + error, LOG_SEVERE);
    	return outp;
    }

	plugin_getinfo(inp, outp);
    dlclose(handle);
    return outp;
}


bool operator<(const download& x, const download& y) {
	return x.id < y.id;
}

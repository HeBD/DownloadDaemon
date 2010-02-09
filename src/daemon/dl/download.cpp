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

#include "download.h"
#include "download_container.h"
#ifndef IS_PLUGIN
#include "../../lib/cfgfile/cfgfile.h"
#include "../tools/curl_callbacks.h"
#include "../tools/helperfunctions.h"
#include "../global.h"
#endif

#include <string>
#include <ctime>
#include <cstdlib>
#include <fstream>
#include <sstream>

using namespace std;

download::download(const std::string& dl_url)
	: url(dl_url), id(0), downloaded_bytes(0), size(1), wait_seconds(0), error(PLUGIN_SUCCESS),
	is_running(false), need_stop(false), status(DOWNLOAD_PENDING), speed(0), can_resume(true) {
	time_t rawtime;
	struct tm* timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	char timestr[20];
	strftime(timestr, 20, "%Y-%m-%d %X", timeinfo);
	add_date = timestr;
	//add_date.erase(add_date.length() - 1);
	is_init = false;
}

//download::download(std::string& serializedDL) {
//	from_serialized(serializedDL);
//}

#ifndef IS_PLUGIN
void download::from_serialized(std::string& serializedDL) {
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
	is_init = false;
	can_resume = true;
}
#endif

download::download(const download& dl) {
	operator=(dl);
}

void download::operator=(const download& dl) {
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

	// handle and is_init are muteable members. on copy, ownership of the handle is taken to the new download object
	// instead of really copying. This is required to make moving downloads up and down in the download list possible.
	handle = dl.handle;
	is_init = dl.is_init;
	dl.handle = NULL;
	dl.is_init = false;
}

download::~download() {
}

std::string download::serialize() {
	if(status == DOWNLOAD_DELETED) {
		return "";
	}
	stringstream ss;
	ss << id << '|' << add_date << '|' << comment << '|' << url << '|' << status << '|'
	<< downloaded_bytes << '|' << size << '|' << output_file << "|\n";
	return ss.str();
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
	plugin_input inp;
	plugin_output outp;
	outp.allows_resumption = false;
	outp.allows_multiple = false;
	outp.offers_premium = false;

	std::string host(get_host());
	std::string plugindir = program_root + "/plugins/";
	correct_path(plugindir);
	if(host == "") {
		return outp;
	}

	struct stat st;
	if(stat(plugindir.c_str(), &st) != 0) {
		return outp;
	}

	std::string pluginfile(plugindir + "/lib" + host + ".so");
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
	void* l_handle = dlopen(pluginfile.c_str(), RTLD_LAZY);
	if (!l_handle) {
		log_string(std::string("Unable to open library file: ") + dlerror() + '/' + pluginfile, LOG_ERR);
		return outp;
	}

	dlerror();	// Clear any existing error

	void (*plugin_getinfo)(plugin_input&, plugin_output&);
	plugin_getinfo = (void (*)(plugin_input&, plugin_output&))dlsym(l_handle, "plugin_getinfo");

	char* l_error;
	if ((l_error = dlerror()) != NULL)  {
		log_string(std::string("Unable to get plugin information: ") + l_error, LOG_ERR);
		dlclose(l_handle);
		return outp;
	}
	inp.premium_user = global_premium_config.get_cfg_value(get_host() + "_user");
	inp.premium_password = global_premium_config.get_cfg_value(get_host() + "_password");
	plugin_getinfo(inp, outp);
	dlclose(l_handle);
	return outp;
}
#endif

bool operator<(const download& x, const download& y) {
	return x.id < y.id;
}

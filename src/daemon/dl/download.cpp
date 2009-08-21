#include <sys/types.h>
#include <sys/stat.h>

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
	: url(url), id(next_id), downloaded_bytes(0), size(1), wait_seconds(0), error(NO_ERROR), status(DOWNLOAD_PENDING) {
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
	error = NO_ERROR;
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

int download::get_download(parsed_download &parsed_dl) {
    parsed_dl.download_url = "";
    parsed_dl.download_filename = "";
    parsed_dl.cookie_file = "";
    parsed_dl.wait_before_download = 0;
    parsed_dl.download_parse_success = false;
    parsed_dl.download_parse_errmsg = "";
    parsed_dl.download_parse_wait = 0;
    parsed_dl.plugin_return_val = 0;

	string host(get_host());
	string plugindir = global_config.get_cfg_value("plugin_dir");
	correct_path(plugindir);
	if(host == "") {
		return INVALID_HOST;
	}

	struct stat st;
	if(stat(plugindir.c_str(), &st) != 0) {
		return INVALID_PLUGIN_PATH;
	}

	string pluginfile(plugindir + host);
	bool use_generic = false;
	if(stat(pluginfile.c_str(), &st) != 0) {
		use_generic = true;
	}

	if(stat(string(plugindir + "/plugin_comm").c_str(), &st) != 0) {
		mkdir(string(plugindir + "/plugin_comm").c_str(), 0755);
	}

	// If the generic plugin is used (no real host-plugin is found), we do "parsing" right here
	if(use_generic) {
	    log_string("No plugin found, using generic download", LOG_WARNING);
		parsed_dl.download_url = url;
		parsed_dl.download_parse_success = 1;
		return 0;
	}

	std::string ex(plugindir + '/' + host + ' ' + int_to_string(id) + ' ' + url);
	parsed_dl.plugin_return_val = system(ex.c_str());
	if(parsed_dl.plugin_return_val == 255) {
		parsed_dl.plugin_return_val = -1;
	}

	std::string tmpfile(plugindir + "/plugin_comm/" + int_to_string(id));
	cfgfile plugin_output(tmpfile, false);

	parsed_dl.download_url = plugin_output.get_cfg_value("download_url");
	parsed_dl.download_filename = plugin_output.get_cfg_value("download_filename");
	parsed_dl.cookie_file = plugin_output.get_cfg_value("cookie_file");
	parsed_dl.wait_before_download = atoi(plugin_output.get_cfg_value("wait_before_download").c_str());
	parsed_dl.download_parse_success = (bool)atoi(plugin_output.get_cfg_value("download_parse_success").c_str());
	parsed_dl.download_parse_errmsg = plugin_output.get_cfg_value("download_parse_errmsg");
	parsed_dl.download_parse_wait = atoi(plugin_output.get_cfg_value("download_parse_wait").c_str());

	remove(tmpfile.c_str());
	return 0;
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

hostinfo download::get_hostinfo() {
	string hostinfo_fn = global_config.get_cfg_value("plugin_dir") + "/hostinfo/" + get_host();
	correct_path(hostinfo_fn);
	hostinfo hinfo;
	struct stat st;
	if(stat(hostinfo_fn.c_str(), &st) != 0) {
		hinfo.offers_premium = false;
		hinfo.allows_multiple_downloads_free = true,
		hinfo.allows_download_resumption_free = true;
		hinfo.requires_cookie = false;
		return hinfo;
	}

	cfgfile hostinfo_file(hostinfo_fn, false);
	hinfo.offers_premium = (bool)atoi(hostinfo_file.get_cfg_value("offers_premium").c_str());
	hinfo.allows_multiple_downloads_free = (bool)atoi(hostinfo_file.get_cfg_value("allows_multiple_downloads_free").c_str());
	hinfo.allows_multiple_downloads_premium = (bool)atoi(hostinfo_file.get_cfg_value("allows_multiple_downloads_premium").c_str());
	hinfo.allows_download_resumption_free = (bool)atoi(hostinfo_file.get_cfg_value("allows_download_resumption_free").c_str());
	hinfo.allows_download_resumption_premium = (bool)atoi(hostinfo_file.get_cfg_value("allows_download_resumption_premium").c_str());
	hinfo.requires_cookie = (bool)atoi(hostinfo_file.get_cfg_value("requires_cookie").c_str());
	string auth_type = hostinfo_file.get_cfg_value("premium_auth_type");
	if(auth_type == "http-auth") {
		hinfo.premium_auth_type = HTTP_AUTH;
	} else {
		hinfo.premium_auth_type = NO_AUTH;
	}
	return hinfo;
}

const char* download::get_error_str() {
	switch(error) {
		case NO_ERROR:
			return "NO_ERROR";
		case MISSING_PLUGIN:
			return "MISSING_PLUGIN";
		case INVALID_HOST:
			return "INVALID_HOST";
		case INVALID_PLUGIN_PATH:
			return "INVALID_PLUGIN_PATH";
		case PLUGIN_ERROR:
			return "PLUGIN_ERROR";
		case CONNECTION_LOST:
			return "CONNECTION_LOST";
		case FILE_NOT_FOUND:
			return "FILE_NOT_FOUND";
		case WRITE_FILE_ERROR:
			return "WRITE_FILE_ERROR";
	}
	return "UNKNOWN_ERROR";
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


bool operator<(const download& x, const download& y) {
	return x.id < y.id;
}

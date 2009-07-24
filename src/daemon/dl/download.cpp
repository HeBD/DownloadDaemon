#include "download.h"
#include "../../lib/cfgfile.h"
#include "../tools/curl_callbacks.h"
#include "../tools/helperfunctions.h"
#include <string>
#include <boost/filesystem.hpp>
#include <ctime>
#include <cstdlib>
#include <fstream>
#include <sstream>

namespace bf = boost::filesystem;
using namespace std;

extern cfgfile global_config;
extern std::string program_root;

download::download(std::string &url, int next_id)
	: url(url), status(DOWNLOAD_PENDING), id(next_id), downloaded_bytes(0), size(1), wait_seconds(0), error(NO_ERROR) {
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
		}
		++curr_pos;
		++entry_num;
	}
	error = NO_ERROR;
	wait_seconds = 0;
	handle = curl_easy_init();
}

std::string download::serialize() {
	stringstream ss;
	ss << id << '|' << add_date << '|' << comment << '|' << url << '|' << status << '|' << downloaded_bytes
	   << '|' << size << '|';
	return ss.str();
}

int download::get_download(parsed_download &parsed_dl) {
	string host(get_host());
	string plugindir = program_root + global_config.get_cfg_value("plugin_dir");
	if(host == "") {
		return INVALID_HOST;
	}

	bf::path plugpath(plugindir);
	if(!bf::exists(plugpath)) {
		return INVALID_PLUGIN_PATH;
	}
	bool success = false;
	bf::directory_iterator end_itr;
	for(bf::directory_iterator itr(plugpath); itr != end_itr; ++itr) {
		if(itr->leaf() == host) {
			success = true;
			break;
		}
	}

	if(!success) {
		return MISSING_PLUGIN;
	}

	if(!bf::exists(plugpath / "plugin_comm")) {
		bf::create_directory(plugpath / "plugin_comm");
	}

	// should be packed in an extra thread so we can break up after a few seconds
	std::string ex(plugindir + '/' + host + ' ' + int_to_string(id) + ' ' + url);
	parsed_dl.plugin_return_val = system(ex.c_str());
	if(parsed_dl.plugin_return_val == 255) {
		parsed_dl.plugin_return_val = -1;
	}

	std::string tmpfile(plugindir + "/plugin_comm/" + int_to_string(id));
	cfgfile plugin_output(tmpfile, false);

	parsed_dl.download_url = plugin_output.get_cfg_value("download_url");
	parsed_dl.cookie_file = plugin_output.get_cfg_value("cookie_file");
	parsed_dl.wait_before_download = atoi(plugin_output.get_cfg_value("wait_before_download").c_str());
	parsed_dl.download_parse_success = (bool)atoi(plugin_output.get_cfg_value("download_parse_success").c_str());
	parsed_dl.download_parse_errmsg = plugin_output.get_cfg_value("download_parse_errmsg");
	parsed_dl.download_parse_wait = atoi(plugin_output.get_cfg_value("download_parse_wait").c_str());

	bf::remove(tmpfile);
	return 0;
}


std::string download::get_host() {
	size_t startpos = 0;

	if(url.find("www.") != std::string::npos) {
		startpos = url.find('.') + 1;
	} else if(url.find("http://") != std::string::npos) {
		startpos = url.find('/') + 2;
	} else {
		return "";
	}
	return url.substr(startpos, url.find('/', startpos) - startpos);

}

hostinfo download::get_hostinfo() {
	string hostinfo_fn = program_root + global_config.get_cfg_value("plugin_dir") + "/hostinfo/" + get_host();
	cfgfile hostinfo_file(hostinfo_fn, false);

	hostinfo hinfo;
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
	}
	return "UNKNOWN_STATUS";
}


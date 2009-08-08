#include <vector>
#include <string>

#include <curl/curl.h>

#include "../dl/download.h"
#include "../dl/download_container.h"
#include "../tools/helperfunctions.h"
#include "../../lib/cfgfile/cfgfile.h"
#include "global_management.h"

#include <sys/types.h>
#include <sys/stat.h>
using namespace std;

extern download_container global_download_list;
extern cfgfile global_config;
extern string program_root;

void tick_downloads() {
	while(true) {
		for(download_container::iterator it = global_download_list.begin(); it != global_download_list.end(); ++it) {
			if(it->wait_seconds > 0 && it->get_status() == DOWNLOAD_WAITING) {
				--it->wait_seconds;
				if(it->wait_seconds == 0) {
					it->set_status(DOWNLOAD_PENDING);
				}
			} else if(it->wait_seconds > 0 && it->get_status() == DOWNLOAD_INACTIVE) {
				it->wait_seconds = 0;
			}
		}

		sleep(1);
	}
}

void reconnect() {
	std::string reconnect_plugin;
	std::string reconnect_policy;
	while(true) {
		reconnect_plugin = global_config.get_cfg_value("reconnect_plugin");
		if(reconnect_plugin.empty()) {
			sleep(10);
			continue;
		}
		reconnect_policy = global_config.get_cfg_value("reconnect_policy");
		if(reconnect_policy.empty()) {
			sleep(10);
			continue;
		}
		bool reconnect_needed = false;
		bool reconnect_allowed = false;
		for(download_container::iterator it = global_download_list.begin(); it != global_download_list.end(); ++it) {
			if(it->get_status() == DOWNLOAD_WAITING) {
				reconnect_needed = true;
			}
		}
		if(!reconnect_needed) {
			sleep(10);
			continue;
		}

		// Always reconnect if needed
		if(reconnect_policy == "HARD") {
			reconnect_allowed = true;

		// Only reconnect if no non-continuable download is running
		} else if(reconnect_policy == "CONTINUE") {
			for(download_container::iterator it = global_download_list.begin(); it != global_download_list.end(); ++it) {
				if(it->get_status() == DOWNLOAD_RUNNING) {
					hostinfo hinfo = it->get_hostinfo();
					if(!hinfo.allows_download_resumption_free) {
						reconnect_allowed = false;
					}
				}
			}

		// Only reconnect if no download is running
		} else if(reconnect_policy == "SOFT") {
			reconnect_allowed = true;
			for(download_container::iterator it = global_download_list.begin(); it != global_download_list.end(); ++it) {
				if(it->get_status() == DOWNLOAD_RUNNING) {
					reconnect_allowed = false;
				}
			}
		}

		if(reconnect_needed && reconnect_allowed) {
			string reconnect_script = program_root + "/reconnect/" + reconnect_plugin;
			struct stat st;
			if(stat(reconnect_script.c_str(), &st) != 0) {
				log_string("Reconnect script for selected router model not found!", LOG_SEVERE);
				sleep(10);
				continue;
			}

			log_string("Reconnecting now!", LOG_WARNING);
			system(reconnect_script.c_str());
			for(download_container::iterator it = global_download_list.begin(); it != global_download_list.end(); ++it) {
				if(it->get_status() == DOWNLOAD_WAITING) {
					it->set_status(DOWNLOAD_PENDING);
				}
			}
			sleep(10);
		}
	}
}

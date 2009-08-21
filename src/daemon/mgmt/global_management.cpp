#include <vector>
#include <string>

#include <curl/curl.h>

#include "../dl/download.h"
#include "../dl/download_container.h"
#include "../dl/download_thread.h"
#include "../tools/helperfunctions.h"
#include "../../lib/cfgfile/cfgfile.h"
#include "global_management.h"

#include <sys/types.h>
#include <sys/stat.h>
using namespace std;

extern download_container global_download_list;
extern cfgfile global_config;
extern cfgfile global_router_config;
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
	std::string router_ip;
	std::string router_username;
	std::string router_password;
	while(true) {
	    if(global_config.get_cfg_value("enable_reconnect") == "0") {
	        sleep(10);
	        continue;
	    }

		reconnect_plugin = global_router_config.get_cfg_value("router_model");
		if(reconnect_plugin.empty()) {
			sleep(10);
			continue;
		}
		reconnect_policy = global_router_config.get_cfg_value("reconnect_policy");
		if(reconnect_policy.empty()) {
			sleep(10);
			continue;
		}
		router_ip = global_router_config.get_cfg_value("router_ip");
		if(router_ip.empty()) {
		    log_string("Reconnecting activated, but no router ip specified", LOG_WARNING);
		    sleep(10);
		    continue;
		}

        router_username = global_router_config.get_cfg_value("router_username");
        router_password = global_router_config.get_cfg_value("router_password");

		bool reconnect_needed = false;
		bool reconnect_allowed = false;
		for(download_container::iterator it = global_download_list.begin(); it != global_download_list.end(); ++it) {
			if(it->get_status() == DOWNLOAD_WAITING) {
				reconnect_needed = true;
			}
		}
		if(!reconnect_needed) {
			sleep(5);
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
        // Only reconnect if no download is running and no download can be started
		} else if(reconnect_policy == "PUSSY") {
		    reconnect_allowed = true;
			for(download_container::iterator it = global_download_list.begin(); it != global_download_list.end(); ++it) {
				if(it->get_status() == DOWNLOAD_RUNNING) {
					reconnect_allowed = false;
				}
			}
			download_container::iterator it = global_download_list.get_next_downloadable();
			if(it != global_download_list.end()) {
			    reconnect_allowed = false;
			}
		} else {
		    log_string("Invalid reconnect policy", LOG_SEVERE);
		    sleep(10);
		    continue;
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
			string ex = reconnect_script + ' ' + router_ip + ' ' + router_username + ' ' + router_password;
			if(system(ex.c_str()) == 0) {
                for(download_container::iterator it = global_download_list.begin(); it != global_download_list.end(); ++it) {
                    if(it->get_status() == DOWNLOAD_WAITING) {
                        it->set_status(DOWNLOAD_PENDING);
                    }
                }
			} else {
			    log_string("Reconnect plugin failed!", LOG_SEVERE);
			}
		}
		sleep(10);
	}
}

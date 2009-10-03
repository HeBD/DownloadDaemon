/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "../lib/cfgfile/cfgfile.h"
#include "mgmt/mgmt_thread.h"
#include "dl/download.h"
#include "dl/download_thread.h"
#include "global.h"
#include "mgmt/global_management.h"
#include "tools/helperfunctions.h"

#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include <iostream>
#include <vector>
#include <string>
#include <climits>
#include <cstdlib>

#include <sys/stat.h>
#include <sys/types.h>
#include <dlfcn.h>

using namespace std;


extern cfgfile global_config;

int main(int argc, char* argv[], char* env[]) {
	if(argc == 2) {
		std::string argv1 = argv[1];
		if(argv1 == "-d" || argv1 == "--daemon") {
			int i = fork();
			if (i < 0) return 1; /* fork error */
			if (i > 0) return 0; /* parent exits */
			/* child (daemon) continues */
		} else if(argv1 == "--help") {
			cout << "Usage: downloaddaemon [-d|--daemon]" << endl;
			return 0;
		}
	}

	struct stat st;

	// getting working dir
	std::string argv0 = argv[0];
	program_root = argv[0];
	if(argv0[0] != '/' && argv0.find('/') != string::npos) {
		// Relative path.. sux, but is okay.
		char* c_old_wd = get_current_dir_name();
		std::string wd = c_old_wd;
		free(c_old_wd);
		wd += '/';
		wd += argv0;
		//
		char* real_path = realpath(wd.c_str(), 0);
		if(real_path == 0) {
			cerr << "Unable to locate executable!" << endl;
			exit(-1);
		}
		program_root = real_path;
		free(real_path);
	} else if(argv0.find('/') == string::npos && !argv0.empty()) {
		// It's in $PATH... let's go!
		std::string env_path, curr_env;
		for(char** curr_c = env; curr_c != 0 && *curr_c != 0; ++curr_c) {
			curr_env = *curr_c;
			if(curr_env.find("PATH") == 0) {
				env_path = curr_env.substr(curr_env.find('=') + 1);
				break;
			}
		}

		std::string curr_path;
		size_t curr_pos = 0, last_pos = 0;
		bool found = false;

		while((curr_pos = env_path.find_first_of(":", curr_pos)) != string::npos) {
			curr_path = env_path.substr(last_pos, curr_pos -  last_pos);
			curr_path += '/';
			curr_path += argv[0];
			if(stat(curr_path.c_str(), &st) == 0) {
				found = true;
				break;
			}

			last_pos = ++curr_pos;
		}

		if(!found) {
			// search the last folder of $PATH which is not included in the while loop
			curr_path = env_path.substr(last_pos, curr_pos -  last_pos);
			curr_path += '/';
			curr_path += argv[0];
			if(stat(curr_path.c_str(), &st) == 0) {
				found = true;
			}
		}

		if(found) {
			// successfully located the file..
			// resolve symlinks, etc
			char* real_path = realpath(curr_path.c_str(), 0);
			if(real_path == NULL) {
				cerr << "Unable to locate executable!" << endl;
				exit(-1);
			} else {
				program_root = real_path;
				free(real_path);
			}
		} else {
			cerr << "Unable to locate executable!" << endl;
			exit(-1);
		}
	} else if(argv0.empty()) {

	}

	// else: it's an absolute path.. nothing to do - perfect!
	program_root = program_root.substr(0, program_root.find_last_of('/'));
	program_root = program_root.substr(0, program_root.find_last_of('/'));
	program_root.append("/share/downloaddaemon/");
	if(stat(program_root.c_str(), &st) != 0) {
		cerr << "Unable to locate program data (should be in bindir/../share/downloaddaemon)" << endl;
		cerr << "We were looking in: " << program_root << endl;
		exit(-1);
	}
	chdir(program_root.c_str());


	if(stat("/etc/downloaddaemon/downloaddaemon.conf", &st) != 0) {
		cerr << "Could not locate configuration file!" << endl;
		exit(-1);
	}

	global_config.open_cfg_file("/etc/downloaddaemon/downloaddaemon.conf", true);
	global_router_config.open_cfg_file("/etc/downloaddaemon/routerinfo.conf", true);
	global_premium_config.open_cfg_file("/etc/downloaddaemon/premium_accounts.conf", true);

	std::string dlist_fn = global_config.get_cfg_value("dlist_file");
	correct_path(dlist_fn);
	global_download_list.from_file(dlist_fn.c_str());

	log_string("DownloadDaemon started successfully", LOG_DEBUG);

	boost::thread mgmt_thread(mgmt_thread_main);
	boost::thread download_thread(download_thread_main);
	boost::thread download_ticker(tick_downloads);
	//boost::thread reconnector(reconnect);

	//reconnector.join();
	download_ticker.join();
	download_thread.join();
	mgmt_thread.join();
}
